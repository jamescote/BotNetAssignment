// File: inputHandler.cpp
// Desc: Implementation of the connection Class.
// Written by: James Cote
// ID: 10146559
// Tutorial: T01

// Includes
#include "connection.h"
#include <limits>
#include <fcntl.h>

// Namespace
using namespace std;

// Define
#define RANDOM_DIGITS 3
#define MAX_TIMEOUT 0.5
#define MAX_BUFFER_SIZE 256

// Default Constructor
connection::connection( const string& sServerName, int iPort, const string& sChannel, string& sErrMsg )
{
	// Initialize Private Variables
	m_sHostName.assign( sServerName );
	m_iPort = iPort;
	m_sChannel.assign( sChannel );
	m_bConnected = false;
	srand( time( NULL ) );
	m_iCounter = 0;
	m_sNickName.assign( "" );

	// Connect to IRC
	m_bConnected = connectSocket( m_iConnectedSocket, m_sClientAddr,
								  m_sHostName, m_iPort, sErrMsg );
}

// Destructor -> lose reference to Server
connection::~connection()
{
	close( m_iConnectedSocket );
}

// Initializes the socket connection to the IRC server, doesn't send any IRC messages yet.
bool connection::connectSocket( int& iSocket, struct sockaddr_in& sAddr,
								const string& sHostName, int iPort, string& sErrMsg )
{
	// Local Variables
	bool bReturnValue = false;
	int iConnectResult = -1;
	iSocket = socket( AF_INET, SOCK_STREAM, 0 );
	struct timeval tv;
	tv.tv_sec = MAX_TIMEOUT;
	tv.tv_usec = 0;
	fd_set connectSet;
	int iFlags;
	// Code pulled from "www.technical-recipes.com/2014/getting-started-with-client-server-applications-in-c/"
	struct hostent *host = gethostbyname( sHostName.c_str() );
	char* localIP;

	// Check creation success
	if ( iSocket > 0 && NULL != host )
	{
		// Zero out Socket Addr
		bzero( (char*) &sAddr, sizeof( sAddr ) );

		// Set socket to be non blocking
		iFlags = fcntl( iSocket, F_GETFL, NULL );
		iFlags |= O_NONBLOCK;
		fcntl( iSocket, F_SETFL, iFlags );

		// Code pulled from "www.technical-recipes.com/2014/getting-started-with-client-server-applications-in-c/"
		localIP = inet_ntoa( *(in_addr*) *host->h_addr_list );

		// Set up Server Address parameters
		sAddr.sin_family = AF_INET;
		sAddr.sin_addr.s_addr = inet_addr( localIP );
		sAddr.sin_port = htons( iPort );

		// Attempt to connect
		iConnectResult = connect( iSocket, (sockaddr*) &sAddr, sizeof( sAddr ) );

		if ( iConnectResult < 0 && EINPROGRESS == errno )	// Couldn't immediately establish connection
		{
			do
			{
				tv.tv_sec = 5; // Set connect wait time to 5 seconds
				FD_ZERO( &connectSet );
				FD_SET( iSocket, &connectSet );
				iConnectResult = select( iSocket + 1, NULL, &connectSet, NULL, &tv );  // Select for Timeout

				switch ( iConnectResult )
				{
					case(0):
						fprintf( stderr, "Error: timeout on Connect.\n" );
						break;
					case (-1):
						fprintf( stderr, "Error on select: %s\n", strerror( errno ) );
						break;
					default:	// Successful connection
						if ( FD_ISSET( iSocket, &connectSet ) )
							bReturnValue = true;
						break;
				}
			} while ( !bReturnValue );
		}
		

		tv.tv_sec = MAX_TIMEOUT; // Set back to read Timeout
		iFlags = fcntl( iSocket, F_GETFL, NULL );
		iFlags &= (~O_NONBLOCK);
		fcntl( iSocket, F_SETFL, iFlags ); // Set socket back to blocking.


		// Set a timeout for reading from socket.
		// Pulled from https://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout
		setsockopt( iSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof( struct timeval ) );
	}

	// Assign an error message on failure to report
	if ( NULL == host )
		sErrMsg.assign( "Host could not be resolved." );
	else if( !bReturnValue )
		sErrMsg.assign( strerror( errno ) );

	return bReturnValue;
}

// Generates a random nickname with a prefix and handles handshaking with IRC server.
void connection::connectToServer( const string& sNamePrefix )
{
	// Local Variables
	string sGeneratedName;
	char sBuffer[ MAX_BUFFER_SIZE ] = { '\0' };
	int iBytesRead = numeric_limits<int>::max();
	m_sNickPrefix.assign( sNamePrefix );

	do
	{
		// Generate Random Name:
		genRandomName( m_sNickPrefix, RANDOM_DIGITS, sGeneratedName );

		// Attempt to connect with generated Nick
		Socket_Write( "NICK " + sGeneratedName + "\n" );
		iBytesRead = Socket_Read( sBuffer, MAX_BUFFER_SIZE );
	} while ( iBytesRead > 0 ); // Will not get a response if Nick is valid

	// Store Nickname
	m_sNickName.assign( sGeneratedName );

	// Finish connection.
	Socket_Write( "USER " + m_sNickName + " * * :" + m_sNickName + "\n" );
	Socket_Write( "JOIN #" + m_sChannel + "\n" );
}

// Attack a given Hostname and Port then report result to the Controller
bool connection::attack( const string& sHostName, int iPort, const string& sControllerName )
{
	// Local Variables
	int iSocket;
	struct sockaddr_in sAddr;
	string sMsg, sErrMsg;
	bool bReturnValue = connectSocket( iSocket, sAddr, sHostName, iPort, sErrMsg );
	++m_iCounter;

	// Successfully connected to hostname
	if ( bReturnValue )
	{
		// Send attack message
		sMsg = m_sNickName + " Counter: " + to_string( m_iCounter ) + "\n";
		bReturnValue = (Socket_Write( sMsg, iSocket ) > 0);
		close( iSocket );

		// Attack didn't work? Store Error Message
		if ( !bReturnValue )
			sErrMsg.assign( strerror( errno ) );
	}

	// Send result message to controller
	if ( bReturnValue )
		Socket_Write( "PRIVMSG " + sControllerName + " :attack successful\r\n" );
	else
		Socket_Write( "PRIVMSG " + sControllerName + " :attack failed, " + sErrMsg + "\r\n" );

	// return
	return bReturnValue;
}

// Splits a String up into a vector of strings by a character deliminator "delim"
// Vector of strings returned in sOutput
void connection::splitString( string sInput, char delim, vector<string>& sOutput)
{
	// Local Variables
	stringstream sStrStream;
	string sElement;

	// Clear any trailing whitespace
	while ( !sInput.empty() && (0 == sInput.back() || isspace( sInput.back() )) )
		sInput.pop_back();

	sStrStream.str( sInput );	// Use String Stream to parse the string

	// Get a line and push the new string back into the vector.
	while (getline(sStrStream, sElement, delim))
		if ( sElement.length() > 0 )
			sOutput.push_back( sElement );
}

// Given a Prefix and a number of digits to generate after prefix, generate a random name and return in sRetName
void connection::genRandomName( const string& sPrefix, int iNumRandomDigits, string& sRetName )
{
	// Start with Prefix
	sRetName.assign( sPrefix );

	// Generate random digits
	for ( int i = 0; i < iNumRandomDigits; ++i )
		sRetName.append( 1, ((rand() % 10) + '0') );
}
/****************************************************************************\
 * Message Reading/Writing													*
\****************************************************************************/

// Throttles the reading to lines from the IRC server, reads from socket and stores information into a stringstream, 
//	returns the next line from the stringstream to the caller.
int connection::getNextLine( string& sNextLine )
{
	// Local Variables
	char sBuffer[ MAX_BUFFER_SIZE ] = { '\0' };
	int iBytesRead;
	string sPing;

	do
	{
		// Clear return and read from socket.
		sNextLine.clear();
		sPing.clear();
		iBytesRead = Socket_Read( sBuffer, MAX_BUFFER_SIZE );

		// If something was read from the socket, write it into the stream
		if ( iBytesRead > 0 )
		{
			m_sStream.clear();
			m_sStream.write( sBuffer, iBytesRead );
		}

		// Get the next line from the stream
		if ( !(EOF == m_sStream.peek()) )
		{
			getline( m_sStream, sNextLine );

			// Handle Ping here since controller doesn't constantly poll for input.
			sPing = sNextLine.substr( 0, sNextLine.find_first_of( ' ' ) );
			if ( !sPing.compare( "PING" ) )
				Socket_Write( "PONG :" + sNextLine.substr( sNextLine.find_first_of( ':' ) ) + "\r\n" );
		}
	} while ( !sPing.compare( "PING" ) ); // Handle Ping Pong

	// Return number of bytes read.
	return sNextLine.length();
}

// Clears string stream then reads and ignores all input waiting on socket stream.
void connection::flushConnection()
{
	// Local Variables
	char sBuffer[ MAX_BUFFER_SIZE ] = { '\0' };
	string sPING( "PING" );
	int iBytesRead = 0;

	// Clear String Stream
	m_sStream.ignore();
	m_sStream.clear();

	// Flush Socket Input
	while ( (iBytesRead = Socket_Read( sBuffer, MAX_BUFFER_SIZE )) > 0 );
}

// Read in a line of text from the client, up to a maximum of iBuffSize
// Returns number of bytes read.
int connection::Socket_Read( char* pBuffer, int iBuffSize )
{
	// Local Variables
	int n = 0;

	// Read from set Socket
	bzero( pBuffer, iBuffSize );
	n = read( m_iConnectedSocket, pBuffer, iBuffSize );

	// Error Handling
	if ( n < 0 && EAGAIN != errno )
		n = error( "Error: No Message Read\n" );
	else if ( n > iBuffSize )
		n = error( "Error: Potential Overflow read." );
	else if ( 0 == n ) // disconnected
	{
		forceReconnect();
		n = Socket_Read( pBuffer, iBuffSize ); // Retry the read.
	}

	// Return number of bytes read.
	return n;
}

// Write sMsg over the connection.
// Returns number of bytes written
int connection::Socket_Write( string sMsg, int iSocket )
{
	// Local Variables
	int n = 0;

	if ( 0 > iSocket )
		iSocket = m_iConnectedSocket;

	// Write to Socket
	n = write( iSocket, sMsg.c_str(), sMsg.size() );

	// Error Handling
	if ( n < 0 )
		n = error( "Error: Unable to Write Message.\n" );
	else if ( 0 == n ) // Reconnect on Disconnect
	{
		forceReconnect();
		n = Socket_Write( sMsg, iSocket ); // Retry the write.
	}

	// Return number of bytes written.
	return n;
}

// Issues a Command on the current channel.
int connection::issueCommand( const string& sCommand )
{
	return Socket_Write( "PRIVMSG #" + m_sChannel + " :" + sCommand + "\r\n" );
}