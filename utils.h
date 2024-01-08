#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

int32_t write_all(int fd, char *buf, size_t n);
int32_t read_full(int fd, char *buf, size_t n);

#endif
