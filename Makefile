CXXFLAGS= -Wall -g -O0 -std=c++11
LDFLAGS =  -lboost_system -lpthread -lz

EXECUTABLES = chat_client chat_server

all: ${EXECUTABLES}

chat_server:chat_message.hpp chat_server.cpp util.hpp

chat_client:chat_message.hpp chat_client.cpp util.hpp

clean:
	rm -f ${EXECUTABLES}
