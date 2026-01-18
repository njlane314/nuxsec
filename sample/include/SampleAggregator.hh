/* -- C++ -- */
/**
 *  @file  sample/include/SampleAggregator.hh
 *
 *  @brief Aggregate Art file provenance into sample fragments.
 */

#ifndef Nuxsec_SAMPLE_SAMPLE_AGGREGATOR_H_INCLUDED
#define Nuxsec_SAMPLE_SAMPLE_AGGREGATOR_H_INCLUDED

#include <string>
#include <vector>

#include "ArtFileProvenanceRootIO.hh"
#include "Sample.hh"

namespace nuxsec
{

class SampleAggregator
{
  public:
    static Sample aggregate(const std::string &sample_name, const std::vector<std::string> &artio_files);

  private:
    static double computeNormalisation(double subrun_pot_sum, double db_tortgt_pot);
    static SampleFragment makeFragment(const ArtFileProvenance &prov, const std::string &artio_path);
};

} // namespace nuxsec

#endif // Nuxsec_SAMPLE_SAMPLE_AGGREGATOR_H_INCLUDED
