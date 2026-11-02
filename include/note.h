#ifndef NOTE_H
#define NOTE_H
#define SPAWN_TIME 1500
#define SICK_WINDOW 45
#define GOOD_WINDOW 90
#define BAD_WINDOW  135
#define SAFE_WINDOW 166
#include <stdint.h>

typedef enum {
    note_waiting,
    note_active,
    note_hit,
    note_missed
} NoteState;

typedef enum {
    judge_sick,
    judge_good,
    judge_bad,
    judge_shit
} Judgement;

typedef struct {
    int strumTime;
    int sustainLength;
    uint8_t column;
    uint8_t type;
    uint8_t mustHit;
    uint8_t noteState;
    uint8_t slot;
} Note;

#endif