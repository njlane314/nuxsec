/* -- C++ -- */
/**
 *  @file  sample/src/SampleAggregator.cc
 *
 *  @brief Implementation for SampleAggregator helpers.
 */

#include "SampleAggregator.hh"

#include <stdexcept>
#include <utility>

namespace nuxsec
{

Sample SampleAggregator::aggregate(const std::string &sample_name,
                                  const std::vector<std::string> &artio_files)
{
    if (artio_files.empty())
    {
        throw std::runtime_error("Sample aggregation requires at least one ArtIO file.");
    }

    Sample out;
    out.sample_name = sample_name;

    for (const auto &path : artio_files)
    {
        ArtFileProvenance prov = ArtFileProvenanceRootIO::read(path);
        if (out.fragments.empty())
        {
            out.kind = prov.kind;
            out.beam = prov.beam;
        }
        else
        {
            if (prov.kind != out.kind)
            {
                throw std::runtime_error("Sample kind mismatch in ArtIO file: " + path);
            }
            if (prov.beam != out.beam)
            {
                throw std::runtime_error("Beam mode mismatch in ArtIO file: " + path);
            }
        }

        SampleFragment fragment = makeFragment(prov, path);
        out.subrun_pot_sum += fragment.subrun_pot_sum;
        out.db_tortgt_pot_sum += fragment.db_tortgt_pot;
        out.db_tor101_pot_sum += fragment.db_tor101_pot;
        out.fragments.push_back(std::move(fragment));
    }

    out.normalisation = computeNormalisation(out.subrun_pot_sum, out.db_tortgt_pot_sum);
    out.normalised_pot_sum = out.subrun_pot_sum * out.normalisation;

    return out;
}

double SampleAggregator::computeNormalisation(double subrun_pot_sum, double db_tortgt_pot)
{
    if (subrun_pot_sum <= 0.0)
    {
        return 1.0;
    }
    if (db_tortgt_pot <= 0.0)
    {
        return 1.0;
    }
    return db_tortgt_pot / subrun_pot_sum;
}

SampleFragment SampleAggregator::makeFragment(const ArtFileProvenance &prov, const std::string &artio_path)
{
    SampleFragment fragment;
    fragment.fragment_name = prov.cfg.stage_name;
    fragment.artio_path = artio_path;
    fragment.subrun_pot_sum = prov.subrun.pot_sum;
    fragment.db_tortgt_pot = prov.db_tortgt_pot;
    fragment.db_tor101_pot = prov.db_tor101_pot;
    fragment.normalisation = computeNormalisation(fragment.subrun_pot_sum, fragment.db_tortgt_pot);
    fragment.normalised_pot_sum = fragment.subrun_pot_sum * fragment.normalisation;
    return fragment;
}

} // namespace nuxsec
