using namespace std;
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "master.h"
#include "helpers.h"
#include "index.h"

void exit_program(int socket_fd, string message) {
	cout << message << endl;
	close(socket_fd);
	exit(1);
}

int main(int argc, char *argv[]) {

	if(argc < 2) {
		cout << "Not enough arguments. Run with a port number as the first argument:\n\t./index_helper <port>" << endl;
		exit(1);
	}

	// Register self as indexing helper with the master node
	int port = atoi(argv[1]);
	int procedure = 3;
	helper_registration_request *request = new helper_registration_request;
	request->port = port;
	request->service = 0; // 0 indicates indexing instead of searching
	
	int outgoing = create_socket();
	if(contact_node(outgoing, MASTER_IP_ADDR, MASTER_PORT) < 0) {
		cout << "Failed to connect to the master node." << endl;
		close(outgoing);
		exit(1);
	}
	if(patient_write(outgoing, (void *)&procedure, sizeof(int)) < 0) {
		exit_program(outgoing, "Error writing procedure id 3 to the master node.");
	}
	if(patient_write(outgoing, (void *)request, sizeof(helper_registration_request)) < 0) {
		exit_program(outgoing, "Error registering with the master node.");
	}
	close(outgoing);
	
	int incoming = create_socket();
	listen_on_port(incoming, port);
	
	while(1) {
		sockaddr_in client_addr;
		int client = accept_client(incoming, &client_addr);
		
		int procedure_id;
		if(patient_read(client, (void *)&procedure_id, sizeof(int)) < 0) {
			exit_program(client, "Error reading procedure id from master indexer.");
		}
		
		// Ping from master - ignore it
		if(procedure_id == -1) {
			close(client);
			continue;
		} else if(procedure_id == 0) { // Index a document
			// Read document id
			int doc_id;
			if(patient_read(client, (void *)&doc_id, sizeof(int)) < 0) {
				exit_program(client, "Error reading document id from master indexer.");
			}
		
			// Read document length and document
			int doc_len;
			if(patient_read(client, (void *)&doc_len, sizeof(int)) < 0) {
				exit_program(client, "Error reading document length from master indexer.");
			}
			char *document = new char[doc_len + 1];
			memset(document, 0, doc_len + 1);
			if(patient_read(client, (void *)document, doc_len) < 0) {
				exit_program(client, "Error reading document from master indexer.");
			}
			
			// Write document to disk, creating docs directory if it doesn't exist
			struct stat sb;
			if(stat("./tiny-google-docs", &sb) != 0) {
				system("mkdir tiny-google-docs");
			}
			ofstream new_doc_file;
			stringstream new_doc_filename;
			new_doc_filename << "./tiny-google-docs/" << doc_id << ".goog";
			new_doc_file.open(new_doc_filename.str().c_str());
			new_doc_file << document;
			new_doc_file.close();
			
			// Construct a partial index
			term_node *partial_index = index_document(document);
			term_node *current = partial_index;
			int num_terms = 0;
			while(current != NULL) {
				num_terms++;
				current = current->next;
			}
			
			// Write number of terms to master indexer
			if(patient_write(client, (void *)&num_terms, sizeof(int)) < 0) {
				exit_program(client, "Error writing number of terms to master indexer.");
			}
			current = partial_index;
			while(current != NULL) { // Write term length, term, and count of each term to master indexer
				int term_len = strlen(current->term);
				if(patient_write(client, (void *)&term_len, sizeof(int)) < 0) {
					exit_program(client, "Error writing term length to master indexer.");
				}
				if(patient_write(client, (void *)current->term, term_len) < 0) {
					exit_program(client, "Error writing term to master indexer.");
				}
				int term_count = current->list->count;
				if(patient_write(client, (void *)&term_count, sizeof(int)) < 0) {
					exit_program(client, "Error writing number of terms to master indexer.");
				}
				current = current->next;
			}
			
			// Clean up the index
			current = partial_index;
			term_node *previous = NULL;
			while(current != NULL) {
				delete current->list;
				delete current->term;
				if(previous) {
					delete previous;
				}
				previous = current;
				current = current->next;
			}
			delete current;
			delete[] document;
			close(client);
		} else if(procedure_id == 1) { // Return a stored document
			// Read document id
			int doc_id;
			if(patient_read(client, (void *)&doc_id, sizeof(int)) < 0) {
				exit_program(client, "Error reading document id from master indexer.");
			}
			
			stringstream filename;
			filename << "./tiny-google-docs/" << doc_id << ".goog";
			ifstream doc_file(filename.str().c_str());
			if(doc_file.is_open()) {
				string doc_string((istreambuf_iterator<char>(doc_file)), istreambuf_iterator<char>());
				int doc_len = doc_string.length();
				char *document = new char[doc_len + 1];
				memset(document + doc_len, 0, 1);
				strncpy(document, doc_string.c_str(), doc_len);
				doc_file.close();
				if(patient_write(client, (void *)&doc_len, sizeof(int)) < 0) {
					close(client);
					cout << "Error writing document length back to the master node." << endl;
					exit(1);
				}
				if(patient_write(client, (void *)document, doc_len) < 0) {
					cout << "Error writing document back to the master node." << endl;
					exit(1);
				}
			} else {
				close(client);
				doc_file.close();
				cout << "Error on index helper when reading file requested by the client." << endl;
				exit(1);
			}
		}
	}
}