// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 toadster172 <toadster172@gmail.com>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "chorus.h"
#include "leapsterHW.h"
#include "string.h"

#define SCREEN_H 160
#define PITCH 240

const uint8_t badAppleVideo[] = {
    #embed "badApple.bin"
};

const uint8_t badAppleAudio[] = {
    #embed "badapple.raw"
};

const uint8_t *decompressFrame(const uint8_t *src, uint8_t *dst) {
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

        dst = pixelSet(dst, filledPixels, color, currRun);
        filledPixels += currRun;
    }

    return src;
}

void setupAudio(void) {
    uint32_t badAppleEnd = (uint32_t) (badAppleAudio + sizeof(badAppleAudio));

    hw_apuWaveEndPtrs[6].high = badAppleEnd >> 16;
    hw_apuWaveEndPtrs[6].low = badAppleEnd & 0xFFFF;

    hw_apuWaveStartPtrs[6].high = ((uint32_t) badAppleAudio) >> 16;
    hw_apuWaveStartPtrs[6].low = ((uint32_t) badAppleAudio) & 0xFFFF;

    hw_apuVolume0[6].data = 0x2000;
    hw_apuVolume1[6].data = 0x2000;

    hw_apuDecayMaybe[6].data = 0x7FFF;

    *hw_apuCommand = APU_CMD_TRIGGER | 6;
}

void entry(void) {
    uint8_t *framebuffer = gp->mpi->kernel->mallocZero(SCREEN_H * PITCH);
    uint32_t startTimestamp = gp->mpi->kernel->getTimestamp(NULL);
    uint32_t lastFrameTimestamp = startTimestamp;

    setupAudio();

    const uint8_t *videoCurr = badAppleVideo;

    for(int i = 0; i < 6572; i++) {
        uint32_t delta = gp->mpi->kernel->getTimestamp(NULL) - lastFrameTimestamp;

        int expectedFrames = (delta / 1000) * 30 + (delta % 1000) / 33;

        if(i > expectedFrames) {
            i--;
            continue;
        }

        videoCurr = decompressFrame(videoCurr, framebuffer);

        gp->mpi->lcd->copyToScreen(framebuffer, 0, 160, true, NULL, true);

        gp->mpi->lcd->unlockScreen();
    }

end:
    for(;;) {
        char buf[20];
        uint32_t n;

        if(!gp->mpi->lcd->lockScreen(true)) {
            continue;
        }

        memset(framebuffer, 0, 160 * PITCH);

        gp->mpi->lcd->copyToScreen(framebuffer, 0, 160, true, NULL, true);

        gp->mpi->lcd->unlockScreen();
    }
}

static bool hbActiveFlag;

static bool hbInit(void) {
    hbActiveFlag = true;
    entry();
    return true;
}

static bool hbDeInit(void) {
    hbActiveFlag = false;
    return true;
}

static bool hbIsActive(void) {
    return hbActiveFlag;
}

static uint32_t hbGetVersion(void) {
    return 0x100;
}

const char hbMPIName[] = "HomebrewMPI";
const char hbMPIDesc[] = "Hack to get native code running";

static const char *hbGetName(void) {
    return hbMPIName;
}

static const char *hbGetDesc(void) {
    return hbMPIDesc;
}

const struct moduleInterface hbMPI = {
    .init = hbInit,
    .deInit = hbDeInit,
    .isActive = hbIsActive,
    .getVersion0 = hbGetVersion,
    .getVersion1 = hbGetVersion,
    .getVersion2 = hbGetVersion,
    .getName = hbGetName,
    .getDesc = hbGetDesc
};
