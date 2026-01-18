/* -- C++ -- */
/**
 *  @file  ana/src/AnalysisDefinition.cc
 *
 *  @brief Compiled analysis definition for template production.
 */

#include "AnalysisDefinition.hh"

#include <sstream>

#include "AnalysisRdfDefinitions.hh"
#include "Sample.hh"

namespace nuxsec
{

const AnalysisDefinition &AnalysisDefinition::Instance()
{
    static const AnalysisDefinition analysis{};
    return analysis;
}

AnalysisDefinition::AnalysisDefinition()
{
    m_name = "nuxsec_default_v1";
    m_tree_name = "MyTree";

    m_templates_1d = {
        {"h_reco_nuE_sig", "Reco nu E (sig)", "sel_signal", "reco_nu_energy", "", 20, 0.0, 2.0},
        {"h_reco_nuE_bkg", "Reco nu E (bkg)", "sel_bkg", "reco_nu_energy", "", 20, 0.0, 2.0},
        {"h_reco_vtxz",
         "Reco vtx z",
         "sel_reco_fv",
         "reco_neutrino_vertex_sce_z",
         "",
         40,
         0.0,
         1050.0},
    };
}

std::string AnalysisDefinition::Templates1DToTsv() const
{
    std::ostringstream os;
    os << "name\ttitle\tselection\tvariable\tweight\tnbins\txmin\txmax\n";
    for (const auto &spec : m_templates_1d)
    {
        os << spec.name << "\t" << spec.title << "\t" << spec.selection << "\t" << spec.variable
           << "\t" << spec.weight << "\t" << spec.nbins << "\t" << spec.xmin << "\t" << spec.xmax
           << "\n";
    }
    return os.str();
}

ProcessorEntry AnalysisDefinition::MakeProcessorEntry(const Sample &sample) const noexcept
{
    ProcessorEntry proc_entry;

    switch (sample.kind)
    {
    case SampleKind::kData:
        proc_entry.source = SourceKind::kData;
        break;
    case SampleKind::kEXT:
        proc_entry.source = SourceKind::kExt;
        proc_entry.trig_nom = sample.db_tor101_pot_sum;
        proc_entry.trig_eqv = sample.subrun_pot_sum;
        break;
    case SampleKind::kOverlay:
    case SampleKind::kDirt:
    case SampleKind::kStrangeness:
        proc_entry.source = SourceKind::kMC;
        proc_entry.pot_nom = sample.db_tortgt_pot_sum;
        proc_entry.pot_eqv = sample.subrun_pot_sum;
        break;
    default:
        proc_entry.source = SourceKind::kUnknown;
        break;
    }

    return proc_entry;
}

} // namespace nuxsec
