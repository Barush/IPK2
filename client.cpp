#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

using namespace std;

string helpmsg = ("\nProgram client\n"
			"Barbora Skrivankova, xskriv01@stud.fit.vutbr.cz\n"
			"Vypise z etc/passwd na serveru <hostname> naslouchajicim na \n"
			"portu <port_number> informace o uzivatelich se zadanym loginem/uid:\n"
			"Pouziti:\n\n ./client -h <hostname> -p <port_number> (-l <logins> | -u <uids>) [-LUGNHS]\n"
			"\t-L: login name\n\t-U: user id\n\t-G: group id\n\t"
			"-N: name\n\t-H: home directory\n\t-S: login shell\n");

enum errNrs{
	EOK = 0,
	EPARAMS,
	ERR_GADDINFO,
	ERR_SOCKET,
	ERR_CONNECT,
	ERR_SEND,
	EUNKNOWN
};

const char *errmsg[] = {
	"Vsechno probehlo ok.\n",
	"Chybne zadane parametry.\n",
	"Chyba pri getaddrinfo.\n",
	"Chyba pri ziskavani socket descriptoru.\n",
	"Chyba pri pripojovani.\n",
	"Chyba pri odesilani pozadavku.\n",
	"Nastala neznama nebo neocekavana chyba.\n"
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

void getParams(int argc, char **argv, TParams *params){
	int c;
	
        int option_index = 0;
	while (1)
    {	static struct option long_options[] =
        {   {"login",     no_argument,       0, 'L'},
            {"uid",  no_argument,       0, 'U'},
            {"gid",  no_argument,       0, 'G'},
            {"name",  no_argument,       0, 'N'},
            {"homeDir",  no_argument,       0, 'H'},
            {"shell",  no_argument,       0, 'S'},
            {"hostname",  required_argument, 0, 'h'},
            {"portNr",  required_argument, 0, 'p'},
            {"byLog",    required_argument, 0, 'l'},
            {"byUid",    required_argument, 0, 'u'},
            {0, 0, 0, 0}
        };
     
        c = getopt_long (argc, argv, "LUGNHSh:p:l:u:", long_options, &option_index);
     
        /* Detekce posledniho parametru*/
        if (c == -1)
          break;
     
        switch (c)
        {   case 'L':
				params->login = true;
				break;    
            case 'U':
				params->uid = true;
				break;     
            case 'G':
				params->gid = true;
				break;           
            case 'N':
				params->name = true;
				break;        
            case 'H':
				params->homeDir = true;
				break;        
            case 'S':
				params->shell = true;
				break;    
            case 'h':
               params->hostname = optarg;
               break;    
            case 'p':
               params->portNr = optarg;
               break;    
            case 'l':
			   optind--;
			   for( ;optind < argc && *argv[optind] != '-'; optind++){		//SOMETHING WRONG HERE....
					params->findLogin.append(argv[optind]);  
					params->findLogin.append(" ");  
			   }
               break;     
            case 'u':
			   optind--;
			   for( ;optind < argc && *argv[optind] != '-'; optind++){		//SOMETHING WRONG HERE....
					params->findUid.append(argv[optind]);  
					params->findUid.append(" ");  
			   }
               break;   
            case '?':
               params->error = EPARAMS;
               break;     
            default:
               params->error = EUNKNOWN;
		}
    }  
    return;
}

void setConnection(TParams *params){
	cout << "Got in the set connection function..\n";
	 struct addrinfo hints, *res, *p;
	 int sockFd, recvd;
	 void *transfer;
	 char buffer[1024];
	 string response;
	 response.clear();
	 
	 memset(&buffer, 0, sizeof(buffer));	
     memset(&hints, 0, sizeof(hints));          
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;
     
     if(getaddrinfo(params->hostname.c_str(), params->portNr.c_str(), &hints, &res) != 0){
		params->error = ERR_GADDINFO;
		return;
     }
     cout << "Got addrinfo" << endl;
     for(p = res; p != NULL; p = p->ai_next) {
        if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
		cout << "Got socket descriptor: " << sockFd << endl;
        if (connect(sockFd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockFd);
            continue;
        }

        break;
    }
     cout << "Connected to eva..." << endl;
     transfer = (void*)params;
     if(send(sockFd, transfer, sizeof(struct Params), 0) < 0){
		printf( "Error sending msg: %s\n", strerror( errno ) );
		params->error = ERR_SEND;
		freeaddrinfo(res);
        return;
     }
     cout << "Message sent." << endl;
     //here comes some magic....if necessary.
     
     while((recvd = recv(sockFd, buffer, sizeof(buffer), MSG_WAITALL)) != 0){
		cout << "Receiving." << endl;
		response.append(buffer, recvd);
     }
     
	cout << response;
     
     freeaddrinfo(res);
     return;
}

int main (int argc, char **argv)
{	TParams params;
	params.error = EOK;
	params.findLogin.clear();
	params.findUid.clear();
	
	getParams(argc, argv, &params);
	if(params.error != EOK){
		cout << errmsg[params.error];
		if(params.error == EPARAMS)
			cout << helpmsg;
		return 1;
	}
	if((params.findLogin.length() > 0) && (params.findUid.length() >0)){
		cout << helpmsg;
		return 1;
	}
	
	setConnection(&params);
	if(params.error != EOK){
		cout << errmsg[params.error] ;
		return 1;
	}
	return 0;
}
