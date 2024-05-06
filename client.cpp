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

static int32_t send_req(int fd, const char *text) {
  uint32_t len = (uint32_t)strlen(text);
  if (len > MAX_BUFFER_SIZE) {
    return -1;
  }

  printf("query len: %d\n", len);

  char wbuf[MESSAGE_SIZE];
  printf("wbuf size: %d\n", (int)sizeof(wbuf));

  memcpy(wbuf, &len, HEADER_SIZE);
  memcpy(&wbuf[HEADER_SIZE], text, len);

  return write_all(fd, wbuf, HEADER_SIZE + len);
}

static int32_t read_res(int fd) {
  // 4 byte header
  char rbuf[MESSAGE_SIZE + 1];
  errno = 0;
  int32_t res = read_full(fd, rbuf, HEADER_SIZE);

  if (res < 0) {
    if (errno == 0) {
      msg_err("read() error", res);
    } else {
      msg_err("read() error", errno);
    }
    return res;
  }

  // parse length
  uint32_t len = 0;
  len = parse_length(rbuf);

  // reply body
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

  // multiple pipelined requests
  const char *query_list[3] = {"hello1", "hello2", "hello3"};
  for (size_t i = 0; i < 3; i++) {
    int32_t err = send_req(fd, query_list[i]);
    if (err) {
      goto L_DONE;
    }
  }

  for (size_t i = 0; i < 3; ++i) {
    int32_t err = read_res(fd);
    if (err) {
      goto L_DONE;
    }
  }

L_DONE:
  close(fd);
  return 0;
}
