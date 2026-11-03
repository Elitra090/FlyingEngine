#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem.h>
#include <maxmod9.h>
#include <string.h>
#include "audio.h"

#define FIFO_MAXMOD 256

static FILE *song;
static bool paused;
static mm_stream stream;

mm_word fill_stream(mm_word length, mm_addr dest, mm_stream_formats format)
{
    if (paused == true) {
        memset(dest, 0, length * 2);
        return length;
    } else {
        size_t bytes_leidos = fread(dest, 1, length * 2, song);

        if (bytes_leidos < length * 2) {
            memset((char*)dest + bytes_leidos, 0, (length * 2) - bytes_leidos);
        }
    }

    return length;
}

int Audio_init() {
    if (!nitroFSInit(NULL)) {
        return 0;
    }
    mm_ds_system sys;
    sys.mod_count = 0;
    sys.samp_count = 0;
    sys.mem_bank = 0;
    sys.fifo_channel = FIFO_MAXMOD;
    mmInit(&sys);
    return 1;
}

int Audio_open(const char *path) {
    song = fopen(path, "rb");
    if (song == NULL) {
        return 0;
    }
    return 1;
}

void Audio_start() {

        stream.sampling_rate = 22050;           // 16KHZ
        stream.buffer_length = 8192;             // Should be adequate
        stream.callback = fill_stream;     // Give stereo filling routine
        stream.format = MM_STREAM_16BIT_MONO; // Select format
        stream.timer = MM_TIMER1;               // Use timer 0
        stream.manual = 1;                      // Auto filling

        // open the stream
        mmStreamOpen(&stream);
}

void Audio_update() {
    mmStreamUpdate();
}

void Audio_pause() {
    paused = true;
}

void Audio_resume() {
    paused = false;
}

void Audio_close() {
    mmStreamClose();
    if (song != NULL) {
        fclose(song);
    }
}




