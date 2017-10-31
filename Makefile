CXXFLAGS= -g -Wall -Wextra -O0 -std=c++11
LDLIBS =  -lboost_system -lpthread

EXECUTABLES = chat_client chat_server

all: ${EXECUTABLES}

chat_server:chat_message.hpp chat_server.cpp

chat_client:chat_message.hpp chat_client.cpp

clean:
	rm -f ${EXECUTABLES}
