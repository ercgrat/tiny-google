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
				input_file.close();
				cout << "Failed to open the specified file." << endl;
				exit(1);
			}
			input_file.close();
			
			// Set up socket with the master and write data
			int socket_fd = create_socket();
			if(contact_node(socket_fd, MASTER_IP_ADDR, MASTER_PORT) < 0) {
				cout << "Failed to connect to the master node." << endl;
				close(socket_fd);
				exit(1);
			}
			int procedure = 1;
			if(patient_write(socket_fd, (void *)&procedure, sizeof(int)) < 0) {
				cout << "Error writing procedure identifier to master." << endl;
			}
			int filepath_len = strlen(filepath);
			if(patient_write(socket_fd, (void *)&filepath_len, sizeof(int)) < 0) {
				cout << "Error writing filepath length to master." << endl;
			}
			if(patient_write(socket_fd, (void *)filepath, filepath_len) < 0) {
				cout << "Error writing filepath to master." << endl;
			}
			if(patient_write(socket_fd, (void *)&doc_len, sizeof(int)) < 0) {
				cout << "Error writing document length to master." << endl;
			}
			if(patient_write(socket_fd, (void *)document, doc_len) < 0) {
				cout << "Error writing document to master." << endl;
			}
			delete[] document;
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
				memset(arg_arr, 0, arg.length() + 1);
				strcpy(arg_arr, arg.c_str());
				args[argc] = arg_arr;
				argc++;
			}
			
			// Set up socket with the master and write data
			int socket_fd = create_socket();
			if(contact_node(socket_fd, MASTER_IP_ADDR, MASTER_PORT) < 0) {
				cout << "Failed to connect to the master node." << endl;
				close(socket_fd);
				exit(1);
			}
			int procedure = 2;
			if(patient_write(socket_fd, (void *)&procedure, sizeof(int)) < 0) {
				cout << "Error writing procedure identifier to master." << endl;
			} else {
				//if(patient_write(socket_fd, (void *)&argc, sizeof(int)) < 0) {
				//	cout << "Error writing argument count to master." << endl;
				//} else {
					//for(int i = 0; i < argc; i++) {
					for(int i = 0; i < 1; i++) {
						int term_len = strlen(args[i]);
						if(patient_write(socket_fd, (void *)&term_len, sizeof(int)) < 0) {
							cout << "Error writing argument " << i << " to master." << endl;
						}
						if(patient_write(socket_fd, (void *)args[i], term_len) < 0) {
							cout << "Error writing argument " << i << " to master." << endl;
						}
					}
				//}
			}
			for(int i = 0; i < argc; i++) {
				delete[] args[i];
			}
			
			int document_count;
			if(patient_read(socket_fd, (void *)&document_count, sizeof(int)) < 0) {
				cout << "Error reading document count from master." << endl;
				exit(1);
			}
			
			for(int i = 0; i < document_count; i++) {
				int doc_name_len;
				if(patient_read(socket_fd, (void *)&doc_name_len, sizeof(int)) < 0) {
					cout << "Error reading document name length " << i << " from master." << endl;
					exit(1);
				}
				char *doc_name = new char[doc_name_len + 1];
				memset(doc_name, 0, doc_name_len + 1);
				if(patient_read(socket_fd, (void *)doc_name, doc_name_len) < 0) {
					cout << "Error reading document name " << i << " from master." << endl;
					exit(1);
				}
				int frequency;
				if(patient_read(socket_fd, (void *)&frequency, sizeof(int)) < 0) {
					cout << "Error reading frequency " << i << " from master." << endl;
					exit(1);
				}
				
				cout << doc_name << ": " << frequency << " occurrences." << endl;
			}
			
			close(socket_fd);
		} else if(input == 3) {
			cout << "Good bye!" << endl;
			break;
		}
	}
	
	
}