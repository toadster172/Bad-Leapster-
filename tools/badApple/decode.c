// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2025, 2026 toadster172 <toadster172@gmail.com>

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>


// < A0, Run n + 1 pixels, transition from white to black
// FX NN, Run n + 1 pixels, set color to X
// EX NN NN, Run n + 1 pixels, set color to X
// DN NN, White run n + 1 pixels
// CN NN, Black run n + 1 pixels

uint8_t *decompressFrame(uint8_t *src, uint8_t *dst) {
    uint16_t filledPixels = 0;
    uint8_t color = 0;

    while(filledPixels < 160 * 160) {
        uint8_t currByte = *(src++);
        uint16_t currRun;

        switch(currByte >> 4) {
            case 0xC:
                color = 0x00;

                currRun = (currByte & 0x0F) + 1;
                currRun += *(src++) << 4;
                break;
            case 0xD:
                color = 0xFF;

                currRun = (currByte & 0x0F) + 1;
                currRun += *(src++) << 4;
                break;
            case 0xE:
                color = currByte & 0x0F;
                color |= color << 4;

                currRun = *(src++) + 1;
                currRun += *(src++) << 8;
                break;
            case 0xF:
                color = currByte & 0x0F;
                color |= color << 4;

                currRun = *(src++) + 1;
                break;
            default:
                color = ~color;

                currRun = currByte + 1;
                break;
        }

        memset(dst, color, currRun * 4);
        dst += currRun * 4;
        filledPixels += currRun;
    }

    return src;
}

int main(void) {
    FILE *f = fopen("badApple.bin", "rb");

    if(!f) {
        printf("Unable to open source file!\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long fileLen = ftell(f);
    fseek(f, 0, SEEK_SET);

    if(!fileLen) {
        printf("File has length 0!\n");
        return 0;
    }

    uint8_t *data = malloc(fileLen);
    uint8_t *origData = data;
    fread(data, fileLen, 1, f);
    fclose(f);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window *w = SDL_CreateWindow("Bad Apple!!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160, 160, 0);
    SDL_Renderer *r = SDL_CreateRenderer(w, -1, 0);
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STATIC, 160, 160);

    int frameCount = 0;

    while(1) {
        SDL_Event e;
        uint8_t buf[160 * 160 * 4];

        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) {
                goto quit;
            }
        }

        data = decompressFrame(data, buf);

        frameCount++;

        SDL_UpdateTexture(t, NULL, buf, 160 * 4);
        SDL_RenderCopy(r, t, NULL, NULL);
        SDL_RenderPresent(r);

        SDL_Delay(1000.0f / 30);

        if(frameCount > 6000) {
            break;
        }
    }

quit:

    SDL_DestroyTexture(t);
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);

    SDL_Quit();

    free(origData);

    return 0;
}
