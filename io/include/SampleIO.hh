/* -- C++ -- */
/**
 *  @file  io/include/SampleIO.hh
 *
 *  @brief Sample data structures and ROOT IO helpers.
 */

#ifndef NUXSEC_IO_SAMPLE_IO_H
#define NUXSEC_IO_SAMPLE_IO_H

#include <string>
#include <vector>

namespace nuxsec
{

namespace sample
{

class SampleIO
{
  public:
    enum class SampleKind
    {
        kUnknown = 0,
        kData,
        kEXT,
        kOverlay,
        kDirt,
        kStrangeness
    };

    enum class BeamMode
    {
        kUnknown = 0,
        kNuMI,
        kBNB
    };

    struct SampleFragment
    {
        std::string fragment_name;
        std::string artio_path;

        double subrun_pot_sum = 0.0;
        double db_tortgt_pot = 0.0;
        double db_tor101_pot = 0.0;

        double normalisation = 1.0;
        double normalised_pot_sum = 0.0;
    };

    struct Sample
    {
        std::string sample_name;
        SampleKind kind = SampleKind::kUnknown;
        BeamMode beam = BeamMode::kUnknown;

        std::vector<SampleFragment> fragments;

        double subrun_pot_sum = 0.0;
        double db_tortgt_pot_sum = 0.0;
        double db_tor101_pot_sum = 0.0;

        double normalisation = 1.0;
        double normalised_pot_sum = 0.0;
    };

    static const char *SampleKindName(SampleKind k);
    static SampleKind ParseSampleKind(const std::string &name);

    static const char *BeamModeName(BeamMode b);
    static BeamMode ParseBeamMode(const std::string &name);

    static void Write(const Sample &sample, const std::string &out_file);
    static Sample Read(const std::string &in_file);
};

} // namespace sample

} // namespace nuxsec

#endif // NUXSEC_IO_SAMPLE_IO_H
