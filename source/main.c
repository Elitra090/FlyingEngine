#include <nds.h>
#include <stdio.h>
#include "conductor.h"
#include "chart.h"
#include "monochrome.h"
#include "audio.h"

#define STRUM_Y  10

int main(void) {
    Conductor cond;
    static Chart chart;
    consoleDemoInit();

    // init
    u32 keys[4] = {KEY_LEFT, KEY_DOWN, KEY_X, KEY_A};

    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_SPRITE);

    oamInit(&oamMain, SpriteMapping_1D_32, false);

    u16 *squareGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    u16 colors[4] = {RGB15(31,0,0), RGB15(0,31,0), RGB15(0,0,31), RGB15(31,31,0)};
    u8 *squareGfx_pixels = (u8*)squareGfx;

    for (int i = 0; i <= 127; i++) {
        squareGfx_pixels[i] = 0x11;
    }
    for (int i = 0; i <= 3; i++) {
        SPRITE_PALETTE[i * 16 + 1] = colors[i];
        oamSet(&oamMain, i, 64 + i * 32, 80, 0, i, SpriteSize_16x16, SpriteColorFormat_16Color, squareGfx, -1, false, false, false, false, false);
    }

    int lastJudge = -2;
    int totalMisses = 0;
    Conductor_init(&cond, CHART_BPM);
    Audio_init();
    Chart_init(&chart);
    Chart_load(&chart);
    // finish init



    Audio_start();
    Conductor_start(&cond);

    while (1) {
        swiWaitForVBlank();
        oamUpdate(&oamMain);
        scanKeys();
        u32 down = keysDown();

        Conductor_update(&cond);
        Chart_update(&chart, cond.songPosition);
        totalMisses += Chart_checkMiss(&chart, cond.songPosition);



        //dibujado
        for (int i = 0; i <= 127; i++) {
            squareGfx_pixels[i] = 0x11;
        }
        for (int i = 0; i <= 3; i++) {
            SPRITE_PALETTE[i * 16 + 1] = colors[i];
            oamSet(&oamMain, i, 64 + i * 24, 10, 0, i, SpriteSize_16x16, SpriteColorFormat_16Color, squareGfx, -1, false, false, false, false, false);
        }
        //

        printf("\x1b[0;0H%d ms      ", cond.songPosition);
        printf("\x1b[1;0Hnotas: %d      ", chart.noteCount);
        printf("\x1b[2;0HnextSpawn: %d      ", chart.nextSpawn);
        printf("\x1b[3;0Hnota0 estado: %d      ", chart.notes[0].noteState);
        printf("\x1b[6;0Hjuicio: %d      ", lastJudge);
        printf("\x1b[7;0Hmissed: %d      ", totalMisses);

        if (cond.state == cond_playing) {
            for (int i = 0; i < 4; i++) {
                if (down & keys[i]) {
                    lastJudge = Chart_hit(&chart, i, cond.songPosition);
                }
            }
        }

        for (int i = 0; i <= chart.nextSpawn - 1; i++) {
            if (chart.notes[i].noteState == note_active) {
                int y = STRUM_Y + (chart.notes[i].strumTime - cond.songPosition) * (192 - STRUM_Y) / SPAWN_TIME;
                int x = 64 + chart.notes[i].column * 24;
                oamSet(&oamMain, chart.notes[i].slot, x, y, 0, chart.notes[i].column, SpriteSize_16x16, SpriteColorFormat_16Color, squareGfx, -1, false, false, false, false, false);
            } else if (chart.notes[i].noteState == note_hit ||chart.notes[i].noteState == note_missed) {
                oamClearSprite(&oamMain, chart.notes[i].slot);
            }
        }

        if (down & KEY_START) {
            if (cond.state == cond_playing) {
                Conductor_pause(&cond);
            } else {
                Conductor_resume(&cond);
            }
        }
    }

    return 0;
}