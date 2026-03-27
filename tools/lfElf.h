#ifndef LF_ELF_H
#define LF_ELF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LfElfData {
    uint8_t *rxSegment;
    uint32_t rxSegmentLength;
    uint32_t rxSegmentAddr;

    uint32_t mpiInterfaceAddr;
    uint32_t mpiNameAddr;
    uint32_t mpiDescAddr;
};

bool extractElfInfo(const char *filename, struct LfElfData *buf);

#ifdef __cplusplus
}
#endif

#endif
