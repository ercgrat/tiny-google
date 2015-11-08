COMP=g++
FLAGS=-Wall -g -std=c99 -I.

all: client

client: client.cpp
	$(COMP) $(FLAGS) -o client client.cpp
