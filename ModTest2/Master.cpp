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

// 16비트 빅엔디언으로 변환 후 버퍼 저장
void putUInt16(std::vector<unsigned char>& buf, int offset, unsigned short value)
{
	unsigned short netValue = htons(value);
	unsigned char* p = reinterpret_cast<unsigned char*> (&netValue);
	buf[offset] = p[0];
	buf[offset + 1] = p[1];
}

// MBAP 헤더 생성 함수
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

// 바이트 벡터 > 헥사 문자열 변환
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
	// Winsock 초기화   
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		std::cerr << "WSAStart up failed" << std::endl;
		return 1;
	}

	// 슬레이브(계측기) 정보 입력
	std::string serverIP;
	std::cout << "계측 장비 IP 주소를 입력해주세요 :";
	std::getline(std::cin, serverIP);

	if (serverIP.empty())
	{
		std::cerr << "IP 주소를 입력하세요" << std::endl;
		WSACleanup();
		return 1;
	}

	std::string serverPort;
	int port;
	std::cout << "계측장비의 Port를 입력해주세요 :";
	std::getline(std::cin, serverPort);

	if (serverPort.empty())
	{
		std::cerr << "포트 번호를 입력하세요" << std::endl;
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
			std::cerr << " 포트번호가 잘못되었습니다. " << std::endl;
			WSACleanup();
			return 1;
		}
	}

	// 클라이언트 소켓 생성 및 연결
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << " 소켓생성 실패 " << std::endl;
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	if (InetPtonA(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) != 1)
	{
		std::cerr << "IP 주소 변환 실패" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cerr << " 연결 실패" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::cout << "연결된 계측기 정보" << serverIP << " : " << port << std::endl;

	// 트랜잭셕 ID 요청마다 증가
	unsigned short transactionId = 1;

	// 사용자 입력 반복 루프
	while (true)
	{
		std::cout << "\n===== Modbus Menu ====" << std::endl;
		std::cout << "1. Read Holding Registers (0X03)" << std::endl;
		std::cout << "2. Read Input Registers (0X04)" << std::endl;
		std::cout << "3. Write Multi Registers (0X10)" << std::endl;
		std::cout << "4. Exit" << std::endl;
		std::cout << "Function Code 입력 :";

		int choice;
		std::cin >> choice;

		if (choice == 4)
		{
			break;
		}

		if (choice < 1 || choice > 4)
		{
			std::cout << "번호 다시 입력" << std::endl;
			continue;
		}

		// 슬레이브 ID 입력
		unsigned int slaveId;
		std::cout << "계측 ID를 입력 : ";
		std::cin >> slaveId;

		std::vector<unsigned char> pdu;

		if (choice == 1 || choice == 2)
		{
			unsigned int startAddr, quantity;

			std::cout << "시작 주소 입력 :";
			std::cin >> startAddr;

			std::cout << "수량 입력 : ";
			std::cin >> quantity;


			// PDU = 펑션코드 (1) + 시작주소 (2) + 수량(2) = 5바이트
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
			std::cout << " 시작 주소 입력 :";
			std::cin >> startAddr;
			std::cout << " 수량 입력 : ";
			std::cin >> quantity;

			// 레지스터값 벡터에 저장
			std::vector<unsigned short> values(quantity);
			std::cout << " 수량 " << quantity << " 레지스터 값 : " << std::endl;

			for (unsigned int i = 0; i < quantity; i++)
			{
				std::cin >> values[i];
			}

			// PDU 구성
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

		// MBAP 헤더생성
		std::vector<unsigned char> mbap = buildMBAPHeader(transactionId, (unsigned short)pdu.size(), (unsigned char)slaveId);



		// 전체 요청 메시지 = MBAP 헤더 + PDU
		std::vector<unsigned char> requestMsg;
		requestMsg.insert(requestMsg.end(), mbap.begin(), mbap.end());
		requestMsg.insert(requestMsg.end(), pdu.begin(), pdu.end());

		// 요청메시지 헥사로
		std::cout << "Request :" << byteToHex(requestMsg) << std::endl;


		const int POLL_INTERVAL_MS = 500;

		while (true)
		{
			// 요청 전송
			int sendResult = send(sock, reinterpret_cast<const char*>(requestMsg.data()), static_cast<int>(requestMsg.size()), 0);

			if (sendResult == SOCKET_ERROR)
			{
				std::cerr << "수신 오류" << std::endl;
				goto outer_loop;
			}

			// 응답  수신
			char recvBuffer[512] = { 0 };
			int recvResult = recv(sock, recvBuffer, static_cast<int>(sizeof(recvBuffer)), 0);

			if (recvResult > 0)
			{
				std::vector<unsigned char> responseVec(recvBuffer, recvBuffer + recvResult);
				std::cout << "응답 : " << byteToHex(responseVec) << std::endl;
			}
			else
			{
				std::cerr << "응답 실패 혹은 연결 끊김" << std::endl;
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
					std::cout << "폴링 모드 끝" << std::endl;
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