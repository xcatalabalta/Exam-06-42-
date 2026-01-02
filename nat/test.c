#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
//
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

//new
typedef struct s_client{
	int 		fd;
	int 		id;
	char		*buf;
}t_client;

//new
typedef struct s_server{
	t_client 	clients[1024];
	int 		max_fd;
	int 		next_id;
	fd_set		read_set;
	fd_set		write_set;
	fd_set		active_set;

}t_server;

//new minimalistic function
void	init_server(t_server *serv, int sock_fd){
	serv->max_fd = sock_fd;
	FD_SET(sock_fd, &srv->active_set);
}

//new
void	err(char *msg, int exit_code){
	if (!msg){
		msg = "Fatal error\n";
	}
	write(2, msg, strlen(msg));
	exit(exit_code);
}

//new
void	sent_to_all(char *msg, int except, t_server *serv){
	for (int i = 0; i <= serv->max_fd; i++){
		if (serv->clients[i].fd > 0 && i != except){
			send(serv->clients[i].fd , msg, strlen(msg), 0);
		}
	}
}

//given
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

//given
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


int main(int argc, char *argv[]) {
	int 				sockfd;//, connfd, len;
	struct sockaddr_in 	servaddr;//, cli; 
	t_serv				serv = {0};//new

	//Check args
	if (ac != 2) {
		err("Wrong number of arguments\n", 1);
	}

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		//printf("socket creation failed...\n"); 
		//exit(0);
		err(NULL, 1);
	}
	/*
	// to comment from main
	else
		printf("Socket successfully created..\n"); 
	*/
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	//servaddr.sin_port = htons(8081); 
	servaddr.sin_port = htons(atoi(argv[1]));//new 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		//printf("socket bind failed...\n"); 
		//exit(0); 
		err(NULL, 1);
	} 
	/*
	else
		printf("Socket successfully binded..\n");
	*/
	if (listen(sockfd, 10) != 0) {
		//printf("cannot listen\n"); 
		//exit(0); 
		err(NULL, 1);
	}
	//Init server
	init_server(&serv, sockfd);	
	while (1)
	{
		serv.read_set = serv.write_set = serv.active_set;
		if (select(serv.max_fd + 1, &serv.read_set, &serv.write_set, NULL, NULL) < 0){
			continue;
		}
		for (int fd = 0; fd <= serv.max_fd; fd++) {
			if(!FD_ISSET(fd, &serv->read_set)) {
				continue;
			}
			if (fd == sockfd) {
				new_client(&serv, sockfd);
			}
			else {
				is_client(&serv, fd);
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
