#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************
 * CLIENT STRUCTURE
 ******************************************************************************/

// Structure to represent a connected client
typedef struct s_client {
	int	fd;      // File descriptor (socket) for this client
	int	id;      // Unique identifier assigned to this client
	char *buf;   // Buffer to accumulate incomplete messages from this client
} t_client;

/******************************************************************************
 * SERVER STATE STRUCTURE (replaces global variables)
 ******************************************************************************/

typedef struct s_server {
	t_client clients[1024];  // Array to store all client connections
	int max_fd;              // Highest file descriptor number
	int next_id;             // Counter for assigning unique IDs
	fd_set read_set;         // Set of fds to check for reading
	fd_set write_set;        // Set of fds to check for writing
	fd_set active_set;       // Master set of all active fds
} t_server;

/******************************************************************************
 * ERROR HANDLING
 ******************************************************************************/

// Print error message to stderr and exit with specified code
void err(char *msg, int exit_code) {
	if (!msg)
		msg = "Fatal error\n";
	write(2, msg, strlen(msg));
	exit(exit_code);
}

/******************************************************************************
 * BROADCAST MESSAGING
 ******************************************************************************/

// Send a message to all connected clients except one (typically the sender)
void send_all(t_server *srv, int except, char *msg) {
	for (int i = 0; i <= srv->max_fd; i++)
		if (srv->clients[i].fd > 0 && i != except)
			send(srv->clients[i].fd, msg, strlen(msg), 0);
}

/******************************************************************************
 * MESSAGE PARSING
 ******************************************************************************/

// Extract one complete message (ending with '\n') from the buffer
// Returns: 1 if message extracted, 0 if no complete message, -1 on error
int extract_message(char **buf, char **msg)
{
	char *newbuf;
	int i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

/******************************************************************************
 * STRING CONCATENATION
 ******************************************************************************/

// Concatenate two strings, allocating new memory
// Frees the original 'buf' pointer
char *str_join(char *buf, char *add)
{
	char *newbuf;
	int len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

/******************************************************************************
 * SERVER INITIALIZATION
 ******************************************************************************/

// Initialize server state structure
void init_server(t_server *srv, int sockfd) {
	srv->max_fd = sockfd;
	srv->next_id = 0;
	FD_ZERO(&srv->active_set);
	FD_SET(sockfd, &srv->active_set);
	bzero(srv->clients, sizeof(srv->clients));
}

/******************************************************************************
 * CLIENT CONNECTION HANDLING
 ******************************************************************************/

// Handle new client connection
void handle_new_client(t_server *srv, int sockfd) {
	int connfd = accept(sockfd, NULL, NULL);
	if (connfd < 0)
		return;
	
	// Update max_fd if necessary
	if (connfd > srv->max_fd)
		srv->max_fd = connfd;
	
	// Initialize new client structure
	srv->clients[connfd].fd = connfd;
	srv->clients[connfd].id = srv->next_id++;
	srv->clients[connfd].buf = NULL;
	
	// Add new client to active set
	FD_SET(connfd, &srv->active_set);
	
	// Notify all other clients about new arrival
	char msg[100];
	sprintf(msg, "server: client %d just arrived\n", srv->clients[connfd].id);
	send_all(srv, connfd, msg);
}

/******************************************************************************
 * CLIENT DISCONNECTION HANDLING
 ******************************************************************************/

// Handle client disconnection
void handle_client_disconnect(t_server *srv, int fd) {
	// Notify all clients about departure
	char msg[100];
	sprintf(msg, "server: client %d just left\n", srv->clients[fd].id);
	send_all(srv, fd, msg);
	
	// Clean up client resources
	FD_CLR(fd, &srv->active_set);
	close(fd);
	free(srv->clients[fd].buf);
	srv->clients[fd].fd = 0;
}

/******************************************************************************
 * CLIENT DATA HANDLING
 ******************************************************************************/

// Handle data received from client
void handle_client_data(t_server *srv, int fd) {
	char buf[1024];
	int r = recv(fd, buf, sizeof(buf) - 1, 0);
	
	// Client disconnected or error
	if (r <= 0) {
		handle_client_disconnect(srv, fd);
		return;
	}
	
	// Data received
	buf[r] = 0;
	srv->clients[fd].buf = str_join(srv->clients[fd].buf, buf);
	if (!srv->clients[fd].buf)
		err(NULL, 1);
	
	// Extract and broadcast all complete messages
	char *msg;
	while (extract_message(&srv->clients[fd].buf, &msg) == 1) {
		char prefix[50];
		sprintf(prefix, "client %d: ", srv->clients[fd].id);
		
		char *full = malloc(strlen(prefix) + strlen(msg) + 1);
		if (!full)
			err(NULL, 1);
		
		full[0] = 0;
		strcat(full, prefix);
		strcat(full, msg);
		
		send_all(srv, fd, full);
		
		free(full);
		free(msg);
	}
}

/******************************************************************************
 * MAIN SERVER FUNCTION
 ******************************************************************************/

int main(int ac, char **av) { 
	int sockfd;
	struct sockaddr_in servaddr;
	t_server srv;  // Server state on stack instead of globals
	
	// Check command line arguments
	if (ac != 2) {
		err("Wrong number of arguments\n", 1);
	}

	// Create TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		err(NULL, 1);
	}
	
	bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));
  
	// Bind socket to address
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		err(NULL, 1);
	}
	
	// Start listening
	if (listen(sockfd, 10) != 0) {
		err(NULL, 1);
	}
	
	// Initialize server state
	init_server(&srv, sockfd);
	
	/**************************************************************************
	 * MAIN EVENT LOOP
	 **************************************************************************/
	
	while (1) {
		// Copy active set to read/write sets
		srv.read_set = srv.write_set = srv.active_set;
		
		// Wait for activity on any socket
		if (select(srv.max_fd + 1, &srv.read_set, &srv.write_set, NULL, NULL) < 0)
			continue;
		
		// Check each file descriptor for activity
		for (int fd = 0; fd <= srv.max_fd; fd++) {
			if (!FD_ISSET(fd, &srv.read_set))
				continue;
			
			// Handle new connection or client data
			if (fd == sockfd)
				handle_new_client(&srv, sockfd);
			else
				handle_client_data(&srv, fd);
		}
	}
	
	return 0;
}
