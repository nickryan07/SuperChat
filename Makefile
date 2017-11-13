CXXFLAGS= -Wall -g -Wextra -O0 -std=c++11
LDLIBS =  -lboost_system -lpthread -lz -lboost_date_time

EXECUTABLES = chat_client chat_server

all: ${EXECUTABLES}

chat_server:chat_message.hpp chat_server.cpp util.hpp

chat_client:chat_message.hpp chat_client.cpp util.hpp

clean:
	rm -f ${EXECUTABLES}
