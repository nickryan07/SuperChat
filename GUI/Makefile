
CPPFLAGS = -Wall -O0 -g -std=c++11

all: chat_server chat_client client_gui

chat_server: chat_server.cpp chat_message.hpp
		g++ ${CPPFLAGS} -o chat_server chat_server.cpp \
			-lboost_system
chat_client: chat_client.cpp chat_message.hpp
		g++ ${CPPFLAGS} -o chat_client chat_client.cpp \
			-lboost_system -lpthread

client_gui: client_gui.cpp chat_message.hpp io.hpp 
	g++ ${CPPFLAGS} -o client_gui \
	       	client_gui.cpp \
		-lfltk -lboost_system -lpthread


clean:
	rm -f chat_server chat_client client_gui
