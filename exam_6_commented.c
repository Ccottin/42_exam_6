#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include  <sys/types.h> 
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENT 1024

/* data struct got :
 * - number of file descriptor : it never decrease and thats why 
 *   you neeed a big number of max client
 * - fds[] : fill for every new connection
 * - missingCarriage[] : boolean to mark if carrriage return is missing ;
 *   while its missing dont print "client %d" at the begining
 *   there are many solutions to this problems whoch differs of this
 *   one ; as long as its handled your code is correct :) */

typedef struct s_data {
	int		nfds;
	int		fds[MAX_CLIENT];
	char	missingCarriage[MAX_CLIENT];
}	t_data;

void	ft_exit(void)
{
	write(2, "Error: fatal\n", strlen("Error: fatal\n"));
	exit(1);
}

void	ft_close(t_data *data)
{
	for (int i = 0; i < data->nfds; i++)
	{
		if (data->fds[i] != -1)
			close(data->fds[i]);
	}
	ft_exit();
}

void	ft_send(t_data *data, char *buffer, int index)
{
	for (int i = 1; i < data->nfds; i++)
	{
		if (data->fds[i] != -1 && i != index)
		{
			if (send(data->fds[i], buffer, strlen(buffer), 0) == -1)
				ft_close(data);
		}
	}
	bzero(buffer, strlen(buffer));
}

void	ft_send1(t_data *data, char *buffer, int index, int rec)
{
	int i = 0;
	int y = 0;
	char	temp[BUFFER_SIZE + 1];
	char	last = buffer[strlen(buffer) - 1];

	bzero(temp, BUFFER_SIZE + 1);
	while (i < rec && buffer[i])
	{
		/* when you find a \n, split and send*/
		if (buffer[i] == '\n')
		{
			buffer[i] = 0;
			sprintf(temp, "%s\n", &buffer[y]);
			ft_send(data, temp, index);
			y = i + 1;
			if (buffer[i + 1])
			{
				sprintf(temp, "client %d : ", index - 1);
				ft_send(data, temp, index);
			}
			data->missingCarriage[index] = 0;
		}
		++i;
	}
	if (last && last != '\n')
	{
		ft_send(data, &buffer[y], index);
		data->missingCarriage[index] = 1;
	}
}

int		main(int ac, char **av)
{
	if (ac < 2)
		ft_exit();

	t_data data;
	/* structure init */
	data.nfds = 1;
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		data.fds[i] = -1;
		data.missingCarriage[i] = 0;
	}
	int	servSocket;
	struct sockaddr_in servaddr;

	/* all of this is in provided files */
	servSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (servSocket == -1)
		ft_exit();
	data.fds[0] = servSocket;
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
	if ((bind(servSocket, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		ft_close(&data);
	if (listen(servSocket, MAX_CLIENT - 2))
		ft_close(&data);

	fd_set	readfds, connectedfds;
	int		rec, newfd, maxfd;
	char	buffer[BUFFER_SIZE + 1];
	char	temp[60];

	bzero(buffer, BUFFER_SIZE + 1);
	bzero(temp, 60);
	maxfd = servSocket;
	FD_ZERO(&connectedfds);
	FD_ZERO(&readfds);
	FD_SET(servSocket, &connectedfds);
	while (7)
	{
		readfds = connectedfds;
		if (select(maxfd + 1, &readfds, 0, 0, 0) == -1)
			ft_close(&data);
		for (int index = 0; index < data.nfds; index++)
		{
			/*accept new connections*/
			if (index == 0 && FD_ISSET(servSocket, &readfds))
			{
				newfd = accept(servSocket, 0, 0);
				if (newfd == -1)
					ft_close(&data);
				data.fds[data.nfds] = newfd;
				sprintf(temp, "server: client %d just arrived\n", data.nfds - 1);
				ft_send(&data, temp, data.nfds );
				++data.nfds;
				if (maxfd < newfd)
					maxfd = newfd;
				FD_SET(newfd, &connectedfds);
			}
			/*message incomming*/
			else if (FD_ISSET(data.fds[index], &readfds))
			{
				rec = recv(data.fds[index], buffer, BUFFER_SIZE, 0);
				if (rec == -1)
					ft_close(&data);
				else if (rec == 0)
				{
					sprintf(temp, "server: client %d just left\n", index - 1);
					ft_send(&data, temp, index);
					if (maxfd == data.fds[index])
					{
						maxfd = servSocket;
						for (int i = 0; i < data.nfds; i++)
						{
							if (maxfd < data.fds[i] && i != index)
								maxfd = data.fds[i];
						}
					}
					/*whipe info about socket so ne error when sending messages*/
					FD_CLR(data.fds[index], &connectedfds);
					close(data.fds[index]);
					data.fds[index] = -1;
				}
				else
				{
					if (!data.missingCarriage[index])
					{
						sprintf(temp, "client %d : ", index - 1);
						ft_send(&data, temp, index);
					}
					/*gather the entire message*/
					while (rec == BUFFER_SIZE)
					{
						ft_send1(&data, buffer, index, rec);
						bzero(buffer, rec);
						rec = recv(data.fds[index], buffer, BUFFER_SIZE, MSG_DONTWAIT);
					}
					if (rec > 0)
					{
						ft_send1(&data, buffer, index, rec);
						bzero(buffer, rec);
					}
				}
			}
		}
	}
	return (0);
}
