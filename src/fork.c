#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include "status.h"
#include "fork.h"

static char *calculate_runtime(time_t runtime);
static struct runner_status_struct *setup_runner_status();

static pid_t watchdog_pid;
static time_t watchdog_starttime;
static long runner_starts = 0;
static const char const *watchdog_status_file_path = "/var/log/embedded-log.status";

static char *old_runner_dumps[4096];
static char runner_status_dump[4096];

char *calculate_runtime(time_t runtime)
{
   static char buffer[100];
   int days = runtime / (24 * 60 * 60);
   runtime -= days * (24 * 60 * 60);

   int hours = runtime / (60 * 60);
   runtime -= hours * (60 * 60);

   int minutes = runtime / 60;
   int seconds = runtime - minutes * 60;

   buffer[0] = '\0';

   if (days > 0)
     sprintf(buffer + strlen(buffer), "%dd ", days);
   if (hours > 0)
     sprintf(buffer + strlen(buffer), "%dh ", hours);
   if (minutes > 0)
     sprintf(buffer + strlen(buffer), "%dm ", minutes);
   if (seconds > 0)
     sprintf(buffer + strlen(buffer), "%ds ", seconds);
   if (buffer[0] != '\0')
     buffer[strlen(buffer) - 1] = '\0';

   return buffer;
}

void watchdog_dump_status()
{
   FILE *fd = NULL;
   fd = fopen(watchdog_status_file_path, "w");
   if (fd == NULL)
     {
	syslog(LOG_CRIT, "Could not open open file %s: %s\n", watchdog_status_file_path, strerror(errno));
	return;
     }
   
   fprintf(fd, "--== embedded-scale watchdog status ==--\n");
   fprintf(fd, "watchdog.startTime = %s", ctime(&watchdog_starttime));
   fprintf(fd, "watchdog.runtime = %s\n", calculate_runtime(time(NULL) - watchdog_starttime));
   fprintf(fd, "watchdog.pid = %d\n", watchdog_pid);
   fprintf(fd, "watchdog.runnerStarts = %d\n", runner_starts);
   fprintf(fd, "\n");

   #define PRINT_RUNNER_TIME(name, variable) sprintf(runner_status_dump + strlen(runner_status_dump), "runner."name" = %s", ctime(&(variable))); sprintf(runner_status_dump + strlen(runner_status_dump), "runner."name".distance = %s\n", calculate_runtime(time(NULL) - (variable)));

   sprintf(runner_status_dump, "runner.pid = %d\n", runner_status->pid);
   PRINT_RUNNER_TIME("startTime", runner_status->starttime);
   PRINT_RUNNER_TIME("lastWeightTime", runner_status->last_weight_time);
   PRINT_RUNNER_TIME("lastPacketTime", runner_status->last_packet_time);
   fwrite(runner_status_dump, strlen(runner_status_dump), 1, fd);

   if (old_runner_dumps[0] != NULL)
     {
	fprintf(fd, "\n--== Old runners ==--\n");
	for(int i=0;i<sizeof(old_runner_dumps)/sizeof(char *);i++)
	  {
	     char *o = old_runner_dumps[i];
	     if (o == NULL)
	       break;

	     fwrite(o, strlen(o), 1, fd);
	     fprintf(fd, "\n");
	  }
     }
   fclose(fd);
}

int watchdog_run()
{
   watchdog_pid = getpid();
   syslog(LOG_INFO, "Running the watchdog");
   watchdog_starttime = time(NULL);
   memset(old_runner_dumps, 0, sizeof(old_runner_dumps));

   while(1)
     {
	pid_t fork_response = fork();

	if (fork_response < 0) {
	   syslog(LOG_CRIT, "Could not spawn the runner: %s", strerror(errno));
	   exit(1);
	}

	if (fork_response > 0) {
	   runner_status->pid = fork_response;
	   runner_status->starttime = time(NULL);
	   runner_starts++;
	   if (runner_status_dump[0] != '\0')
	     {
		for(int i=0;i<sizeof(old_runner_dumps)/sizeof(char *);i++)
		  if (old_runner_dumps[i] == NULL) 
		    {
		       old_runner_dumps[i] = strdup(runner_status_dump);
		       break;
		    }
	     }
	   syslog(LOG_CRIT, "Spawned a new runner at: %d", runner_status->pid);
	   while(1)
	     {
		watchdog_dump_status();
		int status;
		int ret = waitpid(runner_status->pid, &status, WNOHANG);
		if (ret < 0) {
		   syslog(LOG_CRIT, "watchdog got response %d from waitpid: %s", ret, strerror(errno));
		   kill(0, SIGKILL);
		   exit(1);
		}

		if (ret != 0)
		  {
		     syslog(LOG_WARNING, "watchdog got a waitdpid response %d from %d, restarting", status, ret);
		     kill(ret, SIGKILL);
		     time_t n = time(NULL);
		     sprintf(runner_status_dump + strlen(runner_status_dump), "waitpid said: %d - killed at %s", status, ctime(&n));
		     break;
		  }
		sleep(1);
	     }
	}

	if (fork_response == 0) 
	  return getpid(); // continue normal flow
     }
}

pid_t daemonize()
{
   pid_t fork_response = fork();
   if (fork_response < 0) {
      syslog(LOG_CRIT, "Could not fork: %s", strerror(errno));
      exit(1);
   }

   if (fork_response > 0) {
      syslog(LOG_INFO, "Spawned a daemon at %d", fork_response);
      closelog();
      printf("Running in the background: %d\n", fork_response);
      exit(0);
   }   
}
