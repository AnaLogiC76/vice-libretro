#include <new>

extern int RETROSID8580FILTER;

// Make sure we're friend of Filter-classes for state-backups
namespace reSID { class Filter_proxy; }
#define SID ::reSID::Filter_proxy

#include "filter.h"
#undef RESID_FILTER_H

// Make sure alternate Filter finds all definitions it expects in its namespace
namespace RESID_NEW_8580_FILTER {
using reSID::chip_model;
using reSID::reg4;
using reSID::reg8;
using reSID::reg12;
using reSID::cycle_count;
#include "filter8580new.h"

#undef SID
}

typedef ::RESID_NEW_8580_FILTER::reSID::Filter Filter_new;

namespace reSID {
  class Filter_proxy {
    /* Emulate class Filter by forwarding everything to Filter-compatible instance f in base-class
    As a bonus, variables directly accessed for saving state are read-only by design. */
    template<class Instance> struct Forward : public Instance
    {
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
      virtual type _getf_ ## name () { return this->f.name; } \
      struct _get_ ## name { Forward &o;_get_ ## name(Forward &o) : o(o) { };operator type() { return o._getf_ ## name(); } } name=*this;

      FWD_VAR(reg12, fc)
      FWD_VAR(reg8, res)
      FWD_VAR(reg4, mode)
      FWD_VAR(reg8, filt)
      FWD_VAR(reg4, vol)
      FWD_VAR(reg8, voice_mask)
#undef FWD_VAR
    };
    /* InstanceRef needs to declare all functions virtual, as all functions are overloaded
    f is never actually accessed so its value doesn't matter. Virtual destructor makes sure the proper
	Filter-destructor is called if someday there are any. */
    class PureVirtual { protected : Filter &f=*(Filter *)nullptr; };
  public :
    struct InstanceRef : public Forward< PureVirtual > { virtual ~InstanceRef() { } };
  private :
    /* Instantiate filter, overwriting the unusable f-reference in base. */
    template<class F> class Instance : public InstanceRef { protected : F f; };
    typedef Forward< Instance<Filter> > Forward_Filter;
    typedef Forward< Instance<Filter_new> > Forward_Filter_new;

  public :
    class Create
    {
      union
      {
        char _old[sizeof(Forward_Filter)];
        char _new[sizeof(Forward_Filter_new)];
      } filter;

    public :
      Create()
      {
        if (RETROSID8580FILTER)
          new(filter._new) Forward_Filter_new();
        else
          new(filter._old) Forward_Filter();
      }
	  /* Currently completely unnecessary as neither Filter-class has a destructor, but maybe one day
	  one of them has. */
      ~Create()
      {
        ((Forward_Filter_new *)filter._new)->~Forward_Filter_new();
      }
      operator InstanceRef &() { return *(InstanceRef *)filter._old; }
    };
  };
}
