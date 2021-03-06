/*!
 * \file gps_l1_ca_pcps_acquisition.cc
 * \brief Adapts a PCPS acquisition block to an AcquisitionInterface for
 *  GPS L1 C/A signals
 * \authors <ul>
 *          <li> Javier Arribas, 2011. jarribas(at)cttc.es
 *          </ul>
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2015  (see AUTHORS file for a list of contributors)
 *
 * GNSS-SDR is a software defined Global Navigation
 *          Satellite Systems receiver
 *
 * This file is part of GNSS-SDR.
 *
 * GNSS-SDR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNSS-SDR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------
 */

#include "gps_l2_m_pcps_acquisition.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/math/distributions/exponential.hpp>
#include <glog/logging.h>
#include <gnuradio/msg_queue.h>
#include "gps_l2c_signal.h"
#include "GPS_L2C.h"
#include "configuration_interface.h"


using google::LogMessage;

GpsL2MPcpsAcquisition::GpsL2MPcpsAcquisition(
        ConfigurationInterface* configuration, std::string role,
        unsigned int in_streams, unsigned int out_streams,
        gr::msg_queue::sptr queue) :
    role_(role), in_streams_(in_streams), out_streams_(out_streams), queue_(queue)
{
    configuration_ = configuration;
    std::string default_item_type = "gr_complex";
    std::string default_dump_filename = "./data/acquisition.dat";

    LOG(INFO) << "role " << role;

    item_type_ = configuration_->property(role + ".item_type",
            default_item_type);
    //float pfa =  configuration_->property(role + ".pfa", 0.0);

    fs_in_ = configuration_->property("GNSS-SDR.internal_fs_hz", 2048000);
    if_ = configuration_->property(role + ".ifreq", 0);
    dump_ = configuration_->property(role + ".dump", false);
    shift_resolution_ = configuration_->property(role + ".doppler_max", 15);

    bit_transition_flag_ = configuration_->property(role + ".bit_transition_flag", false);

    if (!bit_transition_flag_)
        {
            max_dwells_ = configuration_->property(role + ".max_dwells", 1);
        }
    else
        {
            max_dwells_ = 2;
        }

    dump_filename_ = configuration_->property(role + ".dump_filename",
            default_dump_filename);

    //--- Find number of samples per spreading code -------------------------
    code_length_ = round((double)fs_in_
            / (GPS_L2_M_CODE_RATE_HZ / (double)GPS_L2_M_CODE_LENGTH_CHIPS));

    vector_length_ = code_length_;

    code_= new gr_complex[vector_length_];

    // if (item_type_.compare("gr_complex") == 0 )
    //         {
    item_size_ = sizeof(gr_complex);
    acquisition_cc_ = pcps_make_acquisition_cc(1, max_dwells_,
            shift_resolution_, if_, fs_in_, code_length_, code_length_,
            bit_transition_flag_, queue_, dump_, dump_filename_);

    stream_to_vector_ = gr::blocks::stream_to_vector::make(item_size_, vector_length_);

    DLOG(INFO) << "stream_to_vector(" << stream_to_vector_->unique_id() << ")";
    DLOG(INFO) << "acquisition(" << acquisition_cc_->unique_id() << ")";
    //        }

    if (item_type_.compare("cshort") == 0)
        {
            cshort_to_float_x2_ = make_cshort_to_float_x2();
            float_to_complex_ = gr::blocks::float_to_complex::make();
        }

    if (item_type_.compare("cbyte") == 0)
        {
            cbyte_to_float_x2_ = make_complex_byte_to_float_x2();
            float_to_complex_ = gr::blocks::float_to_complex::make();
        }
    //}
    //else
    // {
    //     LOG(WARNING) << item_type_
    //             << " unknown acquisition item type";
    // }

    gnss_synchro_ = 0;
    threshold_ = 0.0;
    doppler_max_ = 5000;
    doppler_step_ = 250;
    channel_internal_queue_ = 0;
    channel_ = 0;
}


GpsL2MPcpsAcquisition::~GpsL2MPcpsAcquisition()
{
    delete[] code_;
}


void GpsL2MPcpsAcquisition::set_channel(unsigned int channel)
{
    channel_ = channel;
    //if (item_type_.compare("gr_complex") == 0)
    //{
    acquisition_cc_->set_channel(channel_);
    //}
}


void GpsL2MPcpsAcquisition::set_threshold(float threshold)
{
    float pfa = configuration_->property(role_ + boost::lexical_cast<std::string>(channel_) + ".pfa", 0.0);

    if(pfa == 0.0)
        {
            pfa = configuration_->property(role_ + ".pfa", 0.0);
        }
    if(pfa == 0.0)
        {
            threshold_ = threshold;
        }
    else
        {
            threshold_ = calculate_threshold(pfa);
        }

    DLOG(INFO) <<"Channel "<<channel_<<" Threshold = " << threshold_;

   // if (item_type_.compare("gr_complex") == 0)
    //    {
            acquisition_cc_->set_threshold(threshold_);
    //    }
}


void GpsL2MPcpsAcquisition::set_doppler_max(unsigned int doppler_max)
{
    doppler_max_ = doppler_max;
    //   if (item_type_.compare("gr_complex") == 0)
    //  {
    acquisition_cc_->set_doppler_max(doppler_max_);
    // }
}


// Be aware that Doppler step should be set to 2/(3T) Hz, where T is the coherent integration time (GPS L2 period is 0.02s)
// Doppler bin minimum size= 33 Hz
void GpsL2MPcpsAcquisition::set_doppler_step(unsigned int doppler_step)
{
    doppler_step_ = doppler_step;
    //   if (item_type_.compare("gr_complex") == 0)
    //      {
    acquisition_cc_->set_doppler_step(doppler_step_);
    //     }

}


void GpsL2MPcpsAcquisition::set_channel_queue(
        concurrent_queue<int> *channel_internal_queue)
{
    channel_internal_queue_ = channel_internal_queue;
    //  if (item_type_.compare("gr_complex") == 0)
    //  {
    acquisition_cc_->set_channel_queue(channel_internal_queue_);
    //  }
}


void GpsL2MPcpsAcquisition::set_gnss_synchro(Gnss_Synchro* gnss_synchro)
{
    gnss_synchro_ = gnss_synchro;
    // if (item_type_.compare("gr_complex") == 0)
    // {
    acquisition_cc_->set_gnss_synchro(gnss_synchro_);
    // }
}


signed int GpsL2MPcpsAcquisition::mag()
{
    // //    if (item_type_.compare("gr_complex") == 0)
    //        {
    return acquisition_cc_->mag();
    //       }
    //   else
    //       {
    //           return 0;
    //      }
}


void GpsL2MPcpsAcquisition::init()
{
    acquisition_cc_->init();
    set_local_code();
}


void GpsL2MPcpsAcquisition::set_local_code()
{
    // if (item_type_.compare("gr_complex") == 0)
    //   {
    gps_l2c_m_code_gen_complex_sampled(code_, gnss_synchro_->PRN, fs_in_);
    acquisition_cc_->set_local_code(code_);

//    //debug
//    std::ofstream d_dump_file;
//    std::stringstream filename;
//    std::streamsize n = 2 * sizeof(float) * (code_length_); // complex file write
//    filename.str("");
//    filename << "../data/local_prn_sampled.dat";
//    d_dump_file.open(filename.str().c_str(), std::ios::out | std::ios::binary);
//    d_dump_file.write((char*)code_, n);
//    d_dump_file.close();

    //  }
}


void GpsL2MPcpsAcquisition::reset()
{
    //  if (item_type_.compare("gr_complex") == 0)
    //  {
    acquisition_cc_->set_active(true);
    //  }
}

void GpsL2MPcpsAcquisition::set_state(int state)
{
    //  if (item_type_.compare("gr_complex") == 0)
    //  {
    acquisition_cc_->set_state(state);
    //  }
}



float GpsL2MPcpsAcquisition::calculate_threshold(float pfa)
{
    //Calculate the threshold
    unsigned int frequency_bins = 0;
    for (int doppler = (int)(-doppler_max_); doppler <= (int)doppler_max_; doppler += doppler_step_)
        {
            frequency_bins++;
        }
    DLOG(INFO) << "Channel " << channel_<< "  Pfa = " << pfa;
    unsigned int ncells = vector_length_ * frequency_bins;
    double exponent = 1 / static_cast<double>(ncells);
    double val = pow(1.0 - pfa, exponent);
    double lambda = double(vector_length_);
    boost::math::exponential_distribution<double> mydist (lambda);
    float threshold = (float)quantile(mydist,val);

    return threshold;
}


void GpsL2MPcpsAcquisition::connect(gr::top_block_sptr top_block)
{
    if (item_type_.compare("gr_complex") == 0)
        {
            top_block->connect(stream_to_vector_, 0, acquisition_cc_, 0);
        }
    else if (item_type_.compare("cshort") == 0)
        {
            top_block->connect(cshort_to_float_x2_, 0, float_to_complex_, 0);
            top_block->connect(cshort_to_float_x2_, 1, float_to_complex_, 1);
            top_block->connect(float_to_complex_, 0, stream_to_vector_, 0);
            top_block->connect(stream_to_vector_, 0, acquisition_cc_, 0);
        }
    else if (item_type_.compare("cbyte") == 0)
        {
            top_block->connect(cbyte_to_float_x2_, 0, float_to_complex_, 0);
            top_block->connect(cbyte_to_float_x2_, 1, float_to_complex_, 1);
            top_block->connect(float_to_complex_, 0, stream_to_vector_, 0);
            top_block->connect(stream_to_vector_, 0, acquisition_cc_, 0);
        }
    else
        {
            LOG(WARNING) << item_type_ << " unknown acquisition item type";
        }

}


void GpsL2MPcpsAcquisition::disconnect(gr::top_block_sptr top_block)
{
    if (item_type_.compare("gr_complex") == 0)
        {
            top_block->disconnect(stream_to_vector_, 0, acquisition_cc_, 0);
        }
    else if (item_type_.compare("cshort") == 0)
        {
            // Since a short-based acq implementation is not available,
            // we just convert cshorts to gr_complex
            top_block->disconnect(cshort_to_float_x2_, 0, float_to_complex_, 0);
            top_block->disconnect(cshort_to_float_x2_, 1, float_to_complex_, 1);
            top_block->disconnect(float_to_complex_, 0, stream_to_vector_, 0);
            top_block->disconnect(stream_to_vector_, 0, acquisition_cc_, 0);
        }
    else if (item_type_.compare("cbyte") == 0)
        {
            // Since a byte-based acq implementation is not available,
            // we just convert cshorts to gr_complex
            top_block->disconnect(cbyte_to_float_x2_, 0, float_to_complex_, 0);
            top_block->disconnect(cbyte_to_float_x2_, 1, float_to_complex_, 1);
            top_block->disconnect(float_to_complex_, 0, stream_to_vector_, 0);
            top_block->disconnect(stream_to_vector_, 0, acquisition_cc_, 0);
        }
    else
        {
            LOG(WARNING) << item_type_ << " unknown acquisition item type";
        }
}


gr::basic_block_sptr GpsL2MPcpsAcquisition::get_left_block()
{
    if (item_type_.compare("gr_complex") == 0)
        {
            return stream_to_vector_;
        }
    else if (item_type_.compare("cshort") == 0)
        {
            return cshort_to_float_x2_;
        }
    else if (item_type_.compare("cbyte") == 0)
        {
            return cbyte_to_float_x2_;
        }
    else
        {
            LOG(WARNING) << item_type_ << " unknown acquisition item type";
            return nullptr;
        }
}


gr::basic_block_sptr GpsL2MPcpsAcquisition::get_right_block()
{
    return acquisition_cc_;
}

