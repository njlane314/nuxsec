/* -- C++ -- */
/**
 *  @file  rdf/include/RDataFrameFactory.hh
 *
 *  @brief Sample loading and column definitions for ROOT RDataFrame.
 */

#ifndef Nuxsec_RDF_RDATAFRAME_FACTORY_H_INCLUDED
#define Nuxsec_RDF_RDATAFRAME_FACTORY_H_INCLUDED

#include <ROOT/RDataFrame.hxx>

#include <string>
#include <vector>

#include "Sample.hh"

namespace nuxsec
{

struct ColumnDefinition
{
    std::string name;
    std::string expression;
    std::string description;
};

class RDataFrameFactory
{
  public:
    static ROOT::RDataFrame loadSample(const Sample &sample, const std::string &tree_name);
    static ROOT::RDF::RNode defineVariables(ROOT::RDF::RNode node,
                                            const std::vector<ColumnDefinition> &definitions);
    static std::vector<std::string> collectFiles(const Sample &sample);
};

} // namespace nuxsec

#endif // Nuxsec_RDF_RDATAFRAME_FACTORY_H_INCLUDED
