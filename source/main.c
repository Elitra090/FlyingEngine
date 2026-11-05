#include <nds.h>
#include <stdio.h>
#include <filesystem.h>
#include "conductor.h"
#include "chart.h"
#include "audio.h"
#include <malloc.h>
#include <unistd.h>

#define STRUM_Y  10



void debug_imprimir_ram_real() {
    struct mallinfo mi = mallinfo();

    // sbrk(0) obtiene el puntero al límite superior actual del heap
    char *limite_heap = (char*)sbrk(0);

    // Límite físico real de la memoria RAM de la NDS original (4MB)
    // Inicia en 0x02000000 + 4MB (0x00400000) = 0x02400000
    char *fin_ram = (char*)0x02400000;

    // Calculamos los bytes vírgenes hasta el final del chip de RAM
    int ram_virgen = fin_ram - limite_heap;

    int ram_libre_real = ram_virgen + mi.fordblks;
    int ram_estatica = (4096 * 1024) - ram_libre_real - mi.uordblks;

    iprintf("\x1b[10;0H");
    // Imprimir saltando algunas líneas para no pisar tus otras variables de ritmo
    iprintf("\n\n\n\n\n\n\n\n");
    iprintf("Sistema/Juego:  %d kb   \n", ram_estatica / 1024);
    iprintf("RAM Usada:      %d kb   \n", mi.uordblks / 1024);
    iprintf("RAM Libre Real: %d kb   \n", ram_libre_real / 1024);
    iprintf("RAM Total NDS:  4096 kb \n");
}

int main(void) {
    Conductor cond;
    static Chart chart;
    consoleDemoInit();
    nitroFSInit(NULL);

    // init
    u32 keys[4] = {KEY_LEFT, KEY_DOWN, KEY_X, KEY_A};

    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_SPRITE);

    oamInit(&oamMain, SpriteMapping_1D_32, false);

    u16 *squareGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    u16 colors[4] = {RGB15(31,0,0), RGB15(0,31,0), RGB15(0,0,31), RGB15(31,31,0)};
    u8 *squareGfx_pixels = (u8*)squareGfx;

    u16 *held_Gfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);

    for (int i = 0; i <= 127; i++) {
        squareGfx_pixels[i] = 0x11;
    }
    for (int i = 0; i <= 3; i++) {
        SPRITE_PALETTE[i * 16 + 1] = colors[i];
        oamSet(&oamMain, i, 64 + i * 32, 80, 0, i, SpriteSize_16x16, SpriteColorFormat_16Color, squareGfx, -1, false, false, false, false, false);
    }

    int lastJudge = -2;
    int totalMisses = 0;
    Conductor_init(&cond, 120);

    Chart_init(&chart);
    Chart_load(&chart, "nitro:/data/songs/bopeebo/bopeebo-hard.chartbin");

    if (!Audio_init()) {
        printf("\x1b[8;0H error 1");
    }
    // finish init



    Audio_open("nitro:/songs/song.raw");
    Audio_start();
    Conductor_start(&cond);

    while (1) {
        swiWaitForVBlank();
        oamUpdate(&oamMain);
        scanKeys();
        u32 down = keysDown();
        u32 held = keysHeld();

        Conductor_update(&cond);
        Audio_update();
        Chart_update(&chart, cond.songPosition);
        totalMisses += Chart_checkMiss(&chart, cond.songPosition);

        int mask = 0;
        for (int i = 0; i < 4; i++) {
            if (held & keys[i]) {
                mask |= (1 << i);
            }
        }
        totalMisses += Chart_updateHolds(&chart, cond.songPosition, mask);

        for (int i = 0; i < 64; i++) {held_Gfx[i] = 0;}
        for (int half = 0; half < 2; half++) {
            int base = half * 32;
            for (int r = 0; r < 8; r++) {
                held_Gfx[base + r*2 + 1] = 0x1100;
                held_Gfx[base + 16 + r*2] = 0x0011;
            }
        }

        //dibujado
        for (int i = 0; i <= 127; i++) {
            squareGfx_pixels[i] = 0x11;
        }
        for (int i = 0; i <= 3; i++) {
            SPRITE_PALETTE[i * 16 + 1] = colors[i];
            oamSet(&oamMain, i, 64 + i * 24, 10, 0, i, SpriteSize_16x16, SpriteColorFormat_16Color, squareGfx, -1, false, false, false, false, false);
        }

        //
        debug_imprimir_ram_real();
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

        int slot = 4;

        for (int i = 0; i <= chart.nextSpawn - 1; i++) {
            int nst = chart.notes[i].noteState;
            if (nst == note_active || nst == note_holding) {
                int y = STRUM_Y + (chart.notes[i].strumTime - cond.songPosition) * (192 - STRUM_Y) / SPAWN_TIME;
                int x = 64 + chart.notes[i].column * 24;
                int yEnd = STRUM_Y + (chart.notes[i].strumTime + chart.notes[i].sustainLength - cond.songPosition) * (192 - STRUM_Y) / SPAWN_TIME;
                if (slot > 127) break;

                if (nst == note_active) {
                    oamSet(&oamMain, slot, x, y, 0, chart.notes[i].column, SpriteSize_16x16, SpriteColorFormat_16Color, squareGfx, -1, false, false, false, false, false);
                    slot++;
                }

                if (chart.notes[i].sustainLength > 0) {
                    int topLimit = (nst == note_active) ? y : STRUM_Y;
                    for (int segY = yEnd + 8 - 16; segY >= topLimit; segY -= 16) {
                        if (segY >= 192) continue;
                        if (slot > 127) break;
                        oamSet(&oamMain, slot, x, segY, 0, chart.notes[i].column,
                               SpriteSize_16x16, SpriteColorFormat_16Color, held_Gfx,
                               -1, false, false, false, false, false);
                        slot++;
                    }
                }
            }

            for (int s = slot; s <= 127; s++) {
                oamClearSprite(&oamMain, s);
            }


            if (down & KEY_START) {
                if (cond.state == cond_playing) {
                    Conductor_pause(&cond);
                    Audio_pause();
                } else {
                    Conductor_resume(&cond);
                    Audio_resume();
                }
            }
        }
    }
    return 0;
}