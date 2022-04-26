#include "ConfigConsumer.hpp"

/*
** ------------------------------- STATIC -------------------------------------
*/

#define ADD_ONE_LEVEL_ACTION(key, actionDefinition, map) { \
	LIST_ACTIONS tmplist; \
	tmplist.push_back(actionDefinition); \
	map[key] = tmplist; \
}

AServerItem *placeholder_consume(Node *node, AServerItem *currentServerItem, bool isLocation)
{
	(void)isLocation;
	(void)node;
	(void)currentServerItem;
	return NULL;
}

MasterServer* getMasterServerFrom(AServerItem *currentServerItem)
{
	MasterServer *ms = dynamic_cast<MasterServer*>(currentServerItem);
	if (!ms)
		throw ConfigConsumer::UnexpectedStateInConsumer();
	return ms;
}

OneServer* getOneServerFrom(AServerItem *currentServerItem)
{
	OneServer *os = dynamic_cast<OneServer*>(currentServerItem);
	if (!os)
		throw ConfigConsumer::UnexpectedStateInConsumer();
	return os;
}

OneLocation* getOneLocationFrom(AServerItem *currentServerItem)
{
	OneLocation *ol = dynamic_cast<OneLocation*>(currentServerItem);
	if (!ol)
	{
		std::cout << " \n Bug's here\n\n";
		throw ConfigConsumer::UnexpectedStateInConsumer();

	}
	return ol;
}

AServerItem *consumeCreateServer(Node *node, AServerItem *currentServerItem, bool isLocation)
{
	if (isLocation)
		throw ConfigConsumer::UnexpectedStateInConsumer();
	MasterServer *ms = getMasterServerFrom(currentServerItem); // ! get MasterServer
	(void)node;
	return ms->createServer();
}

AServerItem *consumeForOneServer(Node *node, AServerItem *currentServerItem, bool isLocation)
{
	if (isLocation)
	{
		OneLocation *ol = getOneLocationFrom(currentServerItem);
		ol->consume(node);
		return ol;
	}
	else
	{
		OneServer *os = getOneServerFrom(currentServerItem); // ! get OneServer
		os->consume(node); // ! OneServer::consume
		return os;
	}
}

ACTION_MAP ConfigConsumer::initializeActionMap()
{
    ACTION_MAP map;
	std::vector<std::string> allowed_parents;
	
	// ! &consumeCreateServer: setup the "server", it has _consume with value is MasterServer::createServer
	// ! &consumeForOneServer: setup the "server_name", it has _consume with value is OneServer::consume
	// ! OneServer::consume will return 
	ADD_ONE_LEVEL_ACTION("server", ActionForKey(0, 2, allowed_parents, &consumeCreateServer), map)
	
	allowed_parents.push_back("server");
	ADD_ONE_LEVEL_ACTION("listen", ActionForKey(2, 4, allowed_parents, &consumeForOneServer), map) 
	ADD_ONE_LEVEL_ACTION("server_name", ActionForKey(2, 4, allowed_parents, &consumeForOneServer), map)
	ADD_ONE_LEVEL_ACTION("location", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map) 

	allowed_parents.push_back("location");
	ADD_ONE_LEVEL_ACTION("index", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map) 
	ADD_ONE_LEVEL_ACTION("root", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map)
	ADD_ONE_LEVEL_ACTION("autoindex", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map) 
	ADD_ONE_LEVEL_ACTION("error_page", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map) 
	ADD_ONE_LEVEL_ACTION("method", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map) 
	ADD_ONE_LEVEL_ACTION("client_max_body_size", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map); 
	ADD_ONE_LEVEL_ACTION("cgi", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map) 
	ADD_ONE_LEVEL_ACTION("cgi_pass", ActionForKey(2, 6, allowed_parents, &consumeForOneServer), map) 
    return map;
}

ACTION_MAP ConfigConsumer::_authorize_key_and_actions = ConfigConsumer::initializeActionMap();

	
AServerItem *ConfigConsumer::consume(int deepness, Node *node, AServerItem *currentServerItem) 
{
	Node::t_inner_args_container &inner_args = node->getInnerArgs(); // ! inner_args: if location {} will get {location}, if: listen 322:3 abc => get:{listen, 323:3, abc}
	if (inner_args.size() == 0)
		return currentServerItem; 
	std::string key = *(inner_args.begin());
	std::cout << "\n... Key is: " << key << std::endl;
	ACTION_MAP::iterator available_actions = ConfigConsumer::_authorize_key_and_actions.find(key); // ! get authorized info for a "key/directive"
	if (available_actions == ConfigConsumer::_authorize_key_and_actions.end()) // ! If there is no action key that is predefined, return (NULL)
		return NULL;

	// ! Loop through each available actions for that key. Normally there is only one: ActionForKey (Ref. initializeActionMap())
	for (LIST_ACTIONS::const_iterator it_list = available_actions->second.begin(); it_list != available_actions->second.end(); it_list++)
	{
		Node *raw_parent = node->getParent()->getParent();
		std::string *parentName = NULL;

		if(raw_parent)
		{
			Node::t_inner_args_container &parent_inner_args = raw_parent->getInnerArgs();
		
			std::cout << "testing... for "<< key << ": at deepness = "<< deepness;
			if (parent_inner_args.size() != 0) // ! if parents is something else than "server"
				parentName = &parent_inner_args[0];
		}
		std::cout << " with parent = " << (parentName ? *parentName : "NO PARENT") << std::endl;

		bool isLocation = false;
		
		if (key.compare("server") == 0 && parentName == NULL)
			return it_list->consume(node, currentServerItem, isLocation);
		if (parentName->compare("location") == 0)
			isLocation = true;
		if (it_list->isValid(deepness, parentName))
			// ! check validity of directive's deepness and its name of parents
			// ! ActionForKey::consume: return / execute the function appropriate to that key (Ref. initializeActionMap())
			return it_list->consume(node, currentServerItem, isLocation);
	}
	std::cout << "error for "<< key << " : "<<deepness<<std::endl;
	return NULL; 
}

int	ConfigConsumer::checkDirectChildrens(Node::t_node_map &childrens, AServerItem* currentServerItem)
{
	try
	{
		 // ! Loop through each key/directive. At first there is only server. Later they are directive under a parent {}
		for(Node::t_node_map ::const_iterator it_map = childrens.begin(); it_map != childrens.end(); it_map++)
		{
			// ! Get a list of server's / current directive
			Node::t_node_list &list_childrens = it_map->second->getDirectChildrensList();
			
			// ! Loop through each server / directive on that list
			for (Node::t_node_list ::const_iterator it_list = list_childrens.begin(); it_list != list_childrens.end(); it_list++)
			{
				AServerItem *nextServerItem = currentServerItem;

				// ! Doing this first. (Before going on some under object, for parsing reasons)
				// ! What does consume do here? 1. Validity check. 2. Run the function appropriate to that directive.
				if((nextServerItem = ConfigConsumer::consume(it_map->second->getDeepness(), it_map->second, nextServerItem)) == NULL)
					return -EXIT_FAILURE;
				
				// ! Go on under objects
				if (ConfigConsumer::checkDirectChildrens((*it_list)->getDirectChildrensMap(), nextServerItem) != EXIT_SUCCESS)
					return -EXIT_FAILURE;
			}
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return -EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

MasterServer *ConfigConsumer::validateEntry(std::string config_path)
{
	Node *firstNode = Node::digestConfigurationFile(config_path);
	if( !firstNode)
		return NULL;

	MasterServer *ms = new MasterServer();

	if (ConfigConsumer::checkDirectChildrens(firstNode->getDirectChildrensMap(), ms) != EXIT_SUCCESS) 
	{
		delete firstNode;
		delete ms;
		std::cout << "Invalid configuration file : Directives are not supported or in wrong Context." << std::endl;
		return NULL;
	}
	delete firstNode;
	return ms;
}

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigConsumer::ConfigConsumer(Node *node) : _node(node)
{
}

ConfigConsumer::ConfigConsumer( const ConfigConsumer & src ) : _node(src._node)
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ConfigConsumer::~ConfigConsumer()
{
	if(_node)
		delete this->_node;
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ConfigConsumer &				ConfigConsumer::operator=( ConfigConsumer const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	(void)rhs;
	return *this;
}

std::ostream & ConfigConsumer::print(std::ostream & o) const
{
	o << (*this->_node);
	return o;
}

std::ostream &			operator<<( std::ostream & o, ConfigConsumer const & i )
{
	i.print(o);
	return o;
}


/*
** --------------------------------- METHODS ----------------------------------
*/

/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */