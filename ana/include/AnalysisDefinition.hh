/* -- C++ -- */
/**
 *  @file  ana/include/AnalysisDefinition.hh
 *
 *  @brief Compiled analysis definition for template production.
 */

#ifndef Nuxsec_ANA_ANALYSIS_DEFINITION_H_INCLUDED
#define Nuxsec_ANA_ANALYSIS_DEFINITION_H_INCLUDED

#include <string>
#include <vector>

#include "TemplateSpec.hh"

namespace nuxsec
{

struct ProcessorEntry;
struct Sample;

/** \brief Compiled analysis configuration for template production. */
class AnalysisDefinition final
{
  public:
    static const AnalysisDefinition &Instance();

    const std::string &Name() const noexcept { return m_name; }
    const std::string &TreeName() const noexcept { return m_tree_name; }
    const std::vector<TemplateSpec1D> &Templates1D() const noexcept { return m_templates_1d; }
    std::string Templates1DToTsv() const;

    ProcessorEntry MakeProcessorEntry(const Sample &sample) const noexcept;

  private:
    AnalysisDefinition();

    std::string m_name;
    std::string m_tree_name;
    std::vector<TemplateSpec1D> m_templates_1d;
};

} // namespace nuxsec

#endif // Nuxsec_ANA_ANALYSIS_DEFINITION_H_INCLUDED
