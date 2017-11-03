#ifndef SRC_SERIAL_H_
#define SRC_SERIAL_H_
#include <termios.h>
#include <time.h>

typedef struct Serial
{
	const char *file;
	speed_t baud;
	int fd;
	time_t last_fetch, last_connect;

	unsigned char buffer[64], *b, *eof_last_string;
} Serial;

Serial *serial_open(const char *file, speed_t baud);
void serial_close(Serial *serial);
unsigned char *serial_read_line(Serial *serial, int line_deliminator);
char serial_check_if_str_is_old(Serial *serial, unsigned char *str);

#endif /* SRC_SERIAL_H_ */
