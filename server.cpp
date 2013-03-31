#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

#define BACKLOG 10     // max pocet spojeni ve fronte

enum errNrs{
	EOK = 0,
	EPARAMS,
	ERR_GADDINFO,
	ERR_BIND,
	ERR_SEND,
	EUNKNOWN
};

typedef struct Params {
	bool login;
	bool uid;
	bool gid;
	bool name;
	bool homeDir;
	bool shell;
	string hostname;
	string portNr;
	string findLogin;
	string findUid;
	int error;
}TParams;

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

string parsePasswd(TParams *request){
	FILE *passwd;
	string chosen;
	char buffer[2048];
	size_t pos, star = 0;
	string pom, pomPiece, line, info;
	
	passwd = fopen("/etc/passwd", "r");
	while(getline() != 0){
		if(request->findLogin.length() > 0 ){
			line.append(buffer);
			pos = line.find(":");
			if(pos != string::npos){
				info = line.substr(start, pos);
			}
			pom = request.findLogin;
			while(pom.length() > 0){
				pos = pom.find(" ");
				if(pos != string::npos){
					pomPiece = pom.substr(start, pos);
					pom.erase(start, pos);
				}
				if(info == pomPiece){
					//POUZIJU SPLIT ZE STACK OVERFLOW (includy uz jsou) a do stringu nahazu jednotlivy kousky /etc/passwd, podle params...
				}
			}
		}
		else if(request->findUid.length() > 0){
		}
		else{
			// error
			return chosen;
		}
	}
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
	 TParams clientReq;
	 
	 memset(&buffer, 0, sizeof(buffer));	
     memset(&hints, 0, sizeof(hints));          
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;
     
     if(getaddrinfo(NULL, portNr, &hints, &res) != 0){
		return ERR_GADDINFO;
     }

    // navazani na volny port
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
        return ERR_BIND;
    }

    freeaddrinfo(res);

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
            if(recv(sockFd, clientReq, sizeof(clientReq), MSG_WAITALL) <= 0)
				perror("recv");
			string result = parsePasswd(&clientReq);
            if (send(childFd, result, sizeof(result), 0) == -1)
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

