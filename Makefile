COMP=g++
FLAGS=-Wall -std=c++11 -g -I.

all: client index master index_helper

client: client.cpp helpers.h master.h
	$(COMP) $(FLAGS) -o client client.cpp

index: index.cpp index.h
	$(COMP) $(FLAGS) -c index.cpp

master: master.cpp index.o helpers.h master.h
	$(COMP) $(FLAGS) -pthread -o master master.cpp index.o

index_helper: index_helper.cpp index.o helpers.h master.h
	$(COMP) $(FLAGS) -o index_helper index_helper.cpp index.o