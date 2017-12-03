CXXFLAGS= -Wall -g -Wextra -O0 -std=c++11
LDLIBS = -lboost_system -lpthread -lfltk -lz -lboost_date_time

EXECUTABLES = chat_client chat_server chat_testing

all: ${EXECUTABLES}

chat_server:chat_message.hpp chat_server.cpp util.hpp

chat_client:chat_message.hpp util.hpp chat_client.cpp

chat_testing:testsuite.cpp test_command_formatting.hpp util.hpp

clean:
	rm -f ${EXECUTABLES}
