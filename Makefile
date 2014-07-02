CC = g++
DEBUG = -g
CFLAGS = -Wall -std=c++11 -c
LLFLAGS = -Wall -std=c++11
LLEFLAGS = -lboost_system -lboost_thread -lpthread -lrt -lboost_program_options

all:	e-ognisko_server e-ognisko_client

clean:
	rm *.o e-ognisko_server e-ognisko_client

e-ognisko_protocol.o : e-ognisko_protocol.hpp e-ognisko_protocol.cpp
	$(CC) $(CFLAGS) e-ognisko_protocol.cpp -o e-ognisko_protocol.o

e-ognisko_fifo_man.o : e-ognisko_protocol.hpp e-ognisko_fifo_man.hpp e-ognisko_fifo_man.cpp
	$(CC) $(CFLAGS) e-ognisko_fifo_man.cpp -o e-ognisko_fifo_man.o

e-ognisko_server_client.o : e-ognisko_protocol.hpp e-ognisko_fifo_man.hpp e-ognisko_server_client.hpp e-ognisko_server_client.cpp
	$(CC) $(CFLAGS) e-ognisko_server_client.cpp -o e-ognisko_server_client.o 

e-ognisko_mixer.o : e-ognisko_mixer.hpp default_mixer.cpp
	$(CC) $(CFLAGS) default_mixer.cpp -o e-ognisko_mixer.o

e-ognisko_udp_mixer.o : e-ognisko_protocol.hpp e-ognisko_server_client.hpp e-ognisko_mixer.hpp e-ognisko_udp_mixer.hpp e-ognisko_udp_mixer.cpp
	$(CC) $(CFLAGS) e-ognisko_udp_mixer.cpp -o e-ognisko_udp_mixer.o 

e-ognisko_udp_router.o : e-ognisko_server_client.hpp e-ognisko_udp_router.hpp e-ognisko_udp_router.cpp
	$(CC) $(CFLAGS) e-ognisko_udp_router.cpp -o e-ognisko_udp_router.o

e-ognisko_report_sender.o : e-ognisko_protocol.hpp e-ognisko_server_client.hpp e-ognisko_report_sender.hpp e-ognisko_report_sender.cpp
	$(CC) $(CFLAGS) e-ognisko_report_sender.cpp -o e-ognisko_report_sender.o

e-ognisko_server : e-ognisko_protocol.o e-ognisko_fifo_man.o e-ognisko_server_client.o e-ognisko_report_sender.o e-ognisko_udp_router.o e-ognisko_mixer.o e-ognisko_udp_mixer.o e-ognisko_server.hpp e-ognisko_server.cpp
	$(CC) $(LLFLAGS) e-ognisko_protocol.o e-ognisko_fifo_man.o e-ognisko_server_client.o e-ognisko_report_sender.o e-ognisko_udp_router.o e-ognisko_mixer.o e-ognisko_udp_mixer.o e-ognisko_server.cpp -o e-ognisko_server $(LLEFLAGS)

e-ognisko_client : e-ognisko_client.cpp e-ognisko_protocol.o
	$(CC) $(LLFLAGS) e-ognisko_protocol.o e-ognisko_client.cpp -o e-ognisko_client $(LLEFLAGS)
