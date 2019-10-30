CPPFlags=-Wall -Wextra -Wshadow -Wconversion
CPP=g++ ${CPPFlags}

all: server client

test:
	${CPP} src/server.cpp -o bin/server -Dverbose=1 -Dstress=1
	${CPP} src/client.cpp -o bin/client -Dverbose=1

server:
	${CPP} src/server.cpp -o bin/server

client:
	${CPP} src/client.cpp -o bin/client

clean:
	rm bin/server bin/client
