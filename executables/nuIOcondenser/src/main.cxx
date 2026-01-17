#include <TChain.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TFileMerger.h>
#include <TNamed.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <TParameter.h>
#include <TTree.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "NuIO/StageResult.h"
#include "NuIO/StageResultIO.h"
#include "NuPOT/BeamRunDB.h"

namespace nucond {

static inline std::string Trim(std::string s) {
  auto notspace = [](unsigned char c){ return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
  s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
  return s;
}

static inline bool IContains(const std::string& hay, const std::string& needle) {
  return nuio::ToLower(hay).find(nuio::ToLower(needle)) != std::string::npos;
}

static inline void EnsureDirLike(const std::string& path) {
  (void)path;
}

static inline std::vector<std::string> ReadFileList(const std::string& filelistPath) {
  std::ifstream fin(filelistPath);
  if (!fin) {
    throw std::runtime_error("Failed to open filelist: " + filelistPath +
                             " (errno=" + std::to_string(errno) + " " + std::strerror(errno) + ")");
  }
  std::vector<std::string> files;
  std::string line;
  while (std::getline(fin, line)) {
    line = Trim(line);
    if (line.empty()) continue;
    if (!line.empty() && line[0] == '#') continue;
    files.push_back(line);
  }
  if (files.empty()) {
    throw std::runtime_error("Filelist is empty: " + filelistPath);
  }
  return files;
}

static inline nuio::SampleKind InferSampleKind(const std::string& stageName) {
  const std::string s = nuio::ToLower(stageName);
  if (s.find("ext") != std::string::npos)      return nuio::SampleKind::kEXT;
  if (s.find("dirt") != std::string::npos)     return nuio::SampleKind::kMCDirt;
  if (s.find("strange") != std::string::npos)  return nuio::SampleKind::kMCStrangeness;
  if (s.find("beam") != std::string::npos)     return nuio::SampleKind::kMCOverlay;
  if (s.find("data") != std::string::npos)     return nuio::SampleKind::kData;
  return nuio::SampleKind::kUnknown;
}

static inline nuio::BeamMode InferBeamMode(const std::string& stageName) {
  const std::string s = nuio::ToLower(stageName);
  if (s.find("numi") != std::string::npos) return nuio::BeamMode::kNuMI;
  if (s.find("bnb")  != std::string::npos) return nuio::BeamMode::kBNB;
  if (s.find("fhc") != std::string::npos || s.find("rhc") != std::string::npos) return nuio::BeamMode::kNuMI;
  return nuio::BeamMode::kUnknown;
}

struct FilePeek {
  std::optional<bool> isData;
  std::optional<bool> isNuMI;
};

static inline FilePeek PeekEventFlags(const std::string& rootFile) {
  FilePeek out;
  std::unique_ptr<TFile> f(TFile::Open(rootFile.c_str(), "READ"));
  if (!f || f->IsZombie()) return out;

  TTree* t = dynamic_cast<TTree*>(f->Get("EventSelectionFilter"));
  if (!t) return out;

  Bool_t is_data = false;
  Bool_t is_numi = false;

  const bool has_is_data = (t->GetBranch("is_data") != nullptr);
  const bool has_is_numi = (t->GetBranch("is_numi") != nullptr);

  if (!has_is_data && !has_is_numi) return out;
  if (t->GetEntries() <= 0) return out;

  if (has_is_data) t->SetBranchAddress("is_data", &is_data);
  if (has_is_numi) t->SetBranchAddress("is_numi", &is_numi);

  t->GetEntry(0);

  if (has_is_data) out.isData = static_cast<bool>(is_data);
  if (has_is_numi) out.isNuMI = static_cast<bool>(is_numi);
  return out;
}

static inline uint64_t PackRunSubrun(int run, int subrun) {
  return (static_cast<uint64_t>(static_cast<uint32_t>(run)) << 32) |
         (static_cast<uint64_t>(static_cast<uint32_t>(subrun)));
}

static inline nuio::SubRunSummary ScanSubRunTree(const std::vector<std::string>& files) {
  nuio::SubRunSummary out;

  TChain chain("SubRun");
  for (const auto& f : files) chain.Add(f.c_str());

  Int_t run = 0;
  Int_t subRun = 0;
  Double_t pot = 0.0;

  if (chain.GetBranch("run") == nullptr || chain.GetBranch("subRun") == nullptr || chain.GetBranch("pot") == nullptr) {
    throw std::runtime_error("SubRun tree missing required branches (run, subRun, pot).");
  }

  chain.SetBranchAddress("run", &run);
  chain.SetBranchAddress("subRun", &subRun);
  chain.SetBranchAddress("pot", &pot);

  const Long64_t n = chain.GetEntries();
  out.n_entries = static_cast<long long>(n);

  std::unordered_set<uint64_t> seen;
  seen.reserve(static_cast<size_t>(n));

  for (Long64_t i = 0; i < n; ++i) {
    chain.GetEntry(i);
    out.pot_sum += static_cast<double>(pot);

    const uint64_t key = PackRunSubrun(run, subRun);
    if (seen.insert(key).second) {
      out.unique_pairs.push_back(nuio::RunSubrun{static_cast<int>(run), static_cast<int>(subRun)});
    }
  }

  std::sort(out.unique_pairs.begin(), out.unique_pairs.end(),
            [](const nuio::RunSubrun& a, const nuio::RunSubrun& b) {
              if (a.run != b.run) return a.run < b.run;
              return a.subrun < b.subrun;
            });

  return out;
}

static inline bool MergeRootFiles(const std::vector<std::string>& files,
                                 const std::string& outFile)
{
  TFileMerger merger(false);
  merger.SetFastMethod(true);
  if (!merger.OutputFile(outFile.c_str(), "RECREATE")) {
    std::cerr << "ERROR: failed to open merger output file: " << outFile << "\n";
    return false;
  }
  for (const auto& f : files) {
    if (!merger.AddFile(f.c_str())) {
      std::cerr << "ERROR: failed to add file to merger: " << f << "\n";
      return false;
    }
  }
  return merger.Merge();
}

struct CLI {
  std::string db_path = "/exp/uboone/data/uboonebeam/beamdb/run.db";
  std::string outdir = "./condensed";
  std::string manifest_path = "./condensed/manifest.root";
  double pot_scale = 1e12;
  bool do_merge = true;
  std::string ext_denom = "EXTTrig";

  std::vector<nuio::StageConfig> stages;
};

static inline void PrintUsage(const char* argv0) {
  std::cerr <<
    "Usage:\n"
    "  " << argv0 << " [options] --stage NAME:FILELIST [--stage NAME:FILELIST ...]\n\n"
    "Options:\n"
    "  --db PATH           Path to run.db (default: /exp/uboone/data/uboonebeam/beamdb/run.db)\n"
    "  --outdir DIR        Output directory (default: ./condensed)\n"
    "  --manifest PATH     Manifest ROOT file (default: ./condensed/manifest.root)\n"
    "  --pot-scale X       Multiply tortgt/tor101 sums by X to get POT (default: 1e12)\n"
    "  --no-merge          Do not merge ROOT files; only write manifest (still scans SubRun + DB)\n"
    "  --ext-denom COL     EXT denominator column (EXTTrig|Gate1Trig|Gate2Trig...) (default: EXTTrig)\n"
    "  --help              Print this message\n";
}

static inline CLI ParseArgs(int argc, char** argv) {
  CLI cli;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto need = [&](const std::string& opt) -> std::string {
      if (i + 1 >= argc) throw std::runtime_error("Missing value for " + opt);
      return argv[++i];
    };

    if (a == "--help" || a == "-h") {
      PrintUsage(argv[0]);
      std::exit(0);
    } else if (a == "--db") {
      cli.db_path = need("--db");
    } else if (a == "--outdir") {
      cli.outdir = need("--outdir");
    } else if (a == "--manifest") {
      cli.manifest_path = need("--manifest");
    } else if (a == "--pot-scale") {
      cli.pot_scale = std::stod(need("--pot-scale"));
    } else if (a == "--no-merge") {
      cli.do_merge = false;
    } else if (a == "--ext-denom") {
      cli.ext_denom = need("--ext-denom");
    } else if (a == "--stage") {
      const std::string spec = need("--stage");
      const auto pos = spec.find(':');
      if (pos == std::string::npos) {
        throw std::runtime_error("Bad --stage spec (expected NAME:FILELIST): " + spec);
      }
      nuio::StageConfig sc;
      sc.stage_name = spec.substr(0, pos);
      sc.filelist_path = spec.substr(pos + 1);
      sc.stage_name = Trim(sc.stage_name);
      sc.filelist_path = Trim(sc.filelist_path);
      if (sc.stage_name.empty() || sc.filelist_path.empty()) {
        throw std::runtime_error("Bad --stage spec (empty name or filelist): " + spec);
      }
      cli.stages.push_back(sc);
    } else {
      throw std::runtime_error("Unknown argument: " + a);
    }
  }

  if (cli.stages.empty()) {
    throw std::runtime_error("No --stage specified.");
  }
  return cli;
}

static inline long long GetDenomFromDB(const nuio::DBSums& s, const std::string& denomCol) {
  const std::string c = nuio::ToLower(denomCol);
  if (c == "exttrig")  return s.EXTTrig_sum;
  if (c == "gate1trig") return s.Gate1Trig_sum;
  if (c == "gate2trig") return s.Gate2Trig_sum;
  return s.EXTTrig_sum;
}

static inline nuio::StageResult ProcessStage(const nuio::StageConfig& cfg,
                                             const nupot::BeamRunDB& db,
                                             double pot_scale,
                                             const std::string& ext_denom_col,
                                             const std::string& outdir,
                                             bool do_merge)
{
  nuio::StageResult r;
  r.cfg = cfg;

  r.input_files = ReadFileList(cfg.filelist_path);

  r.kind = InferSampleKind(cfg.stage_name);
  r.beam = InferBeamMode(cfg.stage_name);

  const FilePeek peek = PeekEventFlags(r.input_files.front());
  if (peek.isNuMI.has_value()) {
    r.beam = (*peek.isNuMI ? nuio::BeamMode::kNuMI : nuio::BeamMode::kBNB);
  }
  if (peek.isData.has_value()) {
    if (*peek.isData) {
      if (r.kind == nuio::SampleKind::kUnknown) r.kind = nuio::SampleKind::kData;
    } else {
      if (r.kind == nuio::SampleKind::kUnknown) r.kind = nuio::SampleKind::kMCOverlay;
    }
  }

  r.subrun = ScanSubRunTree(r.input_files);

  r.dbsums = db.SumRuninfoForSelection(r.subrun.unique_pairs);

  r.db_tortgt_pot = r.dbsums.tortgt_sum * pot_scale;
  r.db_tor101_pot = r.dbsums.tor101_sum * pot_scale;

  r.scale = 1.0;

  if (r.kind == nuio::SampleKind::kMCOverlay || r.kind == nuio::SampleKind::kMCDirt || r.kind == nuio::SampleKind::kMCStrangeness) {
    const double pot_mc = r.subrun.pot_sum;
    const double pot_data = r.db_tortgt_pot;

    if (pot_mc > 0.0 && pot_data > 0.0) {
      r.scale = pot_data / pot_mc;
    } else {
      r.scale = 0.0;
    }
  } else if (r.kind == nuio::SampleKind::kEXT) {
    const long long num = r.dbsums.EA9CNT_sum;
    const long long den = GetDenomFromDB(r.dbsums, ext_denom_col);
    if (num > 0 && den > 0) r.scale = static_cast<double>(num) / static_cast<double>(den);
    else r.scale = 0.0;
  } else {
    r.scale = 1.0;
  }

  const std::string outFile = outdir + "/" + cfg.stage_name + ".condensed.root";

  if (do_merge) {
    const bool ok = MergeRootFiles(r.input_files, outFile);
    if (!ok) {
      throw std::runtime_error("ROOT merge failed for stage " + cfg.stage_name + " -> " + outFile);
    }
    nuio::StageResultIO::Write(r, outFile);
  }

  return r;
}

}  // namespace nucond

int main(int argc, char** argv) {
  using namespace nucond;

  try {
    const CLI cli = ParseArgs(argc, argv);

    EnsureDirLike(cli.outdir);

    nupot::BeamRunDB db(cli.db_path);

    std::vector<nuio::StageResult> results;
    results.reserve(cli.stages.size());

    for (const auto& st : cli.stages) {
      std::cerr << "[nuIOcondenser] stage=" << st.stage_name
                << " filelist=" << st.filelist_path << "\n";
      nuio::StageResult r = ProcessStage(st, db, cli.pot_scale, cli.ext_denom, cli.outdir, cli.do_merge);

      std::cerr << "  kind=" << nuio::SampleKindName(r.kind)
                << " beam=" << nuio::BeamModeName(r.beam)
                << " files=" << r.input_files.size()
                << " unique_pairs=" << r.subrun.unique_pairs.size()
                << " subrun_pot_sum=" << r.subrun.pot_sum
                << " db_tortgt_pot=" << r.db_tortgt_pot
                << " EA9CNT=" << r.dbsums.EA9CNT_sum
                << " EXTTrig=" << r.dbsums.EXTTrig_sum
                << " scale=" << r.scale
                << "\n";

      results.push_back(std::move(r));
    }

    {
      std::unique_ptr<TFile> mf(TFile::Open(cli.manifest_path.c_str(), "RECREATE"));
      if (!mf || mf->IsZombie()) {
        throw std::runtime_error("Failed to create manifest file: " + cli.manifest_path);
      }

      TTree t("Stages", "Condenser stage summary");
      std::string stage_name;
      std::string kind;
      std::string beam;
      std::string condensed_file;
      double subrun_pot_sum = 0.0;
      double db_tortgt_pot = 0.0;
      long long ea9cnt_sum = 0;
      long long exttrig_sum = 0;
      double scale = 1.0;
      long long n_files = 0;
      long long n_pairs = 0;

      t.Branch("stage_name", &stage_name);
      t.Branch("kind", &kind);
      t.Branch("beam", &beam);
      t.Branch("condensed_file", &condensed_file);
      t.Branch("subrun_pot_sum", &subrun_pot_sum);
      t.Branch("db_tortgt_pot", &db_tortgt_pot);
      t.Branch("ea9cnt_sum", &ea9cnt_sum);
      t.Branch("exttrig_sum", &exttrig_sum);
      t.Branch("scale", &scale);
      t.Branch("n_files", &n_files);
      t.Branch("n_pairs", &n_pairs);

      for (const auto& r : results) {
        stage_name = r.cfg.stage_name;
        kind = nuio::SampleKindName(r.kind);
        beam = nuio::BeamModeName(r.beam);
        condensed_file = cli.outdir + "/" + r.cfg.stage_name + ".condensed.root";
        subrun_pot_sum = r.subrun.pot_sum;
        db_tortgt_pot = r.db_tortgt_pot;
        ea9cnt_sum = r.dbsums.EA9CNT_sum;
        exttrig_sum = r.dbsums.EXTTrig_sum;
        scale = r.scale;
        n_files = static_cast<long long>(r.input_files.size());
        n_pairs = static_cast<long long>(r.subrun.unique_pairs.size());
        t.Fill();
      }

      mf->cd();
      t.Write();

      TDirectory* d = mf->mkdir("NuCondenser");
      d->cd();
      TNamed("db_path", cli.db_path.c_str()).Write("db_path", TObject::kOverwrite);
      TParameter<double>("pot_scale", cli.pot_scale).Write("pot_scale", TObject::kOverwrite);
      TNamed("ext_denom_column", cli.ext_denom.c_str()).Write("ext_denom_column", TObject::kOverwrite);
      TParameter<int>("did_merge", cli.do_merge ? 1 : 0).Write("did_merge", TObject::kOverwrite);

      mf->Write();
      mf->Close();
    }

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "FATAL: " << e.what() << "\n";
    return 1;
  }
}
