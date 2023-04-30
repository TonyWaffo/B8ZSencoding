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

void b8sz(char*);
bool eightZeros(char* , int);
void transform(char* , int , int );

bool approval=false;
void *thread_recv_msg(void *sock)
{
	int their_sock = *((int *)sock);
	char msg[500];
	char last_letters[8];
	int len;
	while((len = recv(their_sock, msg, 500, 0)) >= 0) {
		msg[len] = '\0';
		// at the reception, it checks the last 3 letters of the response. yes= approval
		strncpy(last_letters,msg+strlen(msg)-7,7);
		last_letters[7] = '\0';
		if(strcmp(last_letters,"confirm")==0 && approval==false){
		   approval=true;
		   fputs(msg,stdout);
		   printf("\n\n");
		}
		memset(msg,'\0',sizeof(msg));
	}
}
int main(int argc, char *argv[])
{
	struct sockaddr_in their_addr;
	int client_socket;
	int portno;
	pthread_t thread_recv_ID;
	char msg[500];
	char encodedMsg[500];
	char username[100];
	char res[600];
	char authorize[50]="approval request";// string that hold the authorization : ready to accept ?
	char ip[INET_ADDRSTRLEN];
	int len;

	if(argc > 3) {
		printf("too many arguments");
		exit(1);
	}
	portno = atoi(argv[2]);
	strcpy(username,argv[1]);

	WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

	client_socket = socket(AF_INET,SOCK_STREAM,0);
	memset(their_addr.sin_zero,'\0', sizeof(their_addr.sin_zero));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(portno);
	their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(client_socket,(struct sockaddr *)&their_addr, sizeof(their_addr)) < 0) {
		perror("connection not esatablished");
		exit(1);
	}
	getnameinfo((struct sockaddr *)&their_addr, sizeof(their_addr), ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
	printf("connected to %s, start chatting\n",ip);

	// send the username of the new client, so it can help server replying to the right client
	send(client_socket,username,strlen(username),0);
	printf("\n username has been added, name is %s \n\n", username);
	printf("\n *****You can write the bits with 0 and 1 without putting any space, even at the beginning. Exemple:1000000000 ***** \n\n");
	
	bool checker=true;// this boolean variable allow to access the loop even tho there is no current message entered (because the loop is still processing data)
	fgets(msg,500,stdin);
	pthread_create(&thread_recv_ID, NULL,thread_recv_msg,&client_socket); /* Create server receiver thread */
	
	while(strlen(msg)>0 || checker==true) {
		if (approval==false){
			b8sz(msg);// call the function that will encode the given bit
			strcpy(encodedMsg,msg);
			// sending approval request to server before sending the encoded data and will receive data before the next loop
			strcpy(res,username);
			strcat(res,":");
			strcat(res,username);
			strcat(res,"-");
			strcat(res,authorize);
			len = send(client_socket,res,strlen(res),0);
			if(len < 0) {
				perror("authorization request not sent");
				exit(1);
			}
			checker=true;
			approval=true;
			memset(msg,'\0', sizeof(msg));
			memset(res,'\0', sizeof(res));
		}


		//then if the request was approved by the server, it will send the encoded message
		if(approval==true){
			strcpy(res,username);
			strcat(res,":");
			strcat(res,username);
			strcat(res,"-");
			strcat(res,encodedMsg);
			len = send(client_socket,res,strlen(res),0);
			if(len < 0) {
				perror("message not sent");
				exit(1);
			}
			memset(msg,'\0', sizeof(msg)); //memset() is used to fill a block of memory with a particular value.
			memset(encodedMsg,'\0', sizeof(encodedMsg));
			memset(res,'\0', sizeof(res));
			checker=false;
			approval=false;// rmake sure to remove any approval after he sends the message 
			fgets(msg,500,stdin);
		}

	}
	pthread_join(thread_recv_ID, NULL);//wait for thread termination.
	closesocket(client_socket);
    WSACleanup();

    return 0;
}


//encode bits with B8SZ
void b8sz(char* msg){
	char encodedMsg[100];
	int i;
	int parity=0;  // parity 0 represents the previous impulsion was negative
    for( i = 0; msg[i] != '\0'; i++) { // this block transfers the bits to the AMI
		if(msg[i]=='0'){
			if(eightZeros(msg,i)){
				transform(msg,i,parity);// if there is 8 consecutives zero, this function will transform it accordingly
				i+=7;
			}else{
				msg[i]='0';
			}
		}else if(msg[i]=='1'){
			if(parity%2==0){
				msg[i]='+';
			}else{
				msg[i]='-';
			}
			parity++;
		}else{
			msg[i] = '\0';
		}
    }
	msg[i] = '\0';
	printf("Encoded message: %s\n\n", msg);
}


//this code checks if there is 8 consecutive zeros
bool eightZeros(char* msg, int i){
	for(int j=i; j<i+8;j++){
		if(msg[j]!='0'){
			return false;
		}
	}
	return true;
}

//transformation for 8 consecutive zeros 
void transform(char* encodedMsg, int i, int parity){
	if(parity%2==0){  
		encodedMsg[i]='0';
		encodedMsg[i+1]='0';
		encodedMsg[i+2]='0';
		encodedMsg[i+3]='-';
		encodedMsg[i+4]='+';
		encodedMsg[i+5]='0';
		encodedMsg[i+6]='+';
		encodedMsg[i+7]='-';
	}else{
		encodedMsg[i]='0';
		encodedMsg[i+1]='0';
		encodedMsg[i+2]='0';
		encodedMsg[i+3]='+';
		encodedMsg[i+4]='-';
		encodedMsg[i+5]='0';
		encodedMsg[i+6]='-';
		encodedMsg[i+7]='+';
	}
}