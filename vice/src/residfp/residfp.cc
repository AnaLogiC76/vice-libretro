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

#include "builders/residfp-builder/residfp/SID.h"

using namespace reSIDfp;

extern "C" {

struct sound_s
{
    /* speed factor */
    int factor;

    /* resid sid implementation */
    reSIDfp::SID *sid;
};

typedef struct sound_s sound_t;

static sound_t *residfp_open(uint8_t *sidstate)
{
    sound_t *psid;
    int i;

    psid = new sound_t;
    psid->sid = new reSIDfp::SID;

#ifdef TODO
    for (i = 0x00; i <= 0x18; i++) {
        psid->sid->write(i, sidstate[i]);
    }
#endif

    return psid;
}

static FILE *wav_fd = NULL;
static int samples = 0;
static uint32_t rate;
static bool wait0;

/* Store number as little endian. */
static void le_store(uint8_t *buf, uint32_t val, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        buf[i] = (uint8_t)(val & 0xff);
        val >>= 8;
    }
}

static int residfp_init(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
	SamplingMethod method;
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

    switch (model) {
      default:
      case 0:
        psid->sid->setChipModel(MOS6581);
//        psid->sid->set_voice_mask(0x07);
        psid->sid->input(0);
        strcpy(model_text, "MOS6581");
        break;
      case 1:
        psid->sid->setChipModel(MOS8580);
//        psid->sid->set_voice_mask(0x07);
        psid->sid->input(0);
        strcpy(model_text, "MOS8580");
        break;
      case 2:
        psid->sid->setChipModel(MOS8580);
//        psid->sid->set_voice_mask(0x0f);
        psid->sid->input(-32768);
        strcpy(model_text, "MOS8580 + digi boost");
        break;
    }
    psid->sid->enableFilter(filters_enabled ? true : false);
    switch (model) {
      default:
      case 0:
        psid->sid->setFilter6581Curve(1.0-((filter_bias_mV+5000.0)/10000.0));
        break;
      case 1:
        psid->sid->setFilter8580Curve(1.0-((filter_bias_mV+5000.0)/10000.0));
        break;
    }
//    psid->sid->enable_external_filter(filters_enabled ? true : false);

    switch (sampling) {
      default:
      case 0:
        method = DECIMATE;
        strcpy(method_text, "decimate");
        break;
      case 1:
        method = RESAMPLE;
        strcpy(method_text, "resample");
        break;
    }

	try
	{
        // from residfp-emu.cpp: Round half frequency to the nearest multiple of 5000
        const int halfFreq = 5000*((static_cast<int>(speed)+5000)/10000);
    psid->sid->setSamplingParameters(cycles_per_sec, method,
                                            speed, std::min(halfFreq, 20000));
	}
	catch (const SIDError &e)
	{
        log_warning(LOG_DEFAULT,
                    "reSID: Out of spec, increase sampling rate or decrease maximum speed");
        return 0;
    }

	log_message(LOG_DEFAULT, "reSID-fp: %s, filter %s, sampling rate %dHz - %s",
                model_text,
                filters_enabled ? "on" : "off",
                speed, method_text);

    /* RIFF/WAV header. */
    unsigned char header[45] = "RIFFllllWAVEfmt \020\0\0\0\001\0ccrrrrbbbb88\020\0datallll";
    uint32_t sample_rate = rate = (uint32_t)speed;
	uint32_t channels=1;
    uint32_t bytes_per_sec = (uint32_t)(speed * channels * 2);

    wav_fd = fopen("d:/temp/global/tmp.wav", "wb");
    if (!wav_fd) {
		printf("wav_fd open failed\n");
        return 1;
    }

    /* Reset number of samples. */
    samples = 0;

    /* Initialize header. */
    le_store(header + 22, (uint32_t)channels, 2);
    le_store(header + 24, sample_rate, 4);
    le_store(header + 28, bytes_per_sec, 4);
    le_store(header + 32, (uint32_t)channels * 2, 2);

    fwrite(header, 1, 44, wav_fd);
	wait0=true;
	printf("wav_open\n");

    return 1;
}

void resid_dump_close()
{
	if (!wav_fd)
		return;

	printf("wav_close\n");
    int res = -1;
    uint8_t rlen[4];
    uint8_t dlen[4];
    uint32_t rifflen = (uint32_t)(samples * 2 + 36);
    uint32_t datalen = (uint32_t)(samples * 2);

    le_store(rlen, rifflen, 4);
    le_store(dlen, datalen, 4);

    fseek(wav_fd, 4, SEEK_SET);
    if (fwrite(rlen, 1, 4, wav_fd) == 4) {
        fseek(wav_fd, 32, SEEK_CUR);
        if (fwrite(dlen, 1, 4, wav_fd) == 4) {
            res = 0;
        }
    }

    fclose(wav_fd);
    wav_fd = NULL;
    if (res != 0) {
        log_debug("ERROR wav_close failed.");
    }
}

static void residfp_close(sound_t *psid)
{
    delete psid->sid;
    delete psid;
}

static uint8_t residfp_read(sound_t *psid, uint16_t addr)
{
    return psid->sid->read(addr);
}

static void residfp_store(sound_t *psid, uint16_t addr, uint8_t byte)
{
    psid->sid->write(addr, byte);
}

static void residfp_reset(sound_t *psid, CLOCK cpu_clk)
{
    psid->sid->reset();
}

static int residfp_calculate_samples(sound_t *psid, short *pbuf, int nr,
                                   int interleave, int *delta_t)
{
/*    short *tmp_buf;
    int retval;

    if (psid->factor == 1000) {*/
        int r=psid->sid->clock(*delta_t, pbuf);
		*delta_t=0;
		if (interleave!=1)
			printf("interleave not supported\n");
		if (r>nr)
			printf("%d bytes exceeded %d buffer size\n",r,nr);
	short *p_start=pbuf, *p_end=pbuf+r;
	if (wait0)
	{
		while (p_start!=p_end && *p_start) ++p_start;
		if (p_start==p_end)
			return r;
		wait0=false;
		printf("%.3fs skipped in wait0\n",float(p_start-pbuf)/rate);
	}
	else
	{
		while (p_start!=p_end && !*p_start) ++p_start;
		while (p_start!=p_end && !*(p_end-1)) --p_end;
	}
    fwrite(pbuf, sizeof(int16_t), p_end-p_start, wav_fd);
	int s_old=samples/rate;
    samples += p_end-p_start;
	int s_new=samples/rate;
	if (s_old!=s_new)
		printf("%ds recorded\n",s_new);
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
