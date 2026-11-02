#include <nds.h>
#include "conductor.h"
#define TICKS_PER_SEC (BUS_CLOCK >> 10)

void Conductor_init(Conductor *c, float bpm) {
    c->songPosition = 0;
    c->offset = 0;
    c->ticks = 0;

    c->state = cond_stopped; // parado
    c->bpm = bpm; // guardar bpm

    if (c->bpm <= 0) {c->bpm = 120;}

    c->crochet = 60000.0f / c->bpm;
    c->stepCrochet =  c->crochet / 4;

}

void Conductor_start(Conductor *c) {
    c->songPosition = 0;
    c->ticks = 0;
    timerStart(0, ClockDivider_1024, 0, NULL);
    timerElapsed(0);
    c->state= cond_playing;
}

void Conductor_update(Conductor *c) {
    if (c->state != cond_playing) {return;}
    c->ticks += timerElapsed(0);
    c->songPosition = (long long)c->ticks * 1000 / TICKS_PER_SEC;
}

void Conductor_pause(Conductor *c) {
    if (c->state == cond_playing) {c->state = cond_paused;}
}

void Conductor_resume(Conductor *c) {
    if (c->state == cond_paused) {c->state = cond_playing;}
}
