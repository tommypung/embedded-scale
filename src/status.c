#include <sys/mman.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include "status.h"

struct runner_status_struct *runner_status = NULL;

void runner_status_init()
{
   runner_status = (struct runner_status_struct *) mmap(NULL, sizeof(*runner_status), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
   memset(runner_status, 0, sizeof(*runner_status));
   if (sem_init(&runner_status->semaphore, 1, 1) == -1)
     syslog(LOG_CRIT, "Could not initialize semaphore in runner_status: %s", strerror(errno));
}
