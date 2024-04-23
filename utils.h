#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

uint32_t parse_length(const char *buf);

int32_t write_all(int fd, char *buf, size_t n);
int32_t read_full(int fd, char *buf, size_t n);

void msg(const char *m);
void msg_err(const char *m, int err_code);

#define MAX_BUFFER_SIZE 4096
#define HEADER_SIZE 4
#define MESSAGE_SIZE MAX_BUFFER_SIZE + HEADER_SIZE

#endif
