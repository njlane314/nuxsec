/* -- C++ -- */
/**
 *  @file  ana/src/RDataFrameFactory.cc
 *
 *  @brief Sample loading and variable definitions for ROOT RDataFrame.
 */

#include "RDataFrameFactory.hh"

#include <utility>

#include "ArtFileProvenanceIO.hh"

namespace nuxsec
{

ROOT::RDataFrame RDataFrameFactory::load_sample(const sample::SampleIO::Sample &sample,
                                                const std::string &tree_name)
{
    std::vector<std::string> files = collect_files(sample);
    return ROOT::RDataFrame(tree_name, files);
}

ROOT::RDF::RNode RDataFrameFactory::define_variables(ROOT::RDF::RNode node,
                                             const std::vector<ColumnDefinition> &definitions)
{
    ROOT::RDF::RNode updated_node = std::move(node);
    for (const ColumnDefinition &definition : definitions)
    {
        updated_node = updated_node.Define(definition.name, definition.expression);
    }

    return updated_node;
}

std::vector<std::string> RDataFrameFactory::collect_files(const sample::SampleIO::Sample &sample)
{
    std::vector<std::string> files;
    files.reserve(sample.inputs.size());
    for (const sample::SampleIO::ProvenanceInput &input : sample.inputs)
    {
        art::Provenance prov = ArtFileProvenanceIO::read(input.art_path);
        for (const auto &path : prov.input_files)
        {
            files.push_back(path);
        }
    }

    return files;
}

} // namespace nuxsec
