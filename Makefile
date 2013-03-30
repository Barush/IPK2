######################################
# Nazev: Makefile
# Ucel kodu: IPK projekt 2
#      makefile pro preklad programu klient - server
# Autor: Barbora Skrivankova, xskriv01, FIT VUT
# Vytvoreno: brezen 2013
######################################

CPPFLAGS=--std=c++98 -Wall -pedantic

all: client server

client: client.o
	g++ $(CPPFLAGS) client.o -o client

server: server.o
	g++ $(CPPFLAGS) server.o -o server

client.o: client.cpp
	g++ $(CPPFLAGS) -c client.cpp

server.o: server.cpp
	g++ $(CPPFLAGS) -c server.cpp

clean: 
	rm -f *.o
	rm client server
