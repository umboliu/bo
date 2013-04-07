/***************************************************************************
 *  EECS 489 PA 2
 *  Bo Liu
 *  umboliu@umich.edu
 *  Xi Han
 *  hanx@umich.edu
 *
 *  This file creates a proxy that listens to client through specified port
 *  and transfer the request/response message between client and server.
 *  
 *  proxy.cpp
 *  	usage: ./proxy <port>
 *
 ***************************************************************************/

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <pthread.h>

const int BUFFER_SIZE = 200000 ;
const int SBUFFER_SIZE = 2000;

using namespace std;

/*Convert lowercase letter into uppercase letter*/
void upper_case_conversion(char* q,int length)
{	int i = 1;
	int dist;
	dist = (int) ('a' - 'A');
	while( i <= length ){
		if( 'a'<=*q && *q<='z' ){
		     *q =(char) (*q - dist);
		}
		q++;
		i++;
	}
}

/*Find the first slash*/
char* find_slash(char* pslash)
{	while( *pslash != '\0'){
		if ( *pslash == '/' ) return pslash;
		else pslash++;
	}
	return pslash;
}

/*Find colon*/
char* find_colon(char* pcolon)
{	while(*pcolon != '\0'){
		if( *pcolon == ':' )  return pcolon;
		else pcolon++;	
	}
	return pcolon;
}

/*Work function for each thread*/
void* work_function(void* arg)
{
	int server_sock,msgsock;
	int buffer_length = 0,length;
	int i=0,j=0;
	bool copying = false;        
	
	char client_buffer[BUFFER_SIZE],server_buffer[SBUFFER_SIZE];
	char str[5][4056];	
        char request[5][4056];
        char url[4056],port[4056];
	char *p,*pslash,*pcolon;
	
	struct sockaddr_in server;
	struct hostent *hp;
	
	char error400[] = "Bad Request\r\n"; 
	char error501[] = "Not Implemented\r\n";
	int err400_length = strlen(error400);
        int err501_length = strlen(error501);

	memset(client_buffer,0,BUFFER_SIZE);
	memset(url,0,4056);
        memset(port,0,4056);
        for (int a=0;a<5;a++){          
                memset(str[a],0,4056);
                memset(request[a],0,4056);
        }

	msgsock = * ((int*)arg);

	/*Read clients's request,break when receives "\r\n\r\n" or "\n\n"*/
	while(true){
	  read(msgsock,client_buffer + buffer_length,BUFFER_SIZE-buffer_length);
	  buffer_length = strlen(client_buffer);
	 
	  if ((client_buffer[buffer_length-1] == '\n' && client_buffer[buffer_length-2] == '\r' &&
	       client_buffer[buffer_length-3] == '\n' && client_buffer[buffer_length-4] == '\r')||
	      (client_buffer[buffer_length-1] == '\n' && client_buffer[buffer_length-2] == '\n')
	     ){
	  		break;
	  }	
	}	
	
	/*Bypass \r,\n, and SPACE and Store the first 5 words of the request message in arrays*/
	p = client_buffer;
	while ( i<=4 && ( *p!='\r' || *(p+1)!='\n'|| *(p+2)!='\r' || *(p+3)!='\n') && (*p!='\n'|| *(p+1)!='\n') ){
		if( *p == ' ' || *p =='\r' || *p=='\n'){
		     j=0;
		     if(copying == true){
			copying = false;i++;
		     }
		     p++;
		}
		else{
	 	     str[i][j] = *p;
		     copying = true;
		     j++;
		     p++;
		}
	} 
	
	/*Convert "host:" and "HTTP://" into upper case*/
        if ( str[1][0] =='/' ){
	     upper_case_conversion( (char*) str[3],5 );/*HOST:*/	
	}
	else{
	     upper_case_conversion ( (char*) str[1],7);/*HTTP:// */
	}	

	/*Parse the client's request*/
	/*----------HTTP/0.9 and HTTP/1.1 are changed into HTTP/1.0, other versions are not allowed--------*/
	if(strcmp(str[2],"HTTP/1.0") != 0){
		if(strcmp(str[2],"HTTP/0.9")==0 || strcmp(str[2],"HTTP/1.1")==0){
			strcpy(str[2],"HTTP/1.0");
		} 
		else{
		        write(msgsock,error400,err400_length);
			close(msgsock);
			delete (int*) arg;
			return 0;
		}
	}

	/*Parse absolute path and relative path*/
	if ( strcmp(str[0],"GET")== 0 ){
    	/*---------------------------ABSOLUTE PATH-------------------------------*/
	    	if( strncmp(str[1],"HTTP://",7)==0){
			//cout<<"err is "<<str[2]<<endl;
			pslash = find_slash(str[1]+7);
			pcolon = find_colon(str[1]+7);
			if( *pslash!='\0' ){
				/*Port number is not specified*/
				if(*pcolon == '\0' || pcolon > pslash){
	    				strncpy( url,str[1]+7,(int)(pslash-str[1]-7) );
	    				length = strlen(str[1]);
	    				strncpy( request[1],str[1]+7+strlen(url),length-7-strlen(url));
				}
				/*Port number is specified*/
				else {
					strncpy( url,str[1]+7,(int)(pcolon-str[1]-7) );
					strncpy( port,str[1]+8+strlen(url),(int)(pslash-str[1]-8-strlen(url)));
					length = strlen(str[1]);
					strncpy(request[1],str[1]+8+strlen(url)+strlen(port),length-8-strlen(url)-strlen(port));
				}
			}
			/*Missing '/' in the end causes error 400*/
			else{
	    			write(msgsock,error400,err400_length);
	    			close(msgsock);
			        delete (int*) arg;
				return 0;
			}
    		}

    	/*---------------------------RELATIVE PATH-------------------------------*/
    		else if( str[1][0]=='/' ){
		        /*header field name "host:" and header value are separated by SPACE*/
			 if( strcmp(str[3],"HOST:")==0 ){
		  	     pslash = find_slash(str[4]);
		             pcolon = find_colon(str[4]);
			     /*Port number is not specified*/
         		     if( *pcolon == '\0'|| pcolon > pslash){
                        	 strncpy(url,str[4],(int) (pslash -str[4]));
                           	 strcpy(request[1],str[1]);
			     }
			    /*Port number is specified*/
			     else{
				   strncpy(url,str[4],(int) (pcolon-str[4]));
				   strncpy(port,str[4]+strlen(url)+1,(int) (pslash-str[4]-strlen(url)-1));
			     	   strcpy(request[1],str[1]);
				 }			
			 } 
			/*header field name "host:" and header value are not separated by SPACE*/ 
			 else if ( strncmp(str[3],"HOST:",5)== 0 ){			
				   pslash = find_slash(str[3]);
				   pcolon = find_colon(str[3]+5);
				   /*Port number is not specified*/
				   if( *pcolon == '\0'|| pcolon > pslash){
                          	       strncpy( url,str[3]+5,(int) (pslash-str[3]-5) );
                            	       strcpy( request[1],str[1]);
				   }
				  /*Port number is specified*/
				   else{
					 strncpy(url,str[3]+5,(int) (pcolon-str[3]-5));
					 strncpy(port,str[3]+5+strlen(url)+1,(int) (pslash-str[3]-5-strlen(url)-1));
					 strcpy(request[1],str[1]);
				   }
			      }
			/*header filed name is not "host:" in relative path form,return error 400 */
			      else{
			   	    write(msgsock,error400,err400_length);
			   	    close(msgsock);
			            delete (int*) arg;
				    return 0;
			      }
		       }
		 /*Missing '/' in relative path, return error400*/   
		     else{
		 	   write(msgsock,error400,err400_length);
			   close(msgsock);
			   delete (int*) arg;
			   return 0;
		     }
	}
	/*Method is not "GET"(case-sesitive), return error 501*/
	else{
	     write(msgsock,error501,err501_length);
	     close(msgsock);
             delete (int*) arg;
	     return 0;
        }  

	strcpy(request[3],url);	

	/*-----------------Connect to server and tramsmit request and response between server and client-----------------------*/
	/*Create a socket to connect to server*/
	server_sock = socket(AF_INET,SOCK_STREAM,0);
        
	/*Set up server configuration with which the client connects*/
	server.sin_family = AF_INET;
	if( strlen(port)!= 0 ){
		server.sin_port = htons(atoi(port));
	}
	else {	
		server.sin_port = htons(80);
	}
	
	/*Return "Page not found" when URL is not valid */
	if ( (hp = gethostbyname(url)) == (void*) NULL ){
		write(msgsock,"Page not found",14);
		close(msgsock);
		delete (int*) arg;
		return 0;
	}
	
	memcpy((char*) &server.sin_addr, (char*) hp->h_addr, hp->h_length);
	
	/*Connect to server*/
	connect(server_sock,(struct sockaddr*) &server,sizeof server);

	/*Write request to server*/
	strcpy(request[0],"GET ");
	strcpy(request[2]," HTTP/1.0\r\nHost: ");
	strcpy(request[4],"\r\n\r\n");
		
	for (int a=0; a<5; a++){
	     write(server_sock,request[a],strlen(request[a]));
	}	
	
	/*Read reponse from server and forward them to client until server finishes transmission*/
	int done_read,done_write;
	while(true){
	      /*Read response from server*/
	      memset(server_buffer,0,SBUFFER_SIZE);
	      done_read = read(server_sock,server_buffer,SBUFFER_SIZE);
              
	      /*Write response to client*/
	      if( done_read == 0 ){
		  /*Close message socket when read is done*/
		  close(msgsock); break;
	      }
	      else{
	            done_write = write(msgsock,server_buffer,done_read);
	      }	      
	}
	
	delete (int*) arg;
	return 0;
}	


int main(int argc, char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "ERROR, no port provided\n");
                exit(1);
        }
	
        int proxy_sock;
        struct sockaddr_in proxy;
	
        /*Create a socket*/
        proxy_sock = socket(AF_INET,SOCK_STREAM,0);

        /*Proxy set up*/
        proxy.sin_family = AF_INET;
        proxy.sin_addr.s_addr = INADDR_ANY;
        proxy.sin_port = htons(atoi(argv[1]));

        /*Bind proxy's socket*/
        if(bind(proxy_sock,(struct sockaddr *)&proxy, sizeof proxy) == -1){
		exit(0);
        }

        /*Set maximum concurrent clients*/
        listen(proxy_sock,10);
	signal(SIGPIPE,SIG_IGN);

	/*Accept request from clients*/
        while(true){
	      int* msgsock = new int;
	      *msgsock = accept(proxy_sock,(struct sockaddr*) NULL, (socklen_t*) NULL);
	      if (*msgsock== -1){
	     	  cout<<"msgsock is -1"<<endl;
              }      
	      else{/*Create a thread for accepted client*/
		   pthread_t tid;
		   int err = pthread_create(&tid,NULL,work_function,msgsock );
		   if(err!=0){
			cout<<"unable to create thread"<<endl;	
		   }
	      }	
	}
	return 0;
}


