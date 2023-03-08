#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <time.h>
#include <sys/time.h>

#include <signal.h>

#include <stdint.h>

static volatile int goon = 1;

void interuptHandler(int code) {
	goon = 0;
}

int sleep_ns(long nsec) {
	
	struct timespec ts;
	int res;

	if (nsec < 0) {
		return -1;
	}

	ts.tv_sec = nsec / 1000000000;
	ts.tv_nsec = nsec % 1000000000;
	
	return nanosleep(&ts, &ts);
}

uint64_t time_ms() {
	struct timeval tval; 
	gettimeofday(&tval, NULL); // get current time
	uint64_t milliseconds = tval.tv_sec*1000LL + tval.tv_usec/1000;
	return milliseconds;
}

void orderTimeBytes(uint64_t localEndian, uint8_t* byteArray) {
	
	uint64_t tmp = localEndian;
	
	for (int i = 0; i < sizeof(uint64_t); i++) {
		uint8_t data = (uint8_t)(tmp % 256);
		byteArray[i] = data;
		tmp /= 256;
	}
}

int main(int argc, char** argv) {
	
	uint16_t broadcast_port = 5070;
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in s;
	
	if (sockfd < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	printf("Starting time broadcast udp server on port %u\n", broadcast_port);
	
	int broadcastEnable=1;
	int ret=setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	
	uint32_t target_addr = 10u+42u*256u+255u*256u*256u*256u;
	struct in_addr ip_addr;
    ip_addr.s_addr = target_addr;
	printf("The IP address is %s\n", inet_ntoa(ip_addr));
	
	memset(&s, '\0', sizeof(struct sockaddr_in));
	s.sin_family = AF_INET;
	s.sin_port = htons(broadcast_port);
	s.sin_addr.s_addr = target_addr; //htonl(INADDR_BROADCAST);
	
	signal(SIGINT, interuptHandler);
	
	uint8_t timesByteArray[sizeof(uint64_t)];
	
	while (goon) {
		
		int flags = 0;
		
		uint64_t ms = time_ms();
		orderTimeBytes(ms, timesByteArray);
		
		int b_written = sendto(sockfd, timesByteArray, sizeof(uint64_t), flags, (struct sockaddr *) &s, sizeof(s));
		
		if (b_written < 0) {
			perror("Failed to write a datagram");
		}
		
		sleep_ns(500000);
		
	}
	
	printf("\nClosing time broadcast udp server\n");
	
	close(sockfd);
	
	return 0;
}
