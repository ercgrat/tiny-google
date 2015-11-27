using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <sys/types.h>
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
		
		int doc_len;
		if(patient_read(client, (void *)&doc_len, sizeof(int)) < 0) {
			exit_program(client, "Error reading document length from master indexer.");
		}
		
		// Ping from master node - ignore it
		if(doc_len == -1) {
			close(client);
			continue;
		}
		
		char *document = new char[doc_len + 1];
		memset(document, 0, doc_len + 1);
		if(patient_read(client, (void *)document, doc_len) < 0) {
			exit_program(client, "Error reading document from master indexer.");
		}
		
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
	}
}