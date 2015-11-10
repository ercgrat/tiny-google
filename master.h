// master.h

struct google_request {
	int procedure;
	char *args[];
};

const char *MASTER_IP_ADDR = "127.0.0.1";
const int MASTER_PORT = 3000;