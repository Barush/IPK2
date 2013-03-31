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

string helpmsg = ("\nProgram server\n"
			"Barbora Skrivankova, xskriv01@stud.fit.vutbr.cz\n"
			"Pri prichozim pozadavku odesle zpet pozadovana data z /etc/passwd\n"
			"Pouziti:\n\n ./server -p <port number>\n");

enum errNrs{
	EOK = 0,
	EPARAMS,
	ERR_GADDINFO,
	ERR_BIND,
	ERR_SEND,
	ERR_REQ,
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

vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


vector<string> split(const string &s, char delim) {
    vector<string> elems;
    return split(s, delim, elems);
}

void fillItIn(TParams *request, string *chosen, vector<string> cell){
	if(request->login){
		(*chosen).append(cell[0]);
		(*chosen).append(" ");
	}
	if(request->uid){
		(*chosen).append(cell[2]);
		(*chosen).append(" ");
	}
	if(request->gid){
		(*chosen).append(cell[3]);
		(*chosen).append(" ");
	}
	if(request->name){
		(*chosen).append(cell[4]);
		(*chosen).append(" ");
	}
	if(request->homeDir){
		(*chosen).append(cell[5]);
		(*chosen).append(" ");
	}
	if(request->shell){
		(*chosen).append(cell[6]);
		(*chosen).append(" ");
	}
	(*chosen).append("\n");
	return;
}

string parsePasswd(TParams *request){
	fstream passwd;
	string chosen, line;
	char buffer[2048];
	vector<string> cell, wanted;
	
	passwd.open("/etc/passwd", fstream::in);
	while((passwd.getline(buffer, 2048)) != 0){
		line.append(buffer);
		cell = split(line, ':');
		if(request->findLogin.length() > 0 ){
			wanted = split(request->findLogin, ' ');
			for(unsigned i = 0; i < wanted.size(); i++){
				if(wanted[i] == cell[0]){
					fillItIn(request, &chosen, cell);
				}
			}
		}
		else if(request->findUid.length() > 0){
			wanted = split(request->findUid, ' ');
			for(unsigned i = 0; i < wanted.size(); i++){
				if(wanted[i] == cell[2]){
					fillItIn(request, &chosen, cell);				
				}
			}
		}
		else{
			request->error = ERR_REQ;
			return chosen;
		}
	}
	return chosen;
}

int serverFunc(string portNr){
	 int yes = 1;
	 struct addrinfo hints, *res, *p;
	 int sockFd, childFd;
	 struct sockaddr_storage their_addr; // connector's address information
     socklen_t sin_size;
     char s[INET6_ADDRSTRLEN];
     struct sigaction sa;
	 char buffer[1024];
	 string response;
	 response.clear();
	 void *clientReqVoid = NULL;
	 TParams *clientReq = NULL;
	 
	 memset(&buffer, 0, sizeof(buffer));	
     memset(&hints, 0, sizeof(hints));          
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;
     
     if(getaddrinfo(NULL, portNr.c_str(), &hints, &res) != 0){
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
            if(recv(sockFd, clientReqVoid, sizeof(clientReq), MSG_WAITALL) <= 0)
				perror("recv");
			clientReq = (TParams*) clientReqVoid;
			string result = parsePasswd(clientReq);
            if (send(childFd, result.data(), sizeof(result), 0) == -1)
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
	string portNr;
	if(argc != 3){
		cout << "Wrong params!" << endl;
		cout << helpmsg;
		return 1;
	}
	else if(strcmp(argv[1], "-p") == 0){
		portNr = argv[2];
	}
	else {
		cout << "Wrong params!" << endl;
		cout << helpmsg;
		return 1;
	}
	
	serverFunc(portNr);
	return 0;
}

