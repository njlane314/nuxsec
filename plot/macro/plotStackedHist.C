// plot/macro/plotStackedHist.C
//
// Run with:
//   ./nuxsec macro plotStackedHist.C
//   ./nuxsec macro plotStackedHist.C 'plotStackedHist("/path/to/samples.tsv","true",false)'
//
// Notes:
//   - This macro loads aggregated samples (samples.tsv -> SampleIO -> original analysis tree)
//   - It runs your analysis column derivations so that "analysis_channels" exists for stacking.
//   - The stack is grouped by "analysis_channels"; expr controls the x-axis variable only.
//   - MC yields are scaled by w_nominal unless an alternative weight is provided.
//   - Output dir/format follow PlotEnv defaults (NUXSEC_PLOT_DIR / NUXSEC_PLOT_FORMAT).
//   - Default input uses the generated event list (event_list_<analysis>.root).

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RDFHelpers.hxx>
#include <TFile.h>
#include <TH1D.h>

#include "AdaptiveBinningService.hh"
#include "AnalysisConfigService.hh"
#include "ColumnDerivationService.hh"
#include "EventListIO.hh"
#include "Plotter.hh"
#include "PlottingHelper.hh"
#include "RDataFrameService.hh"
#include "SampleCLI.hh"
#include "SampleIO.hh"


using namespace nu;

namespace
{
bool looks_like_event_list_root(const std::string &p)
{
    const auto n = p.size();
    if (n < 5 || p.substr(n - 5) != ".root")
        return false;

    std::unique_ptr<TFile> f(TFile::Open(p.c_str(), "READ"));
    if (!f || f->IsZombie())
        return false;

    const bool has_refs = (f->Get("sample_refs") != nullptr);
    const bool has_events_tree = (f->Get("events") != nullptr);
    const bool has_event_tree_key = (f->Get("event_tree") != nullptr);

    return has_refs && (has_events_tree || has_event_tree_key);
}
} // namespace

int plot_stacked_hist_impl(const std::string &expr,
                           const std::string &samples_tsv,
                           int nbins,
                           double xmin,
                           double xmax,
                           const std::string &mc_weight,
                           const std::string &extra_sel,
                           bool use_logy)
{
    ROOT::EnableImplicitMT();

    const std::string list_path = samples_tsv.empty() ? default_event_list_root() : samples_tsv;
    std::cout << "[plotStackedHist] input=" << list_path << "\n";

    if (!looks_like_event_list_root(list_path))
    {
        std::cerr << "[plotStackedHist] input is not an event list root file: " << list_path << "\n";
        return 1;
    }

    std::cout << "[plotStackedHist] mode=event_list\n";

    EventListIO el(list_path);
    ROOT::RDataFrame rdf = el.rdf();

    auto mask_data = el.mask_for_data();
    auto mask_ext = el.mask_for_ext();
    auto mask_mc = el.mask_for_mc_like();

    auto filter_by_mask = [](ROOT::RDF::RNode n, std::shared_ptr<const std::vector<char>> mask) {
        return n.Filter(
            [mask](int sid) {
                return sid >= 0
                       && sid < static_cast<int>(mask->size())
                       && (*mask)[static_cast<size_t>(sid)];
            },
            {"sample_id"});
    };

    ROOT::RDF::RNode base = rdf;
    ROOT::RDF::RNode node_data = filter_by_mask(base, mask_data);
    ROOT::RDF::RNode node_ext = filter_by_mask(base, mask_ext);
    ROOT::RDF::RNode node_mc = filter_by_mask(base, mask_mc)
                                   .Filter([mask_ext](int sid) {
                                       return !(sid >= 0
                                                && sid < static_cast<int>(mask_ext->size())
                                                && (*mask_ext)[static_cast<size_t>(sid)]);
                                   },
                                   {"sample_id"});

    std::vector<Entry> entries;
    entries.reserve(3);

    std::vector<const Entry *> mc;
    std::vector<const Entry *> data;

    ProcessorEntry rec_mc;
    rec_mc.source = Type::kMC;

    ProcessorEntry rec_ext;
    rec_ext.source = Type::kExt;

    ProcessorEntry rec_data;
    rec_data.source = Type::kData;

    entries.emplace_back(make_entry(std::move(node_mc), rec_mc));
    Entry &e_mc = entries.back();
    mc.push_back(&e_mc);

    entries.emplace_back(make_entry(std::move(node_ext), rec_ext));
    Entry &e_ext = entries.back();
    mc.push_back(&e_ext);

    entries.emplace_back(make_entry(std::move(node_data), rec_data));
    Entry &e_data = entries.back();
    data.push_back(&e_data);

    if (!extra_sel.empty())
    {
        e_mc.selection.nominal.node = e_mc.selection.nominal.node.Filter(extra_sel);
        e_ext.selection.nominal.node = e_ext.selection.nominal.node.Filter(extra_sel);
        e_data.selection.nominal.node = e_data.selection.nominal.node.Filter(extra_sel);
    }

    Plotter plotter;
    auto &opt = plotter.options();
    opt.use_log_y = use_logy;
    opt.legend_on_top = true;
    opt.annotate_numbers = true;
    opt.show_ratio = true;
    opt.show_ratio_band = true;
    opt.adaptive_binning = true;
    opt.adaptive_min_sumw = 25.0;
    opt.adaptive_max_relerr = 0.30;
    opt.adaptive_fold_overflow = true;
    opt.x_title = expr;
    opt.y_title = "Events";

    const double pot_data = el.total_pot_data();
    const double pot_mc = el.total_pot_mc();
    opt.total_protons_on_target = (pot_data > 0.0 ? pot_data : pot_mc);
    opt.beamline = el.beamline_label();

    const std::string weight = mc_weight.empty() ? "w_nominal" : mc_weight;

    std::vector<std::unique_ptr<TH1D>> owned_fine_hists;
    std::vector<const TH1D *> fine_hists;
    owned_fine_hists.reserve(3);
    fine_hists.reserve(3);

    const auto add_fine_hist = [&](ROOT::RDF::RNode node, const std::string &name) {
        auto h = node.Histo1D({name.c_str(), "", nbins, xmin, xmax}, expr, weight);
        std::unique_ptr<TH1D> h_clone(static_cast<TH1D *>(h->Clone((name + "_clone").c_str())));
        h_clone->SetDirectory(nullptr);
        fine_hists.push_back(h_clone.get());
        owned_fine_hists.push_back(std::move(h_clone));
    };

    add_fine_hist(e_mc.selection.nominal.node, "h_reco_vtx_axis_mc");
    add_fine_hist(e_ext.selection.nominal.node, "h_reco_vtx_axis_ext");
    add_fine_hist(e_data.selection.nominal.node, "h_reco_vtx_axis_data");

    int axis_nbins = nbins;
    double axis_xmin = xmin;
    double axis_xmax = xmax;

    const auto cfg = AdaptiveBinningService::config_from(opt);
    auto h_sum = AdaptiveBinningService::instance().sum_hists(
        fine_hists,
        "h_reco_vtx_axis_sum",
        cfg.fold_overflow);

    if (h_sum)
    {
        const auto edges = AdaptiveBinningService::instance().edges_min_stat(*h_sum, cfg);
        if (edges.size() >= 2)
        {
            axis_nbins = static_cast<int>(edges.size()) - 1;
            axis_xmin = edges.front();
            axis_xmax = edges.back();
        }
    }

    std::string draw_expr = expr;
    if (axis_nbins > 0)
    {
        const double bin_width = (axis_xmax - axis_xmin) / static_cast<double>(axis_nbins);
        axis_xmax += 2.0 * bin_width;
        axis_nbins += 2;
        draw_expr = "(" + expr + ") + " + std::to_string(bin_width);
    }

    TH1DModel spec = make_spec(expr, axis_nbins, axis_xmin, axis_xmax, weight);
    spec.expr = draw_expr;
    spec.sel = Preset::Empty;

    plotter.draw_stack(spec, mc, data);
    return 0;
}


int plotStackedHist(const std::string &samples_tsv = "",
                    const std::string &extra_sel = "true",
                    bool use_logy = false)
{
    const int nbins = 50;
    const std::string mc_weight = "w_nominal";

    int status = plot_stacked_hist_impl("reco_neutrino_vertex_sce_z",
                                        samples_tsv,
                                        nbins,
                                        0.0,
                                        1000.0,
                                        mc_weight,
                                        extra_sel,
                                        use_logy);
    if (status != 0)
        return status;

    status = plot_stacked_hist_impl("reco_neutrino_vertex_sce_x",
                                    samples_tsv,
                                    nbins,
                                    0.0,
                                    250.0,
                                    mc_weight,
                                    extra_sel,
                                    use_logy);
    if (status != 0)
        return status;

    return plot_stacked_hist_impl("reco_neutrino_vertex_sce_y",
                                  samples_tsv,
                                  nbins,
                                  -150.0,
                                  150.0,
                                  mc_weight,
                                  extra_sel,
                                  use_logy);
}
