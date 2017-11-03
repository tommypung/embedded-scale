#ifndef _FORK_H_
#define _FORK_H_

pid_t daemonize();
int watchdog_run();
void watchdog_dump_status();

#endif