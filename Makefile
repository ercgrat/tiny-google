COMP=g++
FLAGS=-Wall -g -I.

all: client index

client: client.cpp
	$(COMP) $(FLAGS) -o client client.cpp

index: index.cpp index.h
	$(COMP) $(FLAGS) -c index.cpp
