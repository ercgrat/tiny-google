Eric Gratta - eag55
Jack Lange
CS 2510 Project 2


--- COMPILING ---

Before compiling, edit master.h to set the desired IP address
for the master server.

Then, run make to compile all files.


--- TESTING ---

Start the master server:
	./master &

Start any number of index helpers on different machines:
	./index_helper <port #> &

Start a client:
	./client
	

--- DEBUGGING ---

To print the contents of the inverted index, uncomment line 162 in master.cpp, then compile.
When a document is indexed, the contents of the inverted index will be printed out.
