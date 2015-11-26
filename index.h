struct freq_node {
	int count;
	int doc_id;
	freq_node *next;
};

struct term_node {
	char *term;
	freq_node *list;
	term_node *next;
};

term_node* index_document(char *document);