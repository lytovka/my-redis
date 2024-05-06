#include "utils.h"
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define MAX_BUFFER_SIZE 4096
#define HEADER_SIZE 4
#define MAX_MESSAGE_SIZE MAX_BUFFER_SIZE + HEADER_SIZE

void die(const char *msg) {
  fprintf(stderr, "[%d] %s\n", errno, msg);
  abort();
}

static void fd_set_nb(int fd) {
  errno = 0;
  int flags = fcntl(fd, F_GETFL, 0);
  if (errno) {
    die("fcntl error after getting file status flags");
    return;
  }

  flags |= O_NONBLOCK;

  errno = 0;
  fcntl(fd, F_SETFL, flags);
  if (errno) {
    die("fcntl error after enabling non-blocking I/O");
  }
}

enum {
  STATE_REQ = 0,
  STATE_RES = 1,
  STATE_END = 2,
};

struct Conn {
  int fd = -1;
  uint32_t state = STATE_REQ;
  // buffer for reading
  size_t rbuf_size = 0;
  uint8_t rbuf[MAX_MESSAGE_SIZE];
  // buffer for writing
  size_t wbuf_size = 0;
  size_t wbuf_sent = 0;
  uint8_t wbuf[MAX_MESSAGE_SIZE];
};

static void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn) {
  if (fd2conn.size() <= conn->fd) {
    fd2conn.resize(conn->fd + 1);
  }
  fd2conn[conn->fd] = conn;
}

static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd) {
  // accept a new connection
  struct sockaddr_in client_addr = {};
  socklen_t socklen = sizeof(client_addr);
  int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
  if (connfd < 0) {
    msg("accept() error");
    return -1;
  }

  // set new connection to non-blocking mode
  fd_set_nb(connfd);

  // creating the struct Conn
  struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));

  if (!conn) {
    close(connfd);
    return -1;
  }

  conn->fd = connfd;
  conn->state = STATE_REQ;
  conn->rbuf_size = 0;
  conn->wbuf_size = 0;
  conn->wbuf_sent = 0;
  conn_put(fd2conn, conn);

  return 0;
}

static bool try_flush_buffer(Conn *conn) {
  ssize_t rv = 0;
  do {
    size_t remain = conn->wbuf_size - conn->wbuf_sent;
    rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
  } while (rv < 0 && errno == EINTR);

  if (rv < 0 && errno == EAGAIN) {
    msg("Got EAGAIN\n");
    return false;
  }

  if (rv < 0) {
    msg("write() error\n");
    conn->state = STATE_END;
    return false;
  }

  conn->wbuf_sent += (size_t)rv;
  assert(conn->wbuf_sent <= conn->wbuf_size);

  if (conn->wbuf_sent == conn->wbuf_size) {
    // response was fully sent, change state back
    conn->state = STATE_REQ;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    return false;
  }

  // still got some data, could try to write again
  return true;
}

static bool state_res(Conn *conn) {
  while (try_flush_buffer(conn)) {
  }
  return true;
}

static bool try_one_request(Conn *conn) {
  if (conn->rbuf_size < HEADER_SIZE) {
    msg("Not enough data in the buffer. Will retry in the next iteration\n");
    return false;
  }
  uint32_t len = 0;
  memcpy(&len, &conn->rbuf[0], HEADER_SIZE);
  if (len > MAX_BUFFER_SIZE) {
    msg("too long message\n");
    conn->state = STATE_END;
    return false;
  }

  if (HEADER_SIZE + len > conn->rbuf_size) {
    return false;
  }

  printf("client fays: %.*s\n", len, &conn->rbuf[HEADER_SIZE]);

  // generate echoing responses
  memcpy(&conn->wbuf[0], &len, HEADER_SIZE);
  memcpy(&conn->wbuf[HEADER_SIZE], &conn->rbuf[HEADER_SIZE], len);
  conn->wbuf_size = HEADER_SIZE + len;

  // remove request from the buffer
  size_t remain = conn->rbuf_size - HEADER_SIZE - len;
  if (remain) {
    memmove(conn->rbuf, &conn->rbuf[HEADER_SIZE + len], remain);
  }
  conn->rbuf_size = remain;

  // change state
  conn->state = STATE_RES;
  state_res(conn);

  return conn->state == STATE_REQ;
}

static bool try_fill_buffer(Conn *conn) {
  assert(conn->rbuf_size < sizeof(conn->rbuf));

  ssize_t rv = 0;
  do {
    size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
    rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
  } while (rv < 0 && errno == EINTR);
  if (rv < 0 && errno == EAGAIN) {
    // got EAGAIN, stop
    msg("got EAGAIN\n");
    return false;
  }
  if (rv < 0) {
    msg("read error");
    conn->state = STATE_END;
    return false;
  }

  if (rv == 0) {
    msg(conn->rbuf_size > 0 ? "unexpected EOF" : "EOF");
    conn->state = STATE_END;
    return false;
  }

  conn->rbuf_size += (size_t)rv;
  assert(conn->rbuf_size <= sizeof(conn->rbuf));

  while (try_one_request(conn)) {
  }
  return (conn->state == STATE_REQ);
}

static void state_req(Conn *conn) {
  while (try_fill_buffer(conn)) {
  }
}

static void connection_io(Conn *conn) {
  switch (conn->state) {
  case STATE_REQ: {
    state_req(conn);
    break;
  }
  case STATE_RES: {
    state_res(conn);
    break;
  }
  default: {
    assert(0);
    break;
  }
  }
}

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    die("could not create socket");
  }

  int val = 1;
  int set_result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
  int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("could not bind\n");
  }

  int listen_result = listen(fd, SOMAXCONN);

  std::vector<Conn *> fd2conn;

  // set the listen fd to nonblocking mode
  fd_set_nb(fd);

  // the event loop
  std::vector<struct pollfd> poll_args;

  while (true) {
    // prepare arguments for poll
    poll_args.clear();
    struct pollfd pfd = {fd, POLLIN, 0};

    poll_args.push_back(pfd);

    for (Conn *conn : fd2conn) {
      if (!conn) {
        continue;
      }
      struct pollfd pfd = {};
      pfd.fd = conn->fd;
      pfd.events = conn->state == STATE_REQ ? POLLIN : POLLOUT;
      pfd.events = pfd.events | POLLERR;
      poll_args.push_back(pfd);
    }

    // poll for active fds
    int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
    if (rv < 0) {
      die("poll error");
    }

    // process active connections
    for (size_t i = 1; i < poll_args.size(); ++i) {
      if (poll_args[i].revents) {
        Conn *conn = fd2conn[poll_args[i].fd];
        connection_io(conn);
        if (conn->state == STATE_END) {
          // client closed normally, or something bad happened. Destroy the
          // connection
          fd2conn[conn->fd] = NULL;
          close(conn->fd);
          free(conn);
        }
      }
    }

    // try to process a new connection if the listening fd is active

    if (poll_args[0].revents) {
      accept_new_conn(fd2conn, fd);
    }
  }

  return 0;
}
