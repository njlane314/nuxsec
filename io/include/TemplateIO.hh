/* -- C++ -- */
/**
 *  @file  io/include/TemplateIO.hh
 *
 *  @brief Collie-style template container with ROOT IO helpers.
 */

#ifndef NUXSEC_IO_TEMPLATE_IO_H
#define NUXSEC_IO_TEMPLATE_IO_H

#include <TNamed.h>

#include <string>
#include <utility>
#include <vector>

class TH1;
class TH2;

namespace nuxsec
{

struct TemplateWriteOptions
{
    std::string top_dir = "nuxsec/results";
    bool overwrite = true;
};

/** \brief CollieDistribution-like template representation for NuXSec with IO helpers. */
class TemplateIO : public TNamed
{
  public:
    static const double kIgnoreExposureScale;

    TemplateIO();
    TemplateIO(const char *name, int n_x, double x_min, double x_max,
               int n_y = 1, double y_min = 0, double y_max = 1);

    TemplateIO(const TemplateIO &rhs);
    TemplateIO &operator=(const TemplateIO &rhs);
    ~TemplateIO() override;

    int n_x() const { return m_nx; }
    int n_y() const { return m_ny; }
    int n_bins() const { return m_true_bin_count; }

    double x_min() const { return m_x_min; }
    double x_max() const { return m_x_max; }
    double y_min() const { return m_y_min; }
    double y_max() const { return m_y_max; }

    double bin_fraction(int ix, int iy = -1) const;
    double bin_yield(int ix, int iy = -1) const;
    double bin_stat_error(int ix, int iy = -1) const;

    double total_yield() const { return m_total_yield; }
    void set_total_yield(double yield);

    void set_bin_fraction(double fraction, int ix, int iy = -1);
    void set_bin_stat_error(double error, int ix, int iy = -1);

    bool fill_from_th1(const TH1 *hist, int rebin = 0);
    bool fill_from_th2(const TH2 *hist, int rebin_x = 0, int rebin_y = 0);

    int n_syst() const { return static_cast<int>(m_syst_names.size()); }
    const std::string &syst_name(int index) const { return m_syst_names.at(index); }

    bool has_systematic(const std::string &name) const;
    int syst_index(const std::string &name) const;

    void add_systematic_frac(const char *name, const TH1 *pos_frac, const TH1 *neg_frac);
    void add_systematic_frac_2d(const char *name, const TH2 *pos_frac, const TH2 *neg_frac);

    bool get_float_flag(const std::string &name) const;
    bool get_log_normal_flag(const std::string &name) const;
    void set_float_flag(const std::string &name, bool on);
    void set_log_normal_flag(const std::string &name, bool on);

    void linearise(const std::vector<std::string> &global_syst_order);
    bool is_linearised() const { return m_linearised; }

    double bin_fraction_varied(int ix, int iy, const double *pulls) const;

    void prepare_exclusion_sums();

    TH1 *make_th1(const std::string &title = "") const;
    TH2 *make_th2(const std::string &title = "") const;

    static void write_histograms(const std::string &root_path,
                                 const std::string &sample_name,
                                 const std::vector<std::pair<std::string, const TH1 *>> &hists,
                                 const TemplateWriteOptions &opt = {});

    static void write_string_meta(const std::string &root_path,
                                  const std::string &sample_name,
                                  const std::string &key,
                                  const std::string &value,
                                  const TemplateWriteOptions &opt = {});

    static void write_double_meta(const std::string &root_path,
                                  const std::string &sample_name,
                                  const std::string &key,
                                  double value,
                                  const TemplateWriteOptions &opt = {});

    static void write_syst_histograms(const std::string &root_path,
                                      const std::string &sample_name,
                                      const std::string &syst_name,
                                      const std::string &variation,
                                      const std::vector<std::pair<std::string, const TH1 *>> &hists,
                                      const TemplateWriteOptions &opt = {});

    static void write_syst_flag_meta(const std::string &root_path,
                                     const std::string &syst_name,
                                     const std::string &key,
                                     const std::string &value,
                                     const TemplateWriteOptions &opt = {});

  private:
    struct TemplateSystBin
    {
        double sigma_pos = 0.0;
        double sigma_neg = 0.0;
        bool asym = false;

        double base_delta_efficiency = 0.0;
        double exclusion_sum = 0.0;

    };

    int index(int ix, int iy) const
    {
        return (m_ny > 1) ? (ix + iy * m_nx) : ix;
    }

    static inline double asym_delta(double pull, double sigma_pos, double sigma_neg)
    {
        return (pull >= 0.0) ? (pull * sigma_pos) : (pull * sigma_neg);
    }

    bool check_stats(const TH1 *hist, int verbose = 1) const;

  private:
    bool m_mutable = false;

    int m_nx = 1;
    int m_ny = 1;
    int m_true_bin_count = 1;
    double m_x_min = -1.0;
    double m_x_max = -1.0;
    double m_y_min = -1.0;
    double m_y_max = -1.0;

    double m_total_yield = 0.0;
    std::vector<double> m_bin_fraction;
    std::vector<double> m_bin_stat;

    std::vector<std::string> m_syst_names;
    std::vector<std::vector<TemplateSystBin>> m_syst;
    std::vector<unsigned char> m_float_flag;
    std::vector<unsigned char> m_log_normal_flag;

    std::vector<int> m_syst_index_outer;
    std::vector<int> m_syst_index_inner;

    bool m_linearised = false;
    std::vector<double> m_lin_bin_fraction;
    std::vector<double> m_lin_bin_stat;
    std::vector<double> m_efficiency_sums;

};

} // namespace nuxsec

#endif // NUXSEC_IO_TEMPLATE_IO_H
