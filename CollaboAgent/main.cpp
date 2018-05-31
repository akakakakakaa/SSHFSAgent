#include <iostream>
#include <fstream>
#include <string>
#include <WinSock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <thread>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5901"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	while (true) {
		WSADATA wsaData;
		int iResult;

		SOCKET ListenSocket = INVALID_SOCKET;
		SOCKET ClientSocket = INVALID_SOCKET;

		struct addrinfo *result = NULL;
		struct addrinfo hints;

		int iSendResult;
		char recvbuf[DEFAULT_BUFLEN];
		int recvbuflen = DEFAULT_BUFLEN;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			return 1;
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the server address and port
		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Setup the TCP listening socket
		iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(result);

		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
		if (iResult == SOCKET_ERROR) {
			printf("set send timeout with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		tv.tv_sec = 3;
		tv.tv_usec = 0;
		iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
		if (iResult == SOCKET_ERROR) {
			printf("set send timeout with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		// Accept a client socket
		int meetSeqLength;
		bool isTimeout = false;

		while (true) {
			ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				printf("accept failed with error: %d\n", WSAGetLastError());
				closesocket(ListenSocket);
				WSACleanup();
				return 1;
			}

			// No longer need server socket
			//closesocket(ListenSocket);
			cout << "accepted" << endl;

			int sent = 0;
			char sendData[4] = { 0x12,0x34,0x56,0x78 };
			while (sent < 4) {
				iResult = send(ClientSocket, sendData + sent, 4 - sent, 0);
				if (iResult < 0) {
					cout << "send error" << endl;
					break;
				}
				else if (iResult == 0) {
					isTimeout = true;
					break;
				}
				else
					sent += iResult;
			}

			if (isTimeout) {
				isTimeout = false;
				closesocket(ClientSocket);
				continue;
			}

			int recved = 0;
			char recvData[4];
			while (recved < 4) {
				iResult = recv(ClientSocket, recvData + recved, 4 - recved, 0);
				if (iResult < 0) {
					cout << "recv error" << endl;
					break;
				}
				else if (iResult == 0) {
					isTimeout = true;
					break;
				}
				else
					recved += iResult;
			}

			if (isTimeout) {
				isTimeout = false;
				closesocket(ClientSocket);
				continue;
			}

			if (recvData[0] == 0x78 && recvData[1] == 0x56 && recvData[2] == 0x34 && recvData[3] == 0x12)
				break;
			else
				closesocket(ClientSocket);
		}
		cout << "connect success" << endl;

		// Receive until the peer shuts down the connection

		int recved = 0;
		while (recved < 4) {
			iResult = recv(ClientSocket, (char*)(&meetSeqLength) + recved, 4 - recved, 0);
			if (iResult < 0) {
				cout << "meetseq recv error" << endl;
				break;
			}
			else if (iResult == 0) {
				isTimeout = true;
				break;
			}
			else
				recved += iResult;
		}
		cout << "meetSeqLength is " << meetSeqLength << endl;

		recved = 0;
		char* meetSeq = new char[meetSeqLength];
		while (recved < meetSeqLength) {
			iResult = recv(ClientSocket, meetSeq + recved, meetSeqLength - recved, 0);
			if (iResult < 0) {
				cout << "meetseq recv error" << endl;
				exit(0);
			}
			else
				recved += iResult;
		}
		string meetSeqStr="";
		for (int i = 0; i < meetSeqLength; i++)
			meetSeqStr += meetSeq[i];
		cout << "meetSeq is " << meetSeqStr << endl;

		recved = 0;
		int passwordLength;
		while (recved < 4) {
			iResult = recv(ClientSocket, (char*)(&passwordLength) + recved, 4 - recved, 0);
			if (iResult < 0) {
				cout << "meetseq recv error" << endl;
				exit(0);
			}
			else
				recved += iResult;
		}
		cout << "passwordLength is " << passwordLength << endl;

		recved = 0;
		char* password = new char[passwordLength];
		while (recved < passwordLength) {
			iResult = recv(ClientSocket, password + recved, passwordLength - recved, 0);
			if (iResult < 0) {
				cout << "meetseq recv error" << endl;
				exit(0);
			}
			else
				recved += iResult;
		}
		string passwordStr="";
		for (int i = 0; i < passwordLength; i++)
			passwordStr += password[i];
		cout << "meetSeq is " << passwordStr << endl;

		string command = "sshfs.exe -h 163.180.117.53 -u " + meetSeqStr + " -p 7877 -d N -r /home/" + meetSeqStr + "/upload -x -y " + passwordStr;
		system(command.c_str());

		delete[] meetSeq;
		delete[] password;

		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

		// cleanup
		closesocket(ListenSocket);
		closesocket(ClientSocket);
		WSACleanup();
	}
}