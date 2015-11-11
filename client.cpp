using namespace std;
#include <iostream>
#include <string>
#include <limits>
#include <fstream>
#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "master.h"
#include "helpers.h"

int contact_master() {
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		cout << "Error creating socket." << endl;
		exit(1);
	}
	sockaddr_in master;
	memset(&master, '0', sizeof(master));
	master.sin_family = AF_INET;
	master.sin_port = htons(MASTER_PORT);
	inet_pton(AF_INET, MASTER_IP_ADDR, &(master.sin_addr));
	if(connect(socket_fd, (sockaddr *)&master, sizeof(master)) < 0) {
		printf("\n Error: Failed to connect to the directory service. \n");
		exit(1);
	}
	return socket_fd;
}

int main() {
	cout << "uiShell for tiny-Google\n--------------------------" << endl;
	
	while(true) {
		cout << "Enter a number to choose one of the following options:" << endl;
		cout << "\t1) Index a document " << endl;
		cout << "\t2) Search indexed documents" << endl;
		cout << "\t3) Quit\n" << endl;
		
		int input = 0;
		if(!(cin >> input)) {
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Invalid input. Try again.\n" << endl;
			continue;
		}
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		
		if(input == 1) { // Send a document indexing request
			// Read in the filepath from the command line
			string filepath_input;
			cout << "Enter the path to the document you wish to index:" << endl;
			getline(cin, filepath_input);
			char filepath[filepath_input.length() + 1];
			memset(filepath + filepath_input.length(), 0, 1);
			strncpy(filepath, filepath_input.c_str(), filepath_input.length());
			
			// Read the file
			char *document;
			int doc_len;
			ifstream input_file(filepath);
			if(input_file.is_open()) {
				string doc_string((istreambuf_iterator<char>(input_file)), istreambuf_iterator<char>());
				doc_len = doc_string.length();
				document = new char[doc_len + 1];
				memset(document + doc_len, 0, 1);
				strncpy(document, doc_string.c_str(), doc_len);
			} else {
				cout << "Failed to open the specified file." << endl;
				exit(1);
			}
			
			// Set up socket with the master and write data
			int socket_fd = contact_master();
			int procedure = 1;
			if(patient_write(socket_fd, (void *)&procedure, sizeof(int)) < 0) {
				cout << "Error writing procedure identifier to master." << endl;
			}
			if(patient_write(socket_fd, (void *)document, doc_len) < 0) {
				cout << "Error writing file to master." << endl;
			}
			close(socket_fd);
		} else if(input == 2) { // Send a document search query
			// Read in arguments
			char *args[32];
			cout << "Enter search terms line by line, then hit enter again to finish:" << endl;
			int argc = 0;
			while(argc < 32) {
				string arg;
				getline(cin, arg);
				if(arg.length() == 0) {
					break;
				}
				char *arg_arr = new char[arg.length() + 1];
				memset(arg_arr + arg.length(), 0, 1);
				args[argc] = arg_arr;
				argc++;
			}
			
			// Set up socket with the master and write data
			int socket_fd = contact_master();
			int procedure = 2;
			if(patient_write(socket_fd, (void *)&procedure, sizeof(int)) < 0) {
				cout << "Error writing procedure identifier to master." << endl;
			}
			if(patient_write(socket_fd, (void *)&argc, sizeof(int)) < 0) {
				cout << "Error writing argument count to master." << endl;
			}
			for(int i = 0; i < argc; i++) {
				if(patient_write(socket_fd, (void *)args[i], strlen(args[i])) < 0) {
					cout << "Error writing argument " << i << " to master." << endl;
				}
			}
			close(socket_fd);
		} else if(input == 3) {
			cout << "Good bye!" << endl;
			break;
		}
	}
	
	
}