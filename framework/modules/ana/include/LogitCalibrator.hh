/* -- C++ -- */
#ifndef heron_ANA_LOGITCALIBRATOR_H
#define heron_ANA_LOGITCALIBRATOR_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "TDirectory.h"
#include "TFile.h"
#include "TParameter.h"
#include "TTree.h"
#include "TTreeFormula.h"
#include "TVectorD.h"

namespace heron {

/** \brief Calibrate a raw CNN logit with none, Platt, or isotonic methods. */
class LogitCalibrator {
 public:
  enum class Method : int { kNone = 0, kPlatt = 1, kIsotonic = 2 };

  LogitCalibrator() = default;

  /// Get configured calibration method.
  Method get_method() const { return method_; }

  /// Set calibration method.
  void set_method(Method m) { method_ = m; }

  /// Get effective signal fraction in calibration sample.
  double get_pi_fit() const { return pi_fit_; }

  /// Set effective signal fraction in calibration sample.
  void set_pi_fit(double pi) { pi_fit_ = clamp_prob_01(pi); }

  /// Get Platt slope parameter.
  double get_a() const { return a_; }

  /// Get Platt offset parameter.
  double get_b() const { return b_; }

  /// Configure Platt parameters and fit prior.
  void set_platt(double a, double b, double pi_fit = 0.5) {
    a_ = a;
    b_ = b;
    pi_fit_ = clamp_prob_01(pi_fit);
    method_ = Method::kPlatt;
  }

  /// Get isotonic bin edges.
  const std::vector<double>& get_edges() const { return edges_; }

  /// Get isotonic bin values.
  const std::vector<double>& get_values() const { return values_; }

  /// Configure isotonic piecewise-constant mapping.
  void set_isotonic_mapping(std::vector<double> edges,
                            std::vector<double> values,
                            double pi_fit = 0.5) {
    if (edges.size() != values.size() + 1) {
      throw std::runtime_error("set_isotonic_mapping: edges.size() must be values.size()+1");
    }
    if (!std::is_sorted(edges.begin(), edges.end())) {
      throw std::runtime_error("set_isotonic_mapping: edges must be sorted ascending");
    }

    edges_ = std::move(edges);
    values_ = std::move(values);
    for (double& p : values_) {
      p = clamp_prob(p);
    }
    pi_fit_ = clamp_prob_01(pi_fit);
    method_ = Method::kIsotonic;
  }

  /// Posterior probability p(y=1|x) under calibration prior pi_fit.
  double prob(double raw_logit) const {
    switch (method_) {
      case Method::kNone: {
        const double log_odds = raw_logit + prior_log_odds(pi_fit_);
        return sigmoid(log_odds);
      }
      case Method::kPlatt:
        return sigmoid(a_ * raw_logit + b_);
      case Method::kIsotonic: {
        if (values_.empty()) {
          throw std::runtime_error("prob: isotonic mapping is empty");
        }
        const int idx = find_isotonic_bin(raw_logit);
        return values_.at(static_cast<size_t>(idx));
      }
      default:
        throw std::runtime_error("prob: unknown method");
    }
  }

  /// Log-odds under calibration prior.
  double log_odds(double raw_logit) const {
    switch (method_) {
      case Method::kNone:
        return raw_logit + prior_log_odds(pi_fit_);
      case Method::kPlatt:
        return a_ * raw_logit + b_;
      case Method::kIsotonic:
        return logit(prob(raw_logit));
      default:
        throw std::runtime_error("log_odds: unknown method");
    }
  }

  /// Prior-independent calibrated LLR.
  double llr(double raw_logit) const {
    return log_odds(raw_logit) - prior_log_odds(pi_fit_);
  }

  /// Posterior for an arbitrary target prior.
  double posterior(double raw_logit, double pi_target) const {
    const double llr_val = llr(raw_logit);
    const double log_odds_target = llr_val + prior_log_odds(clamp_prob_01(pi_target));
    return sigmoid(log_odds_target);
  }

  /// Target-prior calibrated log-odds.
  double log_odds_target(double raw_logit, double pi_target) const {
    const double llr_val = llr(raw_logit);
    return llr_val + prior_log_odds(clamp_prob_01(pi_target));
  }

  /// Fit Platt scaling from vectors.
  void fit_platt(const std::vector<double>& x,
                 const std::vector<int>& y,
                 const std::vector<double>* w = NULL,
                 int max_iter = 60,
                 double tol = 1e-10,
                 double l2 = 0.0) {
    check_inputs(x, y, w);
    compute_pi_fit(y, w);

    double a = 1.0;
    double b = prior_log_odds(pi_fit_);

    auto nll = [&](double a_val, double b_val) -> double {
      double nll_value = 0.0;
      for (size_t i = 0; i < x.size(); ++i) {
        const double wi = (w ? (*w)[i] : 1.0);
        const double z = a_val * x[i] + b_val;
        const double p = clamp_prob(sigmoid(z));
        const int yi = (y[i] != 0) ? 1 : 0;
        nll_value -= wi * (yi * std::log(p) + (1 - yi) * std::log(1.0 - p));
      }
      if (l2 > 0.0) {
        nll_value += 0.5 * l2 * (a_val * a_val + b_val * b_val);
      }
      return nll_value;
    };

    double prev = nll(a, b);

    for (int iter = 0; iter < max_iter; ++iter) {
      double ga = 0.0;
      double gb = 0.0;
      double haa = 0.0;
      double hab = 0.0;
      double hbb = 0.0;

      for (size_t i = 0; i < x.size(); ++i) {
        const double wi = (w ? (*w)[i] : 1.0);
        const double z = a * x[i] + b;
        const double p = clamp_prob(sigmoid(z), 1e-12);
        const double s = p * (1.0 - p);
        const double yi = (y[i] != 0) ? 1.0 : 0.0;
        const double r = p - yi;

        ga += wi * r * x[i];
        gb += wi * r;

        haa += wi * s * x[i] * x[i];
        hab += wi * s * x[i];
        hbb += wi * s;
      }

      if (l2 > 0.0) {
        ga += l2 * a;
        gb += l2 * b;
        haa += l2;
        hbb += l2;
      }

      const double det = haa * hbb - hab * hab;
      if (!(det > 0.0) || !std::isfinite(det)) {
        break;
      }

      const double da = (hbb * ga - hab * gb) / det;
      const double db = (-hab * ga + haa * gb) / det;

      double step = 1.0;
      double a_try = a;
      double b_try = b;
      double n_try = prev;

      for (int ls = 0; ls < 25; ++ls) {
        a_try = a - step * da;
        b_try = b - step * db;
        n_try = nll(a_try, b_try);
        if (std::isfinite(n_try) && n_try <= prev) {
          break;
        }
        step *= 0.5;
      }

      const double max_update = std::max(std::abs(a_try - a), std::abs(b_try - b));
      a = a_try;
      b = b_try;
      prev = n_try;

      if (max_update < tol) {
        break;
      }
    }

    a_ = a;
    b_ = b;
    method_ = Method::kPlatt;
  }

  /// Fit isotonic calibration from vectors.
  void fit_isotonic(const std::vector<double>& x,
                    const std::vector<int>& y,
                    const std::vector<double>* w = NULL,
                    double min_prob = 1e-6,
                    double max_prob = 1.0 - 1e-6) {
    check_inputs(x, y, w);
    compute_pi_fit(y, w);

    if (!(min_prob > 0.0 && max_prob < 1.0 && min_prob < max_prob)) {
      throw std::runtime_error("fit_isotonic: invalid min_prob/max_prob");
    }

    struct Pt {
      double x;
      double y;
      double w;
    };

    std::vector<Pt> pts;
    pts.reserve(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
      const double wi = (w ? (*w)[i] : 1.0);
      pts.push_back(Pt{x[i], (y[i] != 0) ? 1.0 : 0.0, wi});
    }

    std::sort(pts.begin(), pts.end(), [](const Pt& a_val, const Pt& b_val) {
      return a_val.x < b_val.x;
    });

    struct Block {
      double w = 0.0;
      double wy = 0.0;
      double mean = 0.0;
      double xmin = 0.0;
      double xmax = 0.0;
    };

    std::vector<Block> blocks;
    blocks.reserve(pts.size());

    for (const auto& p : pts) {
      Block b;
      b.w = p.w;
      b.wy = p.w * p.y;
      b.mean = (b.w > 0.0) ? (b.wy / b.w) : 0.0;
      b.xmin = p.x;
      b.xmax = p.x;
      blocks.push_back(b);

      while (blocks.size() >= 2) {
        const Block& b2 = blocks.back();
        const Block& b1 = blocks[blocks.size() - 2];
        if (b1.mean <= b2.mean) {
          break;
        }

        Block merged;
        merged.w = b1.w + b2.w;
        merged.wy = b1.wy + b2.wy;
        merged.mean = (merged.w > 0.0) ? (merged.wy / merged.w) : 0.0;
        merged.xmin = b1.xmin;
        merged.xmax = b2.xmax;

        blocks.pop_back();
        blocks.pop_back();
        blocks.push_back(merged);
      }
    }

    const int n = static_cast<int>(blocks.size());
    if (n <= 0) {
      throw std::runtime_error("fit_isotonic: no blocks produced");
    }

    std::vector<double> edges;
    std::vector<double> vals;
    edges.resize(static_cast<size_t>(n + 1));
    vals.resize(static_cast<size_t>(n));

    const double neg = -1e300;
    const double pos = 1e300;
    edges[0] = neg;

    for (int i = 0; i < n; ++i) {
      double p = blocks[static_cast<size_t>(i)].mean;
      p = std::max(min_prob, std::min(max_prob, p));
      vals[static_cast<size_t>(i)] = p;

      if (i < (n - 1)) {
        const double boundary = 0.5 *
            (blocks[static_cast<size_t>(i)].xmax + blocks[static_cast<size_t>(i + 1)].xmin);
        edges[static_cast<size_t>(i + 1)] = boundary;
      }
    }

    edges[static_cast<size_t>(n)] = pos;

    edges_ = std::move(edges);
    values_ = std::move(vals);
    method_ = Method::kIsotonic;
  }

  /// Fit Platt scaling from tree expressions.
  void fit_platt(TTree* t,
                 const char* logit_expr,
                 const char* label_expr,
                 const char* weight_expr = NULL,
                 Long64_t max_entries = -1,
                 int max_iter = 60,
                 double tol = 1e-10,
                 double l2 = 0.0) {
    std::vector<double> x;
    std::vector<int> y;
    std::vector<double> w;
    read_tree_columns(t, logit_expr, label_expr, weight_expr, max_entries,
                      x, y, ((weight_expr && std::strlen(weight_expr)) ? &w : NULL));
    fit_platt(x, y, (w.empty() ? NULL : &w), max_iter, tol, l2);
  }

  /// Fit isotonic calibration from tree expressions.
  void fit_isotonic(TTree* t,
                    const char* logit_expr,
                    const char* label_expr,
                    const char* weight_expr = NULL,
                    Long64_t max_entries = -1,
                    double min_prob = 1e-6,
                    double max_prob = 1.0 - 1e-6) {
    std::vector<double> x;
    std::vector<int> y;
    std::vector<double> w;
    read_tree_columns(t, logit_expr, label_expr, weight_expr, max_entries,
                      x, y, ((weight_expr && std::strlen(weight_expr)) ? &w : NULL));
    fit_isotonic(x, y, (w.empty() ? NULL : &w), min_prob, max_prob);
  }

  /// Save calibration parameters to ROOT file.
  void save_to_root(const char* filename, const char* dir_name = "calib") const {
    if (!filename || !dir_name) {
      throw std::runtime_error("save_to_root: null filename/dir_name");
    }

    TFile f(filename, "RECREATE");
    if (f.IsZombie()) {
      throw std::runtime_error(std::string("save_to_root: cannot open ") + filename);
    }

    TDirectory* d = f.mkdir(dir_name);
    if (!d) {
      throw std::runtime_error("save_to_root: cannot create directory");
    }

    d->cd();

    TParameter<int> p_method("method", static_cast<int>(method_));
    TParameter<double> p_a("A", a_);
    TParameter<double> p_b("B", b_);
    TParameter<double> p_pi("pi_fit", pi_fit_);

    p_method.Write();
    p_a.Write();
    p_b.Write();
    p_pi.Write();

    TVectorD v_edges(static_cast<int>(edges_.size()));
    TVectorD v_vals(static_cast<int>(values_.size()));
    for (int i = 0; i < v_edges.GetNrows(); ++i) {
      v_edges[i] = edges_[static_cast<size_t>(i)];
    }
    for (int i = 0; i < v_vals.GetNrows(); ++i) {
      v_vals[i] = values_[static_cast<size_t>(i)];
    }

    v_edges.Write("edges");
    v_vals.Write("values");

    f.Write();
    f.Close();
  }

  /// Load calibration parameters from ROOT file.
  static LogitCalibrator load_from_root(const char* filename, const char* dir_name = "calib") {
    if (!filename || !dir_name) {
      throw std::runtime_error("load_from_root: null filename/dir_name");
    }

    TFile f(filename, "READ");
    if (f.IsZombie()) {
      throw std::runtime_error(std::string("load_from_root: cannot open ") + filename);
    }

    TDirectory* d = dynamic_cast<TDirectory*>(f.Get(dir_name));
    if (!d) {
      throw std::runtime_error(std::string("load_from_root: missing directory ") + dir_name);
    }

    TParameter<int>* p_method = dynamic_cast<TParameter<int>*>(d->Get("method"));
    TParameter<double>* p_a = dynamic_cast<TParameter<double>*>(d->Get("A"));
    TParameter<double>* p_b = dynamic_cast<TParameter<double>*>(d->Get("B"));
    TParameter<double>* p_pi = dynamic_cast<TParameter<double>*>(d->Get("pi_fit"));
    TVectorD* v_edges = dynamic_cast<TVectorD*>(d->Get("edges"));
    TVectorD* v_vals = dynamic_cast<TVectorD*>(d->Get("values"));

    if (!p_method || !p_a || !p_b || !p_pi) {
      throw std::runtime_error("load_from_root: missing required parameters (method/A/B/pi_fit)");
    }

    LogitCalibrator c;
    c.method_ = static_cast<Method>(p_method->GetVal());
    c.a_ = p_a->GetVal();
    c.b_ = p_b->GetVal();
    c.pi_fit_ = clamp_prob_01(p_pi->GetVal());

    if (v_edges && v_vals) {
      c.edges_.resize(static_cast<size_t>(v_edges->GetNrows()));
      c.values_.resize(static_cast<size_t>(v_vals->GetNrows()));
      for (int i = 0; i < v_edges->GetNrows(); ++i) {
        c.edges_[static_cast<size_t>(i)] = (*v_edges)[i];
      }
      for (int i = 0; i < v_vals->GetNrows(); ++i) {
        c.values_[static_cast<size_t>(i)] = clamp_prob((*v_vals)[i]);
      }
    }

    f.Close();
    return c;
  }

 private:
  Method method_{Method::kNone};
  double a_{1.0};
  double b_{0.0};
  double pi_fit_{0.5};
  std::vector<double> edges_;
  std::vector<double> values_;

  static double sigmoid(double z) {
    if (z >= 0.0) {
      const double ez = std::exp(-z);
      return 1.0 / (1.0 + ez);
    }

    const double ez = std::exp(z);
    return ez / (1.0 + ez);
  }

  static double clamp_prob_01(double pi, double eps = 1e-12) {
    if (!std::isfinite(pi)) {
      return 0.5;
    }
    if (pi < eps) {
      return eps;
    }
    if (pi > (1.0 - eps)) {
      return 1.0 - eps;
    }
    return pi;
  }

  static double clamp_prob(double p, double eps = 1e-15) {
    if (!std::isfinite(p)) {
      return 0.5;
    }
    if (p < eps) {
      return eps;
    }
    if (p > (1.0 - eps)) {
      return 1.0 - eps;
    }
    return p;
  }

  static double prior_log_odds(double pi) {
    pi = clamp_prob_01(pi);
    return std::log(pi) - std::log1p(-pi);
  }

  static double logit(double p) {
    p = clamp_prob(p);
    return std::log(p) - std::log1p(-p);
  }

  int find_isotonic_bin(double x) const {
    auto it = std::upper_bound(edges_.begin(), edges_.end(), x);
    int idx = static_cast<int>(std::distance(edges_.begin(), it)) - 1;
    if (idx < 0) {
      idx = 0;
    }

    const int n = static_cast<int>(values_.size());
    if (idx >= n) {
      idx = n - 1;
    }
    return idx;
  }

  static void check_inputs(const std::vector<double>& x,
                           const std::vector<int>& y,
                           const std::vector<double>* w) {
    if (x.empty()) {
      throw std::runtime_error("check_inputs: empty x");
    }
    if (x.size() != y.size()) {
      throw std::runtime_error("check_inputs: x/y size mismatch");
    }
    if (w && (w->size() != x.size())) {
      throw std::runtime_error("check_inputs: x/w size mismatch");
    }

    if (w) {
      for (double wi : *w) {
        if (!std::isfinite(wi) || wi <= 0.0) {
          throw std::runtime_error("check_inputs: weights must be finite and > 0");
        }
      }
    }
  }

  void compute_pi_fit(const std::vector<int>& y, const std::vector<double>* w) {
    double sw = 0.0;
    double swy = 0.0;

    for (size_t i = 0; i < y.size(); ++i) {
      const double wi = (w ? (*w)[i] : 1.0);
      sw += wi;
      swy += wi * ((y[i] != 0) ? 1.0 : 0.0);
    }

    pi_fit_ = clamp_prob_01((sw > 0.0) ? (swy / sw) : 0.5);
  }

  static void read_tree_columns(TTree* t,
                                const char* x_expr,
                                const char* y_expr,
                                const char* w_expr,
                                Long64_t max_entries,
                                std::vector<double>& x_out,
                                std::vector<int>& y_out,
                                std::vector<double>* w_out) {
    if (!t) {
      throw std::runtime_error("read_tree_columns: null TTree");
    }
    if (!x_expr || !y_expr) {
      throw std::runtime_error("read_tree_columns: null expressions");
    }

    TTreeFormula fx("fx", x_expr, t);
    TTreeFormula fy("fy", y_expr, t);
    std::unique_ptr<TTreeFormula> fw;
    if (w_expr && std::strlen(w_expr) > 0) {
      fw = std::make_unique<TTreeFormula>("fw", w_expr, t);
    }

    const Long64_t n_all = t->GetEntries();
    const Long64_t n = ((max_entries >= 0) && (max_entries < n_all)) ? max_entries : n_all;

    x_out.clear();
    y_out.clear();
    if (w_out) {
      w_out->clear();
    }

    x_out.reserve(static_cast<size_t>(std::max<Long64_t>(0, n)));
    y_out.reserve(static_cast<size_t>(std::max<Long64_t>(0, n)));
    if (w_out) {
      w_out->reserve(static_cast<size_t>(std::max<Long64_t>(0, n)));
    }

    int current_tree_number = -1;

    for (Long64_t i = 0; i < n; ++i) {
      const Long64_t local = t->LoadTree(i);
      if (local < 0) {
        break;
      }

      if (t->GetTreeNumber() != current_tree_number) {
        current_tree_number = t->GetTreeNumber();
        fx.UpdateFormulaLeaves();
        fy.UpdateFormulaLeaves();
        if (fw) {
          fw->UpdateFormulaLeaves();
        }
      }

      t->GetEntry(i);

      const double xv = fx.EvalInstance();
      const double yv = fy.EvalInstance();
      const double wv = fw ? fw->EvalInstance() : 1.0;

      if (!std::isfinite(xv) || !std::isfinite(yv) || !std::isfinite(wv)) {
        continue;
      }
      if (wv <= 0.0) {
        continue;
      }

      x_out.push_back(xv);
      y_out.push_back((yv > 0.5) ? 1 : 0);
      if (w_out) {
        w_out->push_back(wv);
      }
    }

    if (x_out.empty()) {
      throw std::runtime_error("read_tree_columns: no usable entries read");
    }
  }
};

}  // namespace heron

#endif
