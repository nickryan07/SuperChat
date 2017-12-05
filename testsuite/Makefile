CXXFLAGS= -Wall -g -Wextra -O0 -std=c++11
LDLIBS = -lz -lboost_date_time
EXECUTABLES = test_suite

all: ${EXECUTABLES}

test_suite:testsuite.cpp test_command_formatting.hpp ../util.hpp
	g++ $(CXXFLAGS) $(LDLIBS) -o test_suite testsuite.cpp

clean:
	rm -f ${EXECUTABLES}
