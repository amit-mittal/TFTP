/**
 * utility.h - header file which contains common function of tftp_c.c and tftp_s.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define MYPORT "4950" // port to be opened on server
#define SERVERPORT "4950" // the port users will be connecting to
#define MAXBUFLEN 550 // get sockaddr, IPv4 or IPv6:
#define MAX_READ_LEN 512 // maximum data size that can be sent on one packet
#define MAX_FILENAME_LEN 100 // maximum length of file name supported
#define MAX_PACKETS 99 // maximum number of file packets
#define MAX_TRIES 3 // maximum number of tries if packet times out
#define TIME_OUT 5 // in seconds


// converts block number to length-2 string
void s_to_i(char *f, int n){
	if(n==0){
		f[0] = '0', f[1] = '0', f[2] = '\0';
	} else if(n%10 > 0 && n/10 == 0){
		char c = n+'0';
		f[0] = '0', f[1] = c, f[2] = '\0';
	} else if(n%100 > 0 && n/100 == 0){
		char c2 = (n%10)+'0';
		char c1 = (n/10)+'0';
		f[0] = c1, f[1] = c2, f[2] = '\0';
	} else {
		f[0] = '9', f[1] = '9', f[2] = '\0';
	}
}

// makes RRQ packet
char* make_rrq(char *filename){
	char *packet;
	packet = malloc(2+strlen(filename));
	memset(packet, 0, sizeof packet);
	strcat(packet, "01");//opcode
	strcat(packet, filename);
	return packet;
}

// makes WRQ packet
char* make_wrq(char *filename){
	char *packet;
	packet = malloc(2+strlen(filename));
	memset(packet, 0, sizeof packet);
	strcat(packet, "02");//opcode
	strcat(packet, filename);
	return packet;
}

// makes data packet
char* make_data_pack(int block, char *data){
	char *packet;
	char temp[3];
	s_to_i(temp, block);
	packet = malloc(4+strlen(data));
	memset(packet, 0, sizeof packet);
	strcat(packet, "03");//opcode
	strcat(packet, temp);
	strcat(packet, data);
	return packet;
}

// makes ACK packet
char* make_ack(char* block){
	char *packet;
	packet = malloc(2+strlen(block));
	memset(packet, 0, sizeof packet);
	strcat(packet, "04");//opcode
	strcat(packet, block);
	return packet;
}

// makes ERR packet
char* make_err(char *errcode, char* errmsg){
	char *packet;
	packet = malloc(4+strlen(errmsg));
	memset(packet, 0, sizeof packet);
	strcat(packet, "05");//opcode
	strcat(packet, errcode);
	strcat(packet, errmsg);
	return packet;
}
