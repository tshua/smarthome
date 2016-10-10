CC = g++
CFLAGS = -Wall
OBJS = ./source/socket_route.o  ./source/sock_server.o ./server/server.o ./source/protocol.o ./phone/client.o ./source/sock_client.o ./source/common.o ./devices/lamp1.o \
       ./devices/lamp2.o ./devices/fan.o ./devices/switch.o
CLIENTOBJS = ./source/protocol.o ./phone/client.o ./source/sock_client.o ./source/socket_route.o
SERVEROBJS =  ./source/protocol.o  ./source/socket_route.o  ./source/sock_server.o ./server/server.o ./source/sock_client.o ./source/common.o
LAMP1OBJS = ./source/protocol.o ./source/sock_client.o ./source/socket_route.o ./devices/lamp1.o ./source/common.o
LAMP2OBJS = ./source/protocol.o ./source/sock_client.o ./source/socket_route.o ./devices/lamp2.o ./source/common.o
FANOBJS = ./source/protocol.o ./source/sock_client.o ./source/socket_route.o ./devices/fan.o ./source/common.o
SWITCHOBJS = ./source/protocol.o ./source/sock_client.o ./source/socket_route.o ./devices/switch.o ./source/common.o




all:$(OBJS)
	$(CC) $(FLAGS) -o s $(SERVEROBJS) -g -lpthread
	$(CC) $(FLAGS) -o c $(CLIENTOBJS) -g -lpthread
	$(CC) $(FLAGS) -o lamp1 $(LAMP1OBJS) -g -lpthread
	$(CC) $(FLAGS) -o lamp2 $(LAMP2OBJS) -g -lpthread
	$(CC) $(FLAGS) -o fan $(FANOBJS) -g -lpthread
	$(CC) $(FLAGS) -o switch $(SWITCHOBJS) -g 

%.o:%.cpp
	$(CC) $(FLAGS) -o $@ -c $< -g

clean:
	find .  -name "*.o" -exec rm -rfv {} \;
	rm s c lamp1 lamp2 fan switch -v
