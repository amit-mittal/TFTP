/**
 * tftp_s.c - tftp server
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utility.h"

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//CHECKS FOR TIMEOUT
int check_timeout(int sockfd, char *buf, struct sockaddr_storage their_addr, socklen_t addr_len){
	fd_set fds;
	int n;
	struct timeval tv;

	// set up the file descriptor set
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	// set up the struct timeval for the timeout
	tv.tv_sec = TIME_OUT;
	tv.tv_usec = 0;

	// wait until timeout or data received
	n = select(sockfd+1, &fds, NULL, NULL, &tv);
	if (n == 0){
		printf("timeout\n");
		return -2; // timeout!
	} else if (n == -1){
		printf("error\n");
		return -1; // error	
	}

	return recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len);
}

int main(void){
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	//===========CONFIGURATION OF SERVER - START===========
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	
	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "SERVER: getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("SERVER: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("SERVER: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "SERVER: failed to bind socket\n");
		return 2;
	}
	freeaddrinfo(servinfo);
	
	printf("SERVER: waiting to recvfrom...\n");
	//===========CONFIGURATION OF SERVER - ENDS===========
	

	//===========MAIN IMPLEMENTATION - STARTS===========
	
	//WAITING FOR FIRST REQUEST FROM CLIENT - RRQ/WRQ
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("SERVER: recvfrom");
		exit(1);
	}
	printf("SERVER: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
	printf("SERVER: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("SERVER: packet contains \"%s\"\n", buf);

	if(buf[0] == '0' && buf[1] == '1'){//READ REQUEST
		//CHECKING IF FILE EXISTS
		char filename[MAX_FILENAME_LEN];
		strcpy(filename, buf+2);

		FILE *fp = fopen(filename, "rb");
		if(fp == NULL || access(filename, F_OK) == -1){ //SENDING ERROR PACKET - FILE NOT FOUND
			fprintf(stderr,"SERVER: file '%s' does not exist, sending error packet\n", filename);
			char *e_msg = make_err("02", "ERROR_FILE_NOT_FOUND");
			printf("%s\n", e_msg);
			sendto(sockfd, e_msg, strlen(e_msg), 0, (struct sockaddr *)&their_addr, addr_len);
			exit(1);
		}

		//STARTING TO SEND FILE
		int block = 1;
		fseek(fp, 0, SEEK_END);
		int total = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		int remaining = total;
		if(remaining == 0)
			++remaining;
		else if(remaining%MAX_READ_LEN == 0)
			--remaining;

		while(remaining>0){
			//READING FILE
			char temp[MAX_READ_LEN+5];
			if(remaining>MAX_READ_LEN){
				fread(temp, MAX_READ_LEN, sizeof(char), fp);
				temp[MAX_READ_LEN] = '\0';
				remaining -= (MAX_READ_LEN);
			} else {
				fread(temp, remaining, sizeof(char), fp);
				temp[remaining] = '\0';
				remaining = 0;
			}

			//SENDING - DATA PACKET
			char *t_msg = make_data_pack(block, temp);
			if((numbytes = sendto(sockfd, t_msg, strlen(t_msg), 0, (struct sockaddr *)&their_addr, addr_len)) == -1){
				perror("SERVER ACK: sendto");
				exit(1);
			}
			printf("SERVER: sent %d bytes\n", numbytes);

			//WAITING FOR ACKNOWLEDGEMENT - DATA PACKET
			int times;
			for(times=0;times<=MAX_TRIES;++times){
				if(times == MAX_TRIES){
					printf("SERVER: MAX NUMBER OF TRIES REACHED\n");
					exit(1);
				}

				numbytes = check_timeout(sockfd, buf, their_addr, addr_len);
				if(numbytes == -1){//error
					perror("SERVER: recvfrom");
					exit(1);
				} else if(numbytes == -2){//timeout
					printf("SERVER: try no. %d\n", times+1);
					int temp_bytes;
					if((temp_bytes = sendto(sockfd, t_msg, strlen(t_msg), 0, p->ai_addr, p->ai_addrlen)) == -1){
						perror("SERVER: ACK: sendto");
						exit(1);
					}
					printf("SERVER: sent %d bytes AGAIN\n", temp_bytes);
					continue;
				} else { //valid
					break;
				}
			}
			printf("SERVER: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
			printf("SERVER: packet is %d bytes long\n", numbytes);
			buf[numbytes] = '\0';
			printf("SERVER: packet contains \"%s\"\n", buf);				
			
			++block;
			if(block>MAX_PACKETS)
				block = 1;
		}
		fclose(fp);
	} else if(buf[0] == '0' && buf[1] == '2'){//WRITE REQUEST
		//SENDING ACKNOWLEDGEMENT
		char *message = make_ack("00");
		char last_recv_message[MAXBUFLEN];strcpy(last_recv_message, buf);
		char last_sent_ack[10];strcpy(last_sent_ack, message);
		if((numbytes = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&their_addr, addr_len)) == -1){
			perror("SERVER ACK: sendto");
			exit(1);
		}
		printf("SERVER: sent %d bytes\n", numbytes);

		char filename[MAX_FILENAME_LEN];
		strcpy(filename, buf+2);
		strcat(filename, "_server");

		if(access(filename, F_OK) != -1){ //SENDING ERROR PACKET - DUPLICATE FILE
			fprintf(stderr,"SERVER: file %s already exists, sending error packet\n", filename);
			char *e_msg = make_err("06", "ERROR_FILE_ALREADY_EXISTS");
			sendto(sockfd, e_msg, strlen(e_msg), 0, (struct sockaddr *)&their_addr, addr_len);
			exit(1);
		}

		FILE *fp = fopen(filename, "wb");
		if(fp == NULL || access(filename, W_OK) == -1){ //SENDING ERROR PACKET - ACCESS DENIED
			fprintf(stderr,"SERVER: file %s access denied, sending error packet\n", filename);
			char *e_msg = make_err("05", "ERROR_ACCESS_DENIED");
			sendto(sockfd, e_msg, strlen(e_msg), 0, (struct sockaddr *)&their_addr, addr_len);
			exit(1);
		}
		
		int c_written;
		do{
			//RECEIVING FILE - PACKET DATA
			if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
				perror("SERVER: recvfrom");
				exit(1);
			}
			printf("SERVER: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
			printf("SERVER: packet is %d bytes long\n", numbytes);
			buf[numbytes] = '\0';
			printf("SERVER: packet contains \"%s\"\n", buf);

			//SENDING LAST ACK AGAIN - AS IT HAS NOT REACHED
			if(strcmp(buf, last_recv_message) == 0){
				sendto(sockfd, last_sent_ack, strlen(last_sent_ack), 0, (struct sockaddr *)&their_addr, addr_len);
				continue;
			}

			//WRITING FILE
			c_written = strlen(buf+4);
			fwrite(buf+4, sizeof(char), c_written, fp);
			strcpy(last_recv_message, buf);

			//SENDING ACKNOWLEDGEMENT - PACKET DATA
			char block[3];
			strncpy(block, buf+2, 2);
			block[2] = '\0';
			char *t_msg = make_ack(block);
			if((numbytes = sendto(sockfd, t_msg, strlen(t_msg), 0, (struct sockaddr *)&their_addr, addr_len)) == -1){
				perror("SERVER ACK: sendto");
				exit(1);
			}
			printf("SERVER: sent %d bytes\n", numbytes);
			strcpy(last_sent_ack, t_msg);
		} while(c_written == MAX_READ_LEN);
		printf("NEW FILE: %s SUCCESSFULLY MADE\n", filename);
		fclose(fp);
	} else {
		fprintf(stderr,"INVALID REQUEST\n");
		exit(1);
	}
	//===========MAIN IMPLEMENTATION - ENDS===========


	close(sockfd);
	
	return 0;
}
