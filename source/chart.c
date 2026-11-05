#include "chart.h"
#include <stdlib.h>
#include <stdio.h>

void Chart_init(Chart *c) {
    c->noteCount = 0;
    c->nextSpawn = 0;
    c->nextSlot = 4;
}

int Chart_addNote(Chart *ch, int strumTime, int sustainLength, int column, int type, int mustHit) {
    if (ch->noteCount < MAX_NOTES) {
        ch->notes[ch->noteCount].strumTime = strumTime;
        ch->notes[ch->noteCount].column = column;
        ch->notes[ch->noteCount].sustainLength = sustainLength;
        ch->notes[ch->noteCount].mustHit = mustHit;
        ch->notes[ch->noteCount].type = type;
        ch->notes[ch->noteCount].noteState = note_waiting;
        ch->noteCount++;
        return 1;
    } else {
        return 0;
    }

}

void Chart_update(Chart *ch, int songPosition) {
    while (ch->nextSpawn < ch->noteCount && ch->notes[ch->nextSpawn].strumTime - songPosition < SPAWN_TIME) {
        ch->notes[ch->nextSpawn].noteState = note_active;
        ch->notes[ch->nextSpawn].slot = ch->nextSlot;
        if (ch->nextSlot > 126) {
            ch->nextSlot = 4;
        } else {
            ch->nextSlot++;
        }
        ch->nextSpawn++;
    }
}

int Chart_hit (Chart *ch, int column, int songPosition) {
    int best = -1;
    int bestDiff = 1000000;

    for (int i = ch->nextSpawn - 1; i >= 0; i--) {
        if (ch->notes[i].noteState == note_active && ch->notes[i].column == column && ch->notes[i].mustHit == 1) {
            int diff = abs(ch->notes[i].strumTime - songPosition);
            if (diff < bestDiff) {
                best = i;
                bestDiff = diff;
            }
        }
    }
    if (best == -1 || bestDiff > SAFE_WINDOW) {return -1;} else {
        if (ch->notes[best].sustainLength > 0) {
            ch->notes[best].noteState = note_holding;
        } else {
            ch->notes[best].noteState = note_hit;
        }

        if (bestDiff <= SICK_WINDOW) {return judge_sick;}
        else if (bestDiff <= GOOD_WINDOW) {return judge_good;}
        else if (bestDiff <= BAD_WINDOW) {return judge_bad;}
        else return judge_shit;
    }
}

int Chart_checkMiss(Chart *ch, int songPosition) {
    int missed = 0;
    for (int i = 0; i <= ch->nextSpawn - 1; i++) {
        if (ch->notes[i].noteState == note_active && songPosition - ch->notes[i].strumTime > SAFE_WINDOW && ch->notes[i].mustHit == 1) {
            ch->notes[i].noteState = note_missed;
            missed++;
        }
    }
    return missed;
}



int Chart_load(Chart* ch, const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) return 0; // Error al abrir el archivo

    int totalNotes = 0;
    if (fread(&totalNotes, sizeof(int), 1, file) != 1) {
        fclose(file);
        return 0;
    }

    // 1. Limpiar el estado del chart
    Chart_init(ch);

    // 2. Leer cada nota del binario e insertarla con Chart_addNote
    ChartNoteData rawNote;
    for (int i = 0; i < totalNotes; i++) {
        if (fread(&rawNote, sizeof(ChartNoteData), 1, file) == 1) {
            // Chart_addNote se encarga de rellenar noteState = note_waiting
            // y alinear los tipos de datos en ch->notes
            if (!Chart_addNote(ch, rawNote.strumTime, rawNote.sustainLength,
                rawNote.column, rawNote.type, rawNote.mustHit)) {
                break; // Se alcanzó MAX_NOTES
            }
        }
        else {
            break; // Error de lectura prematuro
        }
    }

    fclose(file);
    return 1;
}


int Chart_updateHolds(Chart *ch, int songPosition, int mask) {
    int missed = 0;
    for (int i = 0; i <= ch->nextSpawn - 1; i++) {
        if (ch->notes[i].noteState == note_holding) {
            if (songPosition >= ch->notes[i].strumTime + ch->notes[i].sustainLength) {
                ch->notes[i].noteState = note_hit;
            } else if (!(mask & (1 << ch->notes[i].column))) {
                ch->notes[i].noteState = note_missed;
                missed++;
            }
        }
    }
    return missed;
}