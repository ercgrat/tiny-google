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

#include <pthread.h>
#include <mutex>

#include "master.h"
#include "helpers.h"
#include "index.h"

term_node *master_index = NULL;
mutex master_index_lock;
char **document_names = new char*[1024];

helper_node *indexers = NULL;
helper_node *last_indexer = NULL;
helper_node *searchers = NULL;
helper_node *last_searcher = NULL;

void *thread_routine_index(void *input) {
	cout << "Spawned indexing master thread." << endl;
	index_thread_data *data = (index_thread_data *)input;
	
	// Read document from the client
	int doc_name_len;
	if(patient_read(data->client_fd, (void *)&doc_name_len, sizeof(int)) < 0) {
		close(data->client_fd);
		cout << "Error reading filepath length from client." << endl;
		pthread_exit(NULL);
	}
	char *doc_name = new char[doc_name_len + 1];
	memset(doc_name, 0, doc_name_len + 1);
	if(patient_read(data->client_fd, (void *)doc_name, doc_name_len) < 0) {
		delete[] doc_name;
		close(data->client_fd);
		cout << "Error reading document from client." << endl;
		pthread_exit(NULL);
	}
	document_names[data->doc_id] = doc_name;
	
	int doc_len;
	if(patient_read(data->client_fd, (void *)&doc_len, sizeof(int)) < 0) {
		close(data->client_fd);
		cout << "Error reading document size from client." << endl;
		pthread_exit(NULL);
	}
	char *document = new char[doc_len + 1];
	memset(document, 0, doc_len + 1);
	if(patient_read(data->client_fd, (void *)document, doc_len) < 0) {
		delete[] document;
		close(data->client_fd);
		cout << "Error reading document from client." << endl;
		pthread_exit(NULL);
	}
	
	// Send document to the indexer
	int indexer_fd = create_socket();
	if(contact_node(indexer_fd, data->helper->ip_addr, data->helper->port) < 0) {
		cout << "Error connecting to indexer at " << data->helper->ip_addr << " on port " << data->helper->port << "." << endl;
		close(indexer_fd);
		pthread_exit(NULL);
	}
	if(patient_write(indexer_fd, (void *)&doc_len, sizeof(int)) < 0) {
		close(indexer_fd);
		close(data->client_fd);
		cout << "Error writing document length to indexer." << endl;
		pthread_exit(NULL);
	}
	if(patient_write(indexer_fd, (void *)document, doc_len) < 0) {
		close(indexer_fd);
		close(data->client_fd);
		cout << "Error writing document to indexer." << endl;
		pthread_exit(NULL);
	}
	delete[] document;
	
	// Read partial index back from the indexer
	int num_terms;
	if(patient_read(indexer_fd, (void *)&num_terms, sizeof(int)) < 0) {
		close(indexer_fd);
		close(data->client_fd);
		cout << "Error reading number of terms from indexer." << endl;
		pthread_exit(NULL);
	}
	
	// Reconstruct partial index sent over the network
	term_node *partial_index = NULL;
	term_node *current = NULL;
	for(int i = 0; i < num_terms; i++) {
		int term_len, frequency;
		char *term;
		
		if(patient_read(indexer_fd, (void *)&term_len, sizeof(int)) < 0) {
			close(indexer_fd);
			close(data->client_fd);
			cout << "Error reading term length " << i << " from indexer." << endl;
			pthread_exit(NULL);
		}
		
		term = new char[term_len + 1];
		memset(term, 0, term_len + 1);
		if(patient_read(indexer_fd, (void *)term, term_len) < 0) {
			close(indexer_fd);
			close(data->client_fd);
			cout << "Error reading term " << i << " from indexer." << endl;
			pthread_exit(NULL);
		}
		
		if(patient_read(indexer_fd, (void *)&frequency, sizeof(int)) < 0) {
			close(indexer_fd);
			close(data->client_fd);
			cout << "Error reading frequency of term <" << term << "> from indexer." << endl;
			pthread_exit(NULL);
		}
		
		term_node *new_term = new term_node;
		new_term->term = term;
		new_term->next = NULL;
		new_term->list = new freq_node;
		new_term->list->doc_id = data->doc_id;
		new_term->list->count = frequency;
		new_term->list->next = NULL;
		if(!partial_index) {
			partial_index = new_term;
			current = new_term;
		} else {
			current->next = new_term;
			current = current->next;
		}
	}
	close(data->client_fd);
	
	// Merge the reconstructed partial index with the master index
	master_index_lock.lock();
	merge_partial_index(&master_index, partial_index);
	print_index(master_index);
	master_index_lock.unlock();
	
	delete data;
	pthread_exit(NULL);
}

void *thread_routine_search(void *input) {
	search_thread_data *data = (search_thread_data *)input;
	
	int search_term_len;
	if(patient_read(data->client_fd, (void *)&search_term_len, sizeof(int)) < 0) {
		close(data->client_fd);
		cout << "Error reading search term length from client." << endl;
		pthread_exit(NULL);
	}
	char *search_term = new char[search_term_len + 1];
	memset(search_term, 0, search_term_len + 1);
	if(patient_read(data->client_fd, (void *)search_term, search_term_len) < 0) {
		delete[] search_term;
		close(data->client_fd);
		cout << "Error reading document from client." << endl;
		pthread_exit(NULL);
	}
	
	master_index_lock.lock();
	term_node *current = master_index;
	while(current != NULL) {
		if(strcmp(current->term, search_term) == 0) {
			int num_docs = 0;
			freq_node *current_freq = current->list;
			while(current_freq != NULL) {
				num_docs++;
				current_freq = current_freq->next;
			}
			
			if(patient_write(data->client_fd, (void *)&num_docs, sizeof(int)) < 0) {
				delete[] search_term;
				close(data->client_fd);
				cout << "Error writing document count to the client." << endl;
				pthread_exit(NULL);
			}
			
			current_freq = current->list;
			while(current_freq != NULL) {
				char *doc_name = document_names[current_freq->doc_id];
				int doc_name_len = strlen(doc_name);
				int count = current_freq->count;
				if(patient_write(data->client_fd, (void *)&doc_name_len, sizeof(int)) < 0) {
					delete[] search_term;
					close(data->client_fd);
					cout << "Error writing document name length to the client." << endl;
					pthread_exit(NULL);
				}
				if(patient_write(data->client_fd, (void *)doc_name, doc_name_len) < 0) {
					delete[] search_term;
					close(data->client_fd);
					cout << "Error writing document name to the client." << endl;
					pthread_exit(NULL);
				}
				if(patient_write(data->client_fd, (void *)&count, sizeof(int)) < 0) {
					delete[] search_term;
					close(data->client_fd);
					cout << "Error writing term frequency to the client." << endl;
					pthread_exit(NULL);
				}
				current_freq = current_freq->next;
			}
			break;
		} else {
			current = current->next;
		}
	}
	master_index_lock.unlock();
	delete[] search_term;
	
	if(!current) {
		int doc_count = 0;
		if(patient_write(data->client_fd, (void *)&doc_count, sizeof(int)) < 0) {
			close(data->client_fd);
			cout << "Error writing zero document count to client." << endl;
			pthread_exit(NULL);
		}
	}
	
	close(data->client_fd);
	delete data;
	pthread_exit(NULL);
}

int main() {
	int socket_fd = create_socket();
	listen_on_port(socket_fd, MASTER_PORT);
	int doc_count = 0;
	
	while(1) // Accept incoming requests from clients or helper registrations
    {
		sockaddr_in client_addr;
		int client = accept_client(socket_fd, &client_addr);
		int procedure;
		if(patient_read(client, &procedure, sizeof(int)) < 0) {
			cout << "Error reading procedure id from client." << endl;
			close(client);
			exit(1);
		}
		
		if(procedure == 1) { // Client document indexing request
			if(!indexers) {
				cout << "No indexers are available." << endl;
				close(client);
				exit(1);
			}
		
			pthread_t thread_id; // Not used for any purpose
			index_thread_data *data = new index_thread_data;
			data->client_fd = client;
			data->doc_id = doc_count;
			data->helper = indexers->entry;
			
			// Search for a live helper
			int ping_socket = create_socket();
			while(contact_node(ping_socket, data->helper->ip_addr, data->helper->port) < 0) {
				printf("\nFailed to connect to index helper: { ip_addr:%s, port:%d } - shuffling\n", data->helper->ip_addr, data->helper->port);
				helper_node *dead = indexers;
				indexers = indexers->next;
				delete dead->entry;
				delete dead;
				data->helper = indexers->entry;
				if(!indexers) {
					cout << "No indexers are available." << endl;
					close(client);
					exit(1);
				}
			}
			int ping_value = -1;
			patient_write(ping_socket, &ping_value, sizeof(int));
			close(ping_socket);
			
			// Shuffle head to the end of the list, round robin access
			last_indexer->next = indexers;
			indexers->previous = last_indexer;
			helper_node *head = indexers->next;
			indexers->next = NULL;
			head->previous = NULL;
			last_indexer = indexers;
			indexers = head;
			
			pthread_create(&thread_id, NULL, &thread_routine_index, (void *)data); // Create indexing thread
			doc_count++;
		} else if(procedure == 2) { // Client index search request
		
			pthread_t thread_id; // Not used for any purpose
			search_thread_data *data = new search_thread_data;
			data->client_fd = client;
			
			pthread_create(&thread_id, NULL, &thread_routine_search, (void *)data); // Create search thread
			
		} else if(procedure == 3) { // Helper registration as new indexer or searcher
			helper_registration_request *request = new helper_registration_request;
			helper_entry *entry = new helper_entry;
			
			if(patient_read(client, (void *)request, sizeof(helper_registration_request)) < 0) {
				cout << "Error reading new helper request." << endl;
				delete request;
				delete entry;
				close(client);
				exit(1);
			}
			
			helper_node **list_head = NULL;
			helper_node **last_list_node = NULL;
			if(request->service == 0) { // Indexing, create new node
				list_head = &indexers;
				last_list_node = &last_indexer;
			} else { // Searching, create new node
				list_head = &searchers;
				last_list_node = &last_searcher;
			}
			
			if(!(*list_head)) {
				(*list_head) = new helper_node;
				(*list_head)->entry = entry;
				(*list_head)->next = NULL;
				(*list_head)->previous = NULL;
				(*last_list_node) = (*list_head);
			} else {
				helper_node *new_head = new helper_node;
				new_head->next = (*list_head);
				new_head->entry = entry;
				(*list_head) = new_head;
			}
			inet_ntop(AF_INET, &(client_addr.sin_addr), entry->ip_addr, INET_ADDRSTRLEN);
			entry->port = request->port;
			delete request;
		}
	}
}