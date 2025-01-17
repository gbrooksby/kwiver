// This file is part of KWIVER, and is distributed under the
// OSI-approved BSD 3-Clause License. See top-level LICENSE file or
// https://github.com/Kitware/kwiver/blob/master/LICENSE for details.

#include "klv_0601.h"

#include "klv_checksum.h"

#include <vital/logger/logger.h>

#include <algorithm>
#include <iomanip>
#include <set>
#include <vector>

namespace kv = kwiver::vital;

namespace kwiver {

namespace arrows {

namespace klv {

namespace {

constexpr size_t checksum_packet_length = 4;
std::vector< uint8_t > const checksum_header = { KLV_0601_CHECKSUM, 2 };

} // namespace <anonymous>

// ----------------------------------------------------------------------------
klv_tag_traits_lookup const&
klv_0601_traits_lookup()
{
  // Constants here are taken from Section 8 of
  // https://gwg.nga.mil/misb/docs/standards/ST0601.17.pdf
  // Descriptions are edited for clarity, brevity, consistency, etc.
  static klv_tag_traits_lookup const lookup = {
#define ENUM_AND_NAME( X ) X, #X
    { {},
      ENUM_AND_NAME( KLV_0601_UNKNOWN ),
      std::make_shared< klv_blob_format >(),
      "Unknown Tag",
      "Unknown tag.",
      0 },
    { {},
      ENUM_AND_NAME( KLV_0601_CHECKSUM ),
      std::make_shared< klv_uint_format >( 2 ),
      "Checksum",
      "Checksum used to detect errors within a ST 0601 packet.",
      0 },
    { // No universal key for members of ST0601
      {},
      // KWIVER enum value
      ENUM_AND_NAME( KLV_0601_PRECISION_TIMESTAMP ),
      // KLV Format: uint64
      std::make_shared< klv_uint_format >( 8 ),
      // Item name
      "Precision Timestamp",
      // Item description
      "Timestamp for all metadata in a ST 0601 local set. Used to coordinate "
      "with Motion Imagery.",
      // Exactly one tag allowed (mandatory)
      1 },
    { {},
      ENUM_AND_NAME( KLV_0601_MISSION_ID ),
      std::make_shared< klv_string_format >(),
      "Mission ID",
      "Descriptive mission identifier to distinguish an event or sortie.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_TAIL_NUMBER ),
      std::make_shared< klv_string_format >(),
      "Platform Tail Number",
      "Identifier of platform as posted.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_HEADING_ANGLE ),
      std::make_shared< klv_uflint_format >( 0.0, 360.0, 2 ),
      "Platform Heading Angle",
      "Angle between longitudinal axis and true north measured in the "
      "horizontal plane. Angles increase in a clockwise direction when "
      "looking from above the platform. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_PITCH_ANGLE ),
      std::make_shared< klv_sflint_format >( -20.0, 20.0, 2 ),
      "Platform Pitch Angle",
      "Angle between longitudinal axis and horizonal plane. Positive angles "
      "above horizonal plane. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_ROLL_ANGLE ),
      std::make_shared< klv_sflint_format >( -50.0, 50.0, 2 ),
      "Platform Roll Angle",
      "Angle between transverse axis and transverse-longitudinal plane. "
      "Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_TRUE_AIRSPEED ),
      std::make_shared< klv_uflint_format >( 0.0, 255.0, 1 ),
      "Platform True Airspeed",
      "True airspeed of the platform: indicated airspeed adjusted for "
      "temperature and altitude. Measured in meters per second.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_INDICATED_AIRSPEED ),
      std::make_shared< klv_uflint_format >( 0.0, 255.0, 1 ),
      "Platform Indicated Airspeed",
      "Indicated airspeed of the platform. Derived from Pitot tube and static "
      "pressure sensors. Measured in meters per second.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_DESIGNATION ),
      std::make_shared< klv_string_format >(),
      "Platform Designation",
      "Model name for the platform. Examples: 'Predator', 'Reaper'.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_IMAGE_SOURCE_SENSOR ),
      std::make_shared< klv_string_format >(),
      "Image Source Sensor",
      "Name of the currently active sensor. Examples: 'EO Nose', 'TESAR "
      "Imagery'.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_IMAGE_COORDINATE_SYSTEM ),
      std::make_shared< klv_string_format >(),
      "Image Coordinate System",
      "Name of the image coordinate system used.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_LATITUDE ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Sensor Latitude",
      "Latitude of the currently active sensor, based on WGS84 ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_LONGITUDE ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Sensor Longitude",
      "Longitude of the currently active sensor, based on WGS84 ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_TRUE_ALTITUDE ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Sensor True Altitude",
      "Altitude of the currently active sensor, relative to Mean Sea Level. "
      "Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_HORIZONTAL_FOV ),
      std::make_shared< klv_uflint_format >( 0.0, 180.0, 2 ),
      "Sensor Horizontal Field of View",
      "Horizonal field of view of the currently active sensor. Measured in "
      "degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_VERTICAL_FOV ),
      std::make_shared< klv_uflint_format >( 0.0, 180.0, 2 ),
      "Sensor Vertical Field of View",
      "Vertical field of view of the currently active sensor. Measured in "
      "degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_RELATIVE_AZIMUTH_ANGLE ),
      std::make_shared< klv_uflint_format >( 0.0, 360.0, 4 ),
      "Sensor Relative Azimuth Angle",
      "Relative rotation angle of the currently active sensor to the platform "
      "longitudinal axis. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_RELATIVE_ELEVATION_ANGLE ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Sensor Relative Elevation Angle",
      "Relative elevation angle of the currently active sensor to the "
      "platform longitudinal-transverse plane. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_RELATIVE_ROLL_ANGLE ),
      std::make_shared< klv_uflint_format >( 0.0, 360.0, 4 ),
      "Sensor Relative Roll Angle",
      "Relative roll angle of the currently active sensor to the platform. "
      "Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SLANT_RANGE ),
      std::make_shared< klv_uflint_format >( 0.0, 5.0e6, 4 ),
      "Slant Range",
      "Distance between currently active sensor and the image center. "
      "Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_WIDTH ),
      std::make_shared< klv_uflint_format >( 0.0, 1.0e4, 2 ),
      "Target Width",
      "Target width within sensor field of view. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FRAME_CENTER_LATITUDE ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Frame Center Latitude",
      "Latitude of image center, based on WGS84 ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FRAME_CENTER_LONGITUDE ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Frame Center Longitude",
      "Longitude of image center, based on WGS84 ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FRAME_CENTER_ELEVATION ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Frame Center Elevation",
      "Altitude of image center, relative to Mean Sea Level.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LATITUDE_POINT_1 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Latitude Point 1",
      "Latitude offset for the upper left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LONGITUDE_POINT_1 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Longitude Point 1",
      "Longitude offset for the upper left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LATITUDE_POINT_2 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Latitude Point 2",
      "Latitude offset for the upper right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LONGITUDE_POINT_2 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Longitude Point 2",
      "Longitude offset for the upper right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LATITUDE_POINT_3 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Latitude Point 3",
      "Latitude offset for the lower right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LONGITUDE_POINT_3 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Longitude Point 3",
      "Longitude offset for the lower right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LATITUDE_POINT_4 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Latitude Point 4",
      "Latitude offset for the lower left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OFFSET_CORNER_LONGITUDE_POINT_4 ),
      std::make_shared< klv_sflint_format >( -0.075, 0.075, 2 ),
      "Offset Corner Longitude Point 4",
      "Longitude offset for the lower left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ICING_DETECTED ),
      std::make_shared< klv_enum_format< klv_0601_icing_detected > >( 1 ),
      "Icing Detected",
      "Flag for whether icing is detected on the aircraft.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_WIND_DIRECTION ),
      std::make_shared< klv_uflint_format >( 0.0, 360.0, 2 ),
      "Wind Direction",
      "Wind direction at the aircraft's location relative to true north. "
      "Angle increases in a clockwise direction when looking from above the "
      "platform. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_WIND_SPEED ),
      std::make_shared< klv_uflint_format >( 0.0, 100.0, 1 ),
      "Wind Speed",
      "Wind speed at the aircraft's location. Measured in meters per second.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_STATIC_PRESSURE ),
      std::make_shared< klv_uflint_format >( 0.0, 5000.0, 2 ),
      "Static Pressure",
      "Static pressure at the aircraft's location. Measured in millibar.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_DENSITY_ALTITUDE ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Density Altitude",
      "Density altitude at the aircraft's location. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OUTSIDE_AIR_TEMPERATURE ),
      std::make_shared< klv_sflint_format >( -127.0, 127.0, 1 ),
      "Outside Air Temperature",
      "Temperature outside of the aircraft. Measured in degrees Celsius.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_LOCATION_LATITUDE ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Target Location Latitude",
      "Calculated target latitude, based on WGS84 ellipsoid. Tracks "
      "crosshair, not image center.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_LOCATION_LONGITUDE ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Target Location Longitude",
      "Calculated target longitude, based on WGS84 ellipsoid. Tracks "
      "crosshair, not image center.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_LOCATION_ELEVATION ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Target Location Elevation",
      "Calculated target altitude, relative to Mean Sea Level. Tracks "
      "crosshair, not image center. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_TRACK_GATE_WIDTH ),
      std::make_shared< klv_uflint_format >( 0.0, 510.0, 1 ),
      "Target Track Gate Width",
      "Width of box around tracked target. Measured in pixels.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_TRACK_GATE_HEIGHT ),
      std::make_shared< klv_uflint_format >( 0.0, 510.0, 1 ),
      "Target Track Gate Height",
      "Height of box around tracked target. Measured in pixels.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_ERROR_ESTIMATE_CE90 ),
      std::make_shared< klv_uflint_format >( 0.0, 4095.0, 2 ),
      "Target Error Estimate - CE90",
      "Radius of 90% confidence for the target location in the horizonal "
      "direction. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_ERROR_ESTIMATE_LE90 ),
      std::make_shared< klv_uflint_format >( 0.0, 4095.0, 2 ),
      "Target Error Estimate - LE90",
      "Radius of 90% confidence for the target location in the vertical "
      "direction. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_GENERIC_FLAG_DATA ),
      std::make_shared< klv_uint_format >( 1 ),
      "Generic Flag Data",
      "Bits representing miscellaneous boolean values.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SECURITY_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "Security Local Set",
      "MISB ST 0102 local set for security metadata.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_DIFFERENTIAL_PRESSURE ),
      std::make_shared< klv_uflint_format >( 0.0, 5000.0, 2 ),
      "Differential Pressure",
      "Differential pressure at the aircraft's location. Equal to total "
      "pressure minus static pressure. Measured in millibar.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_ANGLE_OF_ATTACK ),
      std::make_shared< klv_sflint_format >( -20.0, 20.0, 2 ),
      "Platform Angle of Attack",
      "Vertical angle between the platform longitudinal axis and the relative "
      "wind. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_VERTICAL_SPEED ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 2 ),
      "Platform Vertical Speed",
      "Vertical speed of aircraft in the zenith direction. Measured in meters "
      "per second.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_SIDESLIP_ANGLE ),
      std::make_shared< klv_sflint_format >( -20.0, 20.0, 2 ),
      "Platform Sideslip Angle",
      "Horizontal angle between the platform longitudinal axis and the "
      "relative wind. Angle increases in a clockwise direction when looking "
      "from above the platform. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_AIRFIELD_BAROMETRIC_PRESSURE ),
      std::make_shared< klv_uflint_format >( 0.0, 5000.0, 2 ),
      "Airfield Barometric Pressure",
      "Local pressure at the airfield. Measured in millibar.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_AIRFIELD_ELEVATION ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Airfield Elevation",
      "Altitude of the airfield, relative to Mean Sea Level. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_RELATIVE_HUMIDITY ),
      std::make_shared< klv_uflint_format >( 0.0, 100.0, 1 ),
      "Relative Humidity",
      "Relative humidity at the aircraft location. Ratio between the current "
      "water vapor density and the saturation point of water vapor density. "
      "Measured in percent.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_GROUND_SPEED ),
      std::make_shared< klv_uflint_format >( 0.0, 255.0, 1 ),
      "Platform Ground Speed",
      "Speed of the aircraft when projected onto the ground plane.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_GROUND_RANGE ),
      std::make_shared< klv_uflint_format >( 0.0, 5.0e6, 4 ),
      "Ground Range",
      "Horizontal distance between the aircraft and the target of interest.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_FUEL_REMAINING ),
      std::make_shared< klv_uflint_format >( 0.0, 1.0e4, 2 ),
      "Platform Fuel Remaining",
      "Current weight of fuel present on the aircraft. Measured in kilograms.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_CALL_SIGN ),
      std::make_shared< klv_string_format >(),
      "Platform Call Sign",
      "Call sign of the platform or operating unit.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_WEAPON_LOAD ),
      std::make_shared< klv_uint_format >( 2 ),
      "Weapon Load",
      "Current weapons stored on aircraft. Deprecated by Item 140 (Weapon "
      "Stores).",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_WEAPON_FIRED ),
      std::make_shared< klv_uint_format >( 1 ),
      "Weapon Fired",
      "Signal when a particular weapon is released.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_LASER_PRF_CODE ),
      std::make_shared< klv_uint_format >( 2 ),
      "Laser PRF Code",
      "A laser's pulse repetition frequency used to mark a target. Three or "
      "four digit number with digits 1-8.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_FOV_NAME ),
      std::make_shared< klv_0601_sensor_fov_name_format >( 1  ),
      "Sensor Field of View Name",
      "Current lens type.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_MAGNETIC_HEADING ),
      std::make_shared< klv_uflint_format >( 0.0, 360.0, 2 ),
      "Platform Magnetic Heading",
      "Angle between longitudingal axis and Magnetic North measured in the "
      "horizontal plane. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_VERSION_NUMBER ),
      std::make_shared< klv_uint_format >( 1 ),
      "UAS Datalink LS Version Number",
      "Major version of MISB ST 0601 used as the source standard when "
      "encoding this local set.",
      1 },
    { {},
      ENUM_AND_NAME( KLV_0601_DEPRECATED ),
      std::make_shared< klv_blob_format >(),
      "Deprecated Tag",
      "This item is deprecated.",
      0 },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTERNATE_PLATFORM_LATITUDE ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Alternate Platform Latitude",
      "Latitude of the platform connected to the UAS via direct datalink.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTERNATE_PLATFORM_LONGITUDE ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Alternate Platform Longitude",
      "Longitude of the platform connected to the UAS via direct datalink.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTERNATE_PLATFORM_ALTITUDE ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Alternate Platform Altitude",
      "Altitude of the platform connected to the UAS via direct datalink.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTERNATE_PLATFORM_NAME ),
      std::make_shared< klv_string_format >(),
      "Alternate Platform Name",
      "Name of the platform connected to the UAS via direct datalink. "
      "Examples: 'Apache', 'Rover'.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTERNATE_PLATFORM_HEADING ),
      std::make_shared< klv_uflint_format >( 0.0, 360.0, 2 ),
      "Alternate Platform Heading",
      "Heading angle of the platform connected to the UAS via direct datalink.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_EVENT_START_TIME ),
      std::make_shared< klv_uint_format >( 8 ),
      "Event Start Time",
      "Start time of scene, project, event, mission, editing event, license, "
      "publication, etc, represented in the number of UTC microseconds "
      "elapsed since midnight, January 1, 1970.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_RVT_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "RVT Local Set",
      "MISB ST 0806 local set for remote video terminals.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_VMTI_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "VMTI Local Set",
      "MISB ST 0903 local set for the video moving target indicator.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_ELLIPSOID_HEIGHT ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Sensor Ellipsoid Height",
      "Altitude of the currently active sensor, relative to the WGS84 "
      "ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTERNATE_PLATFORM_ELLIPSOID_HEIGHT ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Alternate Platform Ellipsoid Height",
      "Altitude of the platform connected to the UAS via direct datalink, "
      "relative to the WGS84 ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OPERATIONAL_MODE ),
      std::make_shared< klv_0601_operational_mode_format >( 1 ),
      "Operational Mode",
      "Mode of operation of the event portrayed in the Motion Imagery.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FRAME_CENTER_HEIGHT_ABOVE_ELLIPSOID ),
      std::make_shared< klv_uflint_format >( -900.0, 19000.0, 2 ),
      "Frame Center Height Above Ellipsoid",
      "Altitude of frame center, relative to the WGS84 ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_NORTH_VELOCITY ),
      std::make_shared< klv_sflint_format >( -327.0, 327.0, 2 ),
      "Sensor North Velocity",
      "Northing velocity of the sensor or platform.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_EAST_VELOCITY ),
      std::make_shared< klv_sflint_format >( -327.0, 327.0, 2 ),
      "Sensor East Velocity",
      "Easting velocity of the sensor or platform.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_IMAGE_HORIZON_PIXEL_PACK ),
      std::make_shared< klv_blob_format >(),
      "Image Horizon Pixel Pack",
      "Location of earth-sky horizon in the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LATITUDE_POINT_1 ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Corner Latitude Point 1 (Full)",
      "Latitude for the upper left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LONGITUDE_POINT_1 ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Corner Longitude Point 1 (Full)",
      "Longitude for the upper left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LATITUDE_POINT_2 ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Corner Latitude Point 2 (Full)",
      "Latitude for the upper right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LONGITUDE_POINT_2 ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Corner Longitude Point 2 (Full)",
      "Longitude for the upper right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LATITUDE_POINT_3 ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Corner Latitude Point 3 (Full)",
      "Latitude for the lower right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LONGITUDE_POINT_3 ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Corner Longitude Point 3 (Full)",
      "Longitude for the lower right corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LATITUDE_POINT_4 ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Corner Latitude Point 4 (Full)",
      "Latitude for the lower left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_CORNER_LONGITUDE_POINT_4 ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Corner Longitude Point 4 (Full)",
      "Longitude for the lower left corner of the image.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_PLATFORM_PITCH_ANGLE ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Platform Pitch Angle (Full)",
      "Angle between longitudinal axis and horizonal plane. Positive angles "
      "above horizonal plane. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_PLATFORM_ROLL_ANGLE ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Platform Roll Angle (Full)",
      "Angle between transverse axis and transverse-longitudinal plane. "
      "Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_PLATFORM_ANGLE_OF_ATTACK ),
      std::make_shared< klv_sflint_format >( -90.0, 90.0, 4 ),
      "Platform Angle of Attack (Full)",
      "Vertical angle between the platform longitudinal axis and the relative "
      "wind. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_FULL_PLATFORM_SIDESLIP_ANGLE ),
      std::make_shared< klv_sflint_format >( -180.0, 180.0, 4 ),
      "Platform Sideslip Angle (Full)",
      "Horizontal angle between the platform longitudinal axis and the "
      "relative wind. Angle increases in a clockwise direction when looking from above the platform. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_MIIS_CORE_IDENTIFIER ),
      std::make_shared< klv_blob_format >( 34 ),
      "MIIS Core Identifier",
      "Binary value of MISB ST 1201 core identifier.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SAR_MOTION_IMAGERY_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "SAR Motion Imagery Local Set",
      "MISB ST 1206 local set for synthetic aperture radar Motion Imagery.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_WIDTH_EXTENDED ),
      std::make_shared< klv_imap_format >( 0.0, 1.5e6 ),
      "Target Width Extended",
      "Target width within sensor field of view. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_RANGE_IMAGE_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "Range Image Local Set",
      "MISB ST 1002 local set for range images.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_GEOREGISTRATION_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "Geo-Registration Local Set",
      "MISB ST 1601 local set for geo-registration.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_COMPOSITE_IMAGING_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "Composite Imaging Local Set",
      "MISB ST 1602 local set for composite imaging.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SEGMENT_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "Segment Local Set",
      "MISB ST 1607 Segment local set for metadata sharing across parent and "
      "child sets.",
      { 0, SIZE_MAX } },
    { {},
      ENUM_AND_NAME( KLV_0601_AMEND_LOCAL_SET ),
      std::make_shared< klv_blob_format >(),
      "Amend Local Set",
      "MISB ST 1607 Amend local set for metadata corrections.",
      { 0, SIZE_MAX } },
    { {},
      ENUM_AND_NAME( KLV_0601_SDCC_FLP ),
      std::make_shared< klv_blob_format >(),
      "SDCC-FLP",
      "MISB ST 101 floating length pack for standard deviation and "
      "cross-correlation metadata.",
      { 0, SIZE_MAX } },
    { {},
      ENUM_AND_NAME( KLV_0601_DENSITY_ALTITUDE_EXTENDED ),
      std::make_shared< klv_imap_format >( -900.0, 40000.0 ),
      "Density Altitude Extended",
      "Density altitude at the aircraft's location. Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_ELLIPSOID_HEIGHT_EXTENDED ),
      std::make_shared< klv_imap_format >( -900.0, 40000.0 ),
      "Sensor Ellipsoid Height Extended",
      "Altitude of the currently active sensor, relative to the WGS84 "
      "ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTERNATE_PLATFORM_ELLIPSOID_HEIGHT_EXTENDED ),
      std::make_shared< klv_imap_format >( -900.0, 40000.0 ),
      "Alternate Platform Ellipsoid Height Extended",
      "Altitude of the platform connected to the UAS via direct datalink, "
      "relative to the WGS84 ellipsoid.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_STREAM_DESIGNATOR ),
      std::make_shared< klv_string_format >(),
      "Stream Designator",
      "Shorthand descriptor for a particular Motion Imagery data stream, "
      "typically delivered over IP.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_OPERATIONAL_BASE ),
      std::make_shared< klv_string_format >(),
      "Operational Base",
      "Indicates the location for the launch recovery equipment.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_BROADCAST_SOURCE ),
      std::make_shared< klv_string_format >(),
      "Broadcast Source",
      "Location where the Motion Imagery is first broadcast. Examples: "
      "'Creech', 'Cannon'.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_RANGE_TO_RECOVERY_LOCATION ),
      std::make_shared< klv_imap_format >( 0.0, 21000.0 ),
      "Range to Recovery Location",
      "Distance from current position to airframe recovery position. Measured "
      "in kilometers.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TIME_AIRBORNE ),
      std::make_shared< klv_uint_format >(),
      "Time Airborne",
      "Number of seconds the aircraft has been airborne.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PROPULSION_UNIT_SPEED ),
      std::make_shared< klv_uint_format >(),
      "Propulsion Unit Speed",
      "Speed at which the engine or motor is rotating.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_COURSE_ANGLE ),
      std::make_shared< klv_imap_format >( 0.0, 360.0 ),
      "Platform Course Angle",
      "Angle between aircraft velocity vector and true north measured in the "
      "horizontal plane. Angles increase in a clockwise direction when "
      "looking from above the platform. Measured in degrees.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ALTITUDE_ABOVE_GROUND_LEVEL ),
      std::make_shared< klv_imap_format >( -900.0, 40000.0 ),
      "Altitude Above Ground Level",
      "Vertical distance between the aircraft and the ground or water. "
      "Measured in meters.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_RADAR_ALTIMETER ),
      std::make_shared< klv_imap_format >( -900.0, 40000.0 ),
      "Radar Altimeter",
      "Vertical distance between the aircraft and the ground or water, as "
      "measured by a radar altimeter.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_CONTROL_COMMAND ),
      std::make_shared< klv_0601_control_command_format >(),
      "Control Command",
      "Record of command from ground control station to aircraft.",
      { 0, SIZE_MAX } },
    { {},
      ENUM_AND_NAME( KLV_0601_CONTROL_COMMAND_VERIFICATION_LIST ),
      std::make_shared< klv_blob_format >(),
      "Control Command Verification List",
      "Acknowledgement from the platform that one or more control commands "
      "were received.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_AZIMUTH_RATE ),
      std::make_shared< klv_imap_format >( -1000.0, 1000.0 ),
      "Sensor Azimuth Rate",
      "Rate at which the sensor is rotating clockwise, when looking down from "
      "above the aircraft. Measured in degrees per second.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_ELEVATION_RATE ),
      std::make_shared< klv_imap_format >( -1000.0, 1000.0 ),
      "Sensor Elevation Rate",
      "Rate at which the sensor is rotating clockwise, when looking at the "
      "aircraft from the side such that the aircraft is pointing left. "
      "Measured in degrees per second.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_ROLL_RATE ),
      std::make_shared< klv_imap_format >( -1000.0, 1000.0 ),
      "Sensor Roll Rate",
      "Rate at which the sensor is rotating clockwise, when looking from "
      "behind the sensor. Measured in degrees per second.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ONBOARD_MI_STORAGE_PERCENT_FULL ),
      std::make_shared< klv_imap_format >( 0.0, 100.0 ),
      "On-board MI Storage Percent Full",
      "Amount of on-board Motion Imagery storage used as a percentage of "
      "total storage.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ACTIVE_WAVELENGTH_LIST ),
      std::make_shared< klv_blob_format >(),
      "Active Wavelength List",
      "List of wavelengths used by the sensor to generate the Motion Imagery.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_COUNTRY_CODES ),
      std::make_shared< klv_blob_format >(),
      "Country Codes",
      "Countries which are associated with the platform and its operation.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_NUMBER_OF_NAVSATS_IN_VIEW ),
      std::make_shared< klv_uint_format >( 1 ),
      "Number of NAVSATs in View",
      "Number of satellites used to determine position.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_POSITIONING_METHOD_SOURCE ),
      std::make_shared< klv_blob_format >( 1 ),
      "Positioning Method Source",
      "Source of the navigation positioning information.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PLATFORM_STATUS ),
      std::make_shared< klv_0601_platform_status_format >(),
      "Platform Status",
      "Operational mode of the platform.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_CONTROL_MODE ),
      std::make_shared< klv_0601_sensor_control_mode_format >(),
      "Sensor Control Mode",
      "Sensor control operational status.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_SENSOR_FRAME_RATE_PACK ),
      std::make_shared< klv_0601_frame_rate_format >(),
      "Sensor Frame Rate Pack",
      "Frame rate of the Motion Imagery at the sensor.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_WAVELENGTHS_LIST ),
      std::make_shared< klv_blob_format >(),
      "Wavelengths List",
      "List of wavelength bands provided by all available sensors.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TARGET_ID ),
      std::make_shared< klv_string_format >(),
      "Target ID",
      "Alpha-numeric identification of the target.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_AIRBASE_LOCATIONS ),
      std::make_shared< klv_blob_format >(),
      "Airbase Locations",
      "Geographic location of take-off and recovery site.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TAKEOFF_TIME ),
      std::make_shared< klv_uint_format >(),
      "Take-off Time",
      "Time when aircraft became airborne.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_TRANSMISSION_FREQUENCY ),
      std::make_shared< klv_imap_format >( 1.0, 99999.0 ),
      "Transmission Frequency",
      "Radio frequency used to transmit the Motion Imagery. Measured in "
      "megahertz.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ONBOARD_MI_STORAGE_CAPACITY ),
      std::make_shared< klv_uint_format >(),
      "On-board MI Storage Capacity",
      "Total capacity of on-board Motion Imagery storage. Measured in "
      "gigabytes.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ZOOM_PERCENTAGE ),
      std::make_shared< klv_imap_format >( 0.0, 100.0 ),
      "Zoom Percentage",
      "For a variable zoom system, the current percentage of zoom.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_COMMUNICATIONS_METHOD ),
      std::make_shared< klv_string_format >(),
      "Communications Method",
      "Type of communications used with platform",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_LEAP_SECONDS ),
      std::make_shared< klv_sint_format >(),
      "Leap Seconds",
      "Number of leap seconds to adjust Precision Timestamp (Item 2) to UTC.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_CORRECTION_OFFSET ),
      std::make_shared< klv_sint_format >(),
      "Correction Offset",
      "Post-flight time adjustment for Precision Timestamp (Item 2) as "
      "needed.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_PAYLOAD_LIST ),
      std::make_shared< klv_blob_format >(),
      "Payload List",
      "List of payloads available on platform.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_ACTIVE_PAYLOADS ),
      std::make_shared< klv_blob_format >(),
      "Active Payloads",
      "List of currently active payloads from the payload list (Item 138).",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_WEAPONS_STORES ),
      std::make_shared< klv_blob_format >(),
      "Weapons Stores",
      "List of weapon stores and statuses.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_WAYPOINT_LIST ),
      std::make_shared< klv_blob_format >(),
      "Waypoint List",
      "List of navigational waypoints and their statuses.",
      { 0, 1 } },
    { {},
      ENUM_AND_NAME( KLV_0601_VIEW_DOMAIN ),
      std::make_shared< klv_blob_format >(),
      "View Domain",
      "Specifies range of possible sensor relative azimuth, elevation, and "
      "roll values.",
      { 0, 1 } }, };
#undef ENUM_AND_NAME

  return lookup;
}

// ----------------------------------------------------------------------------
std::ostream&
operator<<( std::ostream& os, klv_0601_icing_detected value )
{
  static std::string strings[ KLV_0601_ICING_DETECTED_ENUM_END + 1 ] = {
    "Detector Off",
    "No Icing Detected",
    "Icing Detected",
    "Unknown Icing Detection Status" };

  return os << strings[ std::min( value, KLV_0601_ICING_DETECTED_ENUM_END ) ];
}

// ----------------------------------------------------------------------------
std::ostream&
operator<<( std::ostream& os, klv_0601_sensor_fov_name value )
{
  static std::string strings[ KLV_0601_SENSOR_FOV_NAME_ENUM_END + 1 ] = {
    "Ultranarrow",
    "Narrow",
    "Medium",
    "Wide",
    "Ultrawide",
    "Narrow Medium",
    "2x Ultranarrow",
    "4x Ultranarrow",
    "Continuous Zoom",
    "Unknown Sensor FOV Name" };

  os << strings[ std::min( value, KLV_0601_SENSOR_FOV_NAME_ENUM_END ) ];
  return os;
}

// ----------------------------------------------------------------------------
std::ostream&
operator<<( std::ostream& os, klv_0601_generic_flag_data_bits value )
{
  static std::string strings[ KLV_0601_GENERIC_FLAG_DATA_BIT_ENUM_END + 1 ] = {
    "Laser Range",
    "Auto-Track",
    "IR Polarity",
    "Icing Status",
    "Slant Range",
    "Image Invalid",
    "Unknown Generic Flag Data Bit" };

  os << strings[ std::min( value, KLV_0601_GENERIC_FLAG_DATA_BIT_ENUM_END ) ];
  return os;
}

// ----------------------------------------------------------------------------
KWIVER_ALGO_KLV_EXPORT
std::ostream&
operator<<( std::ostream& os, klv_0601_operational_mode value )
{
  static std::string strings[ KLV_0601_OPERATIONAL_MODE_ENUM_END + 1 ] = {
    "Other",
    "Operational",
    "Training",
    "Exercise",
    "Maintenance",
    "Test",
    "Unknown Operational Mode" };

  os << strings[ std::min( value, KLV_0601_OPERATIONAL_MODE_ENUM_END ) ];
  return os;
}

// ----------------------------------------------------------------------------
KWIVER_ALGO_KLV_EXPORT
std::ostream&
operator<<( std::ostream& os, klv_0601_platform_status value )
{
  static std::string strings[ KLV_0601_PLATFORM_STATUS_ENUM_END + 1 ] = {
    "Active",
    "Preflight",
    "Preflight - Taxiing",
    "Run-up",
    "Take-off",
    "Ingress",
    "Manual Operation",
    "Automated Orbit",
    "Transitioning",
    "Egress",
    "Landing",
    "Landing - Taxiing",
    "Landed - Parked",
    "Unknown Platform Status" };

  return os << strings[ std::min( value, KLV_0601_PLATFORM_STATUS_ENUM_END ) ];
}

// ----------------------------------------------------------------------------
KWIVER_ALGO_KLV_EXPORT
std::ostream&
operator<<( std::ostream& os, klv_0601_sensor_control_mode value )
{
  static std::string strings[ KLV_0601_SENSOR_CONTROL_MODE_ENUM_END + 1 ] = {
    "Off",
    "Home Position",
    "Uncontrolled",
    "Manual Control",
    "Calibrating",
    "Auto - Holding Position",
    "Auto - Tracking",
    "Unknown Sensor Control Mode" };

  os << strings[ std::min( value, KLV_0601_SENSOR_CONTROL_MODE_ENUM_END ) ];
  return os;
}

// ----------------------------------------------------------------------------
std::ostream&
operator<<( std::ostream& os, klv_0601_control_command const& value )
{
  return os << "{ " << "ID: " << value.id
            << ", String: \"" << value.string
            << "\", Timestamp: " << ( value.timestamp
                                      ? std::to_string( value.timestamp )
                                      : "(empty)" )
            << " }";
}

// ----------------------------------------------------------------------------
bool
operator==( klv_0601_control_command const& lhs,
            klv_0601_control_command const& rhs )
{
  return lhs.id == rhs.id &&
         lhs.string == rhs.string &&
         lhs.timestamp == rhs.timestamp;
}

// ----------------------------------------------------------------------------
bool
operator<( klv_0601_control_command const& lhs,
           klv_0601_control_command const& rhs )
{
  if( lhs.id < rhs.id )
  {
    return true;
  }
  if( lhs.id > rhs.id )
  {
    return false;
  }

  if( lhs.string < rhs.string )
  {
    return true;
  }
  if( lhs.string > rhs.string )
  {
    return false;
  }

  return lhs.timestamp < rhs.timestamp;
}

// ----------------------------------------------------------------------------
klv_0601_control_command_format
::klv_0601_control_command_format() : klv_data_format_< data_type >{ 0 }
{}

// ----------------------------------------------------------------------------
klv_0601_control_command
klv_0601_control_command_format
::read_typed( klv_read_iter_t& data, size_t length ) const
{
  klv_0601_control_command result;
  auto const begin = data;
  auto const remaining_length = [ & ]() -> size_t {
                                  return length - std::distance( begin, data );
                                };
  result.id = klv_read_ber_oid< uint16_t >( data, remaining_length() );

  auto const length_of_string =
    klv_read_ber< size_t >( data, remaining_length() );
  if( length_of_string > remaining_length() )
  {
    VITAL_THROW( kwiver::vital::metadata_buffer_overflow,
                 "reading command string overruns data buffer" );
  }

  result.string = klv_read_string( data, length_of_string );
  switch( remaining_length() )
  {
    case 0:
      result.timestamp = 0;
      break;
    case 8:
      result.timestamp = klv_read_int< uint64_t >( data, 8 );
      break;
    default:
      VITAL_THROW( kwiver::vital::metadata_exception,
                   std::to_string( remaining_length() ) +
                   " bytes left over for timestamp while parsing "
                   "command pack; expected 0 or 8" );
      break;
  }
  return result;
}

// ----------------------------------------------------------------------------
void
klv_0601_control_command_format
::write_typed( klv_0601_control_command const& value,
               klv_write_iter_t& data, size_t length ) const
{
  auto const begin = data;
  auto const remaining_length = [ & ]() -> size_t {
                                  return length - std::distance( begin, data );
                                };
  klv_write_ber_oid( value.id, data, remaining_length() );

  auto const length_of_string = klv_string_length( value.string );
  klv_write_ber( length_of_string, data, remaining_length() );
  klv_write_string( value.string, data, remaining_length() );
  if( value.timestamp )
  {
    if( remaining_length() < 8 )
    {
      VITAL_THROW( kwiver::vital::metadata_buffer_overflow,
                   "writing control command timestamp overflows buffer" );
    }
    klv_write_int( value.timestamp, data, 8 );
  }
}

// ----------------------------------------------------------------------------
size_t
klv_0601_control_command_format
::length_of_typed( klv_0601_control_command const& value,
                   VITAL_UNUSED size_t length_hint ) const
{
  return klv_ber_oid_length( value.id ) +
         klv_ber_length( klv_string_length( value.string ) ) +
         klv_string_length( value.string ) +
         ( value.timestamp ? 8 : 0 );
}

// ----------------------------------------------------------------------------
std::string
klv_0601_control_command_format
::description() const
{
  return "control command of " + length_description();
}

// ----------------------------------------------------------------------------
std::ostream&
operator<<( std::ostream& os, klv_0601_frame_rate const& value )
{
  if( value.denominator == 1 )
  {
    return os << value.numerator;
  }
  if( value.denominator == 0 ) {
    return os << "(invalid)";
  }
  return os << std::fixed << std::setprecision( 3 )
            << ( static_cast< double >( value.numerator ) /
                 value.denominator );
}

// ----------------------------------------------------------------------------
bool
operator==( klv_0601_frame_rate const& lhs, klv_0601_frame_rate const& rhs )
{
  return lhs.numerator == rhs.numerator && lhs.denominator == rhs.denominator;
}

// ----------------------------------------------------------------------------
bool
operator<( klv_0601_frame_rate const& lhs, klv_0601_frame_rate const& rhs )
{
  if( lhs.numerator < rhs.numerator )
  {
    return true;
  }
  if( lhs.numerator == rhs.numerator )
  {
    return lhs.denominator < rhs.denominator;
  }
  return false;
}

// ----------------------------------------------------------------------------
klv_0601_frame_rate_format
::klv_0601_frame_rate_format() : klv_data_format_< data_type >{ 0 } {}

// ----------------------------------------------------------------------------
klv_0601_frame_rate
klv_0601_frame_rate_format
::read_typed( klv_read_iter_t& data, size_t length ) const
{
  klv_0601_frame_rate result;
  auto const begin = data;
  auto const remaining_length = [ & ]() -> size_t {
                                  return length - std::distance( begin, data );
                                };
  result.numerator = klv_read_ber_oid< uint32_t >( data, remaining_length() );
  result.denominator =
    remaining_length()
    ? klv_read_ber_oid< uint32_t >( data, remaining_length() ) : 1;
  return result;
}

// ----------------------------------------------------------------------------
void
klv_0601_frame_rate_format
::write_typed( klv_0601_frame_rate const& value,
               klv_write_iter_t& data, size_t length ) const
{
  auto const begin = data;
  auto const remaining_length = [ & ]() -> size_t {
                                  return length - std::distance( begin, data );
                                };
  klv_write_ber_oid( value.numerator, data, remaining_length() );
  if( value.denominator != 1 )
  {
    klv_write_ber_oid( value.denominator, data, remaining_length() );
  }
}

// ----------------------------------------------------------------------------
size_t
klv_0601_frame_rate_format
::length_of_typed( klv_0601_frame_rate const& value,
                   VITAL_UNUSED size_t length_hint ) const
{
  return klv_ber_oid_length( value.numerator ) +
         ( ( value.denominator == 1 )
           ? 0 : klv_ber_oid_length( value.denominator ) );
}

// ----------------------------------------------------------------------------
std::string
klv_0601_frame_rate_format
::description() const
{
  return "frame rate of " + length_description();
}

// ----------------------------------------------------------------------------
klv_local_set
klv_0601_local_set_format
::read_typed( klv_read_iter_t& data, size_t length ) const
{
  constexpr size_t timestamp_packet_length = 10;

  if( length < timestamp_packet_length  )
  {
    VITAL_THROW( kv::metadata_exception,
                 "packet too small; timestamp is not present" );
  }

  // Ensure timestamp tag and length are present
  auto timestamp_it = data;
  auto const timestamp_tag = klv_read_int< uint8_t >( timestamp_it, 1 );
  auto const timestamp_length = klv_read_int< uint8_t >( timestamp_it, 1 );
  if( timestamp_tag != KLV_0601_PRECISION_TIMESTAMP || timestamp_length != 8 )
  {
    VITAL_THROW( kv::metadata_exception,
                 "timestamp not present at beginning of packet" );
  }

  // Read rest of packet as normal
  auto const result =
    klv_local_set_format::read_typed( data, length );

  return result;
}

// ----------------------------------------------------------------------------
uint16_t
klv_0601_local_set_format
::calculate_checksum( klv_read_iter_t data, size_t length ) const
{
  return klv_running_sum_16( checksum_header.begin(), checksum_header.end(),
                             klv_running_sum_16( data, data + length ) );
}

// ----------------------------------------------------------------------------
uint16_t
klv_0601_local_set_format
::read_checksum( klv_read_iter_t data, size_t length ) const
{
  if( length < checksum_packet_length )
  {
    VITAL_THROW( kv::metadata_buffer_overflow,
                 "packet too small; checksum is not present" );
  }
  data += length - checksum_packet_length;

  if( !std::equal( checksum_header.cbegin(), checksum_header.cend(), data ) )
  {
    VITAL_THROW( kv::metadata_exception,
                 "checksum header not present" );
  }
  data += checksum_header.size();

  return klv_read_int< uint16_t >( data, 2 );
}

// ----------------------------------------------------------------------------
void
klv_0601_local_set_format
::write_checksum( uint16_t checksum,
                  klv_write_iter_t& data, size_t max_length ) const
{
  if( max_length < checksum_packet_length )
  {
    VITAL_THROW( kv::metadata_buffer_overflow,
                 "writing checksum packet overflows data buffer" );
  }
  data = std::copy( checksum_header.cbegin(), checksum_header.cend(), data );
  klv_write_int( checksum, data, 2 );
}

// ----------------------------------------------------------------------------
size_t
klv_0601_local_set_format
::checksum_length() const
{
  return checksum_packet_length;
}

// ----------------------------------------------------------------------------
std::string
klv_0601_local_set_format
::description() const
{
  return "ST 0601 local set" + length_description();
}

// ----------------------------------------------------------------------------
std::ostream&
operator<<( std::ostream& os, klv_0601_tag tag )
{
  return os << klv_0601_traits_lookup().by_tag( tag ).name();
}

// ----------------------------------------------------------------------------
klv_0601_local_set_format
::klv_0601_local_set_format()
  : klv_local_set_format{ klv_0601_traits_lookup() }
{}

// ----------------------------------------------------------------------------
klv_uds_key
klv_0601_key()
{
  // From Section 6.2 of
  // https://gwg.nga.mil/misb/docs/standards/ST0601.17.pdf
  return { 0x060E2B34020B0101, 0x0E01030101000000 };
}

} // namespace klv

} // namespace arrows

} // namespace kwiver
