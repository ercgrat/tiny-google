// master.h

const char *MASTER_IP_ADDR = "127.0.0.1";
const int MASTER_PORT = 3000;

struct helper_registration_request {
	int service;
	unsigned short port;
};

struct helper_entry {
	unsigned short port;
	char ip_addr[INET_ADDRSTRLEN];
	int dead;
};

struct helper_node {
	helper_entry *entry;
	helper_node *next;
	helper_node *previous;
};

struct index_thread_data {
	int client_fd;
	int doc_id;
	helper_entry *helper;
};

struct search_thread_data {
	int client_fd;
};

struct retrieve_thread_data {
	int client_fd;
};