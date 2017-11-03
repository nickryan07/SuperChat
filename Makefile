CXXFLAGS= -Wall -g -O0 -std=c++11
LDLIBS =  -lboost_system -lpthread 

EXECUTABLES = client server

all: ${EXECUTABLES}

server:server.cpp

chat_client:client.cpp

clean:
	rm -f ${EXECUTABLES}
