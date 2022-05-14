#include "MasterServer.hpp"

/*
** ---------------------------------- STATIC ----------------------------------
*/

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

MasterServer::MasterServer()
{}

MasterServer::MasterServer(const MasterServer & src): AServerItem()
{
    if (this != &src)
        *this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

MasterServer::~MasterServer()
{
	for (unsigned int index = 0; index < this->_configAllServer.size(); index++)
		delete this->_configAllServer[index];
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

MasterServer & MasterServer::operator=(const MasterServer & rhs)
{
    this->_configAllServer = rhs._configAllServer;
    return (*this);
}


AServerItem *MasterServer::consume(Node *node)
{
	(void)node;
	return this->createServer();
}

/*
** --------------------------------- METHODS ----------------------------------
*/

OneServer *MasterServer::createServer()
{
	this->_configAllServer.push_back(new OneServer());
	return this->_configAllServer[this->_configAllServer.size() - 1];
}


std::ostream &			MasterServer::print( std::ostream & o) const
{
	o << "I'm Master Server !" << std::endl;
	for (size_t i = 0; i < this->_configAllServer.size(); i++)
		o << *(this->_configAllServer[i]) << std::endl;
	
	return o;
}

int	MasterServer::build()
{
    _max_fd = 0;
    unsigned int ms_size = _configAllServer.size();

    /*************************************************************/
    /* Initialize the master fd_set                              */
    /*************************************************************/
    FD_ZERO(&_fdSet);

    for(unsigned int i = 0; i < ms_size; i++)
	{
        int fd;
		if(_configAllServer[i]->build() == 0) // bind fd and address, listen to fd
        {
            std::cout << "\nWaiting for connections...\n";
            
            // add socket to set _fdSet
            fd = _configAllServer[i]->getFD();
            FD_SET(fd, &_fdSet);
            
            if (fd > _max_fd)
                _max_fd = fd;
        }
        else // ! Error while building OneServer
            return (-1);
	}
    if (_max_fd == 0) // ! Error to build MasterServer
    {
        return (-1);
    }
    return 0;
}



void MasterServer::run()
{
    /*************************************************************/
    /* Loop waiting for incoming connects or for incoming data   */
    /* on any of the connected sockets.                          */
    /*************************************************************/
    while (1)
    {
        // init_fd();
        // do_select();
        // check_fd();
    }
}




/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */ 
// OLD VERSION
// void MasterServer::run() 
// {
//     /*************************************************************/
//     /* Loop waiting for incoming connects or for incoming data   */
//     /* on any of the connected sockets.                          */
//     /*************************************************************/
//     while (1)
//     {
//         // init_fd();
//         // do_select();
//         // check_fd();
//         fd_set          readfds;
//         fd_set          writefds;
//         struct timeval  timeout;
//         int activity = 0;

//         while (activity == 0)
//         {
//             /*************************************************************/
//             /* Initialize the timeval struct to 3 minutes.  If no        */
//             /* activity after 3 minutes this program will end.           */
//             /*************************************************************/
//             timeout.tv_sec  = 3 * 60;
//             timeout.tv_usec = 0;
            
//             /**********************************************************/
//             /* Copy the _fdSet (got from binding part above) over to  */
//             /* the readfds.                                           */
//             /* Copy the _ready (if it exists) over to the writefds.   */
//             /**********************************************************/
//             memcpy(&readfds, &_fdSet, sizeof(_fdSet));
            
//             FD_ZERO(&writefds);
//             for (std::vector<int>::iterator it = _ready.begin() ; it != _ready.end() ; it++)
// 				FD_SET(*it, &writefds);

//             /*************************************************************/
//             /* Call select() and wait 3 minutes for it to complete.      */
//             /* Wait for one or more fd become "ready" to read and write  */
//             /*************************************************************/
//             activity = select(_max_fd + 1, &readfds, &writefds, NULL, &timeout);

//             /**********************************************************/
//             /* Check to see if the select call failed.                */
//             /**********************************************************/
//             if (activity < 0)
//             {
//                 std::cout << "select() failed" << std::endl;
//                 break;
//             }
//         }
//         /**********************************************************/
//         /* Check to see if the 3 minute time out expired.         */
//         /**********************************************************/
//         if (activity == 0)
//         {
//             std::cerr << "select() time out. End program." << std::endl;
//             break ;

//             // Todo: next: close / clear opened socket
//             FD_ZERO(&_fdSet);
//         }
//         /**********************************************************/
//         /* If the program continues until this point, it means    */
//         /* that one or more descriptors are readable. Need to     */
//         /* determine which ones they are                          */
//         /**********************************************************/
//         for (std::vector< OneServer *>::iterator it = _configAllServer.begin(); activity && it != _configAllServer.end(); it++)
//         {
//             /*******************************************************/
//             /* Check to see if this descriptor is ready            */
//             /*******************************************************/
//             if (FD_ISSET((*it)->getFD(), &writefds))
//             {
//                 // todo: send
//                 int ret;
//                 std::string buffer_response = "";
//                 ret = send((*it)->getFD(), buffer_response.c_str(), buffer_response.size(), 0);
//                 if (ret < 0)
//                 {
//                     perror("  send() failed");
//                     close ((*it)->getFD());
//                     continue ;
//                 }
//                 break;
//             }
//             else if (FD_ISSET((*it)->getFD(), &readfds))
//             {
//                 // todo: receive
//                 /**********************************************/
//                 /* Accept each incoming connection.  If       */
//                 /* accept fails with EWOULDBLOCK, then we     */
//                 /* have accepted all of them.  Any other      */
//                 /* failure on accept will cause us to end the */
//                 /* server.                                    */
//                 /**********************************************/
//                 int new_fd = accept((*it)->getFD(), NULL, NULL);
//                 if (new_fd < 0)
//                 {
//                     perror("    accept() fails");
//                     close ((*it)->getFD());
//                     break ;
//                 }

//                 /**********************************************/
//                 /* Receive request sent from browser.         */
//                 /**********************************************/
//                 const uint EASILY_ENOUGH = 100000;
//                 char* buffer_recv = new char[EASILY_ENOUGH + 1];
//                 int bytesRead = recv((*it)->getFD(), buffer_recv, EASILY_ENOUGH, MSG_WAITALL); // FREEZES UNTIL I KILL THE CLIENT
//                 if (bytesRead == -1) {
//                     perror("recv");
//                     close((*it)->getFD());
//                     continue ;
//                 }
//                 buffer_recv[bytesRead] = 0;
//                 _ready.push_back((*it)->getFD());


//                 /**********************************************/
//                 /* Print the received request                 */
//                 /**********************************************/
//                 printf("bytes read: %d\n", bytesRead);
//                 printf("Request from Browser:\n%s", buffer_recv);
//                 delete[] buffer_recv;
//                 break ;
//             }
//         }
//     }
// }

