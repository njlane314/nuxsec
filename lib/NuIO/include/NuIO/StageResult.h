#ifndef NU_IO_STAGE_RESULT_H
#define NU_IO_STAGE_RESULT_H

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace nuio {

inline std::string ToLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  return s;
}

enum class SampleKind {
  kUnknown = 0,
  kData,
  kEXT,
  kMCOverlay,
  kMCDirt,
  kMCStrangeness
};

inline const char* SampleKindName(SampleKind k) {
  switch (k) {
    case SampleKind::kData:          return "data";
    case SampleKind::kEXT:           return "ext";
    case SampleKind::kMCOverlay:     return "mc_overlay";
    case SampleKind::kMCDirt:        return "mc_dirt";
    case SampleKind::kMCStrangeness: return "mc_strangeness";
    default:                         return "unknown";
  }
}

enum class BeamMode {
  kUnknown = 0,
  kNuMI,
  kBNB
};

inline const char* BeamModeName(BeamMode b) {
  switch (b) {
    case BeamMode::kNuMI: return "numi";
    case BeamMode::kBNB:  return "bnb";
    default:              return "unknown";
  }
}

struct RunSubrun {
  int run = 0;
  int subrun = 0;
};

struct SubRunSummary {
  double pot_sum = 0.0;
  long long n_entries = 0;
  std::vector<RunSubrun> unique_pairs;
};

struct DBSums {
  double tortgt_sum = 0.0;
  double tor101_sum = 0.0;
  double tor860_sum = 0.0;
  double tor875_sum = 0.0;

  long long EA9CNT_sum = 0;
  long long E1DCNT_sum = 0;
  long long EXTTrig_sum = 0;
  long long Gate1Trig_sum = 0;
  long long Gate2Trig_sum = 0;

  long long n_pairs_loaded = 0;
};

struct StageConfig {
  std::string stage_name;
  std::string filelist_path;
};

struct StageResult {
  StageConfig cfg;
  SampleKind kind = SampleKind::kUnknown;
  BeamMode beam = BeamMode::kUnknown;

  std::vector<std::string> input_files;

  SubRunSummary subrun;
  DBSums dbsums;

  double scale = 1.0;

  double db_tortgt_pot = 0.0;
  double db_tor101_pot = 0.0;
};

inline SampleKind SampleKindFromName(const std::string& name) {
  const std::string v = ToLower(name);
  if (v == "data") return SampleKind::kData;
  if (v == "ext") return SampleKind::kEXT;
  if (v == "mc_overlay") return SampleKind::kMCOverlay;
  if (v == "mc_dirt") return SampleKind::kMCDirt;
  if (v == "mc_strangeness") return SampleKind::kMCStrangeness;
  return SampleKind::kUnknown;
}

inline BeamMode BeamModeFromName(const std::string& name) {
  const std::string v = ToLower(name);
  if (v == "numi") return BeamMode::kNuMI;
  if (v == "bnb") return BeamMode::kBNB;
  return BeamMode::kUnknown;
}

}  // namespace nuio

#endif  // NU_IO_STAGE_RESULT_H
