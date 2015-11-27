// helpers.h

int create_socket() {
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		cout << "Error creating socket." << endl;
		exit(1);
	}
	return socket_fd;
}

int contact_node(int socket_fd, const char *ip_addr, int port) {
	sockaddr_in node;
	memset(&node, '0', sizeof(node));
	node.sin_family = AF_INET;
	node.sin_port = htons(port);
	inet_pton(AF_INET, ip_addr, &(node.sin_addr));
	return connect(socket_fd, (sockaddr *)&node, sizeof(node));
}

void listen_on_port(int socket_fd, int port) {
	sockaddr_in sockaddr;
	memset(&sockaddr, '0', sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(socket_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
		printf("Error binding socket to port %d.\n", port);
		exit(1);
	}
	listen(socket_fd, 100);
}

int accept_client(int socket_fd, sockaddr_in *client_addr) {
	socklen_t client_len = sizeof(sockaddr_in);
	int client_fd = accept(socket_fd, (struct sockaddr*)client_addr, &client_len);
	return client_fd;
}

int patient_write(int socket_fd, void *data, int data_size) {
	int bytes_written = write(socket_fd, ((char *)data), data_size);
	while(bytes_written < data_size) {
		if(bytes_written < 0) {
			cout << "Error writing data to the socket." << endl;
			return -1;
		}
		bytes_written = bytes_written  + write(socket_fd, ((char *)data) + bytes_written, data_size - bytes_written);
	}
	return bytes_written;
}

int patient_read(int socket_fd, void *data, int data_size) {
	int bytes_read = read(socket_fd, ((char *)data), data_size);
	while(bytes_read < data_size) {
		if(bytes_read < 0) {
			cout << "Error reading data from the socket." << endl;
			return -1;
		}
		bytes_read = bytes_read + read(socket_fd, ((char *)data) + bytes_read, data_size - bytes_read);
	}
	return bytes_read;
}