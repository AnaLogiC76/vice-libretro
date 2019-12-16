/*! \file resid/sid.h */

//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2010  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#ifndef RESID_SID_H
#define RESID_SID_H

#include "resid-config.h"
#include "voice.h"
#ifdef __LIBRETRO__
#include <stdio.h>
#include "filter.h"
extern int RETROSID8580FILTER;
namespace reSID { class SID; }
namespace RESID_NEW_8580_FILTER {
using reSID::chip_model;
using reSID::reg4;
using reSID::reg8;
using reSID::reg12;
using reSID::cycle_count;
#define SID ::reSID::SID;
#undef RESID_FILTER_H
#include "filter8580new.h"
#undef SID
}
typedef ::RESID_NEW_8580_FILTER::reSID::Filter Filter_new;
#include <algorithm>
#else
#if NEW_8580_FILTER
#include "filter8580new.h"
#else
#include "filter.h"
#endif
#endif
#include "extfilt.h"
#include "pot.h"

namespace reSID
{

class SID
{
public:
  SID();
  ~SID();

  void set_chip_model(chip_model model);
  void set_voice_mask(reg4 mask);
  void enable_filter(bool enable);
  void adjust_filter_bias(double dac_bias);
  void enable_external_filter(bool enable);
  bool set_sampling_parameters(double clock_freq, sampling_method method,
  double sample_freq, double pass_freq = -1,
  double filter_scale = 0.97);
  void adjust_sampling_frequency(double sample_freq);

  void clock();
  void clock(cycle_count delta_t);
  int clock(cycle_count& delta_t, short* buf, int n, int interleave = 1);
  void reset();

  // Read/write registers.
  reg8 read(reg8 offset);
  void write(reg8 offset, reg8 value);

  // Read/write state.
  class State
  {
  public:
    State();

    char sid_register[0x20];

    reg8 bus_value;
    cycle_count bus_value_ttl;
    cycle_count write_pipeline;
    reg8 write_address;
    reg4 voice_mask;

    reg24 accumulator[3];
    reg24 shift_register[3];
    cycle_count shift_register_reset[3];
    cycle_count shift_pipeline[3];
    reg16 pulse_output[3];
    cycle_count floating_output_ttl[3];

    reg16 rate_counter[3];
    reg16 rate_counter_period[3];
    reg16 exponential_counter[3];
    reg16 exponential_counter_period[3];
    reg8 envelope_counter[3];
    EnvelopeGenerator::State envelope_state[3];
    bool hold_zero[3];
    cycle_count envelope_pipeline[3];
  };

  State read_state();
  void write_state(const State& state);

  // 16-bit input (EXT IN).
  void input(short sample);

  // 16-bit output (AUDIO OUT).
  short output();

 protected:
  static double I0(double x);
  int clock_fast(cycle_count& delta_t, short* buf, int n, int interleave);
  int clock_interpolate(cycle_count& delta_t, short* buf, int n, int interleave);
  int clock_resample(cycle_count& delta_t, short* buf, int n, int interleave);
  int clock_resample_fastmem(cycle_count& delta_t, short* buf, int n, int interleave);
  void write();

  chip_model sid_model;
  Voice voice[3];
  Filter filter_old;
  Filter_new filter_new;
  class Filter_base
  {
  protected :
	  SID &s;

	  Filter_base(SID &sid)
		  : s(sid)
	  {
	  }
  };
  template<class F, class BASE> class Filter_wrap : public BASE
  {
	  int test=0;
	  F &f;

	  Filter_wrap(SID &s, Filter &)
		  :	BASE(s),f(s.filter_old)
	  {
		  printf("Filter_wrap<Filter_old> &f=%p\n",&f);
	  }
	  Filter_wrap(SID &s, Filter_new &)
		  : BASE(s),f(s.filter_new)
	  {
		  printf("Filter_wrap<Filter_new> &f=%p\n",&f);
	  }
	  Filter_wrap(SID &s, Filter_wrap<Filter, Filter_base> &)
		  : BASE(s),f(*(Filter_wrap<Filter, Filter_base> *)&s.filter_fwd)
	  {
		  printf("Filter_wrap<root>       &f=%p\n",&f);
		  set(RETROSID8580FILTER,MOS8580);
	  }
  public :
	  Filter_wrap(SID &sid)
	:	Filter_wrap(sid, *(F *)nullptr)
	  {
	  }
	  void reinit(chip_model model)
	  {
		  printf("%p.reinit &f=%p output=%p\n",*(unsigned long long *)this,&f,&output);
		  f.enable_filter(true);
		  f.set_chip_model(model);
		  f.set_voice_mask(7);
		  f.input(0);
		  f.reset();
	  }
	  void set(int variant, chip_model model)
	  {
		  printf("%p.set_begin &f=%p\n",*(unsigned long long *)this, &f);
		  switch (variant)
		  {
		  case 1:
		  {
			  auto p=new(&this->s.filter_fwd) Filter_wrap<Filter_new,Filter_wrap<Filter,Filter_base > >(this->s);
			  p->reinit(model);
		  }
			  break;
		  default:
		  {
			  auto p=new(&this->s.filter_fwd) Filter_wrap<Filter,Filter_base>(this->s);
			  p->reinit(model);
		  }
			  break;
		  }
		  printf("%p=>%p.set_end\n",*(unsigned long long *)this, &f);
	  }
	  // Forwarding functions from filter.h
	  virtual void enable_filter(bool enable) { f.enable_filter(enable); }
	  virtual void adjust_filter_bias(double dac_bias) { f.adjust_filter_bias(dac_bias); }
	  virtual void set_chip_model(chip_model model)
	  {
		  set_chip_model(model, *(F *)nullptr);
	  }
	  void set_chip_model(chip_model model, Filter_wrap<Filter, Filter_base> &)
	  {
		set(RETROSID8580FILTER,model);
		printf("With %s filters\n",RETROSID8580FILTER ? "new" : "old");
	  }
	  template<class ANY> void set_chip_model(chip_model model, ANY &) { f.set_chip_model(model); }
	  virtual void set_voice_mask(reg4 mask) { f.set_voice_mask(mask); }

	  virtual void clock(int voice1, int voice2, int voice3) { f.clock(voice1,voice2,voice3); }
	  virtual void clock(cycle_count delta_t, int voice1, int voice2, int voice3) { f.clock(delta_t,voice1,voice2,voice3); }
	  virtual void reset() { f.reset(); }

	  // Write registers.
	  virtual void writeFC_LO(reg8 v) { f.writeFC_LO(v); }
	  virtual void writeFC_HI(reg8 v) { f.writeFC_HI(v); }
	  virtual void writeRES_FILT(reg8 v) { f.writeRES_FILT(v); }
	  virtual void writeMODE_VOL(reg8 v) { f.writeMODE_VOL(v); }

	  // SID audio input (16 bits).
	  virtual void input(short sample) { f.input(sample); }

	  // SID audio output (16 bits).
	  virtual short output() { return f.output(); }

#if 0
#define FWD_VAR(type,name) \
	  struct _get_ ## name { F &f;_get_ ## name(F &f) : f(f) { };operator type() { printf("%p._get_"#name"()\n",this);return f.name; } } name=f;
#else
#define FWD_VAR(type,name) \
	  virtual type _getf_ ## name () { printf("%p._getf_"#name"()\n",this);return f.name; } \
	  struct _get_ ## name { Filter_wrap &o;_get_ ## name(Filter_wrap &o) : o(o) { };operator type() { return o._getf_ ## name(); } } name=*this;
#endif

	  FWD_VAR(reg12, fc)
	  FWD_VAR(reg8, res)
	  FWD_VAR(reg4, mode)
	  FWD_VAR(reg8, filt)
	  FWD_VAR(reg4, vol)
	  FWD_VAR(reg8, voice_mask)
#undef FWD_VAR
  };
  char filter_fwd[std::max(sizeof(Filter_wrap<Filter, Filter_base>),sizeof(Filter_wrap<Filter_new, Filter_wrap<Filter, Filter_base > >))];
  Filter_wrap<Filter_wrap<Filter, Filter_base>, Filter_base> filter=*this;
  ExternalFilter extfilt;
  Potentiometer potx;
  Potentiometer poty;

  reg8 bus_value;
  cycle_count bus_value_ttl;

  // The data bus TTL for the selected chip model
  cycle_count databus_ttl;

  // Pipeline for writes on the MOS8580.
  cycle_count write_pipeline;
  reg8 write_address;

  double clock_frequency;

  enum {
    // Resampling constants.
    // The error in interpolated lookup is bounded by 1.234/L^2,
    // while the error in non-interpolated lookup is bounded by
    // 0.7854/L + 0.4113/L^2, see
    // http://www-ccrma.stanford.edu/~jos/resample/Choice_Table_Size.html
    // For a resolution of 16 bits this yields L >= 285 and L >= 51473,
    // respectively.
    FIR_N = 125,
    FIR_RES = 285,
    FIR_RES_FASTMEM = 51473,
    FIR_SHIFT = 15,

    RINGSIZE = 1 << 14,
    RINGMASK = RINGSIZE - 1,

    // Fixed point constants (16.16 bits).
    FIXP_SHIFT = 16,
    FIXP_MASK = 0xffff
  };

  // Sampling variables.
  sampling_method sampling;
  cycle_count cycles_per_sample;
  cycle_count sample_offset;
  int sample_index;
  short sample_prev, sample_now;
  int fir_N;
  int fir_RES;
  double fir_beta;
  double fir_f_cycles_per_sample;
  double fir_filter_scale;

  // Ring buffer with overflow for contiguous storage of RINGSIZE samples.
  short* sample;

  // FIR_RES filter tables (FIR_N*FIR_RES).
  short* fir;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(RESID_SID_CC)

// ----------------------------------------------------------------------------
// Read 16-bit sample from audio output.
// ----------------------------------------------------------------------------
RESID_INLINE
short SID::output()
{
  return extfilt.output();
}


// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void SID::clock()
{
  int i;

  // Clock amplitude modulators.
  for (i = 0; i < 3; i++) {
    voice[i].envelope.clock();
  }

  // Clock oscillators.
  for (i = 0; i < 3; i++) {
    voice[i].wave.clock();
  }

  // Synchronize oscillators.
  for (i = 0; i < 3; i++) {
    voice[i].wave.synchronize();
  }

  // Calculate waveform output.
  for (i = 0; i < 3; i++) {
    voice[i].wave.set_waveform_output();
  }

  // Clock filter.
  filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

  // Clock external filter.
  extfilt.clock(filter.output());

  // Pipelined writes on the MOS8580.
  if (unlikely(write_pipeline)) {
    write();
  }

  // Age bus value.
  if (unlikely(!--bus_value_ttl)) {
    bus_value = 0;
  }
}

#endif // RESID_INLINING || defined(RESID_SID_CC)

} // namespace reSID

#endif // not RESID_SID_H
