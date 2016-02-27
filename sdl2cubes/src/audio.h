#ifndef CUBES_AUDIO_H
#define CUBES_AUDIO_H

typedef struct AudioReader AudioReader;

AudioReader *arInit(const char *filename);
void arClose(AudioReader *ar);

int arGetRate(AudioReader *ar);
int arGetChannels(AudioReader *ar);
int arGetSampleBits(AudioReader *ar);

int arRead(AudioReader *ar, unsigned char *dest, int len);

#endif
