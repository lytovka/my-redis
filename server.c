#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

void msg(const char *m){
	fprintf(stderr, "%s\n", m);
}

void die(const char *msg){
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}

static void do_something(int connfd){
	char rbuf[64] = {};
	ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
	if(n < 0) {
		msg("read() error");
		return;
	}
	printf("client says: %s\n", rbuf);

	char wbuf[] = "world";
	write(connfd, wbuf, strlen(wbuf));
}

const size_t k_max_msg = 4096;

static int32_t one_request(int connfd){
	char rbuf[4 + k_max_msg + 1];
	errno = 0;

	// 4 byte header
	int32_t err = read_full(connfd, rbuf, 4);
	if(err){
		if(errno == 0){
			msg("EOF");
		}
		return err;
	}

	uint32_t len = 0;
	memcpy(&len, rbuf, 4);
	if(len > k_max_msg){
		return -1;
	}

	// request body
	err = read_full(connfd, &rbuf[4], len);
	if(err){
		msg("read() request body error");
		return err;
	}

	rbuf[4 + len] = '\0';
	printf("client says: %s\n", &rbuf[4]);

	// reply using the same protocol
	const char reply[] = "world";
	char wbuf[4 + sizeof(reply)];
	len = (uint32_t) strlen(reply);
	memcpy(wbuf, &len, 4);
	memcpy(&wbuf[4], reply, len);
	return write_all(connfd, wbuf, 4 + len);
}

int main(){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	printf("fd: %d\n", fd);
	if(fd < 0) {
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

	if(rv){
		die("bind()");
	}

	// listen
	rv = listen(fd, SOMAXCONN);

	if(rv){
		die("listen()");
	}

	while(1){
		struct sockaddr_in client_addr = {};
		socklen_t socklen = sizeof(client_addr);
		int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
		if(connfd < 0){
			continue;
		}
		while(1){
			int32_t err = one_request(connfd);
			if(err) {
				break;
			}
		}
		close(connfd);
	}

	return 0;
}
