#ifndef _STATUS_H_
#define _STATUS_H_
#include <sys/types.h>
#include <time.h>
#include <semaphore.h>

struct runner_status_struct 
{
   time_t starttime, last_weight_time, last_packet_time, ping;
   pid_t pid;
   sem_t semaphore;
   int num_invalid_packets;
};

extern struct runner_status_struct *runner_status;
void runner_status_init();
#endif
