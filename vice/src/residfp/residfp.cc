#include "vice.h"

extern "C" {

/* QNX has problems with const and inline definitions
   in its string.h file when using g++ */

#ifndef __QNX__
#include <string.h>
#else
extern char *strcpy(char *s1, char *s2);
#endif

#include "sid/sid.h" /* sid_engine_t */
#include "lib.h"
#include "log.h"
//#include "resid.h"
#include "resources.h"
#include "sid-snapshot.h"
#include "types.h"

} // extern "C"

#include "builders/residfp-builder/residfp.h"
#include "sidplayfp/SidConfig.h"
#include "builders/residfp-builder/residfp-emu.h"

using libsidplayfp::ReSIDfp;

extern "C" {

/// System event context
libsidplayfp::EventScheduler eventScheduler;
struct FakeEvent : public libsidplayfp::Event { FakeEvent() : Event("") { } void event() { } } fakeEvent;

struct sound_s
{
    /* speed factor */
    int factor;

    /* resid sid implementation */
	ReSIDfpBuilder *sid_builder;
	ReSIDfp *sid;
};

typedef struct sound_s sound_t;

static sound_t *residfp_open(uint8_t *sidstate)
{
    sound_t *psid;
    int i;

    psid = new sound_t;
	psid->sid_builder = new ReSIDfpBuilder( "ReSIDfp" );
	psid->sid_builder->create (1);

#ifdef TODO
    for (i = 0x00; i <= 0x18; i++) {
        psid->sid->write(i, sidstate[i]);
    }
#endif

    return psid;
}

static CLOCK last_clk;
extern CLOCK maincpu_clk;

static int residfp_init(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
	SidConfig::sampling_method_t method;
	bool fast_resampling;
    char model_text[100];
    char method_text[100];
    int filters_enabled, model, sampling, passband_percentage, gain_percentage, filter_bias_mV;

    if (resources_get_int("SidFilters", &filters_enabled) < 0) {
        return 0;
    }

    if (resources_get_int("SidModel", &model) < 0) {
        return 0;
    }

    if ((model == 1) || (model == 2)) {
        /* 8580 */
/*        if (resources_get_int("SidResid8580Passband", &passband_percentage) < 0) {
            return 0;
        }

        if (resources_get_int("SidResid8580Gain", &gain_percentage) < 0) {
            return 0;
        }*/

        if (resources_get_int("SidResid8580FilterBias", &filter_bias_mV) < 0) {
            return 0;
        }
    } else {
        /* 6581 */
/*        if (resources_get_int("SidResidPassband", &passband_percentage) < 0) {
            return 0;
        }

        if (resources_get_int("SidResidGain", &gain_percentage) < 0) {
            return 0;
        }*/

        if (resources_get_int("SidResidFilterBias", &filter_bias_mV) < 0) {
            return 0;
        }
    }

	if (resources_get_int("SidResidSampling", &sampling) < 0) {
        return 0;
    }

	auto es=&eventScheduler;

    switch (model) {
      default:
      case 0:
		  psid->sid=(ReSIDfp *)psid->sid_builder->lock(es,SidConfig::MOS6581,false);
        strcpy(model_text, "MOS6581");
        break;
      case 1:
		  psid->sid=(ReSIDfp *)psid->sid_builder->lock(es,SidConfig::MOS8580,false);
        strcpy(model_text, "MOS8580");
        break;
      case 2:
		  psid->sid=(ReSIDfp *)psid->sid_builder->lock(es,SidConfig::MOS8580,true);
        strcpy(model_text, "MOS8580 + digi boost");
        break;
    }
    psid->sid_builder->filter(filters_enabled ? true : false);
    switch (model) {
      default:
      case 0:
        psid->sid_builder->filter6581Curve(1.0-((filter_bias_mV+5000.0)/10000.0));
        break;
      case 1:
        psid->sid_builder->filter8580Curve(1.0-((filter_bias_mV+5000.0)/10000.0));
        break;
    }

    switch (sampling) {
      default:
      case 0:
        fast_resampling = true;
		method = SidConfig::INTERPOLATE;
        strcpy(method_text, "fast");
        break;
      case 1:
        fast_resampling = false;
		method = SidConfig::INTERPOLATE;
        strcpy(method_text, "interpolate");
        break;
      case 2:
        fast_resampling = false;
		method = SidConfig::RESAMPLE_INTERPOLATE;
        strcpy(method_text, "resample_interpolate");
        break;
    }

	{
		psid->sid->sampling(cycles_per_sec, speed, method, fast_resampling);
		psid->sid->bufferpos(0);
		last_clk=maincpu_clk;
	}

	log_message(LOG_DEFAULT, "reSID-fp: %s, filter %s, sampling rate %dHz - %s",
                model_text,
                filters_enabled ? "on" : "off",
                speed, method_text);

    return 1;
}

static void residfp_close(sound_t *psid)
{
	if (psid->sid)
		psid->sid_builder->unlock(psid->sid);
    delete psid->sid_builder;
    delete psid;
	resid_dump_close();
}

static uint8_t residfp_read(sound_t *psid, uint16_t addr)
{
	eventScheduler.schedule(fakeEvent, maincpu_clk-last_clk, libsidplayfp::EVENT_CLOCK_PHI2);
	last_clk=maincpu_clk;
	eventScheduler.clock();
    return psid->sid->read(addr);
}

static void residfp_store(sound_t *psid, uint16_t addr, uint8_t byte)
{
	eventScheduler.schedule(fakeEvent, maincpu_clk-last_clk, libsidplayfp::EVENT_CLOCK_PHI2);
	last_clk=maincpu_clk;
	eventScheduler.clock();
    psid->sid->write(addr, byte);
}

static void residfp_reset(sound_t *psid, CLOCK cpu_clk)
{
    eventScheduler.reset();
    last_clk=0;
    psid->sid->reset(0);
}

static int residfp_calculate_samples(sound_t *psid, short *pbuf, int nr,
                                   int interleave, int *delta_t)
{
/*    short *tmp_buf;
    int retval;

    if (psid->factor == 1000) {*/
//        int r=psid->sid->clock(*delta_t, pbuf);
		eventScheduler.schedule(fakeEvent, *delta_t);
		last_clk+=*delta_t;
		eventScheduler.clock();
		psid->sid->clock();
		int r=psid->sid->bufferpos();
		memcpy(pbuf,psid->sid->buffer(),r*sizeof(short));
		psid->sid->bufferpos(0);
		*delta_t=0;
		if (interleave!=1)
			printf("interleave not supported\n");
		if (r>nr)
			printf("%d bytes exceeded %d buffer size\n",r,nr);
		return r;
/*    }
    tmp_buf = getbuf(2 * nr * psid->factor / 1000);
    retval = psid->sid->clock(*delta_t, tmp_buf, nr * psid->factor / 1000, interleave) * 1000 / psid->factor;
    memcpy(pbuf, tmp_buf, 2 * nr);
    return retval;*/
}

static void residfp_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
}

static char *residfp_dump_state(sound_t *psid)
{
    return lib_stralloc("");
}

static void residfp_state_read(sound_t *psid, sid_snapshot_state_t *sid_state)
{
	printf("residfp_state_read unsupported\n");
}

static void residfp_state_write(sound_t *psid, sid_snapshot_state_t *sid_state)
{
	printf("residfp_state_write unsupported\n");
}

sid_engine_t residfp_hooks =
{
    residfp_open,
    residfp_init,
    residfp_close,
    residfp_read,
    residfp_store,
    residfp_reset,
    residfp_calculate_samples,
    residfp_prevent_clk_overflow,
    residfp_dump_state,
    residfp_state_read,
    residfp_state_write
};

} // extern "C"
