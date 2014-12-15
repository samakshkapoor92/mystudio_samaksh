/* 
ECEN602: Computer Networks
HW3 programming assignment: HTTP1.0 proxy server
Team14: Samaksh Kapoor, Jung In Koh
Date: Oct. 23. 2014
*/

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <time.h>
#include <unistd.h>

#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 1024
#define MAXCACHESIZE 10

struct cache {
	char ip[200];
	char file_name[1024];
	char data[MAXDATASIZE];
	struct tm server_time;
	struct tm expire_time;
	struct tm last_modified_time;
	time_t current_time_stamp;
	FILE *file;
};

void url_mycode(char *url, char *url_mycode) {
    char rfc[256] = {0};
    int i = 0;
    for (; i < 256; i++) {
        rfc[i] = isalnum(i)||i == '~'||i == '-'||i == '.'||i == '_' 
            ? i : 0;
    }   
    
    char *tb = rfc;
    
    for (; *url; url++) {
        if (tb[*url]) sprintf(url_mycode, "%c", tb[*url]);
        else        sprintf(url_mycode, "%%%02X", *url);
        while (*++url_mycode);
    }   
}


fd_set readfds, masterfds;
struct cache cache_list[MAXCACHESIZE];
int current_cache_size = 0;

int Is_cached(char *ip, char *name);
int Is_staled_data(int loc);
void replace_by_LRU(int loc); 
void create_cache(char *buf, char *ip, char *name, FILE *fp);


int main(int argc, char const *argv[])
{
	int port, sockfd, newfd, serverfd;
	int clen, maxfds, numbytes, pass; 
	int i, loc, total_size = 0, filesize;
	int init = 1;
	char *ip, *tmp, *file_path, *p1, *p2, *file_name;
	char buf[MAXDATASIZE], rbuf[MAXDATASIZE], sbuf[MAXDATASIZE];
	struct hostent *host;
	struct sockaddr_in proxy_addr, client_addr, server_addr;
	size_t len;
	FILE *fp;

	FD_ZERO(&readfds);
	FD_ZERO(&masterfds);

	if (argc != 3) {
		fprintf(stderr,"Usage: ./proxy <ip_to_bind> <port_to_bind>\n");
        return -1;
	}
	ip = argv[1];

	if ((host = gethostbyname(ip)) == NULL) {
		fprintf(stderr, "[ERROR] Failed to match hostname\n");
		return -1;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		close(sockfd);
		fprintf(stderr, "[ERROR] Failed to create a socket\n");
		return -1;
	}

	bzero((char *)&proxy_addr, sizeof(proxy_addr));
	port = atoi(argv[2]);
	inet_pton(AF_INET, ip, &(proxy_addr.sin_addr));
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) == -1) {
		close(sockfd);
		fprintf(stderr, "[ERROR] Failed to bind\n");
		return -1;
	}

	FD_SET(sockfd, &masterfds);
    maxfds = sockfd;

	while (1) {
		if (listen(sockfd, BACKLOG) == -1) {
	        fprintf(stderr, "[ERROR] Failed to listen\n");
	        return -1;
	    }
	    puts("\n\n[INFO] Waiting for connections...");

	    readfds = masterfds;
        if((select(maxfds + 1, &readfds, NULL, NULL, NULL)) == -1) {
			fprintf(stderr, "[ERROR] Failed to select\n");
			return -1;
		}
		puts("[INFO] Completed select");

		for (i = 0; i <= maxfds; i++) {
			if (FD_ISSET(i, &readfds)) {
				if (i == sockfd) {
					init = 1;
					clen = sizeof(client_addr);
				    if ((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &clen)) == -1) {
				    	close(sockfd);
				    	close(newfd);
				    	fprintf(stderr, "[ERROR] Failed to accept\n");
						return -1;
				    }
				   
				    memset(buf, 0, MAXDATASIZE);
				    if (recv(newfd, buf, MAXDATASIZE, 0) == -1) {
				    	close(sockfd);
				    	close(newfd);
				    	fprintf(stderr, "[ERROR] Failed to receive\n");
						return -1;
				    }
				    printf("[INFO] Connected client's request: \n%s", buf);

					bzero((char *)&server_addr, sizeof(server_addr));
				    server_addr.sin_family = AF_INET;

					p1 = strstr(buf, "/");
					p2 = strstr(buf, " HTTP");
					len = p2 - p1;
					file_path = (char *)malloc(sizeof(char)*len+1);
					strncpy(file_path, p1, len);
					file_path[len] = '\0';
					file_name = malloc(strlen(file_path));
					strncpy(file_name, p1, len);
					//file_name = 
					printf("\n\nfilename1: %s\n\n",file_name);
					/*while (file_name != NULL) {
						if (strstr(file_name, "/") != NULL)
							break;
						//file_name = strtok(NULL, "/");
					}*/
					//file_name = strtok(file_name,"/");
					char file_name2[1024];
					url_mycode(file_name, file_name2);
					int i;
					for(i=0; file_name2[i]!='\0';i++)
						file_name[i] = file_name2[i];
					
					printf("\n\nfilename2: %s\n\n",file_name);

				    tmp = strstr(buf, "HOST:");
				    sscanf(tmp, "%s %s", ip, ip);
				    if ((host = gethostbyname(ip)) == NULL) {
						fprintf(stderr, "[ERROR] Failed to match hostname\n");
						return -1;
					}
					loc = Is_cached(ip, file_name);
					pass = loc < 0 ? 1 : Is_staled_data(loc);
					printf("loc: %d\n", loc);
				    if (pass) {
					    /* new data or staled data */
					    if (loc < 0)	
					    	puts("[INFO] This data is a new data");
					    else 
					    	puts("[INFO] This data is staled");

					    // 1. connect to server
						bcopy(host->h_addr, (char *)&server_addr.sin_addr, host->h_length);
						port = atoi("80");
						server_addr.sin_port = htons(port);

						if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
							fprintf(stderr, "[ERROR] Failed to create server socket\n");
							return -1;
						}
						if (connect(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
							fprintf(stderr, "[ERROR] Failed to connect to server\n");
							return -1;
						}

						printf("[INFO] Connected to remote server: ip - %s, port - %d\n", ip, port);
						memset(rbuf, 0, MAXDATASIZE);
					    
					    if (send(serverfd, buf, MAXDATASIZE, 0) < 0)
					    	fprintf(stderr, "[ERROR] Failed to send\n");
					    total_size = 0;
					    printf("\n\n\nfilename : %s\n\n\n",file_name);
					    remove(file_name);
					    fp = fopen(file_name,"a");
					    while (1) {
						    if ((numbytes = recv(serverfd, rbuf, MAXDATASIZE, 0)) < 0) {
						    	fprintf(stderr, "[ERROR] Failed to receive\n");
						    	fclose(fp);
						    	break;
						    } else if (numbytes == 0) {
						    	fclose(fp);
						    	break;
						    } else {
						    	if (init) {
									printf("[INFO] received data: \n%s", rbuf);
									init = 0;
								}
						    	total_size += numbytes;
						    	rbuf[numbytes] = '\0';
						    	//fputs(rbuf, fp);
						    	fwrite(rbuf, strlen(rbuf), 1, fp);
						    }
						}
						printf("total_size: %d\n", total_size);

					    // 2. save data at cache
					    create_cache(rbuf, ip, file_name, fp);
					    
					    // 3. send data to client
					    if((fp = fopen(file_name, "r")) == NULL) {
							fprintf(stderr, "Error: Can't find the file: %s\n", file_name);
							return -1;
						} else {
							puts("Completed opening file.");
						}
					    do {
					    	//puts("sending");
					    	if ((filesize = fread(sbuf, 1, MAXDATASIZE, fp)) < 0) {
					    		fprintf(stderr, "Error: Can't read the file: %s\n", file_name);
								break;
					    	}
					    	send(newfd, sbuf, filesize, 0);
					    	//printf("filesize: %d\n", filesize);

					    	if (filesize < MAXDATASIZE)		fclose(fp);
					    } while (filesize == MAXDATASIZE);
					    //send(newfd, rbuf, MAXDATASIZE, 0);
					    
					} else {
						/* cached data and not staled */
				    	puts("[INFO] This data is a cached data");
				    	// 1. find cache - move it to 0
				    	if (loc != 0)
				    		replace_by_LRU(loc);
						
						// 2. send it to client
						if((fp = fopen(file_name, "r")) == NULL) {
							fprintf(stderr, "Error: Can't find the file: %s\n", file_name);
							return -1;
						} else {
							puts("Completed opening file.");
							do {
						    	if ((filesize = fread(sbuf, 1, MAXDATASIZE, fp)) < 0) {
						    		fprintf(stderr, "Error: Can't read the file: %s\n", file_name);
									return -1;
						    	}
						    	send(newfd, sbuf, filesize, 0);
						    	if (filesize < MAXDATASIZE)		fclose(fp);
						    } while (filesize == MAXDATASIZE);
				    	//send(newfd, cache_list[0].data, sizeof(cache_list[0].data), 0);
						}
				    }
				} else {
					FD_CLR(i, &masterfds);
					close(i);
				}
			} // FD_ISSET
		} // for
	} // while(1)

	return 0;
}

int Is_cached(char *ip, char *name) 
{
	int i;
	for (i = 0; i <= current_cache_size; i++) {
		if (!strcmp(ip, cache_list[i].ip) && !strcmp(name, cache_list[i].file_name)) 
			return i; 
	}
	return -1;
}

int Is_staled_data(int loc) {
	time_t now;
	time(&now);
	if (difftime(mktime(&cache_list[loc].expire_time), (long) now) > 0)
		return 0;
	else 
		return 1;
}

void replace_by_LRU(int loc) 
{
	int i;
	cache_list[0] = cache_list[loc];
	for (i = 1; i < loc; i++) {
		cache_list[i] = cache_list[i-1];
	}
}

void create_cache(char *buf, char *ip, char *name, FILE *fp) 
{
	struct cache new_cache;
	int i;
	char *tmp, tmp_buf[MAXDATASIZE];
	time_t now, tmp_time;
	
	memset(tmp_buf, MAXDATASIZE, 0);
    tmp = strstr(buf, "Expires:");
    if (tmp) {
    	strncpy(tmp_buf, tmp+14, 24);
    	printf("\n[Debug_mode] Expries:%s\n", tmp_buf);
    	strptime(tmp_buf, "%d %b %Y %H:%M:%S", &new_cache.expire_time);
    	tmp_time = mktime(&new_cache.expire_time);
    	printf("\n[Debug_mode] new_cache.expire_time: %ld\n", (long) tmp_time);
    }
    tmp = strstr(buf, "Date:");
    if (tmp) {
    	strncpy(tmp_buf, tmp+11, 24);
    	printf("\n[Debug_mode] Date:%s\n", tmp_buf);
    	strptime(tmp_buf, "%d %b %Y %H:%M:%S", &new_cache.server_time);
    	tmp_time = mktime(&new_cache.server_time);
    	printf("\n[Debug_mode] new_cache.server_time: %ld\n", (long) tmp_time);
    }
    tmp = strstr(buf, "Last-Modified:");
    if (tmp) {
    	strncpy(tmp_buf, tmp+20, 24);
    	printf("\n[Debug_mode] Last-modified:%s\n", tmp_buf);
    	strptime(tmp_buf, "%d %b %Y %H:%M:%S", &new_cache.last_modified_time);
    	tmp_time = mktime(&new_cache.last_modified_time);
    	printf("\n[Debug_mode] new_cache.last_modified_time: %ld\n", (long) tmp_time);//strptime(tmp_buf, &new_cache.last_modified_time);
    }
    time(&now);
    new_cache.current_time_stamp = now;
    printf("\n[Debug_mode] current_time_stamp: %ld\n", (long) new_cache.current_time_stamp);


    strcpy(new_cache.data, buf);
    strcpy(new_cache.ip, ip);
    strcpy(new_cache.file_name, name);
    new_cache.file = fp;

    if (current_cache_size == MAXCACHESIZE) {
    	remove(cache_list[i].file_name);
    	for (i = 0; i < MAXCACHESIZE - 1; i ++) 
    		cache_list[i] = cache_list[i + 1];
    	cache_list[i] = new_cache;
    } else {
    	cache_list[current_cache_size++] = new_cache;
    }

}
