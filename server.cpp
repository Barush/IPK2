#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

#define BACKLOG 10     // max pocet spojeni ve fronte

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int serverFunc(string portNr){
	 int yes = 1;
	 struct addrinfo hints, *res, *p;
	 int sockFd, childFd;
	 size_t recvd;    
	 struct sockaddr_storage their_addr; // connector's address information
     socklen_t sin_size;
     char s[INET6_ADDRSTRLEN];
     struct sigaction sa;
	 char buffer[1024];
	 string response;
	 response.clear();
	 
	 memset(&buffer, 0, sizeof(buffer));	
     memset(&hints, 0, sizeof(hints));          
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;
     
     if(getaddrinfo(NULL, "65432", &hints, &res) != 0){
		//params->error = ERR_GADDINFO;
		return 1;
     }

    // loop through all the results and bind to the first we can
    for(p = res; p != NULL; p = p->ai_next) {
        if ((sockFd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockFd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockFd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(res); // all done with this structure

    if (listen(sockFd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        childFd = accept(sockFd, (struct sockaddr *)&their_addr, &sin_size);
        if (childFd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockFd); // child doesn't need the listener
            if (send(childFd, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(childFd);
            exit(0);
        }
        close(childFd);  // parent doesn't need this
    }
  return 0;
}

int main(int argc, char **argv)
{
}

