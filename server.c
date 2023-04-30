#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#pragma comment(lib, "ws2_32.lib")
void decode(char*);

struct client_info {
	int sockno;
	char ip[INET_ADDRSTRLEN];
};

// creation of the structure containing the socket number and username
struct info_client{
	char name[100];
	int sock_nb;
};
struct info_client clients[100];
int n = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void send_to_all(char *msg,int curr,char *recept)
{
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++) {
		if( strcmp(clients[i].name, recept) == 0 ) {
			if(send(clients[i].sock_nb,msg,strlen(msg),0) < 0) {
				perror("sending failure");
				continue;
			}else{
				printf("\n%s\n",msg);
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}
void * thread_recv_msg(void *sock)
{
	struct client_info cl = *((struct client_info *)sock);
	char msg[500];
	char receiver[100];
	int len;
	char res[600];
	char last_letters[8];
	int i;
	int j;

	// Receives the message, help extract the potential receiver of the message (if any), and the encoded data
	while((len = recv(cl.sockno,msg,500,0)) > 0) {
		msg[len] = '\0';
        char *ptr1, *ptr2;
        ptr1 = strstr(msg, ":");

        if (ptr1 != NULL) {
            ptr1 += 1;
            ptr2 = strstr(ptr1, "-");
            if (ptr2 != NULL) {
                int word_len = ptr2 - ptr1;
                strncpy(receiver, ptr1, word_len);
                receiver[word_len] = '\0';
                strcpy(ptr1, ptr2+1);
            }
        }

		//Check the received message to confirm if it's an approval request or an encoded data
		strcpy(last_letters,msg+strlen(msg)-7);
		last_letters[7]='\0';
		if(strcmp(last_letters,"request")==0){
			strcpy(msg+strlen(msg)-7, "confirm"); // approve the request by putting confirm at the end of the message
		}else{
			decode(msg); //decode the encoded data
			strcpy(msg,receiver);
			strcat(msg,":message received successfully");
		}

		//send the messsage back to the client (if applicable)
		send_to_all(msg, cl.sockno,receiver);
		memset(msg,'\0',sizeof(msg));
	}
	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n", cl.ip);
	for(i = 0; i < n; i++) {
		if(clients[i].sock_nb == cl.sockno) {
			j = i;
			while(j < n-1) {
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	n--;
	pthread_mutex_unlock(&mutex);
}
int main(int argc,char *argv[])
{
	struct sockaddr_in my_addr,their_addr;
	int server_socket;
	int client_socket;
	socklen_t their_addr_size;
	int portno;
	char username[100];
	char authorize[50];
	int ulen;
	int auhtorizeLen;
	pthread_t  thread_recv_ID; /* Thread ID from pthread_create()*/

	
	char msg[500];
	int len;
	struct client_info cl;
	char ip[INET_ADDRSTRLEN];;
	;
	if(argc > 2) {
		printf("too many arguments");
		exit(1);
	}
	portno = atoi(argv[1]);
	 WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

	server_socket = socket(AF_INET,SOCK_STREAM,0);
	memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(portno);
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	
	their_addr_size = sizeof(their_addr);

	if(bind(server_socket,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
		perror("binding unsuccessful");
		exit(1);
	}

	if(listen(server_socket, 5) != 0) {
		perror("listening unsuccessful");
		exit(1);
	}

	while(1) {
		if((client_socket = accept(server_socket,(struct sockaddr *)&their_addr,&their_addr_size)) < 0) {
			perror("accept unsuccessful");
			exit(1);
		}
		pthread_mutex_lock(&mutex);
		getnameinfo((struct sockaddr *)&their_addr, sizeof(their_addr), ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
		printf("%s connected\n",ip);
		printf("New Client connected from port# %d and IP %s\n", ntohs(their_addr.sin_port), inet_ntoa(their_addr.sin_addr));
		cl.sockno = client_socket;
		strcpy(cl.ip,ip);
		//assigning socket to client
		clients[n].sock_nb = client_socket;
		//username received
		ulen=recv(client_socket, username, sizeof(username),0);
		username[ulen] = '\0';
		// socket number and username into an array of structure
		strcpy(clients[n].name, username);
		n++;
		pthread_create(&thread_recv_ID, NULL, thread_recv_msg, &cl); /* Create server receiver thread */
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

// this block of code retrieve the encoded data from the message and convert it into his original format which is a serie of bits
void decode(char* rawMsg){
	//retrieve the encoded data from the received message
	char msg[100];
	char *start;
	char *decodedMsg;
    start = strchr(rawMsg, ':');
    if (start != NULL) {
        decodedMsg= start+1;
    }
	strcpy(msg, decodedMsg);
	printf("\n\nReceived data: %s\n", msg);
	printf("\n");

	// Converts to the origiginal format of bit
	for( int i = 0; msg[i] != '\0'; i++) {
		if((msg[i]=='0' && msg[i+1]=='0' && msg[i+2]=='0' && msg[i+3]=='+' && msg[i+4]=='-' && msg[i+5]=='0' && msg[i+6]=='-' && msg[i+7]=='+') || (msg[i]=='0' && msg[i+1]=='0' && msg[i+2]=='0' && msg[i+3]=='-' && msg[i+4]=='+' && msg[i+5]=='0' && msg[i+6]=='+' && msg[i+7]=='-') ){
			for( int j=i; j<i+8;j++){
				msg[j]='0'; 
			}
			i=i+7;
		}else if(msg[i]=='+' || msg[i]=='-' ){
			msg[i]='1';
		}else{
			msg[i]='0';
		}
    }
	printf("Decoded message: %s\n", msg);
	printf("\n");
}