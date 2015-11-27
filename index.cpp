using namespace std;
#include <iostream>
#include <cstdlib>
#include <cstring>

#include "index.h"

freq_node* find_freq_node(term_node **head, char *term) { // Searches for node with term, or creates one
	if(!(*head)->term) { // List is empty
		(*head)->term = new char[strlen(term) + 1];
		strcpy((*head)->term, term);
		(*head)->list = new freq_node;
		(*head)->list->count = 0;
		return (*head)->list;
	}
	
	term_node *previous = NULL;
	term_node *current = *head;
	
	while(1) {
		if(!current) { // Reached the end of the list
			previous->next = new term_node;
			previous->next->term = new char[strlen(term) + 1];
			strcpy(previous->next->term, term);
			previous->next->list = new freq_node;
			previous->next->list->count = 0;
			previous->next->next = NULL;
			return previous->next->list;
		}
		
		if(strcmp(current->term, term) == 0) { // Found a node that already has the term
			return current->list;
		} else if(strcmp(current->term, term) > 0) {
			if(previous) { // Term is < current term, append new node to previous
				previous->next = new term_node;
				previous->next->term = new char[strlen(term) + 1];
				strcpy(previous->next->term, term);
				previous->next->next = current;
				previous->next->list = new freq_node;
				previous->next->list->count = 0;
				return previous->next->list;
			} else { // Term is < head term, create new head
				term_node *new_head = new term_node;
				new_head->term = new char[strlen(term) + 1];
				strcpy(new_head->term, term);
				new_head->next = current;
				new_head->list = new freq_node;
				new_head->list->count = 0;
				*head = new_head;
				return new_head->list;
			}
		}
		previous = current;
		current = current->next;
	}
	
	return NULL; // Should never reach this point
}

term_node* index_document(char *document) {
	term_node *head = new term_node;
	head->term = NULL;
	head->next = NULL;
	
	int pos = 0;
	int last_term = 0;
	int doc_len = strlen(document);
	
	while(pos < doc_len) { // Search document for terms
		char c = document[pos];
		if(c == ' ' || c == '\n' || c == '\t' || c == '\r' || pos == doc_len - 1) { // Reached space; extract token and add count
			if(pos == last_term) { // Skip sequences of spaces/tabs/newlines
				last_term++;
			} else {
				char *term = new char[(pos - last_term) + 1];
				memset(term, 0, (pos - last_term) + 1);
				strncpy(term, document + last_term, pos - last_term);
				
				last_term = pos + 1;
				
				freq_node *node = find_freq_node(&head, term);
				delete term;
				
				node->count = node->count + 1;
				node->doc_id = 0;
			}
		}
		pos++;
	}
	
	return head;
}

void merge_partial_index(term_node **master, term_node *partial) {
	
	// If master does not exist, assign the partial as the master
	if(!(*master)) {
		*master = partial;
		return;
	}
	
	term_node *parent = NULL;
	term_node *current = *master;
	term_node *node_to_add = partial;
	
	while(node_to_add != NULL) {
		// Term already in the index, delete the partial index term node (node_to_add) and insert its frequency node
		if(strcmp(current->term, node_to_add->term) == 0) {
			freq_node *current_freq = current->list;
			freq_node *freq_to_add = node_to_add->list;
			term_node *temp_node_to_add = node_to_add->next;
			delete node_to_add;
			node_to_add = temp_node_to_add;
			
			// Frequency being added is greatest, just set as head of freq_node list
			if(freq_to_add->count > current_freq->count) {
				current->list = freq_to_add;
				freq_to_add->next = current_freq;
			} else { // Search for the correct position of the frequency node
				freq_node *parent_freq = current_freq;
				current_freq = current_freq->next;
				while(1) {
					if(!current_freq) {
						parent_freq->next = freq_to_add;
						break;
					} else if(freq_to_add->count > current_freq->count) {
						parent_freq->next = freq_to_add;
						freq_to_add->next = current_freq;
						break;
					} else {
						parent_freq = current_freq;
						current_freq = current_freq->next;
					}
				}
			}
		} else if(strcmp(current->term, node_to_add->term) > 0) { // Term being added not in the index yet, found the correct position for it
			if(parent) {
				parent->next = node_to_add;
				node_to_add = node_to_add->next;
				parent->next->next = current;
			} else {
				*master = node_to_add;
				node_to_add = node_to_add->next;
				parent = *master;
				(*master)->next = current;
			}
		} else if(current->next == NULL) { // Reached end of the list, just append the remaining nodes from the partial index
			current->next = node_to_add;
			return;
		} else { // Have not found the right alphabetical position or matching term yet
			parent = current;
			current = current->next;
		}
	}
}

void print_index(term_node *master) {
	term_node *current = master;
	while(current != NULL) {
		freq_node *current_freq = current->list;
		cout << current->term << ": ";
		while(current_freq != NULL) {
			cout << "(" << current_freq->doc_id << ", " << current_freq->count << ")~> ";
			current_freq = current_freq->next;
		}
		cout << endl;
		current = current->next;
	}
}