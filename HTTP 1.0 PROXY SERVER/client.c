/* 
ECEN602: Computer Networks
HW3 programming assignment: HTTP1.0 client GET request
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
#include <unistd.h>

#define MAXDATASIZE 1024

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
        return &(((struct sockaddr_in*)sa)->sin_addr);

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc, char const *argv[])
{
	int rv, numbytes,i;
	int sockfd;
	int line_count;
	int init = 1;
	int total_size = 0;
	char *url, *file_path, *tmp_url, *file_name, *tmp;
	char buf[MAXDATASIZE], line[1024];
	char s[INET6_ADDRSTRLEN];
	struct addrinfo hints, *servinfo, *p;
	FILE *fp, *tmp_fp;

	if (argc != 4) {
		fprintf(stderr,"Usage: ./client <proxy_ip> <proxy_port> <url>\n");
        return -1;
	}

	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "[INFO] getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

	url = argv[3];
	printf("[Debug_mode] URL: %s\n", url);

	
	// loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            fprintf(stderr, "[ERROR] Cannot create socket\n");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            fprintf(stderr, "[ERROR] Cannot connect to server\n");
            inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
            fprintf(stderr, "[Debug_mode] p->ai_addr: %s, \t p->ai_addrlen: %d\n", s, p->ai_addrlen);
            continue;
        }
        break;
    }
	
	if (p == NULL) 
        return -1;

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("[INFO] connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
	
	// pack GET request
	if (strstr(url, "http://") != NULL) 
		url = url + 7;
	//file_name = url;
	file_name = malloc(strlen(url));
	for(i = 0; url[i] != '\0'; i++){
		file_name[i] = url[i];
	}
	file_name[i] = '\0';
	printf("\n\nURL: %s\n\n",url);
	for(i=0;file_name[i]!='\0';i++) {
		if(file_name[i]=='/')
			file_name[i]='-';
	}	
	//char *file_name2;
	//url_encode(url, file_name);
		//file_name = file_name2;
printf("\n\nfilename1: %s", file_name);

	if ((tmp_url = strstr(url, "/")) == NULL)
		sprintf(buf, "GET / HTTP/1.0\nHOST:%s\n\n", url);
	else {
		file_path = malloc(strlen(tmp_url));
		strncpy(file_path, tmp_url, strlen(tmp_url));
		tmp_url = strtok(url, "/");
		sprintf(buf, "GET %s HTTP/1.0\nHOST: %s\n\n", file_path, tmp_url);
		//file_name = malloc(strlen(url));
		//rncpy(file_name, url, strlen(url));
		//file_name = url;
		/*file_name = strtok(file_path, "/");
		while (file_name != NULL) {
			if (strstr(file_name, ".txt") != NULL)
				break;
			file_name = strtok(NULL, "/");
		}*/
		
	}

	// send GET request
	if (send(sockfd, buf, MAXDATASIZE, 0) == -1) {
		fprintf(stderr, "[ERROR] Failed to send HTTP header\n");
		return -1;
	} 

	printf("[INFO] Sent Get request: \n%s", buf);

	// receive message
	memset(buf, 0, MAXDATASIZE);
	remove(file_name);
	printf("\n\nfilename: %s\n",file_name);

	int c=-1;

        fp = fopen(file_name, "a");
	while (1) {
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) < 0) {
			fprintf(stderr, "[ERROR] Failed to receive message\n");
			fclose(fp);
			return -1;
		} else if (numbytes < MAXDATASIZE) {
			//printf("numbytes: %d\n", numbytes);
			total_size += numbytes;
	    	buf[numbytes] = '\0';

		printf("\n\nIn elseif %s\n\n",buf);

	    	fputs(buf, fp);
			break;
		} else {
			//if (init) {
			//	printf("[INFO] received data: \n%s\n", buf);
			//	init = 0;
			//}
			//printf("numbytes: %d\n", numbytes);
			total_size += numbytes;
	    	buf[numbytes] = '\0';
		printf("\n\nIn else %s\n\n",buf);
		if(c==-1)
		{
						

				char *start=strstr(buf, "\r\n\r\n");
				c=0;
				fputs(start+4, fp);
				continue;
		   
		  
		  }
	    	fputs(buf, fp);
		} 
	}	
	fclose(fp);
	/*fp = fopen(file_name, "r");
	tmp_fp = fopen("tmp.txt", "w+");
	while (fgets(line, sizeof(line), fp) != NULL ){
	    if (line_count > 12){
	        fprintf(tmp_fp, "%s", line);
	    }
	    line_count++;
	}
	remove(file_name);
	rename("tmp.txt", file_name);
    */
	printf("total_size: %d\n", total_size);
	//printf("[INFO] Received message: \n%s", buf);

	return 0;
}
