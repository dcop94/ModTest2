#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define PORT 502
#define REG_COUNT 125 //�������� ����                                                                                                                    v  

// �������� �迭
unsigned short holdingRegisters[REG_COUNT];
unsigned short inputRegisters[REG_COUNT];

std::mutex regMutex;

// �������� �ʱ�ȭ
void initRegisters()
{
	for (int i = 0; i < REG_COUNT; i++)                                                                                                                                                                                                                                                                                                                                                         
	{
		holdingRegisters[i] = 0;
		inputRegisters[i] = 123;
	}
}

// �򿣵�� ��ȯ
void putUInt16(std::vector<unsigned char>& buf, int offset, unsigned short value)
{
	// value�� htons ��Ʈ��ũ �°� �򿣵�� ��ȯ
	unsigned short netValue = htons(value);
	
	// netValue �ּҸ� unsigned char �����ͷ� ��ȯ�ϸ� �� ����Ʈ ����
	unsigned char* p = reinterpret_cast<unsigned char*>(&netValue);

	// ��ȯ�� �� buf�� ����
	buf[offset] = p[0]; // ���� ����Ʈ
	buf[offset + 1] = p[1]; // ���� ����Ʈ
}

// ���� ���� �Լ�
std::vector<unsigned char> exResponse(unsigned short transactionId, unsigned char unitId, unsigned char functionCode, unsigned char exceptionCode)
{
	std::vector<unsigned char> resp(9, 0);

	// Ʈ����� ID ����, ���� 8��Ʈ �򿣵��
	resp[0] = (transactionId >> 8) & 0xFF; // Ʈ����� ID ���� 8��Ʈ
	resp[1] = transactionId & 0xFF; // Ʈ����� ID ���� 8��Ʈ

	// �������� ID TCP�� �׻� 0
	resp[2] = 0;
	resp[3] = 0;

	// Length Unit ID 1����Ʈ, PDU 2����Ʈ
	resp[4] = 0;
	resp[5] = 3; // Length = unit id(1) pdu (2)

	// �����̺� ID �ĺ�
	resp[6] = unitId;

	// ��������
	resp[7] = functionCode | 0x80;

	// ���� �߻��˸�
	resp[8] = exceptionCode;

	return resp;
}

// ����ڵ庰 ó�� �Լ�
std::vector<unsigned char> readHoldingRegisters(unsigned short transactionId, unsigned char unitId, const std::vector<unsigned char>& request)
{
	// ��û �޽��� ���� �˻�, 12����Ʈ ���� ª���� ��������
	if (request.size() < 12)
	{
		return exResponse(transactionId, unitId, 0x03, 0x03);
	}

	// PDU���� �����ּ� �� �������� �� �б� �ε��� 7���� (�ε��� 7�� ����ڵ� 0x03)
	unsigned short startAddr = (request[8] << 8) | request[9]; // �� ����Ʈ ��ġ�� 16��Ʈ
	unsigned short quantity = (request[10] << 8) | request[11]; // ���� �������� ��
	std::cout << "[0x03] Read Holding Registers - �����ּ� : " << startAddr << ", ���� :" << quantity << std::endl;

	// �ּ� ���� �� ���� �˻� 
	if (quantity < 1 || quantity > 125 || (startAddr + quantity) > REG_COUNT)
	{
		return exResponse(transactionId, unitId, 0x03, 0x02); // �������ͼ��� 1���� �۰ų�, 125�ʰ��ϸ� �ȵǰ� �������� ������ �Ѿ���� Ȯ�� �����߻��̸� illegal data address ���� �߻�
	}

	int byteCount = quantity * 2;
	int pduLength = 1 + 1 + byteCount;
	int totalLength = 7 + pduLength;

	std::vector<unsigned char> resp(totalLength, 0);

	resp[0] = (transactionId >> 8) & 0xFF;
	resp[1] = transactionId & 0xFF;
	resp[2] = 0;
	resp[3] = 0;
	unsigned short lengthField = 1 + pduLength;
	resp[4] = (lengthField >> 8) & 0xFF;
	resp[5] = lengthField & 0xFF;
	resp[6] = unitId;

	resp[7] = 0x03;
	resp[8] = byteCount;
	
	int idx = 9;
	{
		std::lock_guard<std::mutex> lock(regMutex);

		for (int i = 0; i < quantity; i++)
		{
			unsigned short regVal = holdingRegisters[startAddr + i];

			resp[idx++] = (regVal >> 8) & 0xFF;
			resp[idx++] = regVal & 0xFF;
		}
	
	}

	return resp;
	
}

std::vector<unsigned char> readInputRegisters(unsigned short transactionId, unsigned char unitId, const std::vector<unsigned char>& request)
{
	// ��û �޽��� ���� �˻�, 12����Ʈ ���� ª���� ��������
	if (request.size() < 12)
	{
		return exResponse(transactionId, unitId, 0x04, 0x03);
	}

	// PDU���� �����ּ� �� �������� �� �б� �ε��� 7���� (�ε��� 7�� ����ڵ� 0x03)
	unsigned short startAddr = (request[8] << 8) | request[9]; // �� ����Ʈ ��ġ�� 16��Ʈ
	unsigned short quantity = (request[10] << 8) | request[11]; // ���� �������� ��
	std::cout << "[0x04] Read Input Registers - �����ּ� : " << startAddr << ", ���� :" << quantity << std::endl;

	// �ּ� ���� �� ���� �˻� 
	if (quantity < 1 || quantity > 125 || (startAddr + quantity) > REG_COUNT)
	{
		return exResponse(transactionId, unitId, 0x04, 0x02); // �������ͼ��� 1���� �۰ų�, 125�ʰ��ϸ� �ȵǰ� �������� ������ �Ѿ���� Ȯ�� �����߻��̸� illegal data address ���� �߻�
	}

	int byteCount = quantity * 2;
	int pduLength = 1 + 1 + byteCount;
	int totalLength = 7 + pduLength;

	std::vector<unsigned char> resp(totalLength, 0);

	resp[0] = (transactionId >> 8) & 0xFF;
	resp[1] = transactionId & 0xFF;
	resp[2] = 0;
	resp[3] = 0;
	unsigned short lengthField = 1 + pduLength;
	resp[4] = (lengthField >> 8) & 0xFF;
	resp[5] = lengthField & 0xFF;
	resp[6] = unitId;

	resp[7] = 0x04;
	resp[8] = byteCount;

	int idx = 9;
	{
		std::lock_guard<std::mutex> lock(regMutex);

		for (int i = 0; i < quantity; i++)
		{
			unsigned short regVal = holdingRegisters[startAddr + i];

			resp[idx++] = (regVal >> 8) & 0xFF;
			resp[idx++] = regVal & 0xFF;
		}
	}
	

	return resp;

}

std::vector<unsigned char> writeMulipleRegisters(unsigned short transactionId, unsigned char unitId, const std::vector<unsigned char>& request)
{
	// ��û �޽��� ���� �˻�, 13����Ʈ ���� ª���� ��������
	if (request.size() < 13)
	{
		return exResponse(transactionId, unitId, 0x10, 0x03);
	}

	// PDU���� �����ּ� �� �������� �� �б� �ε��� 7���� (�ε��� 7�� ����ڵ� 0x03)
	unsigned short startAddr = (request[8] << 8) | request[9]; // �� ����Ʈ ��ġ�� 16��Ʈ
	unsigned short quantity = (request[10] << 8) | request[11]; // ���� �������� ��
	unsigned char byteCount = request[12]; // ���� ���۵Ǵ� ������ ����Ʈ �� 
	std::cout << "[0x10] Write Multiple Registers - �����ּ� : " << startAddr << ", ���� :" << quantity << std::endl;

	// �ּ� ���� �� ���� �˻� 
	if (quantity < 1 || quantity > 123 || (startAddr + quantity) > REG_COUNT || byteCount != quantity * 2)
	{
		return exResponse(transactionId, unitId, 0x10, 0x02); // �������ͼ��� 1���� �۰ų�, 125�ʰ��ϸ� �ȵǰ� �������� ������ �Ѿ���� Ȯ�� �����߻��̸� illegal data address ���� �߻�
	}

	if (request.size() < 13 + quantity * 2)
	{
		return exResponse(transactionId, unitId, 0x10, 0x03);
	}

	int dataIdx = 13;
	{
		std::lock_guard<std::mutex> lock(regMutex);

		for (int i = 0; i < quantity; i++)
		{
			unsigned short value = (request[dataIdx] << 8) | request[dataIdx + 1];
			holdingRegisters[startAddr + i] = value;
			dataIdx += 2;
		}
	}
	
	int totalLength = 7 + 1 + 2 + 2; // 12����Ʈ

	std::vector<unsigned char> resp(totalLength, 0);

	resp[0] = (transactionId >> 8) & 0xFF;
	resp[1] = transactionId & 0xFF;
	resp[2] = 0;
	resp[3] = 0;
	resp[4] = 0;
	resp[5] = 6;
	resp[6] = unitId;

	resp[7] = 0x10;
	resp[8] = (startAddr >> 8) & 0xFF;
	resp[9] = startAddr & 0xFF;
	resp[10] = (quantity >> 8) & 0xFF;
	resp[11] = quantity & 0xFF;

	return resp;

}

std::vector<unsigned char> handleRequest(const std::vector<unsigned char>& request)
{
	if (request.size() < 8)
	{
		return std::vector<unsigned char>(); // ª�� ��û ����
	}

	unsigned short transactionId = (request[0] << 8) | request[1];
	unsigned short protocolId = (request[2] << 8) | request[3];
	unsigned short length = (request[4] << 8) | request[5];
	unsigned char unitId = request[6];

	if (protocolId != 0)
	{
		return std::vector<unsigned char>();
	}

	if (request.size() < 7 + (length - 1))
	{
		return std::vector<unsigned char>();
	}

	unsigned char functionCode = request[7];
	std::vector<unsigned char> response;

	switch (functionCode)
	{
	case 0x03:
		response = readHoldingRegisters(transactionId, unitId, request);
		break;

	case 0x04:
		response = readInputRegisters(transactionId, unitId, request);
		break;

	case 0x10:
		response = writeMulipleRegisters(transactionId, unitId, request);
		break;

	default:
		response = exResponse(transactionId, unitId, functionCode, 0x01);
		break;
	}

	return response;
}

// Ŭ���̾�Ʈ ��û ó�� �ڵ�
void handleClient(SOCKET clientSocket)
{
	while (true)
	{
		unsigned char mbap[7];
		int bytesRecv = recv(clientSocket, reinterpret_cast<char*>(mbap), 7, 0);

		if (bytesRecv != 7)
		{
			std::cerr << "MBAP ��� ���� ���� :" << bytesRecv << " ����Ʈ ���� " << std::endl;
			break;
		}

		unsigned short lengthField = (mbap[4] << 8) | mbap[5];
		int rem = lengthField - 1;

		std::vector<unsigned char> pdu(rem, 0);

		if (rem > 0)
		{
			bytesRecv = recv(clientSocket, reinterpret_cast<char*>(&pdu[0]), rem, 0);

			if (bytesRecv != rem)
			{
				std::cerr << "PDU ���� ����: " << bytesRecv << "����Ʈ ����" << std::endl;
				break;
			}
		}

		// MBAP ��� �� PDU�� �ϳ��� ��û ���ͷ� ����
		std::vector<unsigned char> request;
		request.insert(request.end(), mbap, mbap + 7);
		request.insert(request.end(), pdu.begin(), pdu.end());

		std::cout << "���� ��û : ";
		for (auto b : request)
		{
			printf("%02X", b);
		}

		std::cout << std::endl;

		// �б��û - ���������� ������Ʈ�� �� ����
		unsigned char functionCode = request[7];

		if (functionCode == 0x03 || functionCode == 0x04)
		{
			while (true)
			{
				std::vector<unsigned char> response = handleRequest(request);

				int sendBytes = send(clientSocket, reinterpret_cast<const char*>(&response[0]), response.size(), 0);

				if (sendBytes == SOCKET_ERROR)
				{
					std::cerr << "���� ����, ���� :" << WSAGetLastError() << std::endl;
					break;
				}
				else
				{
					std::cout << "Ŭ���̾�Ʈ�� " << sendBytes << " ����Ʈ ����" << std::endl;

					Sleep(1000);
				}
			
			}
			break; // ��Ʈ���ָ�� ���� Ŭ���̾�Ʈ ���� ����
		}

		// �����û �� ���� ����
		else 
		{
			std::vector<unsigned char> response = handleRequest(request);

			if (!response.empty())
			{
				int sendBytes = send(clientSocket, reinterpret_cast<const char*>(&response[0]), response.size(), 0);

				if (sendBytes == SOCKET_ERROR)
				{
					std::cerr << "���� ����, ���� :" << WSAGetLastError() << std::endl;
					break;
				}
				else
				{
					std::cout << "Ŭ���̾�Ʈ�� " << sendBytes << " ����Ʈ ����" << std::endl;

				}
			}
		}
	}
	closesocket(clientSocket);
	std::cout << "Ŭ���̾�Ʈ ��������" << std::endl;
}

int main()
{
	// �������� �ʱ�ȭ
	initRegisters();

	// ���� �����忡�� �������Ͱ� �ֱ��� ���� (�� ������ ����)
	std::thread updater([]()
		{
			while (true)
			{
				{
					std::lock_guard<std::mutex> lock(regMutex);

					for (int i = 0; i < REG_COUNT; i++)
					{
						holdingRegisters[i] = rand() % 0XFFFF;
						inputRegisters[i] = 100 + (rand() % 50);
					}
				}
				Sleep(1000);
			}
		});
	updater.detach();

	WSADATA wsaData;
	SOCKET serverSocket, clientSocket;

	// Winsock �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		std::cerr << "winsock �ʱ�ȭ ���� : " << WSAGetLastError() << std::endl;
		return 1;
	}

	// ���� ���� ����
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket == INVALID_SOCKET)
	{
		std::cerr << "���� ���� ���� ���� : " << WSAGetLastError() << std::endl;
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
		std::cout << "���ε� ���� : " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// ����
	if (listen(serverSocket, 5) == SOCKET_ERROR)
	{
		std::cerr << "������ ���� :" << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	std::cout << PORT << "��Ʈ���� ���� �������� " << std::endl;

	// ���� ���
	while (true)
	{
		// Ŭ���̾�Ʈ ���� Accept
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "���� ���� : " << WSAGetLastError() << std::endl;
			continue;
		}

		std::cout << "Ŭ���̾�Ʈ ����" << std::endl;

		// Ŭ���̾�Ʈ �� ������ ����
		std::thread clientThread(handleClient, clientSocket);
		clientThread.detach(); // ������ ������ ����
	}

	// ���� ���� ����
	closesocket(serverSocket);
	WSACleanup();
	return 0;

}