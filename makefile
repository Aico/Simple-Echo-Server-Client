all: server client
server: Server.o ErrorHandle.o
	echo Linking $@ ...
	gcc -Wall $^ -o $@

client: Client.o ErrorHandle.o
	echo Linking $@ ...
	gcc -Wall -pthread $^ -o $@

Client.o: Client.c ErrorHandle.h
	gcc -Wall -lpthread -I. -c Client.c
ErrorHandle.o: ErrorHandle.c ErrorHandle.h
	gcc -Wall -c ErrorHandle.c
Server.o: Server.c ErrorHandle.h
	gcc -Wall -I. -c Server.c

clean:
	rm -f *.o *~
	rm -f server client
	echo Command \"clean\" done !!
