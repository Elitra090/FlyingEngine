#ifndef CHART_H
#define CHART_H
#include "note.h"
#define MAX_NOTES 5000

typedef struct {
    Note notes[MAX_NOTES];
    int noteCount;
    int nextSpawn;
    int nextSlot;
} Chart;

void Chart_init(Chart *chart);
int Chart_addNote(Chart *ch, int strumTime, int sustainLength, int column, int type, int mustHit);
void Chart_update(Chart *ch, int songPosition);
int Chart_updateHolds(Chart *ch, int songPosition, int mask);
int Chart_hit (Chart *ch, int column, int songPosition);
int Chart_checkMiss(Chart *ch, int songPosition);
void Chart_load(Chart *ch);

#endif
