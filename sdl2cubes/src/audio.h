#ifndef CUBES_AUDIO_H
#define CUBES_AUDIO_H

typedef struct AudioReader AudioReader;

// open audio source by filename
AudioReader *arInit(const char *filename);
// close audio source
void arClose(AudioReader *ar);
// get sampling rate for audio source
int arGetRate(AudioReader *ar);
// get channel count for audio source
int arGetChannels(AudioReader *ar);
// get bits per sample for audio source
int arGetSampleBits(AudioReader *ar);
// try to read audio from source; return number of bytes acquired
int arRead(AudioReader *ar, unsigned char *dest, int len);

#endif
