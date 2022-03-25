/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: scarboni <scarboni@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/19 22:03:00 by scarboni          #+#    #+#             */
/*   Updated: 2022/03/25 09:02:41 by scarboni         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.h"
#include "Node.hpp"

#define DEFAULT_CONFIG_PATH "./config_files/default.conf"

int		main(int ac, char *av[])
{
	std::string config_path;
	if (ac > 2)
	{
		std::cout << "Usage : ./webserv [configuration file]" << std::endl;
		return 0;
	}
	if (ac < 2)
	{
		config_path = DEFAULT_CONFIG_PATH;
		std::cout << "Taking default configuration file" << std::endl;
	}
	else config_path = av[1];

	Node *firstNode = Node::digestConfigurationFile(config_path);
	if( !firstNode)
	{
		return 0;
	}
	// you can get all servers objects like that :
	Node::t_node_list servers = firstNode->getChildrenByFirstName("server");
	for(Node::t_node_list ::const_iterator it = servers.begin(); it != servers.end(); it++)
		std::cout << **it << std::endl; //two * because the list contain pointers to nodes
	// you can get all listen objects like that too :
	Node::t_node_list listen_sub_first_server = (*servers.begin())->getChildrenByFirstName("listen");
	for(Node::t_node_list ::const_iterator it = listen_sub_first_server.begin(); it != listen_sub_first_server.end(); it++)
		std::cout << **it << std::endl; //two * because the list contain pointers to nodes
	if(!firstNode)
		return 0;
	delete firstNode;
	return 0;
}