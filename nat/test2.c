#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
//new 3 libs
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

/* CONNECTED CLIENT STRUCTURE */
typedef struct s_client{
	int 	fd;		// Flie descriptor (socket) of the client)
	int 	id;		// Unique identifier for the client
	char 	*buf;	// buffer to accumulate the message
} t_client;

/* SERVER STATE STRUCTURE */
typedef struct s_server{
	t_client	clients[1024];	// Array of clients
	int			max_fd;			// Highest fd 
	int			next_id;		// Counter of unique IDs
	fd_set		read_set;		// Set of fds to check for reading
	fd_set		write_set;		// Set of fds to check for writting
	fd_set		active_set;		// Set of active fds
} t_server;

/* SERVER INITIALIZATION */
void init_server(t_server *srv, int sock_fd)
{
	srv->max_fd = sock_fd;
	FD_SET(sock_fd, &srv->active_set);
}

/* ERROR HANDLING & EXIT CODE */
void err (char *msg, int exit_code)
{
	if (!msg)
		msg = "Fatal error\n";
	write(2, msg, strlen(msg));
	exit(exit_code);
}

/* BROADCAST FUNCTION*/
void send_all(char *msg, t_server *srv, int except)
{
	for (int i = 0; i <= srv->max_fd; i++)
	{
		if (srv->clients[i].fd > 0 && i != except)
		{
			send(srv->clients[i].fd, msg, strlen(msg), 0);
		}
	}
}

/* Given OK function */
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

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

/* Given OK function */
char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

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

/*
main to modify
*/
int main(int argc, char **argv) 
{
	int 				sockfd, connfd;//, len;
	struct sockaddr_in	servaddr;//, cli; 
	t_server			serv = {0}; //new =>  put to zero what possible

	//1 verification of args
	if (argc != 2)
	{
		err("Wrong number of arguments\n", 1);
	}

	//socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //given
	if (sockfd == -1) 
	{ 
		//printf("socket creation failed...\n");
		//exit(0);
		err(NULL, 1);
	}
	/*
	else
		printf("Socket successfully created..\n"); 
	*/
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));//htons(8081); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
	{ 
		//printf("socket bind failed...\n"); 
		//exit(0); 
		err(NULL, 1);
	}
	/*
	else
		printf("Socket successfully binded..\n");
	*/
	if (listen(sockfd, 10) != 0) 
	{
		//printf("cannot listen\n"); 
		//exit(0); 
		err(NULL, 1);
	}
/* HERE STARTS THE NEW FUNCTIONS */
	//1 INIT SERVER
	init_server(&serv, sockfd);
	//2 MAIN LOOP
	while (1)
	{
		//2.1 Copy active set to read & write sets
		serv.read_set = serv.write_set = serv.active_set;
		//2.2 Wait for activity
		if (select(serv.max_fd + 1, &serv.read_set, &serv.write_set, NULL, NULL) < 0)
		{
			continue;
		}
		//2.3 Check each file descriptor for activity
		for (int fd = 0; fd <= serv.max_fd; fd++)
		{
			if (!FD_ISSET(fd, &serv.read_set))
			{
				continue;
			}
			//2.3.1 New client
			if (fd == sockfd)
			{
				connfd = accept(sockfd, NULL, NULL);
				if (connfd < 0)// Accept failed
				{
					continue;
				}
				if (connfd > serv.max_fd)
				{
					serv.max_fd = connfd;
				}
				// 2.3.1.1 Initialize new client
				serv.clients[connfd].fd = connfd;//new fd
				serv.clients[connfd].id = serv.next_id++; //assigned new id (and increment next id in server
				serv.clients[connfd].buf = NULL;
				FD_SET(connfd, &serv.active_set);// Add new socket in active
				//2.3.1.2 Create message and broadcast it
				char msg[100]; //new variable
				sprintf(msg, "server: client %d just arrived\n", serv.clients[connfd].id);
				send_all(msg, &serv, connfd);
			}
			//2.3.2 Existing client
			else
			{
				char 	buf[1024];
				int 	r = recv(fd ,buf, sizeof(buf) - 1, 0);// check if there was a message
				if (r <= 0) // nothing received => client has left
				{
					char msg[100];
					sprintf(msg, "server: client %d has left\n", serv.clients[fd].id);
					send_all(msg, &serv, fd);
					// 2.3.2.0 Clear resources
					FD_CLR(fd, &serv.active_set);
					close(fd);
					free(serv.clients[fd].buf);
					serv.clients[fd].fd = 0;
				}
				else //something received
				{
					buf[r] = 0; //close string
					serv.clients[fd].buf = str_join(serv.clients[fd].buf, buf);
					if (!serv.clients[fd].buf) // reading failed
					{
						err(NULL, 1);
					}
					//2.3.2.1 extract message and broadcast it
					char *msg;
					while(extract_message(&serv.clients[fd].buf, &msg) > 0)
					{
						char  prefix[50];
						sprintf(prefix, "client %d: ", serv.clients[fd].id);
						char *output = malloc(strlen(prefix) + strlen(msg) + 1);
						if (!output)
						{
							err(NULL, 1);
						}
						output[0] = 0;
						strcat(output, prefix);
						strcat(output, msg);
						send_all(output, &serv, fd);
						free(output);
						free(msg);
					}
				}
			}
		}
	}

/*	
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n");
*/
}
