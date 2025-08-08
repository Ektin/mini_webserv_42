/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_webserver.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aminakov <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/08 20:38:34 by aminakov          #+#    #+#             */
/*   Updated: 2025/08/08 20:45:38 by aminakov         ###   ########.fr       */
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
	char 	*newbuf;
	int	i;

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
