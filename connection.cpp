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
#define MAX_TIMEOUT 1
#define MAX_BUFFER_SIZE 256

// Returns the singleton instance of connection

// Default Constructor
connection::connection( const string& sServerName, int iPort, const string& sChannel, string sNickName )
{
	m_sHostName.assign( sServerName );
	m_iPort = iPort;
	m_sChannel.assign( sChannel );
	m_bConnected = false;
	srand( time( NULL ) );
	m_iCounter = 0;
	m_sNickName.assign( sNickName );
	
	m_bConnected = connectSocket( m_iConnectedSocket, m_sClientAddr,
								  m_sHostName, m_iPort );
}

// Destructor -> lose reference to Server
connection::~connection()
{
	close( m_iConnectedSocket );
}

// Initializes the socket connection to the IRC server, doesn't send any IRC messages yet.
bool connection::connectSocket( int& iSocket, struct sockaddr_in& sAddr,
								const string& sHostName, int iPort )
{
	// Return Value
	bool bReturnValue = false;
	int iConnectResult = -1;
	iSocket = socket( AF_INET, SOCK_STREAM, 0 );
	struct timeval tv;
	tv.tv_sec = MAX_TIMEOUT;
	tv.tv_usec = 0;
	fd_set connectSet;
	int iFlags;

	// Check creation success
	if ( iSocket > 0 )
	{
		// Zero out Socket Addr
		bzero( (char*) &sAddr, sizeof( sAddr ) );

		// Set socket to be non blocking
		iFlags = fcntl( iSocket, F_GETFL, NULL );
		iFlags |= O_NONBLOCK;
		fcntl( iSocket, F_SETFL, iFlags);

		// Code pulled from "www.technical-recipes.com/2014/getting-started-with-client-server-applications-in-c/"
		struct hostent *host = gethostbyname( sHostName.c_str() );
		char* localIP = inet_ntoa( *(in_addr*) *host->h_addr_list );

		// Set up Server Address parameters
		sAddr.sin_family = AF_INET;
		sAddr.sin_addr.s_addr = inet_addr( localIP );
		sAddr.sin_port = htons( iPort );

		// Attempt to connect
		do
		{
			iConnectResult = connect( iSocket, (sockaddr*) &sAddr, sizeof( sAddr ) );
			bReturnValue = !iConnectResult;

			cout << "Connection returned: " << iConnectResult << endl;

			if ( (iConnectResult < 0) && (EINPROGRESS != errno) )
			  {
				fprintf( stderr, "Error: %s\n", strerror( errno ) );
				break;
			  }
			else if ( !bReturnValue )	// Couldn't immediately establish connection
			{
				cout << "Unable to immediately establish connection!\n";
				tv.tv_sec = 5; // Set connect wait time to 5 seconds
				FD_ZERO( &connectSet );
				FD_SET( iSocket, &connectSet );
				iConnectResult = select( iSocket + 1, NULL, &connectSet, NULL, &tv );

				cout << "Select returned: " << iConnectResult << endl;
				switch ( iConnectResult )
				{
					case(0):
						fprintf( stderr, "Error: timeout on Connect.\n" );
						break;
					case (-1):
						fprintf( stderr, "Error on select: %s\n", strerror( errno ) );
						break;
					default:	// Successful connection
						if( FD_ISSET( iSocket, &connectSet ) )
							bReturnValue = true;
						break;
				}
			}
		} while ( !bReturnValue );

		cout << "Successfully connected!\n";
		tv.tv_sec = MAX_TIMEOUT; // Set back to read Timeout
		iFlags = fcntl( iSocket, F_GETFL, NULL );
		iFlags &= (~O_NONBLOCK);
		fcntl( iSocket, F_SETFL, iFlags ); // Set socket back to blocking.


		// Set a timeout for reading from socket.
		// Pulled from https://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout
		setsockopt( iSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof( struct timeval ) );
	}

	return bReturnValue;
}

void connection::connectToServer( const string& sNamePrefix )
{
	// Local Variables
	string sGeneratedName;
	char sBuffer[ MAX_BUFFER_SIZE ] = { '\0' };
	int iBytesRead = numeric_limits<int>::max();

	do
	{
		// Generate Random Name:
		if( m_sNickName.empty() )
			genRandomName( sNamePrefix, RANDOM_DIGITS, sGeneratedName );

		Socket_Write( "NICK " + sGeneratedName + "\n" );
		iBytesRead = Socket_Read( sBuffer, MAX_BUFFER_SIZE );
	} while ( iBytesRead > 0 );

	if( m_sNickName.empty() )
		m_sNickName.assign( sGeneratedName );

	Socket_Write( "USER " + m_sNickName + " * * :" + m_sNickName + "\n" );
	Socket_Write( "JOIN #" + m_sChannel + "\n" );
}

bool connection::attack( const string& sHostName, int iPort )
{
	// Local Variables
	int iSocket;
	struct sockaddr_in sAddr;
	string sMsg;
	bool bReturnValue = connectSocket( iSocket, sAddr, sHostName, iPort );
	++m_iCounter;

	if ( bReturnValue )
	{
		sMsg = m_sNickName + " Counter: " + to_string( m_iCounter ) + "\n";
		bReturnValue = (Socket_Write( sMsg, iSocket ) > 0);
		close( iSocket );
	}

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

void connection::genRandomName( const string& sPrefix, int iNumRandomDigits, string& sRetName )
{
	sRetName.assign( sPrefix );

	for ( int i = 0; i < iNumRandomDigits; ++i )
		sRetName.append( 1, ((rand() % 10) + '0') );
}
/****************************************************************************\
 * Message Reading/Writing													*
\****************************************************************************/

// Throttles the reading to lines from the IRC server, reads from socket and stores information into a stringstream, 
//	returns the next line fo the stringstream to the caller.
int connection::getNextLine( string& sNextLine )
{
	// Local Variables
	char sBuffer[ MAX_BUFFER_SIZE ] = { '\0' };
	int iBytesRead;
	
	// Clear return and read from socket.
	sNextLine.clear();
	iBytesRead = Socket_Read( sBuffer, MAX_BUFFER_SIZE );

	// If something was read from the socket, write it into the stream
	if ( iBytesRead > 0 )
	{
		m_sStream.clear();
		m_sStream.write( sBuffer, iBytesRead );
	}
	else if ( !iBytesRead )
	{
		cout << "Socket Was Closed!\n";
		/* Disconnected Reconnect
		close( m_iConnectedSocket );
		m_bConnected = connectSocket( m_iConnectedSocket, m_sClientAddr,
									  m_sHostName, m_iPort );
		//*/
	}

	// Get the next line from the stream
	if ( !(EOF == m_sStream.peek()) )
	{
		getline( m_sStream, sNextLine );
		cout << "Read:: " << sNextLine << endl;
	}

	// Return number of bytes read.
	return sNextLine.length();
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
	if ( n < 0 && EWOULDBLOCK != errno )
		n = error( "Error: No Message Read\n" );
	else if ( n > iBuffSize )
		n = error( "Error: Potential Overflow read." );

	// Return number of bytes read.
	return n;
}

void connection::sendStatusMsg( const string& sTarget )
{
	Socket_Write( "PRIVMSG " + sTarget + " : Bot\n" );
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

	// Return number of bytes written.
	return n;
}
