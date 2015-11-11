// helpers.h

int contact_node(const char *ip_addr, int port) {
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		cout << "Error creating socket." << endl;
		exit(1);
	}
	sockaddr_in node;
	memset(&node, '0', sizeof(node));
	node.sin_family = AF_INET;
	node.sin_port = htons(port);
	inet_pton(AF_INET, ip_addr, &(node.sin_addr));
	if(connect(socket_fd, (sockaddr *)&node, sizeof(node)) < 0) {
		printf("\nError: Failed to connect to node: { ip_addr:%s, port:%d }\n", ip_addr, port);
		exit(1);
	}
	return socket_fd;
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