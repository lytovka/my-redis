#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

void die(const char *msg) {
  fprintf(stderr, "[%d] %s\n", errno, msg);
  abort();
}

static int32_t query(int fd, const char *text) {
  uint32_t len = (uint32_t)strlen(text);
  if (len > MAX_BUFFER_SIZE) {
    return -1;
  }

  char wbuf[MESSAGE_SIZE];
  memcpy(wbuf, &len, HEADER_SIZE);
  memcpy(&wbuf[HEADER_SIZE], text, len);
  int32_t res = write_all(fd, wbuf, HEADER_SIZE + len);

  if (res < 0) {
    if (errno == 0) {
      msg("EOF after write_all()");
    } else {
      msg_err("write_all() error", errno);
    }
    return res;
  }

  char rbuf[MESSAGE_SIZE + 1];
  errno = 0;

  res = read_full(fd, rbuf, HEADER_SIZE);

  if (res < 0) {
    if (errno == 0) {
      msg_err("read() error", res);
    } else {
      msg_err("read() error", errno);
    }
    return res;
  }

  // reply body
  len = parse_length(rbuf);

  res = read_full(fd, &rbuf[HEADER_SIZE], len);

  if (res < 0) {
    if (errno == 0) {
      msg_err("read() error", res);
    } else {
      msg_err("read() error", errno);
    }
    return res;
  }

  rbuf[4 + len] = '\0';
  printf("server says: %s\n", &rbuf[4]);

  return 0;
}

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "fd: %d", fd);
    die("unknown file descriptor");
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("connect to server");
  }

  // multiple requests
  int32_t err = query(fd, "hello1");
  if (err) {
    close(fd);
    return err;
  }

  err = query(fd, "hello2");
  if (err) {
    close(fd);
    return err;
  }

  err = query(fd, "hello3");
  if (err) {
    close(fd);
    return err;
  }

  close(fd);
  return 0;
}
