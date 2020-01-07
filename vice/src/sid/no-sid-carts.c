#include "lib.h"

void sid_set_enable(int value)
{
}

int sid_resources_init(void)
{
	return 0;
}

int sid_cmdline_options_init(int sid_type)
{
	return 0;
}

typedef struct {} snapshot_t;

int sid_snapshot_write_module(snapshot_t *s)
{
	return -1;
}

int sid_snapshot_read_module(snapshot_t *s)
{
	return -1;
}

typedef struct {} sound_t;

sound_t *sid_sound_machine_open(int chipno)
{
	return 0;
}

int sid_sound_machine_init_vbr(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
	return 0;
}

int sid_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
	return 0;
}

void sid_sound_machine_close(sound_t *psid)
{
}

uint8_t sid_sound_machine_read(sound_t *psid, uint16_t addr)
{
	return 0;
}

void sid_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t byte)
{
}

void sid_sound_machine_reset(sound_t *psid, CLOCK cpu_clk)
{
}

int sid_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, int *delta_t)
{
	return 0;
}

int sid_sound_machine_cycle_based(void)
{
	return 0;
}

int sid_sound_machine_channels(void)
{
	return 1;
}

uint8_t sid_read(uint16_t addr)
{
	return 0;
}

uint8_t sid_peek(uint16_t addr)
{
	return 0;
}

void sid_store(uint16_t addr, uint8_t byte)
{
}

int sid_dump(void)
{
	return 0;
}

void sid_reset(void)
{
}

void sid_set_machine_parameter(long clock_rate)
{
}

void sid_sound_machine_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
}

char *sid_sound_machine_dump_state(sound_t *psid)
{
    return lib_stralloc("");
}

void sid_sound_machine_enable(int enable)
{
}

int sid_set_engine_model(int engine, int model)
{
	return 0;
}
