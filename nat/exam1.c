#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

typedef struct s_client{
	int		fd;
	int		id;
	char	*buf;
} t_client;

typedef struct s_server{
	t_client	clients[1024];
	int			max_fd;
	int			next_id;
	fd_set		read_set;
	fd_set		write_set;
	fd_set		active_set;
} t_server;

void err(char *msg)
{
	if (!msg)
		msg = "Fatal error\n";
	write(2, msg, strlen(msg));
	exit(1);
}

void send_all(char *msg, t_server *serv, int except)
{
	for (int i = 0; i <= serv->max_fd; i++)
	{
		if (serv->clients[i].fd > 0 && i != except)
			send(serv->clients[i].fd, msg, strlen(msg), 0);
	}
}

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


int main(int ac, char **av) 
{
	int sockfd, connfd;//, len;
	struct sockaddr_in servaddr;//, cli;
	t_server	serv = {0};

	if (ac != 2)
	{
		err("Wrong number of arguments\n");
	}

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		//printf("socket creation failed...\n"); 
		//exit(0);
		err(NULL);
	}
	/*
	else
		printf("Socket successfully created..\n");
	*/
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));//htons(8081); // <= originally given // atoi may fail but we don't need to handle it
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		//printf("socket bind failed...\n"); 
		//exit(0); 
		err(NULL);
	}
	/*
	else
		printf("Socket successfully binded..\n");
	*/
	if (listen(sockfd, 10) != 0) {
		//printf("cannot listen\n"); 
		//exit(0); 
		err(NULL);
	}
	// ALL new from here
	serv.max_fd = sockfd;// include new socket as max
	FD_SET(sockfd, &serv.active_set);// include new socket in active
	// initiate loop
	while (1)
	{
		// initialize fd_sets
		serv.read_set = serv.write_set = serv.active_set;
		// select loop if none active restrat the loop
		if (select(serv.max_fd + 1, &serv.read_set, &serv.write_set, NULL, NULL) < 0)
		{
			continue;
		}
		// check fds
		for (int fd = 0; fd <= serv.max_fd; fd++)
		{
			if (!FD_ISSET(fd, &serv.read_set)) // the fd does not belong to the set group
			{
				continue;
			}
			if (fd == sockfd) // new client from nc 
			{
				connfd = accept(sockfd, NULL, NULL);// we accept the new connection
				if (connfd < 0)// Skip this fd. Go back to the select() loop
				{
					continue;
				}
				if (connfd > serv.max_fd)// assignation depends on open fds
				{
					serv.max_fd = connfd;
				}
				// new client
				serv.clients[connfd].fd = connfd;// The fd is the created in connfd = accept(sockfd, NULL, NULL)
				serv.clients[connfd].id = serv.next_id++;// updates and expands next_id
				serv.clients[connfd].buf = NULL;// Sets buf to NULL
				FD_SET(connfd, &serv.active_set); // include the socket created from accept to the active set
				// build the welcome message and broadcast it:
				char msg[100];
				sprintf(msg, "server: client %d just arrived\n", serv.clients[connfd].id);
				send_all(msg, &serv, connfd);
			}
			else // fd active different from socket
			{
				char	buff[1024];// to store the incomming message
				int		r = recv(fd, buff, sizeof(buff) - 1, 0);// reading fron fd, storing to buff
				if (r <= 0)// r == 0 client disconnected gracefully (TCP FIN); r < 0 error occurred while reception, thus disconnected
				{
					char	msg[100];// Generous but safe 30 is fair enough
					sprintf(msg, "server: client %d just left\n", serv.clients[fd].id);
					send_all(msg, &serv, fd);
					FD_CLR(fd, &serv.active_set);// Removes fd from active_set 
					close(fd);// close fd
					free(serv.clients[fd].buf); // Free buff if any
					serv.clients[fd].fd = 0;// Set the client fd to 0
				}
				else // > 0 received that many bytes of data
				{
					// 1) char    buff[1024] is written from r = recv(fd, buff, sizeof(buff) - 1, 0).
					buff[r] = 0;// 2) closes the string buff stored from recv => sizeof(buff) - 1
					serv.clients[fd].buf = str_join(serv.clients[fd].buf, buff);// 3) Appends the received message to serv.clients[fd].buf
					if (!serv.clients[fd].buf)// 3) Protection to if join fails
						err(NULL);
					char	*msg;// 4) temporary char *
					while(extract_message(&serv.clients[fd].buf, &msg) > 0)// 5) Extracts the line breaks to print each line on the screen
					{
						char	prefix[50];// 6) generous — "client 1023: " is only 13 chars. 50 is safe but wasteful.
						sprintf(prefix, "client %d: ", serv.clients[fd].id);// 7) Initializes prefix with 'client num: '
						char	*output = malloc(strlen(prefix) + strlen(msg) + 1);// 8) Message to print = prefix + msg + 1
						if (!output)
							err(NULL);// 9) Protection to fail
						output[0] = 0;// 10) Closes the message to print
						sprintf(output, "%s%s", prefix, msg);// 11) Assigns prefix + msg to output
						send_all(output, &serv, fd);// 12) broadcast to all but the sender
						free(output);// free malloc created char * output
						free(msg);// free msg => malloc updated in extract_message
					}
				}
			}
		}
	}
	// end of new
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
