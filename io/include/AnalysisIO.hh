/* -- C++ -- */
/**
 *  @file  io/include/AnalysisIO.hh
 *
 *  @brief File-backed analysis IO helpers.
 */

#ifndef NUXSEC_IO_ANALYSISIO_H
#define NUXSEC_IO_ANALYSISIO_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <TH1.h> // ensures TH1 is complete for std::unique_ptr destruction in user TUs

namespace nuxsec
{
namespace io
{

struct AnalysisHeader
{
    std::string schema;
    std::string analysis_name;
    std::string analysis_tree;
    std::string created_utc;
    std::string sample_list_source;
};

struct AnalysisSampleRef
{
    std::string sample_name;
    std::string sample_rootio_path;

    int sample_kind = -1;
    int beam_mode = -1;

    double subrun_pot_sum = 0.0;
    double db_tortgt_pot_sum = 0.0;
    double db_tor101_pot_sum = 0.0;
};

class AnalysisIO
{
  public:
    enum class OpenMode
    {
        kRead,
        kUpdate
    };

    static constexpr const char *kSchema = "nuxsec_analysisio_v1";
    static constexpr const char *kFamilyTemplates1D = "templates1d";

    explicit AnalysisIO(std::string analysis_root, OpenMode mode = OpenMode::kRead);
    ~AnalysisIO();

    AnalysisIO(const AnalysisIO &) = delete;
    AnalysisIO &operator=(const AnalysisIO &) = delete;

    AnalysisIO(AnalysisIO &&) noexcept;
    AnalysisIO &operator=(AnalysisIO &&) noexcept;

    const std::string &path() const noexcept;

    const AnalysisHeader &header() const;

    std::vector<AnalysisSampleRef> samples() const;

    std::string template_specs_1d_tsv() const;

    std::unique_ptr<TH1> get_hist1d(const std::string &family,
                                    const std::string &sample_name,
                                    const std::string &hist_name) const;

    void put_histograms(const std::string &family,
                        const std::string &sample_name,
                        const std::vector<std::pair<std::string, const TH1 *>> &hists);

    void flush();

    static void init(const std::string &analysis_root,
                     const AnalysisHeader &header,
                     const std::vector<AnalysisSampleRef> &samples,
                     const std::string &template_specs_1d_tsv,
                     const std::string &template_specs_source = "compiled");

  private:
    class Impl;
    std::unique_ptr<Impl> m;
};

} // namespace io
} // namespace nuxsec

#endif // NUXSEC_IO_ANALYSISIO_H
