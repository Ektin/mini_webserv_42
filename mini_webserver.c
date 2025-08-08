/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_webserver.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aminakov <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/08 20:38:34 by aminakov          #+#    #+#             */
/*   Updated: 2025/08/08 21:28:40 by aminakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

int	extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int		i;

	*msg = NULL;
	if (NULL == *buf)
		return (0);
	if (0 == strlen(*buf))
	{
		free(*buf);
		*buf = NULL;
		return (0);
	}
	i = 0;
	while ('\0' != (*buf)[i])
	{
		if ('\n' == (*buf)[i])
		{
			newbuf = calloc(sizeof(*newbuf), strlen(*buf + i + 1) + 1);
			if (NULL == newbuf)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = '\0';
			*buf = newbuf;
			return (1);
		}
		++i;
	}
	return (0);
}

char	*str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (NULL == buf)
		len = 0;
	else
		len = strlen(buf);
	if (NULL == add)
		return (buf);
	newbuf = (char *)malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (NULL == newbuf)
		return (0);
	newbuf[0] = '\0';
	if (NULL != buf)
	{
		strcat(newbuf, buf);
		free(buf);
	}
	strcat(newbuf, add);
	return (newbuf);
}

void	print_error_exit(char *msg)
{
	if (!msg)
		write(STDERR_FILENO, "Fatal error\n", 12);
	else
		write(STDERR_FILENO, msg, strlen(msg));
	exit(EXIT_FAILURE);
}

typedef struct Server
{
	int			sockfd;
	struct sockaddr_in	addr, claddr;
	socklen_t		claddrlen;

	fd_set			afds, rfds, wfds;

	int			maxfd;
	int			maxclid;

	char	buf_read[4];
	int	buf_read_size;

	char	buf_write[64];
	int	buf_write_size;

	int	clid[65536];
	char	*clmsg[65536];
}	Server;

void	setup_constants(Server *serv)
{
	serv->buf_read_size = sizeof(serv->buf_read) / sizeof(serv->buf_read[0]);
	serv->buf_write_size = sizeof(serv->buf_write) / sizeof(serv->buf_write[0]);
}

void	setup_server(Server *serv, char *port)
{
	// setting up buffer sizes
	setup_constants(serv);

	// creating socket and checking
	serv->sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (-1 == serv->sockfd)
		print_error_exit(NULL);

	// setting up auxilliary parameters of the server
	serv->maxfd = serv->sockfd;
	serv->maxclid = -1;

	// setting address
	bzero(&serv->addr, sizeof(serv->addr));
	serv->addr.sin_family = AF_INET;
	serv->addr.sin_port = htons(atoi(port));
	serv->addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//serv->addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // the two last lines do the same 

	// setting client's address
	bzero(&serv->claddr, sizeof(serv->claddr));
	serv->claddrlen = sizeof(serv->claddr);

	// binding port and address and checking
	if (0 > bind(serv->sockfd, (struct sockaddr const *)&serv->addr, sizeof(serv->addr)))
		print_error_exit(NULL);

	// listening
	if (0 > listen(serv->sockfd, SOMAXCONN))
		print_error_exit(NULL);

	// initializing fd_sets
	FD_ZERO(&serv->afds);
	FD_SET(serv->sockfd, &serv->afds);	
}

void	notify_others(Server *serv, char *msg, int author_fd)
{
	for (int fd = 0; fd <= serv->maxfd; ++fd)
	{
		if (!FD_ISSET(fd, &serv->wfds))
			continue ;
		if (fd == author_fd)
			continue ;
		send(fd, msg, strlen(msg), 0);
	}
}

void	send_message(Server *serv, int author_fd)
{
	char	*msg;

	while (extract_message(&(serv->clmsg[author_fd]), &msg))
	{
		sprintf(serv->buf_write, "client %d: ", serv->clid[author_fd]);
		notify_others(serv, serv->buf_write, author_fd);
		notify_others(serv, msg, author_fd);
		free(msg);
		msg = NULL;
	}
}

void	register_new_client(Server *serv)
{
	int	fd;

	fd = accept(serv->sockfd, (struct sockaddr *)&serv->claddr, &serv->claddrlen);
	if (fd < 0)
		return ;
	serv->maxfd = fd > serv->maxfd ? fd : serv->maxfd;
	serv->clid[fd] = ++serv->maxclid;
	serv->clmsg[fd] = NULL;
	FD_SET(fd, &serv->afds);
	sprintf(serv->buf_write, "server: client %d just arrived\n", serv->clid[fd]);
	notify_others(serv, serv->buf_write, fd);
}

void	remove_client(Server *serv, int fd)
{
	sprintf(serv->buf_write, "server: client %d just left\n", serv->clid[fd]);
	notify_others(serv, serv->buf_write, fd);
	free(serv->clmsg[fd]);
	serv->clmsg[fd] = NULL;
	FD_CLR(fd, &serv->afds);
	close(fd);
}

void	server_loop(Server *serv)
{
	int	res;

	serv->rfds = serv->wfds = serv->afds;

	if (0 > select(serv->maxfd + 1, &serv->rfds, &serv->wfds, NULL, NULL))
		print_error_exit(NULL);

	for (int fd = 0; fd <= serv->maxfd; ++fd)
	{
		if (!FD_ISSET(fd, &serv->rfds))
			continue ;
		if (fd == serv->sockfd)
		{
			register_new_client(serv);
		}
		else
		{
			res = recv(fd, serv->buf_read, serv->buf_read_size - 1, 0);
			serv->buf_read[res] = '\0';
			if (res <= 0)
			{
				remove_client(serv, fd);
				break ;
			}
			serv->clmsg[fd] = str_join(serv->clmsg[fd], serv->buf_read);
			send_message(serv, fd);
		}
	}
}

int	main(int argc, char *argv[])
{
	if (1 == argc)
		print_error_exit("Wrong number of arguments\n");

	Server	serv;

	setup_server(&serv, argv[1]);
	while (1)
	{
		server_loop(&serv);
	}
	return (0);
}
