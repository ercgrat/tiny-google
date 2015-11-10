// helpers.h

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