#include <stdio.h>
namespace reSID { struct Filter_proxy; }
#define SID ::reSID::Filter_proxy
#include "filter.h"
extern int RETROSID8580FILTER;
namespace RESID_NEW_8580_FILTER {
using reSID::chip_model;
using reSID::reg4;
using reSID::reg8;
using reSID::reg12;
using reSID::cycle_count;
#undef RESID_FILTER_H
#include "filter8580new.h"
#undef SID
}
typedef ::RESID_NEW_8580_FILTER::reSID::Filter Filter_new;
#include <algorithm>

namespace reSID {
	struct Filter_proxy {
  /* Emulate class Filter by forwarding everything to Filter-compatible instance f in base-class
  As a bonus, variables directly accessed for saving state are read-only by design. */
  template<class Filter_instance> struct Filter_wrap : public Filter_instance
  {
  public :
	  // Forwarding functions from filter.h
	  virtual void enable_filter(bool enable) { this->f.enable_filter(enable); }
	  virtual void adjust_filter_bias(double dac_bias) { this->f.adjust_filter_bias(dac_bias); }
	  virtual void set_chip_model(chip_model model) { this->f.set_chip_model(model); }
	  virtual void set_voice_mask(reg4 mask) { this->f.set_voice_mask(mask); }

	  virtual void clock(int voice1, int voice2, int voice3) { this->f.clock(voice1,voice2,voice3); }
	  virtual void clock(cycle_count delta_t, int voice1, int voice2, int voice3) { this->f.clock(delta_t,voice1,voice2,voice3); }
	  virtual void reset() { this->f.reset(); }

	  // Write registers.
	  virtual void writeFC_LO(reg8 v) { this->f.writeFC_LO(v); }
	  virtual void writeFC_HI(reg8 v) { this->f.writeFC_HI(v); }
	  virtual void writeRES_FILT(reg8 v) { this->f.writeRES_FILT(v); }
	  virtual void writeMODE_VOL(reg8 v) { this->f.writeMODE_VOL(v); }

	  // SID audio input (16 bits).
	  virtual void input(short sample) { this->f.input(sample); }

	  // SID audio output (16 bits).
	  virtual short output() { return this->f.output(); }

#define FWD_VAR(type,name) \
	  virtual type _getf_ ## name () { printf("%p._getf_"#name"()\n",this);return this->f.name; } \
	  struct _get_ ## name { Filter_wrap &o;_get_ ## name(Filter_wrap &o) : o(o) { };operator type() { return o._getf_ ## name(); } } name=*this;

	  FWD_VAR(reg12, fc)
	  FWD_VAR(reg8, res)
	  FWD_VAR(reg4, mode)
	  FWD_VAR(reg8, filt)
	  FWD_VAR(reg4, vol)
	  FWD_VAR(reg8, voice_mask)
#undef FWD_VAR
  };
  /* Filter_wrap_base needs to declare all functions virtual, as all functions are overloaded
  f is never actually accessed so its value doesn't matter. */
  class Filter_noinstance { protected : Filter &f=*(Filter *)nullptr; };
  typedef Filter_wrap< Filter_noinstance > Filter_wrap_base;
  /* Instantiate filter, overwriting the unusable f-reference in base. */
  template<class F> class Filter_instance : public Filter_wrap_base { protected : F f; };
  typedef Filter_wrap< Filter_instance<Filter> > Filter_wrap_old;
  typedef Filter_wrap< Filter_instance<Filter_new> > Filter_wrap_new;

  struct Filter_wrap_setup
  {
	  union
	  {
		  char _old[sizeof(Filter_wrap_old)];
		  char _new[sizeof(Filter_wrap_new)];
	  } filter;

	  Filter_wrap_setup()
	  {
		  printf("Filter_wrap_setup %d\n",RETROSID8580FILTER);
		  if (RETROSID8580FILTER)
			  new(filter._new) Filter_wrap_new();
		  else
			  new(filter._old) Filter_wrap_old();
		  printf("Filter_wrap_setup done\n");
	  }
	  operator Filter_wrap_base &() { return *(Filter_wrap_base *)filter._old; }
  };
};
}
