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
	fd_set		active;
	fd_set		write;
	fd_set		read;
}t_server;

void err(char *msg){
	if (!msg)
		msg = "Fatal error\n";
	write(2, msg, strlen(msg));
	exit (1);
}

void broadcast(char *msg, t_server *serv, int nop){
	for(int i = 0; i <= serv->max_fd; i++){
		if (serv->clients[i].fd > 0 && serv->clients[i].id != nop)
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


int main(int ac, char **av) {
	int 				sockfd, connfd;//, len;
	struct sockaddr_in 	servaddr;//, cli;
	t_server			server ={0};

	if (ac != 2){
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
	servaddr.sin_port = htons(atoi(av[1]));//htons(8081); 
  
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
	server.max_fd = sockfd;
	FD_SET(sockfd, &server.active);
	///FD_SET(STDIN_FILENO, &server.active);
	while (1)
	{
		server.write = server.read = server.active;
		if (select(server.max_fd + 1, &server.read, &server.write, NULL, NULL) < 0)
		{
			continue;
		}
		for (int fd = 0; fd <= server.max_fd; fd++)
		{
			if (!FD_ISSET(fd, &server.read))
			{
				continue;
			}
			if (fd == sockfd)
			{
				connfd = accept(fd, NULL, NULL);
				if (connfd < 0)
				{
					continue;
				}
				if (connfd > server.max_fd)
				{
					server.max_fd = connfd;
					//printf("New kid on the block fd is %d, connfd is %d\n", fd, connfd);
				}
				FD_SET(connfd, &server.active);
				server.clients[connfd].fd = connfd;
				server.clients[connfd].id = server.next_id++;
				server.clients[connfd].buff = NULL;
				char	hello[50];
				sprintf(hello, "server: client %d just arrived\n", server.clients[connfd].id);
				broadcast(hello, &server, server.clients[connfd].id);
			}
			else
			{
				char	add[1024];
				int r = recv(fd, add, sizeof(add) - 1, 0);
				if (r <= 0)
				{
					char	bye[50];
					sprintf(bye, "server: client %d just left\n", server.clients[fd].id);
					broadcast(bye, &server, server.clients[fd].id);
					FD_CLR(server.clients[fd].fd, &server.active);
					server.clients[fd].fd = 0;
					free(server.clients[fd].buff);
					close(fd);
				}
				else
				{
					add[r] = '\0';
					server.clients[fd].buff = str_join(server.clients[fd].buff, add);
					if (!server.clients[fd].buff)
						err(NULL);
					char	*msg;
					while(extract_message(&server.clients[fd].buff, &msg) > 0)
					{
						char	prefix[50];

						sprintf(prefix, "client %d: ", server.clients[fd].id);
						char	*output = malloc(strlen(prefix) + strlen(msg) + 1);
						if (!output)
							err(NULL);
						output[0] = '\0';
						sprintf(output, "%s%s", prefix, msg);
						broadcast(output, &server, server.clients[fd].id);
						free(msg);
						free(output);
					}

					

				}
			}
		}
	}
	return 0;
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
