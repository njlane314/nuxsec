/* -- C++ -- */
/// \file macros/macro/include/MacroStyle.hh
/// \brief Shared plotting style helpers for ROOT macros.

#ifndef heron_macro_MACROSTYLE_H
#define heron_macro_MACROSTYLE_H

#include <TStyle.h>

namespace heron {
namespace macro {

/// \brief Apply a consistent default plotting style across macros.
inline void apply_default_plot_style()
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);
  gStyle->SetTitleFont(42, "XYZ");
  gStyle->SetLabelFont(42, "XYZ");
  gStyle->SetTitleSize(0.050, "XYZ");
  gStyle->SetLabelSize(0.045, "XYZ");
  gStyle->SetTitleOffset(1.10, "X");
  gStyle->SetTitleOffset(1.35, "Y");
  gStyle->SetPadLeftMargin(0.15);
  gStyle->SetPadBottomMargin(0.12);
  gStyle->SetPadTopMargin(0.10);
  gStyle->SetPadRightMargin(0.12);
  gStyle->SetLegendBorderSize(0);
  gStyle->SetLineWidth(2);
  gStyle->SetPadTickX(1);
  gStyle->SetPadTickY(1);
}

} // namespace macro
} // namespace heron

#endif
