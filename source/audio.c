#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem.h>
#include "audio.h"

static int id = -1;

int Audio_init(AudioTrack *t) {
    soundEnable();
    if (nitroFSInit(NULL) == false) {
        return 0;
    }
    return 1;
}

int Audio_load(AudioTrack *t, const char *path, int freq) {
    FILE* s = fopen(path, "rb");
    if (s == NULL) {
            return 0;
    }
    fseek(s, 0, SEEK_END);
    t->size = ftell(s);
    fseek(s, 0, SEEK_SET);
    t->data = malloc(t->size);
    if (t->data == NULL) {
        fclose(s);
        return 0;
    }
    u32 read = fread(t->data, 1, t->size, s);
    if (read != t->size) {
        free(t->data);
        t->data = NULL;
        fclose(s);
        return 0;
    }
    DC_FlushRange(t->data, t->size);
    t->freq = freq;
    fclose(s);
    return 1;
}

void Audio_start(AudioTrack *t) {
    id = soundPlaySample(t->data, SoundFormat_ADPCM, t->size, t->freq, 127, 64, false, 0);
}

void Audio_free(AudioTrack *t) {
    if (t->data != NULL) {
        if (id >= 0) {
            soundKill(id);
            id = -1;
        }
        free(t->data);
        t->data = NULL;
        t->size = 0; 
    }
}

void Audio_pause() {
    if (id >= 0) {
        soundPause(id);
    }
}
void Audio_resume() {
    if (id >= 0) {
        soundResume(id);
    }
}









