#ifndef CONDUCTOR_H
#define CONDUCTOR_H
#include <stdint.h>

typedef enum {
    cond_stopped,
    cond_playing,
    cond_paused
} ConductorState;

typedef struct {
    int songPosition;
    float bpm;
    int crochet;
    int stepCrochet;
    int offset;
    ConductorState state;
    uint32_t ticks;
} Conductor;

void Conductor_init(Conductor *c, float bpm);
void Conductor_start(Conductor *c);
void Conductor_update(Conductor *c);
void Conductor_pause(Conductor *c);
void Conductor_resume(Conductor *c);

#endif