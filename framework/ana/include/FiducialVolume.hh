/* -- C++ -- */
/**
 *  @file  ana/include/FiducialVolume.hh
 *
 *  @brief Fiducial volume helpers and default bounds used by analysis models.
 */

#ifndef HERON_ANA_FIDUCIAL_VOLUME_H
#define HERON_ANA_FIDUCIAL_VOLUME_H

#include <ROOT/RDataFrame.hxx>

namespace heron
{

namespace fiducial
{

struct VolumeBounds
{
    float min_x;
    float max_x;
    float min_y;
    float max_y;
    float min_z;
    float max_z;
};

constexpr VolumeBounds k_default_active_volume = {5.f, 251.f, -110.f, 110.f, 20.f, 986.f};
constexpr VolumeBounds k_default_reco_volume_gap = {5.f, 251.f, -110.f, 110.f, 675.f, 775.f};

inline ROOT::RDF::RNode filter_on(ROOT::RDF::RNode node, const char *col)
{
    return node.Filter([](bool pass) { return pass; }, {col});
}

template <typename T>
bool is_within(const T &value, float low, float high)
{
    return value > low && value < high;
}

template <typename X, typename Y, typename Z>
bool is_in_volume(const X &x, const Y &y, const Z &z, const VolumeBounds &bounds)
{
    return is_within(x, bounds.min_x, bounds.max_x) &&
           is_within(y, bounds.min_y, bounds.max_y) &&
           is_within(z, bounds.min_z, bounds.max_z);
}

template <typename X, typename Y, typename Z>
bool is_in_active_volume(const X &x, const Y &y, const Z &z)
{
    return is_in_volume(x, y, z, k_default_active_volume);
}

template <typename X, typename Y, typename Z>
bool is_in_reco_volume_excluding_gap(const X &x, const Y &y, const Z &z)
{
    return is_in_active_volume(x, y, z) &&
           (z < k_default_reco_volume_gap.min_z || z > k_default_reco_volume_gap.max_z);
}

} // namespace fiducial

} // namespace heron

#endif // HERON_ANA_FIDUCIAL_VOLUME_H
