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
			strncpy(filepath, filepath_input.c_str(), filepath_input.length());
			
			// Read the file
			char *document;
			ifstream input_file(filepath, ios::ate | ios::binary);
			if(input_file.is_open()) {
				input_file.ignore(numeric_limits<streamsize>::max());
				int input_file_size = input_file.gcount();
				input_file.clear();
				input_file.seekg(0, ios_base::beg);
				
				document = new char[input_file_size];
				while(!input_file.eof()) {
					input_file >> document;
				}
			} else {
				cout << "Failed to open the specified file." << endl;
			}
			
			// Set up socket with the master and write data
			int socket_fd = contact_master();
			//int procedure = 1;
			//patient_write(socket_fd,(void *)&procedure, sizeof(int));
			
			close(socket_fd);
		} else if(input == 2) { // Send a document search query
		} else if(input == 3) {
			cout << "Good bye!" << endl;
			break;
		}
	}
	
	
}