// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2025, 2026 toadster172 <toadster172@gmail.com>

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define NORMALIZE_MONOCHROME(N) (N >= 0x88 ? 0xFF : 0x00)
#define NORMALIZE_COLOR(N) ((N >> 4) | ((N >> 4) << 4))

size_t encode(uint8_t *dst, uint16_t runLen, uint8_t currColor, uint8_t prevColor) {
    if(runLen > 0x0FFF ||
      (runLen > 0xFF && (currColor != 0x00 && currColor != 0xFF))) {
        *(dst++) = 0xE0 | (currColor & 0x0F);
        *(dst++) = runLen & 0xFF;
        *(dst++) = runLen >> 8;

        return 3;
    } else if(runLen > 0xFF) {
        *(dst++) = 0xC0 | ((currColor & 1) << 4) | (runLen & 0x0F);
        *(dst++) = runLen >> 4;

        return 2;
    } else if((currColor == (uint8_t ) ~prevColor) && runLen < 0xA0) {
        *(dst++) = runLen;

        return 1;
    } else {
        *(dst++) = 0xF0 | (currColor & 0x0F);
        *(dst++) = runLen;

        return 2;
    }
}

int main(void) {


    char path[4096];

    SDL_Init(0);
    IMG_Init(IMG_INIT_PNG);

    uint8_t *frameBuf = malloc(0x10000000);
    size_t frameBufIndex = 0;

    size_t totalBytes = 0;

    for(int i = 0;; i++) {
        printf("Conving frame %i, %llu\n", i, frameBufIndex);
        sprintf(path, "frames2/frame%05d.png", i + 1);

        uint8_t greyscale = false;

        // Yeah we're going to cheat. Fitting things into 8 MB is hard, okay?
        //   At some point it might be nice to properly filter the source so that
        //   this isn't needed
        if ((i > 2620 && i < 3475) ||
            (i > 1790 && i < 1850)) {
            greyscale = true;
        }

        SDL_Surface *raw = IMG_Load(path);

        if(!raw) {
            break;
        }

        SDL_LockSurface(raw);

//        printf("Opened %i, format: %s\n", i + 1, SDL_GetPixelFormatName(raw->format->format));

        if(raw->format->format != SDL_PIXELFORMAT_RGB24) {
            printf("SDL_PIXELFORMAT_RGB24 required!\n");
            break;
        }

        if(raw->h != 160 || raw->w != 160) {
            printf("Image dimensions not 160x160!\n");
            break;
        }

        uint8_t *rawPixels = raw->pixels;

        // We're going to make the assumption that all 3 colors are actually the same

        uint8_t currColor = greyscale ? NORMALIZE_COLOR(*rawPixels) : NORMALIZE_MONOCHROME(*rawPixels);

        uint8_t prevColor = 0xAB; // We just need something that'll never match the ~condition in encode
        int runLen = -1;

        // FX NN, Run n + 1 pixels, set color to X
        // EX NN NN, Run n + 1 pixels, set color to X
        // DN NN, White run n + 1 pixels
        // CN NN, Black run n + 1 pixels
        // < A0, Run n + 1 pixels, transition from white to black

        for(int j = 0; j < raw->h; j++) {
            rawPixels = ((uint8_t *) raw->pixels) + j * raw->pitch;

            for(int k = 0; k < raw->w; k++, rawPixels += 3) {
                uint8_t pixelColor = greyscale ? NORMALIZE_COLOR(*rawPixels) : NORMALIZE_MONOCHROME(*rawPixels);

                if(pixelColor != currColor) {
                    frameBufIndex += encode(frameBuf + frameBufIndex, runLen, currColor, prevColor);

                    prevColor = currColor;
                    currColor = pixelColor;
                    runLen = 0;
                } else {
                    runLen++;
                }
            }

            rawPixels += raw->pitch;
        }

        frameBufIndex += encode(frameBuf + frameBufIndex, runLen, currColor, prevColor);

        totalBytes += frameBufIndex;

        SDL_UnlockSurface(raw);
        SDL_FreeSurface(raw);
    }

    FILE *out = fopen("badApple.bin", "wb");
    fwrite(frameBuf, frameBufIndex, 1, out);
    fclose(out);

    free(frameBuf);

    IMG_Quit();
    SDL_Quit();

    return 0;
}
