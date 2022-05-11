#include "OneServer.hpp"

/*
** ---------------------------------- STATIC ----------------------------------
*/

DIRECTIVES_MAP OneServer::initializeDirectivesMap()
{
	DIRECTIVES_MAP map;
	map["location"] = &ALocation::addLocation;
	map["server_name"] = &ALocation::addServerName;
	map["listen"] = &ALocation::addListen;

	map["index"] = &ALocation::addIndex;
	map["root"] = &ALocation::addRoot;
	map["autoindex"] = &ALocation::addAutoIndex;
	map["method"] = &ALocation::addMethod;
	map["client_max_body_size"] = &ALocation::addMaxSize;
	map["error_page"] = &ALocation::addErrorPage;
	map["cgi"] = &ALocation::addCgi;
    return map;
}

DIRECTIVES_MAP OneServer::_directives_to_setter = OneServer::initializeDirectivesMap();


AServerItem *OneServer::consume(Node *node)
{
	if(!node)
		return NULL; // Or throw, whichever feels best
	
	Node::t_inner_args_container &inner_args = node->getInnerArgs(); // ! get all elements of server key (get all servers)
	if (inner_args.size() == 0)
		return NULL; // Or throw, whichever feels best
	
	// ! Get corresponding function to function pointer named directiveConsumer, based on the name of the directive
	AServerItem *(OneServer::*directiveConsumer)(Node*) = OneServer::_directives_to_setter[*(inner_args.begin())];
	
	if(!directiveConsumer)
		return NULL; // Or throw, whichever feels best
	// ! Call and Run the corresponding function, but how to change/add value to the same OneServer???
	// ! The function does some job and change inside value of OneServer object. (return this)
	return (this->*directiveConsumer)(node); 
}

static bool isNumber(std::string value)
{
	unsigned int i = 0;
	while(isdigit(value[i])) i++;
	if (i != strlen(value.c_str()))
		return false;
	return true;
}

static int getNumber(std::string value)
{
	if (isNumber(value) == false)
		throw ALocation::InvalidValueError();
	long int size = atol(value.c_str());
	if (size > INT_MAX)
		throw ALocation::InvalidValueError();
	else
		return size;
}


static std::vector<std::string> split(const std::string& s, char seperator)
{
   std::vector<std::string> output;

    std::string::size_type prev_pos = 0, pos = 0;

    while((pos = s.find(seperator, pos)) != std::string::npos)
    {
        std::string substring( s.substr(prev_pos, pos-prev_pos) );

        output.push_back(substring);

        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word

    return output;
}

uint32_t getDecimalValueOfIPV4_String(const char* ipAddress)
{
    uint8_t ipbytes[4]={};
    unsigned int i =0;
    int8_t j=3;
    while (ipAddress+i && i<strlen(ipAddress))
    {
       char digit = ipAddress[i];
       if (isdigit(digit) == 0 && digit!='.'){
           return 0;
       }
        j=digit=='.'?j-1:j;
        ipbytes[j]= ipbytes[j]*10 + atoi(&digit);

        i++;
    }

    uint32_t a = ipbytes[0];
    uint32_t b =  ( uint32_t)ipbytes[1] << 8;
    uint32_t c =  ( uint32_t)ipbytes[2] << 16;
    uint32_t d =  ( uint32_t)ipbytes[3] << 24;
    return a+b+c+d;
}


/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

OneServer::OneServer() //Todo: put default value to each directive
{
	_server_name.push_back("");
	_listen = t_listen();
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

OneServer::~OneServer()
{
	for (std::map< std::string, OneLocation* >::const_iterator it = this->_location.begin(); it != this->_location.end(); it++)
		delete it->second;
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

std::ostream &			OneServer::print( std::ostream & o) const
{
	o << "\tI'm OneServer and I have as _server_name = ";
	for (unsigned long i = 0; i < _server_name.size(); i++)
		o << _server_name[i] << " ";

	o << "\t_listen: with address = " << _listen._address;
	o << ", port = " << _listen._port;
	o << ", default_server = " << _listen._default_server;
	
	o << "\t_index = ";
	for (unsigned long i = 0; i < _index.size(); i++)
		o << _index[i] << " ";
	o << "\t_root = " << _root;
	o << "\t_autoindex = " << _autoindex;
	o << "\t_method = ";
	for (std::set<std::string>::const_iterator it = _method.begin(); it != _method.end(); it++)
		o << *it << " ";
	o << "\t_client_max_body_size = " << _client_max_body_size;

	o << "\t_error_page: with codes = ";
	for (unsigned long i = 0; i < _error_page.errorCodes.size(); i++)
		o << _error_page.errorCodes[i] << " ";
	o << ", and uri = " << _error_page.uri;

	o << "\t_cgi = " << _cgi;

	
	o << std::endl;
	for (std::map< std::string, OneLocation* >::const_iterator it = this->_location.begin(); it != this->_location.end(); it++)
		o << "\t\t" << "_location = "<< it->first << "\t" << *(it->second) << std::endl;
	
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

AServerItem *OneServer::addServerName(Node *node)
{
	std::cout << "OneServer I'm trying to add a server_name directive from " << *node;
	
	if (this->_server_name[0].compare("") == 0 && this->_server_name.size() == 1 )
	{
		_server_name.clear();
		Node::t_inner_args_container values = node->get_inner_args();
		for (unsigned long i = 1; i < values.size(); i++)
			_server_name.push_back(values[i]);
	}
	else
		throw MultipleDeclareError();
	return this;
}

AServerItem *OneServer::addLocation(Node *node)
{
	std::cout << "OneServer I'm trying to add a location directive from " << *node;

	Node::t_inner_args_container values = node->get_inner_args();
	if(values.size() != 2) // necessary because juste after you try to read values[1]
		throw IncompleteDirective();
	OneLocation * location_value = new OneLocation();
	if (_location.find(values[1]) != _location.end())
		throw DuplicateUriError();
	_location[values[1]] =  location_value;
	return location_value;
}


AServerItem *OneServer::addListen(Node *node)
{
	std::cout << "OneServer I'm trying to add a listen directive from " << *node;
	
	if (_listen._port == 80 && _listen._address == LOCALHOST && _listen._default_server.compare("") == 0) 
	{
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() < 2 || values.size() > 3)
			throw InvalidValueError();
		if (values.size() == 3)
		{
			_listen._default_server.assign(values[2]);
			std::vector<std::string> output = split(values[1], ':');
			if (output.size() == 2)
			{
				_listen._address = getDecimalValueOfIPV4_String(output[0].c_str());
				_listen._port = getNumber(output[1]);
			}
			else if(output.size() == 1)
			{
				if(isNumber(output[0]))
					_listen._port = getNumber(output[0]);
				else
					_listen._address = getDecimalValueOfIPV4_String(output[0].c_str());
			}
		}
		else if (values.size () == 2)
		{
			if (isNumber(values[1]))
				_listen._port = getNumber(values[1]);
			else
				_listen._address = getDecimalValueOfIPV4_String(values[1].c_str());
		}
	}
	else
		throw MultipleDeclareError();
	return this;
}


AServerItem *OneServer::addIndex(Node *node)
{
	std::cout << "OneServer I'm trying to add a index directive from " << *node ;
	if (this->_index[0].compare("index.html") == 0 && this->_index.size() == 1 )
	{
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() < 2)
			throw InvalidValueError();
		_index.clear();
		for (unsigned long i = 1; i < values.size(); i++)
			_index.push_back(values[i]);
	}
	else
		throw MultipleDeclareError();
	return this;
}

AServerItem *OneServer::addRoot(Node *node)
{
	std::cout << "OneServer I'm trying to add a root directive from " << *node;
	if (this->_root.compare("html") == 0)
	{
		_root.clear();
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() != 2)
			throw IncompleteDirective();
		_root.assign(values[1]);
	}
	else
		throw MultipleDeclareError();
	return this;
}

AServerItem *OneServer::addAutoIndex(Node *node)
{
	std::cout << "OneLocation I'm trying to add a method directive from " << *node;
	if (this->_autoindex == false)
	{
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() != 2)
			throw IncompleteDirective();
		if (values[1].compare("on") == 0)
			_autoindex = true;
		else if (values[1].compare("off") == 0)
			_autoindex = false;
		else
			throw InvalidValueError();
	}
	else
		throw MultipleDeclareError();
	return this;
}

AServerItem *OneServer::addMethod(Node *node)
{
	std::cout << "OneServer I'm trying to add a Method directive from " << *node ;
	if (this->_method.begin()->compare("GET") == 0 && this->_method.size() == 1 )
	{
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() < 2)
			throw InvalidValueError();
		_method.clear();
		for (unsigned long i = 1; i < values.size(); i++)
			_method.insert(values[i]);
		for (std::set<std::string>::const_iterator it = _method.begin(); it != _method.end(); it++)
		{
			if (it->compare("GET") != 0 && it->compare("POST") != 0 && it->compare("DELETE") != 0)
				throw InvalidValueError();
		}
	}
	else
		throw MultipleDeclareError();
	return this;
}


AServerItem *OneServer::addMaxSize(Node *node)
{
	std::cout << "OneServer I'm trying to add a client_max_body_size directive from " << *node;
	if (this->_client_max_body_size == 1)
	{
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() != 2)
			throw IncompleteDirective();
		_client_max_body_size = getNumber(values[1]);
	}
	else
		throw MultipleDeclareError();
	return this;
}


AServerItem *OneServer::addErrorPage(Node *node)
{
	std::cout << "OneServer I'm trying to add a error_page directive from " << *node;
	if (1) // ! Todo: add condition for _error_code is added for the first time
	{
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() < 3)
			throw IncompleteDirective();
		for (unsigned int i = 1; i < values.size() - 1; i++)
		{
			int code = getNumber(values[i]);
			this->_error_page.errorCodes.push_back(code);
		}
		this->_error_page.uri = values[values.size() - 1];
	}
	else
		throw MultipleDeclareError();
	return this;
}

AServerItem *OneServer::addCgi(Node *node)
{
	std::cout << "OneServer I'm trying to add a cgi directive from " << *node;
	
	if (this->_cgi.compare("") == 0)
	{
		_cgi.clear();
		Node::t_inner_args_container values = node->get_inner_args();
		if (values.size() != 2)
			throw IncompleteDirective();
		_cgi.assign(values[1]);
	}
	else
		throw MultipleDeclareError();
	return this;
}

int OneServer::build()
{
   int rc, on = 1;
   /*************************************************************/
   /* Create an AF_INET stream socket to receive incoming       */
   /* connections on                                            */
   /*************************************************************/
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd == 0)
    {
        std::cerr << "Fail to set socket" << std::endl;
        return(-1);
    }

   /*************************************************************/
   /* Allow socket descriptor to be reuseable                   */
   /*************************************************************/
	rc = setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if (rc < 0)
	{
		std::cerr << "setsockopt() failed" << std::endl;
		return (-1);
	}

	/*************************************************************/
	/* Set address (host) and port                               */
	/*************************************************************/
	this->setAddress();

   /*************************************************************/
   /* Bind the socket                                           */
   /*************************************************************/
	rc = bind(_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0;
	if (rc < 0)
    {
        std::cerr << "Fail to bind to port " << _listen._port << std::endl;
        return(-1);
    }

	/*************************************************************/
	/* Try to specify maximun of client pending connection for   */
	/*   the master socket (server_fd)                           */
	/*************************************************************/
    rc = listen(_fd, MAX_CLIENTS);
	if (rc < 0)
    {
        std::cerr << "Fail to listen" << std::endl;
        return(-1);
    }
	// std::cout << "One server is built on port: "<< _listen._port;
	// std::cout << ", address: " << _listen._address;
	// std::cout << ", default server: " << _listen._default_server;
	return (0);
}


void OneServer::setAddress()
{
	memset((char *)&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
    _address.sin_addr.s_addr = htonl(_listen._address);
    _address.sin_port = htons(_listen._port);
}


/*
** --------------------------------- ACCESSOR ---------------------------------
*/

DIRECTIVES_MAP & OneServer::getDirectiveMap()
{
	return this->_directives_to_setter;
}

int	OneServer::getFD()
{
	return _fd;
}

/* ************************************************************************** */