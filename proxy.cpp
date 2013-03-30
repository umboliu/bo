/******************************************
 *  EECS 489 PA 1
 *  Bo Liu
 *  umboliu@umich.edu
 *
 *  This file creates a client that sends message to server through
 *  the specified port, the end of the message is maked by hitting 
 *  <ENTER> twice or encounters EOF. 
 *  
 *  client.cpp
 *  	usage: ./client <host_name> <port>
 *             
 *  If you connect to local machine, use localhost as <host_name>
 *  You can use pipe to redirect the file content to the input, 
 *      e.g., ./client hostname port < data
 *
 *****************************************/

#include "header.h"
#include <iostream>
#include <string.h>

using namespace std;

void upper_case_conversion(char* q)
{	while( *q!='\r' && *(q+1)!='\n' ){
		if( 'a'<=*q && *q<='z' ){
		     *q =(char) (*q - 32);
		}
		q++;
	}
}



int main(int argc, char *argv[])
{
	if (argc < 2) {
                fprintf(stderr, "ERROR, no port provided\n");
                exit(1);
        }

	int proxy_sock,server_sock,msgsock;
	struct sockaddr_in proxy;
	struct sockaddr_in server;
	struct hostent *hp;
	char* host_name;
	char client_buffer[BUFFER_SIZE],server_buffer[BUFFER_SIZE];
	
	/*Create a socket*/
	proxy_sock = socket(AF_INET,SOCK_STREAM,0);
	
	/*Proxy set up*/
	proxy.sin_family = AF_INET;
        proxy.sin_addr.s_addr = INADDR_ANY;
        proxy.sin_port = htons(atoi(argv[1]));
	
	/*Bing socket to proxy*/
	if(bind(proxy_sock,(struct sockaddr *)&proxy, sizeof proxy) == -1){
               //error("Unable to bind the specified port");
               exit(0);
        }

	/*Set maximum concurrent clients*/
        listen(proxy_sock, MAX_CONCURRENT);

	//while(true)
        //{       wait for client requests here
        msgsock = accept(proxy_sock,(struct sockaddr*) NULL, (socklen_t*) NULL);//last two parameters?
        read(msgsock,client_buffer,BUFFER_SIZE);
        //cout<< client_buffer <<endl;
        
	char str[4][2048];
	char url[2048];
	char* p;
	bool copying = false;
	int length,i=0,j=0;
	int count = 0,count1 =0;	
	/*Extract from client buffer*/
	char* q;
	p = client_buffer;
	q = p;
	while ( *q != '\0'){
		count1++;
		q++;
	}
	cout<<"count1 is:"<< count1<<endl;
	upper_case_conversion(p);
	while ( *p!='\r' && *(p+1)!='\n' ){
		count++;
		if( *p == ' '|| *p == '\n' ){
		     j=0;
		     if(copying == true){
			copying = false;i++;
		     }
		}
		else{
		str[i][j] = *p;
		copying = true;
		j++;
		}
		p++;
	} 
	
	cout<<"count is:"<<count<<endl;
	cout<<"i is:"<<i<<endl; 
	cout<<"str[0] is:"<<str[0]<<endl;
	cout<<"str[1] is:"<<str[1]<<endl;
	cout<<"str[2] is:"<<str[2]<<endl;
	cout<<"str[3] is:"<<str[3]<<endl;
	/*
	if( strcmp(str[0],"GET")  == 0 ) cout<<"OK1\n";
	if( strncmp(str[1],"http://",7) == 0 ) cout<<"OK2\n";
	if( strncmp(str[2],"HTTP/1.0",8)  == 0 ) cout<<"OK3\n";
	*/
	for (int a=0;a<4;a++)
		memset(str[a],0,2048);
	memset(url,0,2048);
	if( i==2 ){
	    length = strlen(str[1]);
	    if( strcmp(str[0],"GET")==0 && strncmp(str[1],"HTTP://",7)==0 && 
		str[1][length-1] == '/' && strncmp(str[2],"HTTP/1.0",8)==0 ){   
	   		strncpy(url,str[1]+7,length-8);
			cout<<"url is:"<<url<<endl;
	    }			
	}
	else if ( i==3 ){
	     	  length = strlen(str[3]);
		  if(strcmp(str[0],"GET")==0 && str[1][0]=='/' &&
	             strncmp(str[2],"HTTP/1.0",8)==0 && strncmp(str[3],"HOST:",5)==0){
		     strncpy(url,str[3]+5,length-5);
		     strcpy(url+length,str[1]);
		     cout<<"url is:"<<url<<endl;	
		  } 
	}	
	
	/*-----------------------Parse and connect to server--------------------*/
	/*Create a socket to connect to server*/
	server_sock = socket(AF_INET,SOCK_STREAM,0);
        
	/*Set up server configuration with which the client connects*/
	server.sin_family = AF_INET;
	server.sin_port = htons(80);//need to be changed?
	//cout<<server.sin_port<<endl;
	hp = gethostbyname(url);
	memcpy((char*) &server.sin_addr, (char*) hp->h_addr, hp->h_length);
	
	/*Connect to server*/
	connect(server_sock,(struct sockaddr*) &server,sizeof server);

	/*Write request to server*/
	length = strlen(url);
	
	char request[4][2048];
	strcpy(request[0],"GET / HTTP/1.0\n");
	strcpy(request[1],"Host:");	
	strcpy(request[2],url);
	strcpy(request[3],"\r\n\r\n");
	/*
	cout<<request[0];
	cout<<request[1];
	cout<<request[2];
	cout<<request[3];
	cout<<request[4];
	cout<<request[5];	   
	*/	

	for (int a=0; a<4; a++){
	     write(server_sock,request[a],strlen(request[a]));
	}		
	
	read(server_sock,server_buffer,2000);
	cout<<server_buffer<<endl;

	
     
	//close(msgsock);
	
	
	
		
	return 0;
}	
