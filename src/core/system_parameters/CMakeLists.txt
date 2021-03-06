# Copyright (C) 2012-2015  (see AUTHORS file for a list of contributors)
#
# This file is part of GNSS-SDR.
#
# GNSS-SDR is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GNSS-SDR is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
#

set(SYSTEM_PARAMETERS_SOURCES
     gnss_satellite.cc
     gnss_signal.cc
     gps_navigation_message.cc
	 gps_ephemeris.cc
	 gps_iono.cc
	 gps_almanac.cc
	 gps_utc_model.cc
	 gps_acq_assist.cc
	 gps_ref_time.cc
	 gps_ref_location.cc
	 galileo_utc_model.cc
	 galileo_ephemeris.cc
	 galileo_almanac.cc
	 galileo_iono.cc
	 galileo_navigation_message.cc
	 sbas_ephemeris.cc
	 sbas_ionospheric_correction.cc
	 sbas_satellite_correction.cc
	 sbas_telemetry_data.cc
	 galileo_fnav_message.cc
	 gps_cnav_ephemeris.cc
	 gps_cnav_navigation_message.cc
	 gps_cnav_iono.cc
	 gps_cnav_utc_model.cc
)


include_directories(
     $(CMAKE_CURRENT_SOURCE_DIR)
     ${CMAKE_SOURCE_DIR}/src/core/receiver
     ${GLOG_INCLUDE_DIRS}
     ${Boost_INCLUDE_DIRS}
     ${GFlags_INCLUDE_DIRS}
)

link_directories(${Boost_LIBRARY_DIR})
file(GLOB SYSTEM_PARAMETERS_HEADERS "*.h")
add_library(gnss_system_parameters ${SYSTEM_PARAMETERS_SOURCES} ${SYSTEM_PARAMETERS_HEADERS})
source_group(Headers FILES ${SYSTEM_PARAMETERS_HEADERS})
add_dependencies(gnss_system_parameters glog-${glog_RELEASE})
target_link_libraries(gnss_system_parameters ${Boost_LIBRARIES})

