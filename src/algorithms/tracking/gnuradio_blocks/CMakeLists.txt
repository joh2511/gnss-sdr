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


if(ENABLE_CUDA)
     set(OPT_TRACKING_BLOCKS ${OPT_TRACKING_BLOCKS} gps_l1_ca_dll_pll_tracking_gpu_cc.cc)
     set(OPT_TRACKING_INCLUDES ${OPT_TRACKING_INCLUDES}  
                               ${CUDA_INCLUDE_DIRS}
                               ${CMAKE_SOURCE_DIR}/src/algorithms/libs/cudahelpers)
     set(OPT_TRACKING_LIBRARIES ${OPT_TRACKING_LIBRARIES} ${CUDA_LIBRARIES}) 
endif(ENABLE_CUDA)

set(TRACKING_GR_BLOCKS_SOURCES
     galileo_e1_dll_pll_veml_tracking_cc.cc
     galileo_volk_e1_dll_pll_veml_tracking_cc.cc
     galileo_e1_tcp_connector_tracking_cc.cc
     gps_l1_ca_dll_fll_pll_tracking_cc.cc
     gps_l1_ca_dll_pll_optim_tracking_cc.cc
     gps_l1_ca_dll_pll_tracking_cc.cc
     gps_l1_ca_tcp_connector_tracking_cc.cc
     galileo_e5a_dll_pll_tracking_cc.cc
     gps_l2_m_dll_pll_tracking_cc.cc
     ${OPT_TRACKING_BLOCKS}   
)

include_directories(
     $(CMAKE_CURRENT_SOURCE_DIR)
     ${CMAKE_SOURCE_DIR}/src/core/system_parameters
     ${CMAKE_SOURCE_DIR}/src/core/interfaces
     ${CMAKE_SOURCE_DIR}/src/core/receiver
     ${CMAKE_SOURCE_DIR}/src/algorithms/tracking/libs
     ${CMAKE_SOURCE_DIR}/src/algorithms/libs
     ${GLOG_INCLUDE_DIRS}
     ${GFlags_INCLUDE_DIRS}
     ${Boost_INCLUDE_DIRS}
     ${GNURADIO_RUNTIME_INCLUDE_DIRS}
     ${VOLK_GNSSSDR_INCLUDE_DIRS}
     ${OPT_TRACKING_INCLUDES}
)

if(ENABLE_GENERIC_ARCH)
    add_definitions( -DGENERIC_ARCH=1 )
endif(ENABLE_GENERIC_ARCH)

file(GLOB TRACKING_GR_BLOCKS_HEADERS "*.h")
add_library(tracking_gr_blocks ${TRACKING_GR_BLOCKS_SOURCES} ${TRACKING_GR_BLOCKS_HEADERS})
source_group(Headers FILES ${TRACKING_GR_BLOCKS_HEADERS})

target_link_libraries(tracking_gr_blocks tracking_lib ${GNURADIO_RUNTIME_LIBRARIES} gnss_sp_libs ${Boost_LIBRARIES} ${VOLK_GNSSSDR_LIBRARIES} ${ORC_LIBRARIES} ${OPT_TRACKING_LIBRARIES})

if(NOT VOLK_GNSSSDR_FOUND)
    add_dependencies(tracking_gr_blocks volk_gnsssdr_module)
endif(NOT VOLK_GNSSSDR_FOUND)
