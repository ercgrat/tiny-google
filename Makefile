COMP=g++
FLAGS=-Wall -g -I.

all: client

client: client.cpp
	$(COMP) $(FLAGS) -o client client.cpp
