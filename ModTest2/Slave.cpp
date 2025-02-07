#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <ctime>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

#define PORT 502
#define REG_COUNT 125 //�������� ����

// �������� �迭
unsigned short holdingRegisters[REG_COUNT];
unsigned short inputRegisters[REG_COUNT];


// �������� �ʱ�ȭ
void initRegisters()
{
	for (int i = 0; i < REG_COUNT; i++)
	{
		holdingRegisters[i] = 0;
		inputRegisters[i] = 123;
	}
}






int main()
{
	WSADATA wsaData;
	SOCKET serverSocket, clientSocket;

	// ���� �õ�
	srand(static_cast<unsigned int>(time(nullptr)));

	// Winsock �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		std::cerr << "Intialize Winsock Error : " << WSAGetLastError() << std::endl;
		return 1;
	}

	// ���� ���� ����
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket == INVALID_SOCKET)
	{
		std::cerr << "Socket Create failed : " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// ���� �ּ� ����
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);

	// ���ε�
	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "Bind failed " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// ����
	if (listen(serverSocket, 5) == SOCKET_ERROR)
	{
		std::cerr << "Listen failed" << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "[SERVER] Listening port " << std::endl;

	// ���ѷ��� Ŭ���̾�Ʈ ���� ���
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(SOCKADDR_IN);

		// Ŭ���̾�Ʈ ���� Accept
		SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Accept failed : " << WSAGetLastError() << std::endl;
			continue;
		}

		std::cout << "Client Connected" << std::endl;

		// Modbus TCP ������ ����
		const int modLen = 10;
		unsigned char txData[modLen];

		for (int i = 0; i < modLen; i++)
		{
			txData[i] = static_cast<unsigned char>(rand() & 0xFF);
		}

		std::cout << "random bytes (hex) : ";
		for (int i = 0; i < modLen; i++)
		{
			printf("%02X", txData[i]);
		}
		std::cout << std::endl;

		// Ŭ���̾�Ʈ ����
		int sendBytes = send(clientSocket, reinterpret_cast<const char*>(txData), modLen, 0);

		if (sendBytes == SOCKET_ERROR)
		{
			std::cerr << "Send failed" << WSAGetLastError() << std::endl;
		}
		else
		{
			std::cout << "Send " << sendBytes << " bytes to client" << std::endl;
		}

		closesocket(clientSocket);

	}


	// ���� ���� ����
	closesocket(serverSocket);
	WSACleanup();
	return 0;


}