/**
 *  @file  bin/artIOaggregator/artIOaggregator.cxx
 *
 *  @brief Main entrypoint for ArtIO provenance generation
 */

#include <cctype>
#include <cerrno>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "NuIO/ArtProvenanceIO.h"
#include "NuIO/RunInfoDB.h"
#include "NuIO/SubRunScanner.h"

namespace
{

std::string Trim(std::string s)
{
    auto notspace = [](unsigned char c)
    {
        return std::isspace(c) == 0;
    };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
    return s;
}

std::vector<std::string> ReadFileList(const std::string &filelistPath)
{
    std::ifstream fin(filelistPath);
    if (!fin)
    {
        throw std::runtime_error("Failed to open filelist: " + filelistPath +
                                 " (errno=" + std::to_string(errno) + " " + std::strerror(errno) + ")");
    }
    std::vector<std::string> files;
    std::string line;
    while (std::getline(fin, line))
    {
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        files.push_back(line);
    }
    if (files.empty())
    {
        throw std::runtime_error("Filelist is empty: " + filelistPath);
    }
    return files;
}

bool IsNuSelectionDataFile(const std::string &path)
{
    const auto pos = path.find_last_of("/\\");
    const std::string name = (pos == std::string::npos) ? path : path.substr(pos + 1);
    return name == "nuselection_data.root";
}

struct CLI
{
    std::string artio_path = "./ArtIO.root";
    nuio::StageCfg stage_cfg;
};

CLI ParseArgs(int argc, char **argv)
{
    if (argc != 2)
    {
        throw std::runtime_error("Usage: artIOaggregator NAME:FILELIST");
    }

    const std::string spec = argv[1];
    const auto pos = spec.find(':');
    if (pos == std::string::npos)
    {
        throw std::runtime_error("Bad stage spec (expected NAME:FILELIST): " + spec);
    }

    CLI cli;
    cli.stage_cfg.stage_name = Trim(spec.substr(0, pos));
    cli.stage_cfg.filelist_path = Trim(spec.substr(pos + 1));

    if (cli.stage_cfg.stage_name.empty() || cli.stage_cfg.filelist_path.empty())
    {
        throw std::runtime_error("Bad stage spec: " + spec);
    }

    return cli;
}

}

int main(int argc, char **argv)
{
    using namespace nuio;

    try
    {
        const std::string db_path = "/exp/uboone/data/uboonebeam/beamdb/run.db";
        const double pot_scale = 1e12;

        const CLI cli = ParseArgs(argc, argv);

        RunInfoDB db(db_path);

        const auto files = ReadFileList(cli.stage_cfg.filelist_path);

        ArtProvenance rec;
        rec.cfg = cli.stage_cfg;
        rec.input_files = files;
        rec.scale = pot_scale;

        if (IsNuSelectionDataFile(files.front()))
        {
            rec.kind = SampleKind::kData;
        }

        rec.subrun = ScanSubRunTree(files);
        rec.runinfo = db.SumRuninfoForSelection(rec.subrun.unique_pairs);
        rec.db_tortgt_pot = rec.runinfo.tortgt_sum * pot_scale;
        rec.db_tor101_pot = rec.runinfo.tor101_sum * pot_scale;

        std::cerr << "[artIOaggregator] add stage=" << rec.cfg.stage_name
                  << " files=" << rec.input_files.size()
                  << " pairs=" << rec.subrun.unique_pairs.size()
                  << " pot_sum=" << rec.subrun.pot_sum
                  << " tortgt=" << rec.runinfo.tortgt_sum * pot_scale
                  << "\n";

        ArtProvenanceIO::Write(rec, cli.artio_path);

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
}
