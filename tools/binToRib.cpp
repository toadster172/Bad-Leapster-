// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 toadster172 <toadster172@gmail.com>

#include "lfElf.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

struct RibHeader {
    char copyrightString[0x18];
    uint16_t version;
    uint16_t ribCount;
    uint32_t deviceStart;
    uint32_t deviceEnd;
    uint32_t fullChecksum;
    uint32_t sparseChecksum;
    uint32_t bootSafeFcnTable;
    uint32_t reserved[4];
    uint32_t ribStart;
};

struct RibEntry {
    uint16_t dataId;
    uint16_t version;
    void *data;
    size_t len;
    bool needsFree;
};

struct RibEntryRaw {
    uint16_t dataId;
    uint16_t version;
    uint32_t data;
};

struct RibGroup {
    uint16_t groupID;
//    uint16_t count;
    std::vector<RibEntry> entries;
};

struct RibGroupRaw {
    uint16_t groupID;
    uint16_t count;
    uint32_t entries;
};

struct RibTable {
    char magic[4];
    uint16_t version;
    uint16_t resourceCount;
    uint32_t reserved[6];
    RibGroupRaw groups[];
};

struct MpiDescriptor {
    uint16_t version; // ?
    uint16_t id;
    uint8_t unk[8];
    uint32_t nameAddr;
    uint32_t descAddr;
    uint16_t interfaceFunctionCount;
    uint16_t moduleFunctionCount;
    uint32_t interfaceFunctionAddr;
    uint32_t moduleFunctionAddr;
};

struct ModuleSubtable {
    uint16_t version;
    uint16_t count;
    uint8_t unk[8];
    uint32_t descriptorOffsets[];
};

#define CART_START 0x80000000

const char lfCopyright[] = "Copyright LeapFrog     ";
const char lfToothString[] = "Lil ducked.  The jet zipped past her head.  Dust flew, Lil sneezed, and Leap turned red.  "
                             "Then Lil got up, about to yell.  Leap gasped, \"Look, Lil!  Your tooth!  It fell!\"";
const char lfApproval[] = "'Approved Content' is content that either is created by LeapFrog or has received final approval "
                          "under contract with LeapFrog.  LeapFrog grants you permission to copy these five sentences only if "
                          "this cartridge contains 'Approved Content'.  If you copy these five sentences into your cartridge, "
                          "a statement will appear on the screen stating that the content has been approved by LeapFrog.  If you "
                          "do not copy these five sentences into your cartridge, a statement will appear on the screen stating that "
                          "the content has not been approved by LeapFrog.  Nothing in these five sentences implies permission to copy them.";

const char justKidding[] = "Not actually coprighted or approved by Leapfrog. Fair use under Sega v. Accolade's precedent. Sue me!";

uint8_t *imageBytes;

void addResource(std::unordered_map<uint16_t, RibGroup> &dict, uint32_t resourceId, void *data, size_t size) {
    auto &group = dict[resourceId >> 16];

    group.groupID = resourceId >> 16;
    group.entries.push_back((RibEntry) {
        .dataId = (uint16_t) resourceId,
        .version = 0x0100,
        .data = data,
        .len = size,
        .needsFree = false,
    });
}

void writeResources(std::unordered_map<uint16_t, RibGroup> &dict) {
    RibGroupRaw *groupPtr = (RibGroupRaw *) (imageBytes + 0x0144);
    uint8_t *entryPtr = (((uint8_t *) groupPtr) + dict.size() * 8);

    for(auto &a : dict) {
        groupPtr->groupID = a.first;
        groupPtr->count = a.second.entries.size();
        groupPtr->entries = ((off_t) entryPtr) - ((off_t) imageBytes) + CART_START;

        uint8_t *dataPtr = entryPtr + groupPtr->entries * 8;

        for(auto &b : a.second.entries) {
            memcpy(dataPtr, b.data, b.len);
            dataPtr += b.len + ((4 - (b.len % 4)) % 4);

            if(b.needsFree) {
                free(b.data);
            }
        }

        entryPtr = dataPtr;
        groupPtr++;
    }
}

int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("Format: bin2Rib output_image input_elf (sd | cart)\n");
        return 0;
    }

    uint32_t baseAddr = 0;

    if(!strcmp(argv[3], "sd")) {
        baseAddr = 0x3C80'0000;
    } else if (!strcmp(argv[3], "cart")) {
        baseAddr = 0x8000'0000;
    } else {
        printf("Format: bin2Rib output_image input_elf (sd | cart)\n");
        return 0;
    }

    LfElfData exeData;

    if(!extractElfInfo(argv[2], &exeData)) {
        printf("Failed to extract necessary data from ELF!\n");
        return 0;
    }

    int f = open(argv[1], O_RDWR);

    if(f == -1) {
        printf("Unable to open input!\n");
        free(exeData.rxSegment);
        return 0;
    }

    uint32_t exeOutOffset = exeData.rxSegmentAddr - baseAddr;
    uint32_t deviceSize = exeOutOffset + exeData.rxSegmentLength + ((4 - (exeData.rxSegmentLength & 3)) & 3);

    // We need 2 more lwords for the checksums. Other lword seems to just be padding but it appears on
    //   official stuff and I'm not taking chances
    ftruncate(f, deviceSize + 0x0C);

    imageBytes = (uint8_t *) mmap(NULL, deviceSize + 0x0C, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
    close(f);

    memcpy(imageBytes + exeOutOffset, exeData.rxSegment, exeData.rxSegmentLength);
    free(exeData.rxSegment);

//    strcpy((char *) imageBytes, justKidding);

    RibHeader *header = (RibHeader *) (imageBytes + 0x100);

    memcpy(header->copyrightString, lfCopyright, sizeof(lfCopyright));

    header->version = 0x0100;
    header->ribCount = 0x0001;
    header->deviceStart = baseAddr;
    header->deviceEnd = baseAddr + deviceSize;
//    header->fullChecksum = baseAddr + deviceSize + 4;
//    header->sparseChecksum = baseAddr + deviceSize + 8;
    header->fullChecksum = 0;
    header->sparseChecksum = 0;
    header->bootSafeFcnTable = 0;

    for(int i = 0; i < 4; i++) {
        header->reserved[i] = 0;
    }

    header->ribStart = baseAddr;

    std::unordered_map<uint16_t, RibGroup> groups;

    // All of this is a hack. In the long run this should be able
    //   to generate a new ROM image instead of patching a preexisting one

    auto ribCount = (uint16_t *) (imageBytes + 0x06);
    *ribCount = 5;

    auto moduleRibGroup = (RibGroupRaw *) (imageBytes + 0x20);

    if(moduleRibGroup->groupID != 0x1001) {
        memmove(imageBytes + 0x28, imageBytes + 0x20, 0x20);
    }

    moduleRibGroup->groupID = 0x1001;
    moduleRibGroup->count = 1;
    moduleRibGroup->entries = baseAddr + 0x1000;

    auto moduleRibEntry = (RibEntryRaw *) (imageBytes + 0x1000);

    moduleRibEntry->dataId = 1;
    moduleRibEntry->version = 0;
    moduleRibEntry->data = baseAddr + 0x100C;

    auto moduleTable = (ModuleSubtable *) (imageBytes + 0x100C);

    moduleTable->version = 0x0100;
    moduleTable->count = 1;
    moduleTable->descriptorOffsets[0] = baseAddr + 0x1800;

    auto mpiDescriptor = (MpiDescriptor *) (imageBytes + 0x1800);

    mpiDescriptor->version = 0x1000;
    mpiDescriptor->id = 0x1040;
    mpiDescriptor->nameAddr = exeData.mpiNameAddr;
    mpiDescriptor->descAddr = exeData.mpiDescAddr;
    mpiDescriptor->interfaceFunctionCount = 8;
    mpiDescriptor->moduleFunctionCount = 0;
    mpiDescriptor->interfaceFunctionAddr = exeData.mpiInterfaceAddr;
    mpiDescriptor->moduleFunctionAddr = 0;



//
//    fwrite(&buf16, 2, 1, input);
//    buf16 = 0x0001; // Rib count
//    fwrite(&buf16, 2, 1, input);
//    uint32_t buf32 = CART_START; // Device start addr
//    fwrite(&buf32, 4, 1, input);
//    buf32 += st.st_size - 0xC; // Device end addr
//    fwrite(&buf32, 4, 1, input);
//    buf32 += 4; // Full checksum addr
//    fwrite(&buf32, 4, 1, input);
//    buf32 += 4; // Sparse checksum addr
//    fwrite(&buf32, 4, 1, input);
//    buf32 = 0; // Boot-safe function table
//    fwrite(&buf32, 4, 1, input);
//
//    for(int i = 0, buf32 = 0; i < 4; i++) {
//        fwrite(&buf32, 4, 1, input); // Reserved values
//    }
//
//    fwrite(&buf32, 4, 1, input); // RIB address
//

    auto imageLwords = (uint32_t *) imageBytes;
    uint32_t currChecksum = 0;

    for(uint32_t i = 0; i < deviceSize / 4; i++) {
        currChecksum += imageLwords[i];
    }

    munmap(imageBytes, deviceSize + 0x0C);
}
