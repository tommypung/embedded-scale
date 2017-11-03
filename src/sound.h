#ifndef SRC_SOUND_H_
#define SRC_SOUND_H_

#define SOUND_ONLINE			1
#define SOUND_OFFLINE			2
#define SOUND_MEASURING			3
#define SOUND_MEASUREMENT_DONE	4

int sound_init();
int sound_cleanup();
void sound_play(int sound);

#endif /* SRC_SOUND_H_ */
