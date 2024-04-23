#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void msg(const char *m) { fprintf(stderr, "%s\n", m); }
void msg_err(const char *m, int error_code) {
  fprintf(stderr, "error code %d: %s\n", error_code, m);
}

uint32_t parse_length(const char *buf) {
  uint32_t len = 0;
  memcpy(&len, buf, HEADER_SIZE);
  return len;
}

int32_t write_all(int fd, char *buf, size_t buf_len) {
  while (buf_len > 0) {
    ssize_t rv = write(fd, buf, buf_len);
    if (rv <= 0) {
      return -1; // error
    }
    assert((size_t)rv <= buf_len);
    buf_len -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

int32_t read_full(int fd, char *buf, size_t buf_len) {
  while (buf_len > 0) {
    ssize_t rv = read(fd, buf, buf_len);
    if (rv <= 0) {
      return -1; // error
    }
    assert((size_t)rv <= buf_len);
    buf_len -= (size_t)rv;
    buf += rv;
  }
  return 0;
}
