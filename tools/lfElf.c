// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 toadster172 <toadster172@gmail.com>

// This really either shouldn't be its own file or should have a less tightly coupled interface
// The reason it is how it is right now is because I don't want to rewrite it with C++-legal goto behaviour

#include "lfElf.h"
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool extractElfInfo(const char *filename, struct LfElfData *buf) {
    memset(buf, 0, sizeof(*buf));

    int fd = open(filename, O_RDONLY);

    if(fd == -1) {
        printf("Failed to ELF file!\n");
        return false;
    }

    elf_version(EV_CURRENT);

    Elf *e = elf_begin(fd, ELF_C_READ, NULL);

    if(e == NULL) {
        printf("Failed to open elf! %s\n", elf_errmsg(elf_errno()));
        goto closeFD;
    }

    size_t shstrtabIndex;

    if(elf_getshdrstrndx(e, &shstrtabIndex) == -1) {
        printf("Failed to get .shstrtab!\n");
        goto closeELF;
    }

    Elf_Scn *section = elf_nextscn(e, NULL);

    if(section == NULL) {
        printf("Failed to open first section of ELF!\n");
        goto closeELF;
    }

    size_t stringTableIndex = 0;
    Elf_Data *symTable = NULL;
    size_t symTableEntSize = 0;

    do {
        GElf_Shdr shdr;

        if(gelf_getshdr(section, &shdr) == NULL) {
            printf("Failed to get section header!\n");
            goto closeELF;
        }

        char *sectionName = elf_strptr(e, shstrtabIndex, shdr.sh_name);

        if(sectionName == NULL) {
            continue;
        }

        if(!strcmp(sectionName, ".symtab")) {
            symTable = elf_getdata(section, NULL);
            symTableEntSize = shdr.sh_entsize;

            if(symTable == NULL || symTableEntSize == 0) {
                printf("Failed to get symtab data!\n");
                goto closeELF;
            }
        } else if(!strcmp(sectionName, ".strtab")) {
            stringTableIndex = elf_ndxscn(section);
        }
    } while((section = elf_nextscn(e, section)) != NULL);

    if(stringTableIndex == 0 || symTable == NULL) {
        printf("Failed to find either .symtab or .strtab in ELF!\n");
        goto closeELF;
    }

    size_t syms = symTable->d_size / symTableEntSize;

    // This is not ideal but for some reason GCC and LD don't feel like generating a symbol
    //   hashtable and this is an easier bodge for now than debugging that
    for(int i = 0; i < syms; i++) {
        GElf_Sym sym;
        const char *symName;

        if(gelf_getsym(symTable, i, &sym) == NULL ||
          (symName = elf_strptr(e, stringTableIndex, sym.st_name)) == NULL) {
            continue;
        }

        if(!strcmp(symName, "hbMPI")) {
            buf->mpiInterfaceAddr = sym.st_value;
        } else if(!strcmp(symName, "hbMPIName")) {
            buf->mpiNameAddr = sym.st_value;
        } else if(!strcmp(symName, "hbMPIDesc")) {
            buf->mpiDescAddr = sym.st_value;
        }
    }

    if(buf->mpiInterfaceAddr == 0 || buf->mpiNameAddr == 0 || buf->mpiDescAddr == 0) {
        printf("Failed to locate symbol for one of hbMPI, hbMPIName, or hbMPIDesc");
        goto closeELF;
    }

    size_t segmentCount;

    if(elf_getphdrnum(e, &segmentCount) != 0) {
        printf("Failed to get segment count!\n");
        goto closeELF;
    }

    size_t rxFileOffset = 0;

    for(int i = 0; i < segmentCount; i++) {
        GElf_Phdr segment;

        if(gelf_getphdr(e, i, &segment) == NULL) {
            continue;
        }

        if(segment.p_flags & PF_X) {
            buf->rxSegment = malloc(segment.p_filesz);
            buf->rxSegmentAddr = segment.p_paddr;
            buf->rxSegmentLength = segment.p_filesz;
            rxFileOffset = segment.p_offset;

            break;
        }
    }

    if(rxFileOffset == 0) {
        printf("Failed to find executable segment!\n");
        goto closeELF;
    }

    elf_end(e);

    lseek(fd, rxFileOffset, SEEK_SET);
    if(read(fd, buf->rxSegment, buf->rxSegmentLength) != buf->rxSegmentLength) {
        printf("Failed to read executable segment!\n");
        free(buf->rxSegment);
        goto closeFD;
    }

    close(fd);
    return true;

closeELF:
    elf_end(e);
closeFD:
    close(fd);

    return false;
}
