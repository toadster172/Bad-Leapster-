#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

char *strcat(char *dst, char *src);
void memset(void *dst, int c, size_t n);
uint8_t *pixelSet(uint8_t *dst, uint8_t pixel, uint8_t color, uint16_t len);

#endif
