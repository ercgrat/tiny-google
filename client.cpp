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
		cout << "\t3) Retrieve a document" << endl;
		cout << "\t4) Quit\n" << endl;
		
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
			} else {
				input_file.close();
				cout << "Failed to open the specified file." << endl;
			}
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
			
			// Set up connection with the master
			int socket_fd = create_socket();
			if(contact_node(socket_fd, MASTER_IP_ADDR, MASTER_PORT) < 0) {
				cout << "Failed to connect to the master node." << endl;
				close(socket_fd);
				exit(1);
			}
			
			// Write request data to the master
			int procedure = 2;
			if(patient_write(socket_fd, (void *)&procedure, sizeof(int)) < 0) {
				cout << "Error writing procedure identifier to master." << endl;
				exit(1);
			}
			if(patient_write(socket_fd, (void *)&argc, sizeof(int)) < 0) {
				cout << "Error writing argument count to master." << endl;
				exit(1);
			}
			for(int i = 0; i < argc; i++) {
				int term_len = strlen(args[i]);
				if(patient_write(socket_fd, (void *)&term_len, sizeof(int)) < 0) {
					cout << "Error writing argument " << i << " to master." << endl;
				}
				if(patient_write(socket_fd, (void *)args[i], term_len) < 0) {
					cout << "Error writing argument " << i << " to master." << endl;
				}
			}
			
			// Read response data from the master
			int document_count;
			if(patient_read(socket_fd, (void *)&document_count, sizeof(int)) < 0) {
				cout << "Error reading document count from master." << endl;
				exit(1);
			}
			
			cout << "Search matched ( " << document_count << " ) documents." << endl;
			for(int i = 0; i < document_count; i++) {
				// Read document id
				int doc_id;
				if(patient_read(socket_fd, (void *)&doc_id, sizeof(int)) < 0) {
					cout << "Error reading document id from master." << endl;
					exit(1);
				}
				
				// Read document name
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
				cout << (i+1) << ") " << doc_name << " (id=" << doc_id << "):" << endl;
				delete[] doc_name;
				
				// Read terms; term counts array is padded in the front by one integer - ignore it
				int *term_counts = new int[argc + 1];
				if(patient_read(socket_fd, (void *)term_counts, sizeof(int)*(argc + 1)) < 0) {
					cout << "Error reading term counts from master." << endl;
					exit(1);
				}
				for(int term = 0; term < argc; term++) {
					cout << "\t\"" << args[term] << "\": " << term_counts[term + 1] << endl;
				}
				delete[] term_counts;
			}
			
			// Clean up memory
			for(int i = 0; i < argc; i++) {
				delete[] args[i];
			}
			
			close(socket_fd);
		} else if(input == 3) {
			cout << "Enter the id of the document you wish to retrieve:" << endl;
			int doc_id = -1;
			while(doc_id < 0) {
				if(!(cin >> doc_id) || doc_id < 0) {
					doc_id = -1;
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					cout << "Invalid input. The document id is an integer greater than or equal to 0:" << endl;
					continue;
				}
			}
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			
			// Set up connection with the master
			int socket_fd = create_socket();
			if(contact_node(socket_fd, MASTER_IP_ADDR, MASTER_PORT) < 0) {
				cout << "Failed to connect to the master node." << endl;
				close(socket_fd);
				exit(1);
			}
			
			// Write request data to the master
			int procedure = 4;
			if(patient_write(socket_fd, (void *)&procedure, sizeof(int)) < 0) {
				cout << "Error writing procedure identifier to master." << endl;
				exit(1);
			}
			if(patient_write(socket_fd, (void *)&doc_id, sizeof(int)) < 0) {
				cout << "Error writing document id to master." << endl;
				exit(1);
			}
			
			// Read data from master
			int success;
			if(patient_read(socket_fd, (void *)&success, sizeof(int)) < 0) {
				cout << "Error reading success status from master." << endl;
				exit(1);
			}
			if(success) {
				// Read document name
				int doc_name_len;
				if(patient_read(socket_fd, (void *)&doc_name_len, sizeof(int)) < 0) {
					cout << "Error reading document name length from master." << endl;
					exit(1);
				}
				char *doc_name = new char[doc_name_len + 1];
				memset(doc_name, 0, doc_name_len + 1);
				if(patient_read(socket_fd, (void *)doc_name, doc_name_len) < 0) {
					cout << "Error reading document from master." << endl;
					exit(1);
				}
				
				// Read document
				int doc_len;
				if(patient_read(socket_fd, (void *)&doc_len, sizeof(int)) < 0) {
					cout << "Error reading document length from master." << endl;
					exit(1);
				}
				char *document = new char[doc_len + 1];
				memset(document, 0, doc_len + 1);
				if(patient_read(socket_fd, (void *)document, doc_len) < 0) {
					cout << "Error reading document from master." << endl;
					exit(1);
				}
				cout << doc_name << endl << "-----------------------------------------------" << endl << document << endl;
			} else {
				cout << "The document you are looking for is unavailable. Either the index is invalid or its owner failed." << endl;
			}
		} else if(input == 4) {
			cout << "Good bye!" << endl;
			break;
		}
	}
	
	
}