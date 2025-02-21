#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <limits>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

// 16��Ʈ �򿣵������ ��ȯ �� ���� ����
void putUInt16(std::vector<unsigned char>& buf, int offset, unsigned short value)
{
	unsigned short netValue = htons(value);
	unsigned char* p = reinterpret_cast<unsigned char*> (&netValue);
	buf[offset] = p[0];
	buf[offset + 1] = p[1];
}

// MBAP ��� ���� �Լ�
std::vector<unsigned char> buildMBAPHeader(unsigned short transactionId, unsigned short pduLength, unsigned char unitId)
{
	unsigned short lengthField = 1 + pduLength;
	std::vector<unsigned char> header(7, 0);

	header[0] = (transactionId >> 8) & 0xFF;
	header[1] = transactionId & 0xFF;
	header[2] = 0;
	header[3] = 0;
	header[4] = (lengthField >> 8) & 0xFF;
	header[5] = lengthField & 0xFF;
	header[6] = unitId;
	return header;
}

// ����Ʈ ���� > ��� ���ڿ� ��ȯ
std::string byteToHex(const std::vector<unsigned char>& vec)
{
	std::ostringstream oss;

	for (auto b : vec)
	{
		oss << std::setw(2) << std::setfill('0') << std::hex << (int)b << " ";
	}
	return oss.str();
}

int main()
{
	// Winsock �ʱ�ȭ   
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		std::cerr << "WSAStart up failed" << std::endl;
		return 1;
	}

	// �����̺�(������) ���� �Է�
	std::string serverIP;
	std::cout << "���� ��� IP �ּҸ� �Է����ּ��� :";
	std::getline(std::cin, serverIP);

	if (serverIP.empty())
	{
		std::cerr << "IP �ּҸ� �Է��ϼ���" << std::endl;
		WSACleanup();
		return 1;
	}

	std::string serverPort;
	int port;
	std::cout << "��������� Port�� �Է����ּ��� :";
	std::getline(std::cin, serverPort);

	if (serverPort.empty())
	{
		std::cerr << "��Ʈ ��ȣ�� �Է��ϼ���" << std::endl;
		WSACleanup();
		return 1;
	}
	else
	{
		try
		{
			port = std::stoi(serverPort);
		}
		catch (...)
		{
			std::cerr << " ��Ʈ��ȣ�� �߸��Ǿ����ϴ�. " << std::endl;
			WSACleanup();
			return 1;
		}
	}

	// Ŭ���̾�Ʈ ���� ���� �� ����
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << " ���ϻ��� ���� " << std::endl;
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	if (InetPtonA(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) != 1)
	{
		std::cerr << "IP �ּ� ��ȯ ����" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cerr << " ���� ����" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::cout << "����� ������ ����" << serverIP << " : " << port << std::endl;

	// Ʈ����� ID ��û���� ����
	unsigned short transactionId = 1;

	// ����� �Է� �ݺ� ����
	while (true)
	{
		std::cout << "\n===== Modbus Menu ====" << std::endl;
		std::cout << "1. Read Holding Registers (0X03)" << std::endl;
		std::cout << "2. Read Input Registers (0X04)" << std::endl;
		std::cout << "3. Write Multi Registers (0X10)" << std::endl;
		std::cout << "4. Exit" << std::endl;
		std::cout << "Function Code �Է� :";

		int choice;
		std::cin >> choice;

		if (choice == 4)
		{
			break;
		}

		if (choice < 1 || choice > 4)
		{
			std::cout << "��ȣ �ٽ� �Է�" << std::endl;
			continue;
		}

		// �����̺� ID �Է�
		unsigned int slaveId;
		std::cout << "���� ID�� �Է� : ";
		std::cin >> slaveId;

		std::vector<unsigned char> pdu;

		if (choice == 1 || choice == 2)
		{
			unsigned int startAddr, quantity;

			std::cout << "���� �ּ� �Է� :";
			std::cin >> startAddr;

			std::cout << "���� �Է� : ";
			std::cin >> quantity;


			// PDU = ����ڵ� (1) + �����ּ� (2) + ����(2) = 5����Ʈ
			pdu.resize(5, 0);

			if (choice == 1)
			{
				pdu[0] = 0x03;
			}
			else
			{
				pdu[0] = 0x04;
			}

			pdu[1] = (startAddr >> 8) & 0xFF;
			pdu[2] = startAddr & 0xFF;
			pdu[3] = (quantity >> 8) & 0xFF;
			pdu[4] = quantity & 0xFF;

		}
		else if (choice == 3)
		{
			unsigned int startAddr, quantity;
			std::cout << " ���� �ּ� �Է� :";
			std::cin >> startAddr;
			std::cout << " ���� �Է� : ";
			std::cin >> quantity;

			// �������Ͱ� ���Ϳ� ����
			std::vector<unsigned short> values(quantity);
			std::cout << " ���� " << quantity << " �������� �� : " << std::endl;

			for (unsigned int i = 0; i < quantity; i++)
			{
				std::cin >> values[i];
			}

			// PDU ����
			pdu.resize(5 + 1 + quantity * 2, 0);
			pdu[0] = 0x10;
			pdu[1] = (startAddr >> 8) & 0xFF;
			pdu[2] = startAddr & 0xFF;
			pdu[3] = (quantity >> 8) & 0xFF;
			pdu[4] = quantity & 0xFF;
			pdu[5] = quantity * 2;

			int idx = 6;

			for (unsigned int i = 0; i < quantity; i++)
			{
				unsigned short val = values[i];
				pdu[idx++] = (val >> 8) & 0xFF;
				pdu[idx++] = val & 0xFF;
			}
		}

		// MBAP �������
		std::vector<unsigned char> mbap = buildMBAPHeader(transactionId, (unsigned short)pdu.size(), (unsigned char)slaveId);



		// ��ü ��û �޽��� = MBAP ��� + PDU
		std::vector<unsigned char> requestMsg;
		requestMsg.insert(requestMsg.end(), mbap.begin(), mbap.end());
		requestMsg.insert(requestMsg.end(), pdu.begin(), pdu.end());

		// ��û�޽��� ����
		std::cout << "Request :" << byteToHex(requestMsg) << std::endl;


		const int POLL_INTERVAL_MS = 500;

		while (true)
		{
			// ��û ����
			int sendResult = send(sock, reinterpret_cast<const char*>(requestMsg.data()), static_cast<int>(requestMsg.size()), 0);

			if (sendResult == SOCKET_ERROR)
			{
				std::cerr << "���� ����" << std::endl;
				goto outer_loop;
			}

			// ����  ����
			char recvBuffer[512] = { 0 };
			int recvResult = recv(sock, recvBuffer, static_cast<int>(sizeof(recvBuffer)), 0);

			if (recvResult > 0)
			{
				std::vector<unsigned char> responseVec(recvBuffer, recvBuffer + recvResult);
				std::cout << "���� : " << byteToHex(responseVec) << std::endl;
			}
			else
			{
				std::cerr << "���� ���� Ȥ�� ���� ����" << std::endl;
				goto outer_loop;
			}

			transactionId++;

			mbap = buildMBAPHeader(transactionId, static_cast<unsigned short> (pdu.size()), static_cast<unsigned char>(slaveId));

			requestMsg.clear();
			requestMsg.insert(requestMsg.end(), mbap.begin(), mbap.end());
			requestMsg.insert(requestMsg.end(), pdu.begin(), pdu.end());

			Sleep(POLL_INTERVAL_MS);

			if (_kbhit())
			{
				char c = _getch();

				if (c == 'q' || c == 'Q')
				{
					std::cout << "���� ��� ��" << std::endl;
					break;
				}
			}
		}

	outer_loop:
		std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');


	}

	closesocket(sock);
	WSACleanup();
	return 0;

}