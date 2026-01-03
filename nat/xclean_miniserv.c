#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

/* JUST TO PLAY */
int ft_same(char *str1, char *str2)
{
	if (strlen(str1) != strlen(str2))
	{
		return (0);
	}
	for (int i = 0; i < (int)(strlen(str1)); i++)
	{
		if (str1[i] != str2[i])
			return (0);
	}
	return (1);
}

int ft_sendto(char *msg)
{
	return (msg[0] == '#' && msg[1] == 'S' && msg[2] == ' ');
}

/* CONNECTED CLIENT STRUCTURE */
typedef struct s_client{
	int 	fd;		// File descriptor (socket) of the client
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

/* FOR FUN */
void send_to(char *msg1, t_server *srv, int to, char *msg2, int skip)
{
	for(int i = 0; i <= srv->max_fd; i++)
	{
		if (srv->clients[i].fd > 0 && srv->clients[i].id == to)
		{
			//write(srv->clients[i].fd, msg1, strlen(msg1));
			send(srv->clients[i].fd, msg1, strlen(msg1), 0);
			for (int j = 0; j <= skip; j++)
			{
				msg2++;
			}
			send(srv->clients[i].fd, msg2, strlen(msg2), 0);
			break;
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

	if (argc != 2)
	{
		err("Wrong number of arguments\n", 1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0); //given
	if (sockfd == -1) 
	{ 
		err(NULL, 1);
	}
	bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));//htons(8081); 
  
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
	{ 
		err(NULL, 1);
	}
	if (listen(sockfd, 10) != 0) 
	{
		err(NULL, 1);
	}
/* HERE STARTS THE NEW FUNCTIONS */
	serv.max_fd = sockfd;
	FD_SET(sockfd, &serv.active_set);
	while (1)
	{
		serv.read_set = serv.write_set = serv.active_set;
		if (select(serv.max_fd + 1, &serv.read_set, &serv.write_set, NULL, NULL) < 0)
		{
			continue;
		}
		for (int fd = 0; fd <= serv.max_fd; fd++)
		{
			if (!FD_ISSET(fd, &serv.read_set))
			{
				continue;
			}
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
				serv.clients[connfd].fd = connfd;//new fd
				serv.clients[connfd].id = serv.next_id++; //assign new id (and increment next id)
				serv.clients[connfd].buf = NULL;
				FD_SET(connfd, &serv.active_set);// Add new socket in active
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
						if (ft_same(msg, "#QUIT\n"))
						{
							char bye[100];
							sprintf(bye, "server: client %d has left\n", serv.clients[fd].id);
							send_all(bye, &serv, fd);
							FD_CLR(fd, &serv.active_set);
							close(fd);
							if (serv.clients[fd].buf)
								free(serv.clients[fd].buf);
							serv.clients[fd].buf = NULL;
							serv.clients[fd].fd = 0;
							free(msg);
							break;

						}
						if (ft_sendto(msg))
						{
							int  	destiny;
							int 	count = 3;
							char	*num = malloc(5);
							while (msg[count] != ' ' && count < 7)
							{
								num[count - 3] = msg[count];
								count++;
							}
							num[count - 3] = 0;
							destiny = atoi(num);
							free(num);
							char  prefix[50];
							sprintf(prefix, "Message from client %d: ", serv.clients[fd].id);
							send_to(prefix, &serv, destiny, msg, count);
							free(msg);
						}
						else
						{
							char  prefix[50];
							sprintf(prefix, "client %d: ", serv.clients[fd].id);
							char *output = malloc(strlen(prefix) + strlen(msg) + 1);
							if (!output)
							{
								err(NULL, 1);
							}
							output[0] = 0;
							sprintf(output, "%s%s", prefix, msg);
							send_all(output, &serv, fd);
							free(output);
							free(msg);
						}
					}

				}
			}
		}
	}

}
