#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include "fork.h"
#include "api.h"
#include "vetek.h"
#include "serial.h"
#include "sound.h"
#include "status.h"

#define OFFLINE 		0
#define ONLINE  		1
#define MEASURING		2
#define FOUND_WEIGHT	        3

static void handle_packet(VetekScalePacket *packet);
static void changeStatus(VetekScalePacket *packet, int newStatus);

static int status = OFFLINE;
static char last_weight[10];
static time_t last_weight_time;
static time_t last_packet_time;
static const unsigned char *house = "Tommy";

int main(int argc, char **argv)
{
   openlog("embedded-scale", LOG_PID|LOG_CONS|LOG_NDELAY, LOG_DAEMON);
   syslog(LOG_INFO, "Starting");
   
   daemonize();

   runner_status_init();

   if (watchdog_run() == -1)
     {
	syslog(LOG_CRIT, "Could not start watchdog");
	exit(2);
     }

   if (api_init() != 0)
     return -1;

   sound_init();

   Serial *serial = serial_open("/dev/ttyUSB0", B9600);
   while(1) {
      time_t now = time(NULL);
      if (now - serial->last_fetch > 2)
	sleep(1);

      char *str = serial_read_line(serial, '\r');
      if (str != NULL)
	{
	   VetekScalePacket *scale = vetek_scale_parse_packet(str);
	   handle_packet(scale);
	}

      if (sem_wait(&runner_status->semaphore) == -1)
	continue;

      runner_status->last_weight_time = last_weight_time;
      runner_status->last_packet_time = last_packet_time;
      sem_post(&runner_status->semaphore);
   }
   serial_close(serial);
   api_cleanup();
   sound_cleanup();
   syslog(LOG_INFO, "Runner is shutting down");
   return 0;   
}

static void handle_packet(VetekScalePacket *packet)
{
	if (packet == NULL)
	{
		if (time(NULL) - last_packet_time > 3)
			changeStatus(NULL, OFFLINE);

		return;
	}

	last_packet_time = time(NULL);

	if (status == OFFLINE)
		changeStatus(packet, ONLINE);

	// Zero weight
	if (strcmp(packet->weight, "   0.00") == 0) {
		if (status == MEASURING || status == FOUND_WEIGHT) {
			changeStatus(packet, ONLINE);
		}
	}
	else // Non-zero weight
	{
		if (status == ONLINE)
			changeStatus(packet, MEASURING);
	}

	if (status == MEASURING)
	{
		// Same weight
		if (strcmp(last_weight, packet->weight) == 0)
		{
			if (last_packet_time - last_weight_time >= 2) {
				changeStatus(packet, FOUND_WEIGHT);
			}
		}
		else // New weight
		{
			memcpy(last_weight, packet->weight, sizeof(last_weight) / sizeof(char));
			last_weight_time = last_packet_time;
		}
	}
}

static void changeStatus(VetekScalePacket *packet, int newStatus)
{
	unsigned char *b;
	int oldStatus = status;
	status = newStatus;
	switch(status)
	{
	case ONLINE:
		last_weight_time = time(NULL);
		if (oldStatus == FOUND_WEIGHT || oldStatus == MEASURING)
			break;
		sound_play(SOUND_ONLINE);
		break;
	case OFFLINE:
		sound_play(SOUND_OFFLINE);
		break;
	case MEASURING:
		sound_play(SOUND_MEASURING);
		break;
	case FOUND_WEIGHT:
		b = packet->weight;
		while(*b == ' ')
			b++;

		sound_play(SOUND_MEASUREMENT_DONE);
		api_register_weight(house, b);
		break;
	}
}

