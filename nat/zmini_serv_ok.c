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
 * CLIENT STRUCTURE AND GLOBAL VARIABLES (NOT GIVEN)
 ******************************************************************************/

// Structure to represent a connected client
typedef struct s_client {
	int	fd;      // File descriptor (socket) for this client
	int	id;      // Unique identifier assigned to this client
	char *buf;   // Buffer to accumulate incomplete messages from this client
} t_client;

// Global array to store all client connections (max 1024)
t_client	clients[1024];

// Highest file descriptor number (needed for select() system call)
int max_fd = 0;

// Counter to assign unique IDs to new clients
int next_id = 0;

// File descriptor sets for select() multiplexing
fd_set read_set;    // Set of file descriptors to check for reading
fd_set write_set;   // Set of file descriptors to check for writing
fd_set active_set;  // Master set of all active file descriptors

/******************************************************************************
 * ERROR HANDLING (NOT GIVEN)
 ******************************************************************************/

// Print error message to stderr and exit with specified code
void	err(char *msg, int exit_code) {
	if (!msg)
		msg = "Fatal error\n";
	write(2, msg, strlen(msg));  // Write to stderr (fd 2)
	exit(exit_code);
}

/******************************************************************************
 * BROADCAST MESSAGING (NOT GIVEN)
 ******************************************************************************/

// Send a message to all connected clients except one (typically the sender)
void	send_all(int except, char *msg) {
	for (int i = 0; i <= max_fd; i++)
		// Only send if it's a valid client and not the excluded one
		if (clients[i].fd > 0 && i != except)
			send(clients[i].fd, msg, strlen(msg), 0);
}

/******************************************************************************
 * MESSAGE PARSING (GIVEN)
 ******************************************************************************/

// Extract one complete message (ending with '\n') from the buffer
// Returns: 1 if message extracted, 0 if no complete message, -1 on error
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	
	i = 0;
	// Search for newline character
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			// Allocate space for remaining buffer content after the newline
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			
			// Copy remaining content to new buffer
			strcpy(newbuf, *buf + i + 1);
			
			// Current buffer becomes the extracted message
			*msg = *buf;
			(*msg)[i + 1] = 0;  // Null-terminate after the newline
			
			// Update buffer pointer to new buffer
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);  // No complete message found
}

/******************************************************************************
 * STRING CONCATENATION  (GIVEN)
 ******************************************************************************/

// Concatenate two strings, allocating new memory
// Frees the original 'buf' pointer
char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	// Get length of existing buffer
	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	
	// Allocate space for combined string
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	
	newbuf[0] = 0;
	
	// Copy old buffer content
	if (buf != 0)
		strcat(newbuf, buf);
	
	free(buf);  // Free old buffer
	
	// Append new content
	strcat(newbuf, add);
	return (newbuf);
}

/******************************************************************************
 * MAIN SERVER FUNCTION //G = Given //X Given but not needed (thus commented) 
 ******************************************************************************/

int main(int ac, char **av) { 
	int sockfd, connfd;//G
	struct sockaddr_in servaddr;//G
	
	/***********************************************************************
	* NEW TO ADD: Check command line arguments (need port number)
	***********************************************************************/
	if (ac != 2) {
		err("Wrong number of arguments\n", 1);
	}
	/***********************************************************************
	* END OF NEW BLOCK
	***********************************************************************/

	/**************************************************************************
	 * SOCKET CREATION AND SETUP
	 **************************************************************************/
	
	// Create TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //G
	if (sockfd == -1) { //G
		//printf("socket creation failed...\n");//X
		//exit(0);//X
		err(NULL, 1);//G
	}
	
	// Initialize max_fd with server socket
	max_fd = sockfd;//G
	
	// Zero out the server address structure
	bzero(&servaddr, sizeof(servaddr)); //G

	// Configure server address
	servaddr.sin_family = AF_INET; //G
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1 (localhost)//G
	/***********************************************************************
	* MODIFIED FROM ORIGINAL MAIN
	************************************************************************/
	//Xservaddr.sin_port = htons(8081); 
	servaddr.sin_port = htons(atoi(av[1]));       // Port from command line
  /***********************************************************************
	* END OF NEW BLOCK
	***********************************************************************/

	// Bind socket to address and port
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { //G
		//printf("socket bind failed...\n");//X
		//exit(0);//X
		err(NULL, 1);
	}
	//else //X
		//printf("Socket successfully binded..\n");//X
	
	// Start listening for connections (queue up to 10)
	if (listen(sockfd, 10) != 0) {//G
		//printf("cannot listen\n");//X 
		//exit(0);//X
		err(NULL, 1);
	}
	
	/**************************************************************************
	 * INITIALIZE SELECT() STRUCTURES
	 **************************************************************************/
	
	// Clear the active file descriptor set
	FD_ZERO(&active_set);
	
	// Add server socket to active set
	FD_SET(sockfd, &active_set);
	
	// Zero out the clients array
	bzero(clients, sizeof(clients));
	
	/**************************************************************************
	 * MAIN EVENT LOOP
	 **************************************************************************/
	
	while (1) {
		// Copy active set to read/write sets (select() modifies them)
		read_set = write_set = active_set;
		
		// Wait for activity on any socket
		if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
			continue;  // Error in select, try again
		
		// Check each file descriptor for activity
		for (int fd = 0; fd <= max_fd; fd++) {
			// Skip if this fd is not ready for reading
			if (!FD_ISSET(fd, &read_set)) {
				continue;
			}
			
			/******************************************************************
			 * HANDLE NEW CLIENT CONNECTION
			 ******************************************************************/
			if (fd == sockfd) {
				// Accept new connection
				connfd = accept(sockfd, NULL, NULL);
				if (connfd < 0) 
					continue;  // Accept failed, continue to next iteration
				
				// Update max_fd if necessary
				if (connfd > max_fd) 
					max_fd = connfd;
				
				// Initialize new client structure
				clients[connfd].fd = connfd;
				clients[connfd].id = next_id++;
				clients[connfd].buf = NULL;
				
				// Add new client to active set
				FD_SET(connfd, &active_set);
				
				// Notify all other clients about new arrival
				char msg[100];
				sprintf(msg, "server: client %d just arrived\n", clients[connfd].id);
				send_all(connfd, msg);
			}
			/******************************************************************
			 * HANDLE CLIENT DATA OR DISCONNECTION
			 ******************************************************************/
			else {
				char buf[1024];
				
				// Receive data from client
				int r = recv(fd, buf, sizeof(buf) - 1, 0);
				
				// Client disconnected or error
				if (r <= 0) {
					// Notify all clients about departure
					char msg[100];
					sprintf(msg, "server: client %d just left\n", clients[fd].id);
					send_all(fd, msg);
					
					// Clean up client resources
					FD_CLR(fd, &active_set);  // Remove from active set
					close(fd);                 // Close socket
					free(clients[fd].buf);     // Free message buffer
					clients[fd].fd = 0;        // Mark as unused
				}
				// Data received from client
				else {
					buf[r] = 0;  // Null-terminate received data
					
					// Append received data to client's buffer
					clients[fd].buf = str_join(clients[fd].buf, buf);
					if (!clients[fd].buf)
						err(NULL, 1);
					
					// Extract and broadcast all complete messages
					char *msg;
					while (extract_message(&clients[fd].buf, &msg) == 1) {
						// Create message with client ID prefix
						char prefix[50];
						sprintf(prefix, "client %d: ", clients[fd].id);
						
						char *full = malloc(strlen(prefix) + strlen(msg) + 1);
						if (!full)
							err(NULL, 1);
						
						full[0] = 0;
						strcat(full, prefix);
						strcat(full, msg);
						
						// Broadcast to all other clients
						send_all(fd, full);
						
						// Clean up
						free(full);
						free(msg);
					}
				}
			}
		}
	}
}
/*******************************************************************
* COMMENT THE REMAINING GIVEN MAIN (numbers = num line in original)*
********************************************************************
86     len = sizeof(cli);
87     connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
88     if (connfd < 0) {
89         printf("server acccept failed...\n");
90         exit(0);
91     }
92     else
93         printf("server acccept the client...\n");
94 }

