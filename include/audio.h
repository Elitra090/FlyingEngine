#ifndef AUDIO_H
#define AUDIO_H
#include <nds.h>

typedef struct {
	u8 *data;
	u32 size;
	int freq;
} AudioTrack;

int Audio_init(AudioTrack *t);
int Audio_load(AudioTrack *t, const char *path, int freq);
void Audio_start(AudioTrack *t);
void Audio_free(AudioTrack *t);
void Audio_pause();
void Audio_resume();

#endif

