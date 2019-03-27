/*!
 * \file dll_pll_veml_tracking_fpga.h
 * \brief Implementation of a code DLL + carrier PLL tracking block using an FPGA.
 * \author Marc Majoral, 2019. marc.majoral(at)cttc.es
 * \author Javier Arribas, 2019. jarribas(at)cttc.es
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2019  (see AUTHORS file for a list of contributors)
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
 * along with GNSS-SDR. If not, see <https://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------
 */

#ifndef GNSS_SDR_DLL_PLL_VEML_TRACKING_FPGA_H
#define GNSS_SDR_DLL_PLL_VEML_TRACKING_FPGA_H

#include "dll_pll_conf_fpga.h"
#include "tracking_2nd_DLL_filter.h"
#include "tracking_2nd_PLL_filter.h"
#include <boost/circular_buffer.hpp>
#include <boost/shared_ptr.hpp>
#include <gnuradio/block.h>
#include <gnuradio/gr_complex.h>  // for gr_complex
#include <gnuradio/types.h>       // for gr_vector_const_void_star
#include <pmt/pmt.h>              // for pmt_t
#include <cstdint>
#include <deque>    // for deque
#include <fstream>  // for ofstream
#include <memory>   // for shared_ptr
#include <string>
#include <utility>

class Fpga_Multicorrelator_8sc;
class Gnss_Synchro;
class dll_pll_veml_tracking_fpga;

using dll_pll_veml_tracking_fpga_sptr = boost::shared_ptr<dll_pll_veml_tracking_fpga>;

dll_pll_veml_tracking_fpga_sptr dll_pll_veml_make_tracking_fpga(const Dll_Pll_Conf_Fpga &conf_);


/*!
 * \brief This class implements a code DLL + carrier PLL tracking block.
 */
class dll_pll_veml_tracking_fpga : public gr::block
{
public:
    ~dll_pll_veml_tracking_fpga();

    void set_channel(uint32_t channel);
    void set_gnss_synchro(Gnss_Synchro *p_gnss_synchro);
    void start_tracking();
    void stop_tracking();

    int general_work(int noutput_items, gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items, gr_vector_void_star &output_items);

    void reset(void);

private:
    friend dll_pll_veml_tracking_fpga_sptr dll_pll_veml_make_tracking_fpga(const Dll_Pll_Conf_Fpga &conf_);
    void msg_handler_telemetry_to_trk(const pmt::pmt_t &msg);
    dll_pll_veml_tracking_fpga(const Dll_Pll_Conf_Fpga &conf_);
    void msg_handler_preamble_index(pmt::pmt_t msg);

    bool cn0_and_tracking_lock_status(double coh_integration_time_s);
    bool acquire_secondary();
    void do_correlation_step(void);
    void run_dll_pll();
    void update_tracking_vars();
    void clear_tracking_vars();
    void save_correlation_results();
    void log_data(bool integrating);
    int32_t save_matfile();

    void run_state_2(Gnss_Synchro &current_synchro_data);

    // tracking configuration vars
    Dll_Pll_Conf_Fpga trk_parameters;
    bool d_veml;
    bool d_cloop;
    uint32_t d_channel;
    Gnss_Synchro *d_acquisition_gnss_synchro;

    //Signal parameters
    bool d_secondary;
    bool interchange_iq;
    double d_signal_carrier_freq;
    double d_code_period;
    double d_code_chip_rate;
    uint32_t d_secondary_code_length;
    uint32_t d_code_length_chips;
    uint32_t d_code_samples_per_chip;  // All signals have 1 sample per chip code except Gal. E1 which has 2 (CBOC disabled) or 12 (CBOC enabled)
    int32_t d_symbols_per_bit;
    std::string systemName;
    std::string signal_type;
    std::string *d_secondary_code_string;
    std::string signal_pretty_name;

    int32_t *d_preambles_symbols;
    boost::circular_buffer<float> d_symbol_history;

    //tracking state machine
    int32_t d_state;
    //Integration period in samples
    int32_t d_correlation_length_ms;
    int32_t d_n_correlator_taps;

    float *d_local_code_shift_chips;
    float *d_prompt_data_shift;
    std::shared_ptr<Fpga_Multicorrelator_8sc> multicorrelator_fpga;
    /*  TODO: currently the multicorrelator does not support adding extra correlator
        with different local code, thus we need extra multicorrelator instance.
        Implement this functionality inside multicorrelator class
        as an enhancement to increase the performance
     */
    gr_complex *d_correlator_outs;
    gr_complex *d_Very_Early;
    gr_complex *d_Early;
    gr_complex *d_Prompt;
    gr_complex *d_Late;
    gr_complex *d_Very_Late;

    bool d_enable_extended_integration;
    int32_t d_extend_correlation_symbols_count;
    int32_t d_current_symbol;

    gr_complex d_VE_accu;
    gr_complex d_E_accu;
    gr_complex d_P_accu;
    gr_complex d_L_accu;
    gr_complex d_VL_accu;
    gr_complex d_last_prompt;

    gr_complex *d_Prompt_Data;

    double d_code_phase_step_chips;
    double d_code_phase_rate_step_chips;
    boost::circular_buffer<std::pair<double, double>> d_code_ph_history;
    double d_carrier_phase_step_rad;
    double d_carrier_phase_rate_step_rad;
    boost::circular_buffer<std::pair<double, double>> d_carr_ph_history;
    // remaining code phase and carrier phase between tracking loops
    double d_rem_code_phase_samples;
    float d_rem_carr_phase_rad;

    // PLL and DLL filter library
    Tracking_2nd_DLL_filter d_code_loop_filter;
    Tracking_2nd_PLL_filter d_carrier_loop_filter;

    // acquisition
    double d_acq_code_phase_samples;
    double d_acq_carrier_doppler_hz;

    // tracking vars
    double d_carr_error_hz;
    double d_carr_error_filt_hz;
    double d_code_error_chips;
    double d_code_error_filt_chips;
    double d_code_freq_chips;
    double d_carrier_doppler_hz;
    double d_acc_carrier_phase_rad;
    double d_rem_code_phase_chips;
    double T_chip_seconds;
    double T_prn_seconds;
    double T_prn_samples;
    double K_blk_samples;
    // PRN period in samples
    int32_t d_current_prn_length_samples;
    // processing samples counters
    uint64_t d_sample_counter;
    uint64_t d_acq_sample_stamp;

    uint64_t d_absolute_samples_offset;

    // CN0 estimation and lock detector
    int32_t d_cn0_estimation_counter;
    int32_t d_carrier_lock_fail_counter;
    std::deque<float> d_carrier_lock_detector_queue;
    double d_carrier_lock_test;
    double d_CN0_SNV_dB_Hz;
    double d_carrier_lock_threshold;
    std::deque<gr_complex> d_Prompt_buffer_deque;
    gr_complex *d_Prompt_buffer;

    // file dump
    std::ofstream d_dump_file;
    std::string d_dump_filename;
    bool d_dump;
    bool d_dump_mat;

    // extra
    int32_t d_correlation_length_samples;
    int32_t d_next_prn_length_samples;
    uint64_t d_sample_counter_next;
};

#endif  //GNSS_SDR_DLL_PLL_VEML_TRACKING_FPGA_H
