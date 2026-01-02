#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
//1 add libraries
#include <stdio.h>
#include <stdlib.b>
#include <sys/select.h>

//2 declare globals
typedef struct s_client{
	int fd;
	int id;
	char *buf;
} t_client;

t_client 	clients[1024];
int max_fd = 0, next_id = 0;
fd_set 	read_set, write_set, active_set;

// 3 code auxiliar functions
void 	err(char *msg, int exit_code) {
	if (!msg)
		msg = "Fatal error\n";
	write(2, msg, strlen(msg));
	exit(exit_code);
}

void 	send_all(int except, char *msg) {
	for (int i = 0; i < max_fd; i++){
		if (clients[i].fd > 0 && i != except)
			send(clients[i].fd, msg, strlen(msg), 0);
	}
}

// 4 leave given auxiliar functions
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

// 4 leave given auxiliar functions
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
// end of 4

// 5 modify main to accept arguments
int main(int argc, char **argv) {
// end of 5
// 6 elliminate or comment not required varialbles 
	int sockfd, connfd;//, len;
	struct sockaddr_in servaddr;//, cli; 
	// end of 6
	//7 check for no arguments
	if (ac != 2) {
		err("Wrong number of arguments\n", 1);
	}
	//end of 7
	// 8 leave sockfd as it is and amend the condition without message
	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
	/*	printf("socket creation failed...\n"); 
		exit(0); 
	*/
		err(NULL, 1);
	}
	//end of 8
	// 9 eliminate or comment else
	/*
	else
		printf("Socket successfully created..\n"); 
	*/
	// end of 9
	// 10 set max_fd to sockfd
	max_fd = sockfd;
	// end of 10
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	// 11 change port to get the arg
	//servaddr.sin_port = htons(8081); 
	servaddr.sin_port = htons(atoi(argv[1]));
	// end of 11
  
	// Binding newly created socket to given IP and verification 
	// 12 use err (message NULL, exit code 1) function instead of printf and exit
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		//printf("socket bind failed...\n"); 
		//exit(0); 
		err(NULL, 1);
		// end 12
	} 
	// 13 comment eliminate else and printf
	/*
	else
		printf("Socket successfully binded..\n");
	*/
	// end 13
	// 14 use err (message NULL, exit code 1) function instead of printf and exit
	if (listen(sockfd, 10) != 0) {
		//printf("cannot listen\n"); 
		//exit(0); 
		err(NULL, 1);
		// end of 14
	}
	// The real program starts here:

	// 15 add }
}

	// 14 comment the rest of the main to work with the code (put at bottom
/*	
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n");
}// end of 14
*/
