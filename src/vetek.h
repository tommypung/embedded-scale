#ifndef SRC_VETEK_H_
#define SRC_VETEK_H_

#define VETEK_TYPE_WEIGHT 1

typedef struct VetekScalePacket
{
	int type;
	char polarity;
	char weight[10];
	char unit;
	char gross_net;
	char status;
} VetekScalePacket;

VetekScalePacket *vetek_scale_parse_packet(char *serial_command_line);
void vetek_scale_print_packet(struct VetekScalePacket *packet);

#endif /* SRC_VETEK_H_ */


