#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>


int main(void) {
	struct timeval tv;
	tv.tv_sec = 2;
	while(1){
		fprintf(stderr, "%ld\n", tv.tv_sec);
	}
}
