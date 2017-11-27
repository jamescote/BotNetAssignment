// file: client.cpp
// Desc: Implements the client side logic for the FTP server transfer.
// Written by: James Cote
// ID: 10146559
// Tutorial: T01

// Includes
#include "connection.h"

// Defines
#define COMMAND 1
#define FILE_NAME 2
#define IP_PORT 3
#define CIPHER 4
#define SECRET_KEY 5
#define MAX_ARG_COUNT 6
#define IP_PORT_COUNT 2

// Globals
input_Handler* pInptHndlr;

// Function Declarations
inline int error( string sMsg );
void outputUseInfo();
void handleCmdLineArgs( int iArgC, char* pArgs[], int& iPort, string& sIP );
bool connectToServer( int iPort, const string& sIP, int& iSocket, struct sockaddr_in& sSocketAddr );
string generateNonce( );
bool authenticate( int iSocket );
void doRead( int iSocket );
void doWrite( int iSocket );

// Main
int main( int iArgC, char* pArgs[] )
{
	// local Variables
	int iSocket, iPort;
	struct sockaddr_in sSocketAddr;
	string sIP, sPlainText;

	// Verify Arguments
	if ( MAX_ARG_COUNT != iArgC )
		outputUseInfo();

	// Initialize Input Handler
	pInptHndlr = input_Handler::getInstance();

	// Handle arguments
	handleCmdLineArgs( iArgC, pArgs, iPort, sIP );

	if ( connectToServer( iPort, sIP, iSocket, sSocketAddr ) )
	{
		// Set up Initialization Vector and Session Key.
		pInptHndlr->setNonce( generateNonce() );
		pInptHndlr->generateIVSK();

		// Send first Message (Unencrypted)
		pInptHndlr->Socket_Write( pInptHndlr->getCipher() + " " + pInptHndlr->getNonce(),
								  iSocket );

		// Authenticate
		if ( authenticate( iSocket ) )
		{
			// Send Command type and File Name
			pInptHndlr->Socket_Write_Encrypted( pInptHndlr->getCommand() + " " + pInptHndlr->getFileName(),
												iSocket );

			// Evaluate response:
			pInptHndlr->Socket_Read_Decrypted( iSocket, sPlainText );

			// If Request is able to be processed, process request.
			if ( !sPlainText.compare( pInptHndlr->getStandardMessage( input_Handler::REQ_SUCCESS ) ) )
			{
				if ( !pInptHndlr->getCommand().compare( "read" ) )
					doRead( iSocket );
				else if ( !pInptHndlr->getCommand().compare( "write" ) )
					doWrite( iSocket );
			}
		}
		else	// Set Error and output Error Message.
			pInptHndlr->error( input_Handler::ERROR_WRONG_KEY );

		// Output Result
		fprintf( stderr, "%s", pInptHndlr->getErrorMessage().c_str() );
	}

	// Clean up.
	delete pInptHndlr;
	close( iSocket );

	return 1;
}

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
	sMessage += "Usage: ./client <command> <filename> <ip>:<port> <cipher> <key>\n";
	sMessage += "\t-<command>: \"read\" or \"write\" options for the server.\n";
	sMessage += "\t-<filename>: the filename to access or write to on the server.\n";
	sMessage += "\t-<ip>: the ip-address of the server to connect to.\n";
	sMessage += "\t-<port>: the port number of the server to connect to.\n";
	sMessage += "\t-<cipher>: \"null\", \"aes128\", or \"aes256\". Sets encryption method between client and server.\n";
	sMessage += "\t-<key>: secret key to use when connecting to the server. Must match server's key.\n";

	fprintf( stderr, "%s", sMessage.c_str() );
	exit( 1 );
}

// Parses command line arguments and sets up Input Handler for connection.
void handleCmdLineArgs( int iArgC, char* pArgs[], int& iPort, string& sIP )
{
	// Local Variables
	vector< string > sIP_Port;

	// Set up Input Handler with input.
	pInptHndlr->setCommand( !strcmp( pArgs[ COMMAND ], "read" ) );
	pInptHndlr->setFileName( pArgs[ FILE_NAME ] );
	pInptHndlr->setSecretKey( pArgs[ SECRET_KEY ] );
	pInptHndlr->setEncryptType( pArgs[ CIPHER ] );

	// Split IP/Port string
	pInptHndlr->splitString( pArgs[ IP_PORT ], ':', sIP_Port );

	// Error Handling
	if ( IP_PORT_COUNT != sIP_Port.size() )
		outputUseInfo();

	// Set Port and IP
	iPort = atoi( sIP_Port[ 1 ].c_str() );
	sIP.assign( sIP_Port[ 0 ] );
}

// Connects to a server on a given port and IP address.
// Returns the connected socket in iSocket and socket information in sSocketAddr.
// Returns true on success and false otherwise.
bool connectToServer( int iPort, const string& sIP, int& iSocket, struct sockaddr_in& sSocketAddr )
{
	// Local Variables
	bool bReturnValue = true;

	// create Socket
	iSocket = socket( AF_INET, SOCK_STREAM, 0 );

	// Check creation success
	if ( (bReturnValue = (iSocket > 0)) )
	{
		// Zero out Socket Addr
		bzero( (char*) &sSocketAddr, sizeof( sSocketAddr ) );

		// Code pulled from "www.technical-recipes.com/2014/getting-started-with-client-server-applications-in-c/"
		struct hostent *host = gethostbyname( sIP.c_str() );
		char* localIP = inet_ntoa( *(in_addr*) *host->h_addr_list );

		// Set up Server Address parameters
		sSocketAddr.sin_family = AF_INET;
		sSocketAddr.sin_addr.s_addr = inet_addr( localIP );
		sSocketAddr.sin_port = htons( iPort );

		bReturnValue = (connect( iSocket, (sockaddr*) &sSocketAddr, sizeof( sSocketAddr ) ) >= 0);

		// Did it work?
		if ( !bReturnValue )
			error( "ERROR connecting to Server.\n" );
	}

	return bReturnValue;
}

// Generates a Nonce value to be used for this session between Server and Client.
string generateNonce( )
{
	// Seed random number generator
	srand( time( NULL ) );
	string sNonce;

	// Generate 16 random digits as a Nonce.
	for ( int i = 0; i < NONCE_LENGTH; ++i )
		sNonce += to_string( rand() % 10 );

	// Set Nonce.
	return sNonce;
}

// Commences authentication protocol with server.
// Returns true on Success, false otherwise.
bool authenticate( int iSocket )
{
	// Local Variables
	string sPlainText, sHMAC;
	bool bGood;

	// Get Challenge
	bGood = pInptHndlr->Socket_Read_Decrypted( iSocket, sPlainText );

	if ( bGood )	// Compute HMAC
		bGood = pInptHndlr->computeHMAC( sPlainText, sHMAC );

	if ( bGood )	// Send response
		bGood = pInptHndlr->Socket_Write_Encrypted( sHMAC, iSocket );

	if ( bGood )	// Read Authorization
		bGood = pInptHndlr->Socket_Read_Decrypted( iSocket, sPlainText );

	// Return result.
	return bGood && !sPlainText.compare( pInptHndlr->getStandardMessage( input_Handler::AUTH_SUCCESS ) );
}

// Handle Read functionality (Server -> Client)
void doRead( int iSocket )
{
	// Local Variables
	string sPlainText;
	int iStatus;

	// Send initial message to indicate Client is ready to receive.
	pInptHndlr->Socket_Write_Encrypted( "Ready", iSocket );
	iStatus = pInptHndlr->Socket_Read_Decrypted( iSocket, sPlainText, true );

	// Check that a Failure hadn't occurred
	iStatus = sPlainText.compare( pInptHndlr->getStandardMessage( input_Handler::AUTH_FAILURE ) );

	// while bytes have been read in, output them to standard output.
	while ( iStatus )
	{
		cout << sPlainText;
		iStatus = pInptHndlr->Socket_Read_Decrypted( iSocket, sPlainText, true );
	}

	// If result is less than 0, an error occurred.
	if ( iStatus < 0 || !sPlainText.empty() )
		pInptHndlr->error( input_Handler::ERROR_NOT_READ );
}

// Writes standard input to Server until EOF reached or Server closes connection.
void doWrite( int iSocket )
{
	// Local Variables
	int iWriteResult = 1;
	char cBlock[ BLOCK_SIZE ] = { 0 };
	string sPlainText;

	// Keep looping until Server responds with error or cin hits eof.
	while ( iWriteResult > 0 && !cin.eof() )
	{
		cin.read( cBlock, BLOCK_SIZE - 1 );			// Read
		sPlainText.assign( cBlock, cin.gcount() );	// Assign to string
		iWriteResult = pInptHndlr->Socket_Write_Encrypted( sPlainText, iSocket );
		memset( cBlock, 0, BLOCK_SIZE );			// Reset buffer.
	}

	// Shutdown Socket to let Server know that the stream is finished.
	shutdown( iSocket, SHUT_WR );

	// Expecting a Finished confirmation to be sent if Successful
	// If Unsuccessful, server will close socket and the read will return 0.
	// Also set error flag if loop broken by iWriteResult being <= 0.
	if ( (0 >= pInptHndlr->Socket_Read_Decrypted( iSocket, sPlainText )) ||
		 (0 >= iWriteResult) )
		pInptHndlr->error( input_Handler::ERROR_NOT_WRITE );
}
