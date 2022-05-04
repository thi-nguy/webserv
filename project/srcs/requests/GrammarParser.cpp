#include "GrammarParser.hpp"

DEFINE_ENUM(e_parsing_states, E_PARSING_STATE_ENUM)
/*
** ---------------------------------- LOCAL ----------------------------------
*/

bool isItTwoDigitHexa(std::string token)
{
	if(token.length() != 2 || ! isxdigit(token.at(0)) || ! isxdigit(token.at(1)))
		return false;
	return true;
}

bool check_OR (std::string token, t_grammar_map &gm)
{
	(void)gm;
	if(token == "/")
		return true;
	return false;
}

bool check_MULTI (std::string token, t_grammar_map &gm)
{
	(void)gm;
	size_t multiPos = token.find("*");
	if (multiPos != std::string::npos && token.find("*", multiPos + 1) != std::string::npos)
		return false;
	std::string tmp_block;
	std::stringstream ss(token);
	while(std::getline(ss, tmp_block, '*'))
	{
		for (size_t i = 0; i < tmp_block.length(); i++)
		{
			if(! isdigit(tmp_block.at(i)))
				return false;
		}
	}
	return true;
}

bool check_VALUE (std::string token, t_grammar_map &gm)
{	
	(void)gm;
	if (token.length() == 4 && token.rfind("0x", 0) == 0 && isItTwoDigitHexa(token.substr(2, 4)))
		return true;
	return false;
}

bool check_QUOTEVALUE (std::string token, t_grammar_map &gm)
{	
	(void)gm;
	if (token.length() == 3 && token.at(0) == '\'' && token.at(2) == '\'')
		return true;
	return false;
}

bool check_INTERVAL (std::string token, t_grammar_map &gm)
{
	(void)gm;
	std::string tmp_block;
	if (token.length() > 4 && (token.length() - 4) % 3 == 0 && token.rfind("0x", 0) == 0)
	{
		std::stringstream ss(token.substr(2, token.length()));
		while(std::getline(ss, tmp_block, '-'))
		{
			if (!isItTwoDigitHexa(tmp_block))
				return false;
		}
		return true;
	}
	return false;
}

bool check_MULTIVALUES (std::string token, t_grammar_map &gm)
{
	(void)gm;
	std::string tmp_block;
	if (token.length() > 4 && (token.length() - 4) % 3 == 0 && token.rfind("0x", 0) == 0)
	{
		std::stringstream ss(token.substr(2, token.length()));
		while(std::getline(ss, tmp_block, '.'))
		{
			if (!isItTwoDigitHexa(tmp_block))
				return false;
		}
		return true;
	}
	return false;
}

bool check_BLOCK (std::string token, t_grammar_map &gm)
{
	(void)gm;
	if(token == "(" || token == ")")
		return true;
	return false;
}

bool check_STRING (std::string token, t_grammar_map &gm)
{
	(void)gm;
	if (token.length() >= 2 && token.at(0) == '"' && token.at(token.length() - 1) == '"' )
		return true;
	return false;
}

bool check_OPTIONAL (std::string token, t_grammar_map &gm)
{
	(void)gm;
	if(token == "[" || token == "]")
		return true;
	return false;
}

bool check_VAR (std::string token, t_grammar_map &gm)
{
	if (gm.find(token) != gm.end())
		return true;
	return false;
}

/*
** ---------------------------------- STATIC ----------------------------------
*/

t_builder_dictionary GrammarParser::initBuilderDictionnary()
{
    t_builder_dictionary vector;
	vector.push_back(std::make_pair(&check_OR, &GrammarParser::consume_OR)); // / 
	vector.push_back(std::make_pair(&check_MULTI, &GrammarParser::consume_MULTI)); // x*y
	vector.push_back(std::make_pair(&check_VALUE, &GrammarParser::consume_VALUE)); //  0x35
	vector.push_back(std::make_pair(&check_QUOTEVALUE, &GrammarParser::consume_QUOTEVALUE)); //  'x'
	vector.push_back(std::make_pair(&check_INTERVAL, &GrammarParser::consume_INTERVAL)); //  0x35-45
	vector.push_back(std::make_pair(&check_MULTIVALUES, &GrammarParser::consume_MULTIVALUES)); // 0x45.35
	vector.push_back(std::make_pair(&check_BLOCK, &GrammarParser::consume_BLOCK)); // ()
	vector.push_back(std::make_pair(&check_STRING, &GrammarParser::consume_STRING)); // ""
	vector.push_back(std::make_pair(&check_OPTIONAL, &GrammarParser::consume_OPTIONAL)); // []
	vector.push_back(std::make_pair(&check_VAR, &GrammarParser::consume_VAR)); //known var

	return vector;
}

t_builder_dictionary GrammarParser::_builderDictionnary = GrammarParser::initBuilderDictionnary();

#define PREPARE_AND_SKIP_EMPTY_LIGNES(str) std::replace_if(str.begin(), str.end(), isblank, ' '); \
		trim(str);\
		if(str == "" || str[0] == ';')\
			continue;

GrammarVariables *parseVar(std::string tmp_line)
{
	std::string tmp_block;
	std::string name;
	std::string type = GetString(NO_VAR_TYPE);
	std::vector<std::string> tokens;
	std::stringstream ss(tmp_line);
	while(std::getline(ss, tmp_block, ' '))
	{
		if(tmp_block[0] == ';')
			break;
		if(tmp_block == "")
			continue;
		tokens.push_back(tmp_block);
	}
	if(tokens.size() <= 2 || tokens[1] != "=")
		return NULL;
	name = tokens.front();
	if(name.find("!") != std::string::npos)
	{
		std::stringstream ss(name);
		if(!std::getline(ss, tmp_block, '!'))
			return NULL;
		type = tmp_block;
		if(!std::getline(ss, tmp_block, '!'))
			return NULL;
		name = tmp_block;
	}
	tokens.erase(tokens.begin(), tokens.begin() + 2);
	GrammarVariables *gv = new GrammarVariables(name, tokens, Gete_var_typeValue(type.c_str()));
	return gv;
}

#define NON_VALID -1

int getBuilderIDForToken(std::string token, t_grammar_map &gramMap)
{
	for (size_t i = 0; i < GrammarParser::_builderDictionnary.size(); i++)
	{
		if(GrammarParser::_builderDictionnary[i].first(token, gramMap))
			return i;
	}
	return NON_VALID;
}

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

static const char *COLORS[] = {
  BOLDGREEN,
  YELLOW,
  RED,
  MAGENTA,
  BOLDRED,
  WHITE,
  BOLDBLUE,
  BOLDMAGENTA,
  BOLDCYAN,
  BOLDWHITE,
  BOLDYELLOW,
  BLUE,
  CYAN,
  GREEN,
  BOLDBLACK,
  BLACK,
  NULL
};


bool checkValidityForVar(GrammarVariables *gv, t_grammar_map &gramMap)
{
	std::vector<std::string> &tokens = gv->getTokens();
	int id = NON_VALID;
	for (size_t i = 0; i < tokens.size(); i++)
	{
		if((id = getBuilderIDForToken(tokens[i], gramMap)) == NON_VALID)
			return false;
		if (!DEBUG)
			std::cout << COLORS[id] << " "<<tokens[i] << " "<< RESET;
	}
	return true;
}

bool isGrammarMapValid(t_grammar_map &gramMap)
{
	for(t_grammar_map::const_iterator it = gramMap.begin(); it != gramMap.end(); it++)
	{
		if (!checkValidityForVar((*it).second, gramMap))
			return false;
	}
	return true;
}

GrammarParser *GrammarParser::build(std::string filename)
{
	std::ifstream	ifs(filename.c_str());
	std::string		tmp_line = "";
	GrammarVariables *gv = NULL;
	t_grammar_map vars;

	if (!ifs.is_open())
	{
		std::cout << "Grammar innaccessible or non existing, can not proceed with request parsing" << std::endl;
		ifs.close();
		return NULL;
	}
	while(std::getline(ifs, tmp_line))
	{
		PREPARE_AND_SKIP_EMPTY_LIGNES(tmp_line)
		if((gv = parseVar(tmp_line)) == NULL)
		{
			ifs.close();
			std::cout << "The grammar file is not correctly formated" << std::endl;
			for(t_grammar_map::const_iterator it = vars.begin(); it != vars.end(); it++)
				delete it->second;
			return NULL;
		}
		vars[gv->getName()] = gv;
	}
	ifs.close();
	if(isGrammarMapValid(vars) || vars.find(ID_BASE_REQUEST) != vars.end())
		return new GrammarParser(vars);
	for(t_grammar_map::const_iterator it = vars.begin(); it != vars.end(); it++)
		delete it->second;
	return NULL;
}

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

GrammarParser::GrammarParser(t_grammar_map vars, std::string request) : 
	_vars(vars), 
	_request(request), 
	_priority_states(), 
	_statementsOpened(), 
	_key_buffer(""), 
	_value_buffer(""), 
	_current_buffer(NULL)
{
}

GrammarParser::GrammarParser( const GrammarParser & src ) : 
	_vars(src._vars), 
	_request(src._request), 
	_priority_states(src._priority_states), 
	_statementsOpened(src._statementsOpened),
	_key_buffer(src._key_buffer), 
	_value_buffer(src._value_buffer), 
	_current_buffer(src._current_buffer)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

GrammarParser::~GrammarParser()
{
	for(t_grammar_map::const_iterator it = this->_vars.begin(); it != this->_vars.end(); it++)
		delete it->second;
	for(std::list<GrammarParserBuilderMarker*>::const_iterator it = this->_priority_states.begin(); it != this->_priority_states.end(); it++)
		delete *it;
	for(std::list<Statements*>::const_iterator it = this->_statementsOpened.begin(); it != this->_statementsOpened.end(); it++)
		delete *it;		
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

GrammarParser &				GrammarParser::operator=( GrammarParser const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	(void)rhs;
	return *this;
}

std::ostream & GrammarParser::print(std::ostream & o) const
{
	for(t_grammar_map::const_iterator it = this->_vars.begin(); it != this->_vars.end(); it++)
		o << *(it->second) << std::endl;
	o << "Priorities :" << this->_priority_states.size() << std::endl;
	for(std::list<GrammarParserBuilderMarker*>::const_iterator it = this->_priority_states.begin(); it != this->_priority_states.end(); it++)
		o << "\t" << **it << std::endl;
	return o;
}

std::ostream &			operator<<( std::ostream & o, GrammarParser const & i )
{
	return i.print(o);
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void GrammarParser::addToBuffer(std::string add, GrammarParserBuilderMarker *gp)
{
	this->_requestIndex += add.size();
	if (!DEBUG)
		std::cout << add;
	gp->addToBuffer(add);
}


bool GrammarParser::tryIncToken()
{
	if(!_priority_states.front()->incToken())
	{
		std::cout<<"deleting:"<< *_priority_states.front() <<std::endl;
		GrammarVariables *gv = _priority_states.front()->getVar();
		if (gv->getType() == ONLY_VALUE || gv->getType() == VALUE)
		{
			this->_parsed_datas[this->_key_buffer] = _priority_states.front()->getBuffer();
			this->_key_buffer = "";
			this->_current_buffer = NULL;
		}
		else if (gv->getType() == KEY)
			this->_key_buffer = _priority_states.front()->getBuffer();
		else if(_priority_states.size() > 2)
			(*(_priority_states.begin()++))->addToBuffer(_priority_states.front()->getBuffer());
	 	delete _priority_states.front();
		_priority_states.pop_front();
		return false;
	}
	return true;
}


void GrammarParser::feed(std::string buff)
{
	this->_request += buff;
}

void GrammarParser::clear()
{
	this->_parsed_datas.clear();
	this->_request = "";
	this->_requestIndex = 0;
	this->_value_buffer = "";
	this->_key_buffer = "";
	this->_current_buffer = NULL;
}

void GrammarParser::initParse()
{
	if (_priority_states.size() != 0)
		return;
	this->_parsed_datas.clear();
	this->_value_buffer = "";
	this->_key_buffer = "";
	this->_current_buffer = NULL;
	this->_requestIndex = 0;
	_priority_states.push_back(new GrammarParserBuilderMarker(0, this->_vars[ID_BASE_REQUEST]));
}
e_parsing_states GrammarParser::parse()
{
	initParse();

int i = 0;
	e_parsing_states current;
	do {
		if(this->_requestIndex >= this->_request.size())
			return PARSE_NOTHING_MORE;
		if(i++ > 50)
			return PARSE_QUIT_DEBUG;
		std::string token = _priority_states.front()->getCurrentToken();
		int id = getBuilderIDForToken(token, this->_vars);
		if(id == NON_VALID)
			return PARSE_FATAL_FAILURE;
		if (!DEBUG)
			std::cout << COLORS[id];
		std::cout<< *_priority_states.front() <<"::"<< id <<"::"<< this->_requestIndex << "::"<<std::endl;
		current = (this->*GrammarParser::_builderDictionnary[id].second)(token, _priority_states.front());
		if (!DEBUG)
			std::cout << RESET;
	}
	while(current != PARSE_NOT_ENOUGH_DATAS && current != PARSE_FATAL_FAILURE);
	return current;
	// std::vector<std::string> &tokens = gv->getTokens();
	// for (size_t i = 0; i < tokens.size(); i++)
	// {
	// 	if(getBuilderIDForToken(tokens[i], gramMap) == NON_VALID)
	// 		return false;
	// }
	// return true;
}


e_parsing_states GrammarParser::consume_OR(std::string token,	GrammarParserBuilderMarker *gp)
{
	tryIncToken();
	(void)token;
	(void)gp;
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_MULTI(std::string token,	GrammarParserBuilderMarker *gp)
{
	tryIncToken();

	std::string tmp_block;
	int min = 0;
	int max = INT_MAX;
	std::stringstream ss(token);
	if(!std::getline(ss, tmp_block, '*'))
		return PARSE_FAILURE;
	if(tmp_block.size() > 0)
		min = strtol(tmp_block.c_str(), NULL, 10);

	if(!std::getline(ss, tmp_block, '*'))
		return PARSE_FAILURE;
	if(tmp_block.size() > 0)
		max = strtol(tmp_block.c_str(), NULL, 10);
	// for (int i = 0; i < min; i++)
	// {
	// 	// _priority_states.push_front(gp->generateDeeperObj());
	// 	tryIncToken();
	// 	/* code */
	// }
	gp->setRep(min, max);
	
	(void)token;
	(void)gp;
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_VALUE(std::string token,	GrammarParserBuilderMarker *gp)
{
	(void)gp;
	long valid_value = strtol(token.substr(2).c_str(), NULL, 16);
	char c = this->_request.at(this->_requestIndex);
	std::string tmp ;
	tmp += c;
	if(c != valid_value)
		return PARSE_FAILURE;
	tryIncToken();
	std::cout<<"CHAR IS :" << tmp << " NEEED "<< valid_value<<std::endl;
	addToBuffer(tmp, gp);
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_QUOTEVALUE(std::string token,	GrammarParserBuilderMarker *gp)
{
	tryIncToken();
	(void)token;
	(void)gp;
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_INTERVAL(std::string token,	GrammarParserBuilderMarker *gp)
{
	tryIncToken();

	long valid_min = strtol(token.substr(2, 4).c_str(), NULL, 16);
	long valid_max = strtol(token.substr(5, 7).c_str(), NULL, 16);
	char c = this->_request.at(this->_requestIndex);
	if(c > valid_max || c < valid_min)
		return PARSE_FAILURE;
	(void)gp;
	tryIncToken();
	std::string tmp ;
	tmp += c;
	std::cout<<"CHAR IS :" << tmp << " NEEED "<< valid_min<<":"<< valid_max<<std::endl;
	addToBuffer(tmp, gp);
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_MULTIVALUES(std::string token,	GrammarParserBuilderMarker *gp)
{
	tryIncToken();
	(void)token;
	(void)gp;
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_BLOCK(std::string token,	GrammarParserBuilderMarker *gp)
{
	tryIncToken();
	(void)token;
	(void)gp;
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_STRING(std::string token,	GrammarParserBuilderMarker *gp)
{
	(void)gp;
	std::string substring;
	std::string subToken = token.substr(1, token.size() - 1);
	if (this->_request.size() - this->_requestIndex < subToken.size())
		return PARSE_NOT_ENOUGH_DATAS;
	substring = this->_request.substr(this->_requestIndex, this->_requestIndex + subToken.size());
	tryIncToken();
	if (substring!= subToken)
		return PARSE_FAILURE;
	addToBuffer(substring, gp);
	return PARSE_SUCCESS;
}

e_parsing_states GrammarParser::consume_OPTIONAL(std::string token,	GrammarParserBuilderMarker *gp)
{
	if (token == "[")
	{
		this->_statementsOpened.push_back(new Statements(gp->getDeepness(), "[", gp->getVar()));
 		tryIncToken();
		
	}
	else
	{
		if(this->_statementsOpened.size() == 0 || !this->_statementsOpened.back()->isTheRightClosingStatement("]", gp->getVar()))
			throw new IllegalParsingState();
		tryIncToken();
	}
	(void)token;
	(void)gp;
	return PARSE_SUCCESS;
}


e_parsing_states GrammarParser::consume_VAR(std::string token,	GrammarParserBuilderMarker *gp)
{
	int tmp_deepness = gp->getDeepness();
 	tryIncToken();
	GrammarVariables *gv =  this->_vars[token];
	if (gv->getType() == ONLY_VALUE)
	{
		this->_key_buffer = "#" + gv->getName();
		this->_current_buffer = &this->_value_buffer;
	}
	else if (gv->getType() == KEY)
		this->_current_buffer = &this->_key_buffer;
	else if (gv->getType() == VALUE)
		this->_current_buffer = &this->_value_buffer;
	_priority_states.push_front(new GrammarParserBuilderMarker(tmp_deepness + 1, gv));
	return PARSE_SUCCESS;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */