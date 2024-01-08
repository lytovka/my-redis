#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "utils.h"

const int32_t k_max_msg = 4096;

void die(const char *msg){
	fprintf(stderr, "[%d] %s\n", errno, msg);
	abort();
}


void msg(const char *m){
	fprintf(stderr, "%s\n", m);
}

static int32_t query(int fd, const char *text){
	uint32_t len = (uint32_t)strlen(text);
	if(len > k_max_msg){
		return -1;
	}

	char wbuf[4 + k_max_msg];
	memcpy(wbuf, &len, 4);
	memcpy(&wbuf[4], text, len);
	int32_t err = write_all(fd, wbuf, 4 + len);
	if(err){
		return err;
	}

	// 4 byte header
	char rbuf[4 +k_max_msg + 1];
	errno = 0;

	err = read_full(fd, rbuf, 4);
	if(err){
		if(errno == 0){
			msg("EOF");
		}
		else {
			msg("read() error");
		}
		return err;
	}

	memcpy(&len ,rbuf, 4);
	if(len > k_max_msg){
		msg("too long");
		return -1;
	}

	//reply body
	err = read_full(fd, &rbuf[4], len);
	if(err){
		msg("read() error");
		return err;
	}

	rbuf[4 + len] = '\0';
	printf("server says: %s\n", &rbuf[4]);

	return 0;
}


int main(){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		fprintf(stderr, "fd: %d", fd);
		die("unknown file descriptor");
	}
	printf("ntohs - %d\n", ntohs(1234));	
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

	int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if(rv){
		die("connect to server");
	}

	// multiple requests
	int32_t err = query(fd, "hello1");
	if(err){
		goto L_DONE;
	}
	
	err = query(fd, "hello2");	
	if(err){
		goto L_DONE;
	}

	err = query(fd, "hello3");	
	if(err){
		goto L_DONE;
	}

	L_DONE:
	close(fd);
	return 0;
}
