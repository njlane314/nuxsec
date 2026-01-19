/* -- C++ -- */
/**
 *  @file  apps/src/nuxsecSampleIOaggregator.cc
 *
 *  @brief Main entrypoint for Sample aggregation.
 */

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AppUtils.hh"
#include "SampleAggregator.hh"
#include "SampleRootIO.hh"

namespace
{

struct Args
{
    std::string sample_name;
    std::string filelist_path;
    std::string output_path;
    std::string sample_list_path;
};

Args parse_args(int argc, char **argv)
{
    if (argc != 2)
    {
        throw std::runtime_error("Usage: nuxsecSampleIOaggregator NAME:FILELIST");
    }

    const std::string spec = argv[1];
    const auto pos = spec.find(':');
    if (pos == std::string::npos)
    {
        throw std::runtime_error("Bad sample spec (expected NAME:FILELIST): " + spec);
    }

    Args args;
    args.sample_name = nuxsec::app::trim(spec.substr(0, pos));
    args.filelist_path = nuxsec::app::trim(spec.substr(pos + 1));

    if (args.sample_name.empty() || args.filelist_path.empty())
    {
        throw std::runtime_error("Bad sample spec: " + spec);
    }

    args.output_path = "build/out/sample/sample_root_" + args.sample_name + ".root";
    args.sample_list_path = "build/out/sample/samples.tsv";

    return args;
}

void update_sample_list(const std::string &list_path, const nuxsec::Sample &sample,
                        const std::string &output_path)
{
    auto entries = nuxsec::app::read_sample_list(list_path, true, false);
    const std::string kind_name = nuxsec::sample_kind_name(sample.kind);
    const std::string beam_name = nuxsec::beam_mode_name(sample.beam);

    bool updated = false;
    for (auto &entry : entries)
    {
        if (entry.sample_name == sample.sample_name &&
            entry.sample_kind == kind_name &&
            entry.beam_mode == beam_name)
        {
            entry.output_path = output_path;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        nuxsec::app::SampleListEntry entry;
        entry.sample_name = sample.sample_name;
        entry.sample_kind = kind_name;
        entry.beam_mode = beam_name;
        entry.output_path = output_path;
        entries.push_back(std::move(entry));
    }

    nuxsec::app::write_sample_list(list_path, std::move(entries));
}

}

int main(int argc, char **argv)
{
    using namespace nuxsec;

    try
    {
        const Args args = parse_args(argc, argv);
        const auto files = nuxsec::app::read_file_list(args.filelist_path);

        std::filesystem::path output_path(args.output_path);
        if (!output_path.parent_path().empty())
        {
            std::filesystem::create_directories(output_path.parent_path());
        }
        std::filesystem::path sample_list_path(args.sample_list_path);
        if (!sample_list_path.parent_path().empty())
        {
            std::filesystem::create_directories(sample_list_path.parent_path());
        }

        Sample sample = SampleAggregator::aggregate(args.sample_name, files);
        SampleRootIO::write(sample, args.output_path);
        update_sample_list(args.sample_list_path, sample, args.output_path);

        std::cerr << "[nuxsecSampleIOaggregator] sample=" << sample.sample_name
                  << " fragments=" << sample.fragments.size()
                  << " pot_sum=" << sample.subrun_pot_sum
                  << " db_tortgt_pot_sum=" << sample.db_tortgt_pot_sum
                  << " normalisation=" << sample.normalisation
                  << " normalised_pot_sum=" << sample.normalised_pot_sum
                  << " output=" << args.output_path
                  << " sample_list=" << args.sample_list_path
                  << "\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
}
