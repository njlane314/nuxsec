// plot/macro/plotMinimal.C
//
// Minimal plotting macro for muon-neutrino selection stages, using
// EventListIO + StackedHist library classes.
//
// Usage:
//   heron --set template macro plotMinimal.C
//   heron --set template macro plotMinimal.C \
//     'plotMinimal("./scratch/out/template/event/events.root")'

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>

#include "framework/modules/io/include/EventListIO.hh"
#include "framework/modules/plot/include/StackedHist.hh"
#include "macros/include/MacroIO.hh"

bool plotMinimal(const char* event_list_root = "./scratch/out/template/event/events.root",
                 const char* output_name = "plotMinimal") {
  if (!event_list_root || !output_name) {
    std::cerr << "plotMinimal: invalid NULL argument" << std::endl;
    return false;
  }

  if (!heron::macro::validate_root_input_path(event_list_root)) {
    std::cerr << "plotMinimal: invalid input ROOT file path" << std::endl;
    return false;
  }

  std::unique_ptr<nu::EventListIO> p_event_list;
  try {
    p_event_list = std::make_unique<nu::EventListIO>(event_list_root, nu::EventListIO::OpenMode::kRead);
  } catch (const std::exception& e) {
    std::cerr << "plotMinimal: failed to open event list: " << e.what() << std::endl;
    return false;
  }

  nu::TH1DModel spec;
  spec.id = output_name;
  spec.name = "Muon neutrino selection stage";
  spec.title = "Muon neutrino selection stage";
  spec.expr = "sel_muon ? 5 : (sel_topology ? 4 : (sel_fiducial ? 3 : (sel_slice ? 2 : (sel_trigger ? 1 : 0))))";
  spec.nbins = 6;
  spec.xmin = -0.5;
  spec.xmax = 5.5;
  spec.sel = Preset::Empty;

  nu::Options opt;
  opt.out_dir = std::filesystem::path(output_name).parent_path().string();
  if (opt.out_dir.empty()) opt.out_dir = ".";
  opt.image_format = "pdf";
  opt.show_ratio = false;
  opt.show_ratio_band = false;
  opt.normalise_by_bin_width = false;
  opt.x_title = "Selection stage (0=All, 1=Trigger, 2=Slice, 3=Fiducial, 4=Topology, 5=Muon)";
  opt.y_title = "Entries";

  nu::StackedHist plot(spec, opt, *p_event_list);
  plot.draw_and_save(opt.image_format);

  std::cout << "plotMinimal: wrote " << output_name << ".pdf" << std::endl;
  return true;
}
