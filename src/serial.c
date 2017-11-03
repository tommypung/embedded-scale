#include <sys/select.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "serial.h"

static int open_fd(const char *file, speed_t baud);

static int open_fd(const char *file, speed_t baud)
{
	struct termios  tty;

	syslog(LOG_INFO, "Opening serial %s\n", file);
	int fd = open(file, O_RDONLY | O_NOCTTY);
	if (fd == -1) {
		syslog(LOG_CRIT, "Could not open the serial port %s: %s\n", file, strerror(errno));
		return -1;
	}

	if (tcgetattr(fd, &tty) != 0) {
		syslog(LOG_ERR, "Could not get the attributes for port %s: %s. Trying to use anyway\n", file, strerror(errno));
		return fd;
	}

	cfsetospeed(&tty, (speed_t)baud);
	cfsetispeed(&tty, (speed_t)baud);

	cfmakeraw(&tty);

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		syslog(LOG_ERR, "Could not set the attributes for port %s: %s. Trying to use anyway\n", file, strerror(errno));
		return fd;
	}

   int flags = fcntl(fd, F_GETFL, 0);
   if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
     syslog(LOG_ERR, "Could not make port %s O_NONBLOCK: %s\n", file, strerror(errno));
    
	return fd;
}

Serial *serial_open(const char *file, speed_t baud)
{
	Serial *s = calloc(sizeof(*s), 1);
	if (s == NULL) {
		fprintf(stderr, "Could not allocate memory for Serial structure, oops");
		exit(-1);
	}

	s->baud = baud;
	s->file = file;
	s->fd = open_fd(file, baud);
	s->b = &s->buffer[0];
	return s;
}

void serial_close(Serial *s)
{
	if (s->fd != -1) {
		syslog(LOG_INFO, "Closing serial %s\n", s->file);
		close(s->fd);
		s->fd = -1;
	}
}

#include <ctype.h>
static unsigned char translate(unsigned char c)
{

	switch(c)
	{
	case 0xB6: return 'B';
	case 0xFF: return '#';
	case 0x02: return '[';
	case 0x00: return ']';
	case '\n': return 'N';
	case '\r': return 'R';
	default:
		if (!isprint(c))
			return '*';
		return c;
	}
}

static void print_serial(Serial *serial)
{
	printf("==========================\n");
	int size = sizeof(serial->buffer) / sizeof(char);
	for(int i=0;i<size;i++)
		printf("%c  ", translate(serial->buffer[i]));
	printf("\n");
	for(int i=0;i<size;i++)
		printf("%.2x ", serial->buffer[i]);
	printf("\n");
	for(int i=0;i<size;i++)
	{
		if (serial->b == &serial->buffer[i])
			printf("b  ");
		else if (serial->eof_last_string == &serial->buffer[i])
			printf("e  ");
		else
			printf("   ");
	}
	printf("\n");
	printf("==========================\n");
}

unsigned char *serial_read_line(Serial *serial, int line_deliminator)
{
   //	print_serial(serial);
	int bytes_left = (sizeof(serial->buffer) / sizeof(char)) - (serial->b - serial->buffer);
	if (serial->eof_last_string != NULL)
	{
		char *eof = serial->b, *c = serial->eof_last_string + 1;
		while(c < eof) {
			if (*c == line_deliminator) {
				char *new_str = serial->eof_last_string + 1;
				serial->eof_last_string = c;
				*c = '\0';
				serial->last_fetch = time(NULL);
				return new_str;
			}
			c++;
		}

		unsigned char *dest = &serial->buffer[0];
		unsigned char *source = serial->eof_last_string + 1;
		for(;source < serial->b; source++, dest++)
			*dest = *source;

		serial->b = &serial->buffer[0] + (serial->b - serial->eof_last_string - 1) - 1;
		serial->eof_last_string = NULL;
	}

	if (serial->fd == -1)
	{
		if (time(NULL) - serial->last_connect > 5)
		{
			serial->last_connect = time(NULL);
			serial->fd = open_fd(serial->file, serial->baud);
		}
	}

	if (serial->fd != -1)
	{
		int bytes_left = (sizeof(serial->buffer) / sizeof(char)) - (serial->b - serial->buffer);
		if (bytes_left == 0) {
			fprintf(stderr, "No bytes left and no new line found, wrapping\n");
			serial->b = &serial->buffer[0];
			return NULL;
		}

	   fd_set fds;
	   FD_ZERO(&fds);
	   FD_SET(serial->fd, &fds);
	   struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
	   int r = select(serial->fd+1, &fds, 0, 0, &tv);
    if (r < 0) 
     {
	syslog(LOG_CRIT, "Error performing select on socket %s: %s - Closing\n", serial->file, strerror(errno));
	close(serial->fd);
	serial->fd = -1;
     }
   if (r == 0) {
      return NULL;
   }

	   int read_bytes = 0;
		if ((read_bytes = read(serial->fd, serial->b, bytes_left)) < 0)
		{
		   if (errno == EAGAIN || errno == EWOULDBLOCK) 
		     return NULL;

		   syslog(LOG_CRIT, "Error reading socket %s: %s - Closing\n", serial->file, strerror(errno));
		   close(serial->fd);
		   serial->fd = -1;
		}

		if (read_bytes > 0) {
			char *line_break = NULL, *s = serial->b;
			for(int i=0;i<read_bytes;i++, s++) {
				if (!line_break && *s == line_deliminator)
					line_break = s;
			}
			if (line_break) {
				serial->eof_last_string = line_break;
				*line_break = '\0';
				serial->b += read_bytes;
				serial->last_fetch = time(NULL);
				return serial->buffer;
			}

			serial->b += read_bytes;
			return NULL;
		}
	}
    else syslog(LOG_CRIT, "Serial interface is down, cannot read packet");
	return NULL;
}

char serial_check_if_str_is_old(Serial *serial, unsigned char *str)
{
	return &serial->buffer[0] < str;
}
