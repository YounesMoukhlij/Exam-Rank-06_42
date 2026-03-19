#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>


typedef struct s_client
{
	int id;
	char message[100000];

} t_client;

t_client clients[1024];
fd_set active_fd, read_fd, write_fd;


char bufferR[100000], bufferW[100000];
int max_fd = 0, next_fd = 0;



void errorM(char *s)
{
	write(2, &s, strlen(s));
	exit(1);
}



void sendM(int fd_s)
{
	for (int i = 0; i <= max_fd ; i++)
	{
		if (FD_ISSET(i, &write_fd) && i != fd_s)
			send(i, bufferW, strlen(bufferW), 0);
	}
}


int main(int ac, char **av)
{
	if (ac != 2)
		errorM("Wrong number of arguments\n");


	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
		error("Fatal error\n");

	max_fd = socket_fd;
	FD_ZERO(&active_fd);
	FD_SET(socket_fd , &active_fd);
	struct sockaddr_in ss;
	memset(&ss, 0 ,sizeof(ss));

	ss.sin_family = AF_INET;
	ss.sin_port = htons(atoi(av[1]));
	ss.sin_addr.s_addr = htonl(0x7F000001);

	if (bind(socket_fd, (const struct sockaddr *)&ss, sizeof(ss)) < 0)
		error("Fatal error\n");

	if (listen(socket_fd, 10)  < 0)
		error("Fatal error\n");

	while (10)
	{
		read_fd = write_fd = active_fd;

		if (select(max_fd + 1, &read_fd, &write_fd, 0 , 0) < 0)
			continue;

		for (int fd = 0 ; fd <= max_fd ; fd++)
		{
			if (FD_ISSET(fd, &read_fd) && fd == socket_fd)
			{
				int acc = accept(socket_fd, 0 , 0);
				if (acc < 0)
					continue;
				if (acc > max_fd)
					max_fd = acc;
				clients[acc].id = next_fd++;
				bzero(clients[acc].message, sizeof(clients[acc].message));
				FD_SET(acc, &active_fd);
				sprintf(bufferW, "server: client %d just arrived\n", clients[acc].id);
				sendM(acc);
				break;
			}
			else if (FD_ISSET(fd, &read_fd))
			{
				int rec = recv(fd, bufferR, sizeof(bufferR)- 1, 0);
				if (rec <= 0)
				{
					sprintf(bufferW, "server: client %d just left\n", clients[fd].id);
					sendM(fd);
					FD_CLR(fd, &active_fd);
					close (fd);
					break;
				}
				else
				{
					bufferR[rec] = '\0';
					for (int i = 0, j = strlen(clients[fd].message) ; i < rec &&  (size_t)j < strlen(clients[fd].message) - 1 ; i++, j++)
					{
						clients[fd].message[j] = bufferR[i];
						if (clients[fd].message[j] == '\n')
						{
							clients[fd].message[j] = '\0';
							sprintf(bufferW, "client %d: %.999980s\n", clients[fd].id , clients[fd].message);
							sendM(fd);
							bzero(clients[fd].message, sizeof(clients[fd].message));
							j -= 1;
						}
					}
				}
			}

		}


	}


}

