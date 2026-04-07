#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

typedef struct s_client{
	int		fd;
	int		id;
	char	*buff;
} t_client;

typedef struct s_server{
	t_client	clients[1024];
	int			max_fd;
	int			next_id;
	fd_set		write_set;
	fd_set		read_set;
	fd_set		active_set;
} t_server;

void my_err(char *msg)
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
	int 				sockfd, connfd;//, len;
	struct sockaddr_in	servaddr;//, cli; 
	t_server			serv = {0};

	if (ac != 2)
	{
		my_err("Wrong number of arguments\n");
	}

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		//printf("socket creation failed...\n"); 
		//exit(0);
		my_err(NULL);
	} 
	/*
	else
		printf("Socket successfully created..\n"); 
	*/
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));//htons(8081); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		//printf("socket bind failed...\n"); 
		//exit(0); 
		my_err(NULL);
	}
	/*
	else
		printf("Socket successfully binded..\n");
	*/
	if (listen(sockfd, 10) != 0) {
		//printf("cannot listen\n"); 
		//exit(0); 
		my_err(NULL);
	}
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
				if (connfd < 0)
				{
					continue;
				}
				if (connfd > serv.max_fd)
				{
					serv.max_fd = connfd;
				}
				serv.clients[connfd].fd = connfd;
				serv.clients[connfd].id = serv.next_id++;
				serv.clients[connfd].buff = NULL;
				FD_SET(connfd, &serv.active_set);
				char 	hello[100];
				sprintf(hello, "server: client %d just arrived\n", serv.clients[connfd].id);
				send_all(hello, &serv, connfd);
			}
			else
			{
				char	buff[1024];
				int		r = recv(fd, buff, sizeof(buff) - 1, 0);
				if (r <= 0)
				{
					char	bye[100];
					sprintf(bye, "server: client %d just left\n", serv.clients[fd].id);
					send_all(bye, &serv, fd);
					FD_CLR(fd, &serv.active_set);
					close(fd);
					free(serv.clients[fd].buff);
					serv.clients[fd].fd = 0;
				}
				else
				{
					buff[r] = 0;
					serv.clients[fd].buff = str_join(serv.clients[fd].buff, buff);
					if (!serv.clients[fd].buff)
						my_err(NULL);
					char	*msg;
					while (extract_message(&serv.clients[fd].buff, &msg) > 0)
					{
						char	pref[50];
						sprintf(pref, "client %d: ", serv.clients[fd].id);
						char	*output = malloc(strlen(pref) + strlen(msg) + 1);
						if (!output)
							my_err(NULL);
						output[0] = '\0';
						sprintf(output, "%s%s", pref, msg);
						send_all(output, &serv, fd);
						free(msg);
						free(output);
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
