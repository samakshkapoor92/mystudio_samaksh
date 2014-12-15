#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_PCKSIZE 		512

#define RRQ   01
#define WRQ   02
#define DATA  03
#define ACK   04
#define ERROR 05

#define WAITACK 0.5

#define NODEF		0
#define NOFILE 		1
#define ACCESS		2
#define DISK		3
#define ILTFTP		4
#define UNKNOWNID	5
#define FILEXIST	6
#define NOUSER		7

struct _PCK {
	unsigned short opcode;
	union {
		unsigned short block;
		unsigned short errcode;
		char filename[1];
	}_PCK_BLOCK;
	char data[MAX_PCKSIZE];
};

fd_set readfds, masterfds;
int retrans_cnt = 0, client_cnt = 0;
struct sockaddr_in servaddr, cliaddr;
struct timeval timeout;    // Time out value

void readfile_senddata(int sfd, struct _PCK *pck);
int recv_ack(int sfd, struct _PCK *last_pck);
void retrans_data(int sfd, struct _PCK *last_pck, int pcksize);
void send_err(int sfd, int err);

int main(int argc, char *argv[]) {
	int sockfd, newfd, server_port, tid, i;
	int maxfds;
	char *server_ip;
	int numbytes, len, val;
	char buf[MAX_PCKSIZE];
	struct _PCK *packet;
	
	FD_ZERO(&readfds);
	FD_ZERO(&masterfds);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if (argc != 3) {    // argv[1]: ip, argv[2]: port
        fprintf(stderr,"Usage: server_ip server_port\n");
        return -1;
    }
    server_ip = argv[1];
    server_port = atoi(argv[2]);

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "Error: Can't create socket\n");
		return -1;
	} 
	printf("Created socket: %d\n", sockfd);
		
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(server_port);
	len = sizeof(cliaddr);

	if(bind(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1) {
    	fprintf(stderr, "Error: Can't bind the socket.\n");
    	return -1;
    }
    puts("Completed binding");
    FD_SET(sockfd, &masterfds);
    maxfds = sockfd;

	while(1){
		readfds = masterfds;
		if((select(maxfds + 1, &readfds, NULL, NULL, NULL)) == -1) {
			fprintf(stderr, "Timeout.\n");
			return -1;
		}
		puts("\nCompleted select");

		for(i = 0; i <= maxfds; i++) {
			if(FD_ISSET(i, &readfds)) {
				if(numbytes = recvfrom(sockfd, buf, MAX_PCKSIZE, 0, (struct sockaddr *)&cliaddr, &len) < 0){
					fprintf(stderr, "Error: Can't receive request.\n");
					return -1;
				}
				puts("Received request from client");

				packet = (struct _PCK *)buf;
				packet->opcode = ntohs(packet->opcode);
				printf("Request_opcode: %d\n", packet->opcode);

				if(i == sockfd) {	// new connection
					if(packet->opcode == RRQ) {
						if((newfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
								fprintf(stderr, "Error: Can't create new socket.\n");
								return -1;
						} 
						printf("\nCreated new socket: %d\n", newfd);

						FD_SET(newfd, &masterfds);
						if(newfd >= maxfds)
							maxfds = newfd;

						tid = 3000 + rand()%600000;
						printf("TID: %d\n", tid);
						bzero(&servaddr, sizeof(servaddr));
						servaddr.sin_family = AF_INET;
						servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
						servaddr.sin_port = htons(tid);

						if(bind(newfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1) {
					    	fprintf(stderr, "Error: Can't bind the new socket.\n");
					    	return -1;
					    }
					    puts("Completed new binding");

						readfile_senddata(newfd, packet);
					}
				} else {
					switch(packet->opcode) {
						case RRQ:
							readfile_senddata(i, packet);
							break;
						case ACK:
							printf("[ACK_OF_ERROR] Closing socket.%d\n", i);
							FD_CLR(i, &masterfds);
							close(i);
							break;
						case ERROR:
							FD_CLR(i, &masterfds);
							close(i);
							break;
						default:
							fprintf(stderr, "Unknown opcode: %d\n", packet->opcode);
							FD_CLR(i, &masterfds);
							close(i);
							break;
					}
				} 
			} // FD_ISSET
		} // for(i = 0; i <= maxfds; i++) 
	}
	return 0;
}

void readfile_senddata(int sfd, struct _PCK *pck) {
	int blocknum = 1, filesize, len, pcksize, ack_val;
	char *filename, sbuf[MAX_PCKSIZE], data_buf[MAX_PCKSIZE];
	FILE *file;
	struct _PCK *sending_pck;

	sending_pck = (struct _PCK *)malloc(sizeof(struct _PCK));

	filename = pck->_PCK_BLOCK.filename;
	if((file = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Error: Can't find the file: %s\n", filename);
		send_err(sfd, NOFILE);
	} else {
		puts("Completed opening file.");

		do {
			if((filesize = fread(data_buf, 1, MAX_PCKSIZE, file)) < 0) {
				fprintf(stderr, "Error: Can't read the file: %s\n", filename);
				send_err(sfd, ACCESS);	
			}
			puts("Completed reading file.");
			printf("Filesize: %d\n", filesize);
			
			// Pack the first part of file to send the client
			sending_pck->opcode = htons(DATA);
			sending_pck->_PCK_BLOCK.block = htons(blocknum);
			memcpy(sending_pck->data, data_buf, filesize);
			memcpy(sbuf, sending_pck, filesize + 4);

			// Send DATA packet
			len = sizeof(cliaddr);
			if((pcksize = sendto(sfd, sbuf, filesize + 4, 0, 
					(struct sockaddr *)&cliaddr, len)) != filesize + 4) {
				fprintf(stderr, "Error: Can't send data.\n");
				exit(1);	
			}
			printf("Sent %d block of the file\n", blocknum);

			if(pcksize < 516) {	
				printf("[LAST_PACKET] Closing socketfd %d\n", sfd);
				fclose(file);
			}

			if((ack_val = recv_ack(sfd, sending_pck)) == -1)
				exit(1);

			blocknum++;
		}while(filesize == MAX_PCKSIZE);
	}
}

int recv_ack(int sfd, struct _PCK *last_pck) {
	int len, numbytes;
	char rbuf[MAX_PCKSIZE];
	struct _PCK *ack;

	// Receive ack or retransmit data
	len = sizeof(cliaddr);
	if((numbytes = recvfrom(sfd, rbuf, MAX_PCKSIZE, 0, (struct sockaddr *)&cliaddr, &len)) < 0) {
		fprintf(stderr, "Error: Can't receive ack.\n");		
		printf("(ack: %d \t last: %d)\n", ack->_PCK_BLOCK.block, ntohs(last_pck->_PCK_BLOCK.block));
		printf("***************************************************\n");
		printf("%d RETRANSMIT: %d time(s).\n", sfd, retrans_cnt);
		printf("***************************************************\n");
		retrans_data(sfd, last_pck, numbytes);
		recv_ack(sfd, last_pck);
	} 

	ack = (struct _PCK *)rbuf;
	ack->opcode = ntohs(ack->opcode);
	ack->_PCK_BLOCK.block = ntohs(ack->_PCK_BLOCK.block);

	switch(ack->opcode) {
		case ACK: 
			puts("Received ACK from client.");
			if (ack->_PCK_BLOCK.block == ntohs(last_pck->_PCK_BLOCK.block)) {
				puts("Data has been received successfully.\n");
			} else {	// missing data
				printf("(ack: %d \t last: %d)\n", ack->_PCK_BLOCK.block, ntohs(last_pck->_PCK_BLOCK.block));
				printf("***************************************************\n");
				printf("RETRANSMIT: %d time(s).\n", retrans_cnt);
				printf("***************************************************\n");
				retrans_data(sfd, last_pck, numbytes);
				recv_ack(sfd, last_pck);
				return 0;
			}
			break;
		case ERROR:
			puts("Received ERROR from client.");
			fprintf(stderr, "[ERROR_OPCODE] Closing socket: %d\n", sfd);
			return 0;	
			break;
		case RRQ:	// Init data(ack from server) is missing.
			puts("Received RRQ from client.");
			printf("(ack: %d \t last: %d)\n", ack->_PCK_BLOCK.block, ntohs(last_pck->_PCK_BLOCK.block));
			printf("***************************************************\n");
			printf("%d RETRANSMIT: %d time(s).\n", sfd, retrans_cnt);
			printf("***************************************************\n");
			retrans_data(sfd, last_pck, numbytes);
			recv_ack(sfd, last_pck);
			return 0;
		default:
			fprintf(stderr, "[UNKNOWN_OPCODE:%d] Closing socket: %d\n", ack->opcode, sfd);
			close(sfd);
			return -1;
			break;
	}
	return 0;
}

void retrans_data(int sfd, struct _PCK *last_pck, int pcksize) {
	int len, numbytes;
	char sbuf[MAX_PCKSIZE];

	memcpy(sbuf, last_pck, pcksize);

	len = sizeof(cliaddr);
	if((sendto(sfd, sbuf, pcksize, 0, 
			(struct sockaddr *)&cliaddr, len)) != pcksize) {
		fprintf(stderr, "Error: Can't re-send data.\n");
		exit(1);	// SEND_ERR
	}
}

void send_err(int sfd, int err) {
	int len = sizeof(cliaddr), errlen;
	struct _PCK *err_pck;
	char errmsg[MAX_PCKSIZE], sbuf[MAX_PCKSIZE];

	err_pck = (struct _PCK *)malloc(sizeof(struct _PCK));

	switch(err){
		case NODEF:
			break;
		case NOFILE:
			errlen = sizeof("File not found.");
			memcpy(errmsg, "File not found.", errlen);
			break;
		case ACCESS:
			errlen = sizeof("No access to file.");
			memcpy(errmsg, "No access to file.", errlen);
			break;
		case DISK:
			break;
		case ILTFTP:
			break;
		case UNKNOWNID:
			break;
		case FILEXIST:
			break;
		case NOUSER:
			break;
	}
	errmsg[errlen] = '\0';
	err_pck->opcode = htons(ERROR);
	err_pck->_PCK_BLOCK.errcode = htons(err);

	memcpy(err_pck->data, errmsg, sizeof(errmsg));
	memcpy(sbuf, err_pck, sizeof(errmsg) + 4);

	if((sendto(sfd, sbuf, sizeof(errmsg) + 4, 0, 
			(struct sockaddr *)&cliaddr, len)) != sizeof(errmsg) + 4) {
		fprintf(stderr, "Error: Can't send error message.\n");
		exit(1);
	}
	recv_ack(sfd, err_pck);
	printf("[ERROR_FUCNTION] closing socketfd %d\n", sfd);
}



