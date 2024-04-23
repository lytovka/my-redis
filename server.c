#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}

static int32_t one_request(int connfd) {
  char rbuf[MESSAGE_SIZE + 1];
  errno = 0;

  int32_t read_result = read_full(connfd, rbuf, HEADER_SIZE);
  if (read_result <= 0) {
    if (errno != 0) {
      msg_err("header read() error", errno);
      return read_result;
    }
  }

  uint32_t len = parse_length(rbuf);
  if (len > MAX_BUFFER_SIZE) {
    return -1;
  }

  read_result = read_full(connfd, &rbuf[HEADER_SIZE], len);
  if (read_result <= 0) {
    if (errno != 0) {
      msg_err("body read() error", errno);
      return read_result;
    }
  }

  rbuf[HEADER_SIZE + len] = '\0';
  printf("client says: %s\n", &rbuf[HEADER_SIZE]);

  // reply using the same protocol
  const char reply[] = "world";
  char wbuf[HEADER_SIZE + sizeof(reply)];
  len = (uint32_t)strlen(reply);
  memcpy(wbuf, &len, HEADER_SIZE);
  memcpy(&wbuf[HEADER_SIZE], reply, len);
  return write_all(connfd, wbuf, HEADER_SIZE + len);
}

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  printf("fd: %d\n", fd);
  if (fd < 0) {
    die("socket()");
  }

  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // bind
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(0);
  int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));

  if (rv) {
    die("bind()");
  }

  // listen
  rv = listen(fd, SOMAXCONN);

  if (rv) {
    die("listen()");
  }

  while (1) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
      continue;
    }
    while (1) {
      int32_t err = one_request(connfd);
      if (err) {
        break;
      }
    }
    close(connfd);
  }

  return 0;
}
