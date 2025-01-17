// This file is part of KWIVER, and is distributed under the
// OSI-approved BSD 3-Clause License. See top-level LICENSE file or
// https://github.com/Kitware/kwiver/blob/master/LICENSE for details.

/// \file
/// \brief Declaration of KLV demuxer.

#include "klv_timeline.h"

namespace kwiver {

namespace arrows {

namespace klv {

// ----------------------------------------------------------------------------
/// Holds state for the process of assembling a \c klv_timeline from a
/// sequence of \c klv_packet.
class KWIVER_ALGO_KLV_EXPORT klv_demuxer
{
public:
  using interval_t = typename klv_timeline::interval_t;
  using interval_map_t = typename klv_timeline::interval_map_t;

  /// \param timeline KLV timeline to modify.
  explicit klv_demuxer( klv_timeline& timeline );

  /// Incorporate next \p packet into the timeline.
  void demux_packet( klv_packet const& packet );

  /// Move the current time to \p timestamp.
  ///
  /// Data in the timeline ahead of \p timestamp is not deleted, but loses its
  /// guarantee of correctness: older packets may overwrite newer data.
  void seek( uint64_t timestamp );

  /// Return the timeline being modified.
  klv_timeline&
  timeline() const;

private:
  using key_t = typename klv_timeline::key_t;

  void demux_unknown( klv_packet const& packet );

  void demux_0104( klv_universal_set const& value );

  void demux_0601( klv_local_set const& value );

  void demux_1108( klv_local_set const& value );

  void demux_single_entry( klv_top_level_tag standard,
                           klv_lds_key tag,
                           uint64_t index,
                           interval_t const& time_interval,
                           klv_value const& value );

  bool check_timestamp( uint64_t timestamp ) const;

  uint64_t m_last_timestamp;
  std::map< klv_uds_key, uint64_t > m_unknown_key_indices;
  klv_timeline& m_timeline;
};

} // namespace klv

} // namespace arrows

} // namespace kwiver
