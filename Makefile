all: server client

test:
	g++ src/server.cpp -o bin/server -Dverbose=1 -Dstress=1
	g++ src/client.cpp -o bin/client -Dverbose=1

server:
	g++ src/server.cpp -o bin/server

client:
	g++ src/client.cpp -o bin/client

clean:
	rm bin/server bin/client
