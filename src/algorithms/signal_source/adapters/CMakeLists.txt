# Copyright (C) 2012-2013  (see AUTHORS file for a list of contributors)
#
# This file is part of GNSS-SDR.
#
# GNSS-SDR is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# at your option) any later version.
#
# GNSS-SDR is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
#

# Optional drivers

if($ENV{GN3S_DRIVER})
     ##############################################
     # GN3S (USB dongle)
     ##############################################

     # find_package(GrGN3S)
     # if not found, build it with ExternalPackage_Add
     include(ExternalProject)     
     ExternalProject_Add(
          gr-gn3s
          SOURCE_DIR ${CMAKE_SOURCE_DIR}/drivers/gr-gn3s
          BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../../../gr-gn3s
          UPDATE_COMMAND ""
          PATCH_COMMAND ""
          INSTALL_COMMAND ""
     )
     
     # Set up variables
     set(GRGN3S_INCLUDE_DIRS ${GRGN3S_DIR}/include ${GRGN3S_DIR} ${GRGN3S_DIR}/src)
     set(GRGN3S_LIBRARIES 
         "${CMAKE_CURRENT_BINARY_DIR}/../../../../gr-gn3s/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gr-gn3s${CMAKE_SHARED_LIBRARY_SUFFIX}"
     ) 
     set(OPT_LIBRARIES ${OPT_LIBRARIES} ${GRGN3S_LIBRARIES})
     set(OPT_DRIVER_INCLUDE_DIRS ${OPT_DRIVER_INCLUDE_DIRS} ${GRGN3S_INCLUDE_DIRS})
     set(OPT_DRIVER_SOURCES ${OPT_DRIVER_SOURCES} gn3s_signal_source.cc)
     
     #  Copy GN3S firmware binary file to install folder
     message(STATUS "Copying the GN3S firmware binary file to install folder")    
     file(COPY ${CMAKE_SOURCE_DIR}/firmware/GN3S_v2/bin/gn3s_firmware.ihx 
          DESTINATION ${CMAKE_SOURCE_DIR}/install/gn3s_firmware.ihx 
     )     
endif($ENV{GN3S_DRIVER})



if($ENV{RTLSDR_DRIVER})
    ################################################################################
    # OsmoSDR - http://sdr.osmocom.org/trac/
    ################################################################################
    find_package(LibOsmoSDR)
    if(NOT LIBOSMOSDR_FOUND)
         message(FATAL_ERROR "libosmosdr required to build gnss-sdr with the optional RTLSDR driver")
    endif(NOT LIBOSMOSDR_FOUND)    
    find_package(GrOsmoSDR REQUIRED)
        if(NOT GROSMOSDR_FOUND)
             message(FATAL_ERROR "gr-osmosdr required to build gnss-sdr with the optional RTLSDR driver")
        endif(NOT GROSMOSDR_FOUND)  
    # set RTL include dirs
    set(RTL_DRIVER_INCLUDE_DIRS 
        ${OPT_DRIVER_INCLUDE_DIRS} 
        ${GROSMOSDR_INCLUDE_DIR}/osmosdr
        ${LIBOSMOSDR_INCLUDE_DIR}
    )
    set(OPT_DRIVER_SOURCES ${OPT_DRIVER_SOURCES} rtlsdr_signal_source.cc)
    set(OPT_LIBRARIES ${OPT_LIBRARIES} ${LIBOSMOSDR_LIBRARIES} ${GROSMOSDR_LIBRARIES})
    set(OPT_DRIVER_INCLUDE_DIRS ${OPT_DRIVER_INCLUDE_DIRS} ${RTL_DRIVER_INCLUDE_DIRS}) 
endif($ENV{RTLSDR_DRIVER})

set(SIGNAL_SOURCE_ADAPTER_SOURCES file_signal_source.cc uhd_signal_source.cc ${OPT_DRIVER_SOURCES})

include_directories(
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${OPT_DRIVER_INCLUDE_DIRS}
     ${CMAKE_SOURCE_DIR}/src/core/system_parameters
     ${CMAKE_SOURCE_DIR}/src/core/interfaces
     ${CMAKE_SOURCE_DIR}/src/algorithms/libs
     ${GNURADIO_CORE_INCLUDE_DIRS}
     ${GNURADIO_GRUEL_INCLUDE_DIRS}
     ${GR_UHD_INCLUDE_DIRS}
     ${UHD_INCLUDE_DIRS}
     ${Boost_INCLUDE_DIRS}
)

add_library(signal_source_adapters ${SIGNAL_SOURCE_ADAPTER_SOURCES})
target_link_libraries(signal_source_adapters gnss_sp_libs ${Boost_LIBRARIES} uhd gnuradio-uhd ${OPT_LIBRARIES})