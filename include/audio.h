#ifndef AUDIO_H
#define AUDIO_H
#include <nds.h>


int Audio_init();
int Audio_open(const char *path);
void Audio_start();
void Audio_update();
void Audio_pause();
void Audio_resume();
void Audio_close();

#endif
