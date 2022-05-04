#ifndef CONFIGDATASTRUCTURES_HPP
# define CONFIGDATASTRUCTURESHPP

#include <string>
#include <vector>

typedef struct 	s_listen {
	std::string 		_address;
	int					_port;
	std::string 		_default_server;

	s_listen(std::string address="", int port=80, std::string default_server="")
	{
		_address = address;
		_port = port;
		_default_server = default_server;
	}
} 				t_listen;

typedef struct 	s_error_page {
	std::vector<int>    errorCodes; 
	std::string         uri;

	s_error_page(std::string uri="")
	{
		uri = uri;
	}		
} 				t_error_page;

#endif /* ************************************************** CONFIGDATASTRUCTURES_HPP */