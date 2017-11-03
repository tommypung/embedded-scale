#include <ao/ao.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "sound.h"

static ao_device *device;
static ao_sample_format format;
static int default_driver;
static char *buffer;
static int buf_size;
static int sample;
static float freq = 440.0;
static int i;

int sound_cleanup()
{
	ao_close(device);
	ao_shutdown();
	return 0;
}

int sound_init()
{
	ao_initialize();

	default_driver = ao_default_driver_id();

	memset(&format, 0, sizeof(format));
	format.bits = 16;
	format.channels = 2;
	format.rate = 44100;
	format.byte_format = AO_FMT_LITTLE;

	/* -- Open driver -- */
	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		fprintf(stderr, "Error opening device.\n");
		return 1;
	}

	return 0;
}

void sound_play(int sound)
{
	printf("Playing sound %d.wav\n", sound);
	if (device == NULL)
		return;

	/* -- Play some stuff -- */
	buf_size = format.bits/8 * format.channels * format.rate;
	buffer = calloc(buf_size,
			sizeof(char));

	for (i = 0; i < format.rate; i++) {
		sample = (int)(0.75 * 32768.0 *
				sin(2 * M_PI * freq * sound * ((float) i/format.rate)));

		/* Put the same stuff in left and right channel */
		buffer[4*i] = buffer[4*i+2] = sample & 0xff;
		buffer[4*i+1] = buffer[4*i+3] = (sample >> 8) & 0xff;
	}
	ao_play(device, buffer, buf_size);

}
