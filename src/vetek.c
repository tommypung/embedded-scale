#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vetek.h"

int vetek_status = 0;
static VetekScalePacket lastPacket;

#define VALID_UNIT(p) ((p)->unit == 'L' || (p)->unit == 'K')
#define VALID_GROSS_NET(p) ((p)->gross_net == 'G' || (p)->gross_net == 'N')
#define VALID_STATUS(p) ((p)->status == ' ' || (p)->status == 'M' || (p)->status == 'O')
#define VALID_POLARITY(p) ((p)->polarity == ' ' || (p)->polarity== '-')

static char VALID_WEIGHT(VetekScalePacket *p)
{
	for(int i=0;i<7;i++)
		if (p->weight[i] != '.' && !isdigit(p->weight[i]) && p->weight[i] != ' ')
			return 0;
	return 1;
}

void vetek_scale_print_packet(struct VetekScalePacket *packet)
{
	printf("Polarity: '%c'\tWeight: %s\tUnit: %c\tGrossNet: %c\tStatus: '%c'\n",
			packet->polarity,
			packet->weight,
			packet->unit,
			packet->gross_net,
			packet->status);
}

VetekScalePacket *vetek_scale_parse_packet(char *serial_command_line)
{
   vetek_status = VETEK_STATUS_OK;
	unsigned char *b = serial_command_line;

	if (*(b++) !=  0x02) {
	   vetek_status = VETEK_STATUS_INVALID_PACKET;
		fprintf(stderr, "Invalid command from vetek scale: {%s} perhaps a one time error?\n", serial_command_line);
		return NULL;
	}

	lastPacket.type = VETEK_TYPE_WEIGHT;
	lastPacket.polarity = *(b++);
	memset(&lastPacket.weight[0], 0, sizeof(lastPacket.weight));
	for(int i=0;*b != 'L' && *b != 'K' && *b != '\0' && i < sizeof(lastPacket.weight) / sizeof(char); i++)
		lastPacket.weight[i] = *(b++);
	lastPacket.unit = *(b++);
	lastPacket.gross_net = *(b++);
	lastPacket.status = (*b++);

	if (*b != '\0'
			|| !VALID_UNIT(&lastPacket)
			|| !VALID_GROSS_NET(&lastPacket)
			|| !VALID_STATUS(&lastPacket)
			|| !VALID_POLARITY(&lastPacket)
			|| !VALID_WEIGHT(&lastPacket)) {
		fprintf(stderr, "Invalid command from vetek scale\n");
		fprintf(stderr, "unit=%d\n", VALID_UNIT(&lastPacket));
		fprintf(stderr, "grossNet=%d\n", VALID_GROSS_NET(&lastPacket));
		fprintf(stderr, "status=%d\n", VALID_STATUS(&lastPacket));
		fprintf(stderr, "polarity=%d\n", VALID_POLARITY(&lastPacket));
		fprintf(stderr, "weight=%d\n", VALID_WEIGHT(&lastPacket));
		vetek_scale_print_packet(&lastPacket);
	}

//	vetek_scale_print_packet(&lastPacket);
	return &lastPacket;
}
