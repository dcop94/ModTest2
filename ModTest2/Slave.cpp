#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define PORT 502
#define REG_COUNT 125 //레지스터 개수

// 레지스터 배열
unsigned short holdingRegisters[REG_COUNT];
unsigned short inputRegisters[REG_COUNT];


// 레지스터 초기화
void initRegisters()
{
	for (int i = 0; i < REG_COUNT; i++)
	{
		holdingRegisters[i] = 0;
		inputRegisters[i] = 123;
	}
}

// 빅엔디언 변환
void putUInt16(std::vector<unsigned char>& buf, int offset, unsigned short value)
{
	// value를 htons 네트워크 맞게 빅엔디언 변환
	unsigned short netValue = htons(value);
	
	// netValue 메모리 주소를 포인터로 두 바이트 복사 (형 변환 bit수맞게)
	unsigned char* p = reinterpret_cast<unsigned char*>(&netValue);

	// buf
	buf[offset] = p[0]; // 높은 바이트
	buf[offset + 1] = p[1]; // 낮은 바이트
}

// 예외 응답 함수
std::vector<unsigned char> exResponse(unsigned short transactionId, unsigned char unitId, unsigned char functionCode, unsigned char exceptionCode)
{
	std::vector<unsigned char> resp(9, 0);

	// 트랜잭션 ID 상위, 하위 8비트 빅엔디언
	resp[0] = (transactionId >> 8) & 0xFF; // 트랜잭션 ID 상위 8비트
	resp[1] = transactionId & 0xFF; // 트랜잭션 ID 하위 8비트

	// 프로토콜 ID TCP는 항상 0
	resp[2] = 0;
	resp[3] = 0;

	// Length Unit ID 1바이트, PDU 2바이트
	resp[4] = 0;
	resp[5] = 3; // Length = unit id(1) pdu (2)

	// 슬레이브 ID 식별
	resp[6] = unitId;

	// 예외응답
	resp[7] = functionCode | 0x80;

	// 오류 발생알림
	resp[8] = exceptionCode;

	return resp;
}

// 펑션코드별 처리 함수
std::vector<unsigned char> readHoldingRegisters(unsigned short transactionId, unsigned char unitId, const std::vector<unsigned char>& request)
{
	// 요청 메시지 길이 검사, 12바이트 보다 짧으면 예외응답
	if (request.size() < 12)
	{
		return exResponse(transactionId, unitId, 0x03, 0x03);
	}

	// PDU에서 시작주소 및 레지스터 수 읽기 인덱스 7부터 (인덱스 7은 펑션코드 0x03)
	unsigned short startAddr = (request[8] << 8) | request[9]; // 두 바이트 합치기 16비트
	unsigned short quantity = (request[10] << 8) | request[11]; // 읽을 레지스터 수
	std::cout << "[0x03] Read Holding Registers - 시작주소 : " << startAddr << ", 수량 :" << quantity << std::endl;

	// 주소 범위 및 수량 검사 
	if (quantity < 1 || quantity > 125 || (startAddr + quantity) > REG_COUNT)
	{
		return exResponse(transactionId, unitId, 0x03, 0x02); // 레지스터수가 1보다 작거나, 125초과하면 안되고 레지스터 범위가 넘어서는지 확인 오류발생이면 illegal data address 오류 발생
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
	for (int i = 0; i < quantity; i++)
	{
		unsigned short regVal = holdingRegisters[startAddr + i];

		resp[idx++] = (regVal >> 8) & 0xFF;
		resp[idx++] = regVal & 0xFF;
	}

	return resp;
	
}

std::vector<unsigned char> readInputRegisters(unsigned short transactionId, unsigned char unitId, const std::vector<unsigned char>& request)
{
	// 요청 메시지 길이 검사, 12바이트 보다 짧으면 예외응답
	if (request.size() < 12)
	{
		return exResponse(transactionId, unitId, 0x04, 0x03);
	}

	// PDU에서 시작주소 및 레지스터 수 읽기 인덱스 7부터 (인덱스 7은 펑션코드 0x03)
	unsigned short startAddr = (request[8] << 8) | request[9]; // 두 바이트 합치기 16비트
	unsigned short quantity = (request[10] << 8) | request[11]; // 읽을 레지스터 수
	std::cout << "[0x04] Read Input Registers - 시작주소 : " << startAddr << ", 수량 :" << quantity << std::endl;

	// 주소 범위 및 수량 검사 
	if (quantity < 1 || quantity > 125 || (startAddr + quantity) > REG_COUNT)
	{
		return exResponse(transactionId, unitId, 0x04, 0x02); // 레지스터수가 1보다 작거나, 125초과하면 안되고 레지스터 범위가 넘어서는지 확인 오류발생이면 illegal data address 오류 발생
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
	for (int i = 0; i < quantity; i++)
	{
		unsigned short regVal = holdingRegisters[startAddr + i];

		resp[idx++] = (regVal >> 8) & 0xFF;
		resp[idx++] = regVal & 0xFF;
	}

	return resp;

}

std::vector<unsigned char> writeMulipleRegisters(unsigned short transactionId, unsigned char unitId, const std::vector<unsigned char>& request)
{
	// 요청 메시지 길이 검사, 13바이트 보다 짧으면 예외응답
	if (request.size() < 13)
	{
		return exResponse(transactionId, unitId, 0x10, 0x03);
	}

	// PDU에서 시작주소 및 레지스터 수 읽기 인덱스 7부터 (인덱스 7은 펑션코드 0x03)
	unsigned short startAddr = (request[8] << 8) | request[9]; // 두 바이트 합치기 16비트
	unsigned short quantity = (request[10] << 8) | request[11]; // 읽을 레지스터 수
	unsigned char byteCount = request[12]; // 실제 전송되는 데이터 바이트 수 
	std::cout << "[0x10] Write Multiple Registers - 시작주소 : " << startAddr << ", 수량 :" << quantity << std::endl;

	// 주소 범위 및 수량 검사 
	if (quantity < 1 || quantity > 123 || (startAddr + quantity) > REG_COUNT || byteCount != quantity * 2)
	{
		return exResponse(transactionId, unitId, 0x10, 0x02); // 레지스터수가 1보다 작거나, 125초과하면 안되고 레지스터 범위가 넘어서는지 확인 오류발생이면 illegal data address 오류 발생
	}

	if (request.size() < 13 + quantity * 2)
	{
		return exResponse(transactionId, unitId, 0x10, 0x03);
	}

	int dataIdx = 13;
	for (int i = 0; i < quantity; i++)
	{
		unsigned short value = (request[dataIdx] << 8) | request[dataIdx + 1];
		holdingRegisters[startAddr + i] = value;
		dataIdx += 2;
	}

	int totalLength = 7 + 1 + 2 + 2; // 12바이트

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

int main()
{
	WSADATA wsaData;
	SOCKET serverSocket, clientSocket;

	// 랜덤 시드
	srand(static_cast<unsigned int>(time(nullptr)));

	// Winsock 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		std::cerr << "Intialize Winsock Error : " << WSAGetLastError() << std::endl;
		return 1;
	}

	// 서버 소켓 생성
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket == INVALID_SOCKET)
	{
		std::cerr << "Socket Create failed : " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// 소켓 주소 설정
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);

	// 바인드
	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "Bind failed " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// 리슨
	if (listen(serverSocket, 5) == SOCKET_ERROR)
	{
		std::cerr << "Listen failed" << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "[SERVER] Listening port " << std::endl;

	// 무한루프 클라이언트 연결 대기
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(SOCKADDR_IN);

		// 클라이언트 연결 Accept
		SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Accept failed : " << WSAGetLastError() << std::endl;
			continue;
		}

		std::cout << "Client Connected" << std::endl;

		// Modbus TCP 랜덤값 생성
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

		// 클라이언트 전송
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


	// 서버 소켓 종료
	closesocket(serverSocket);
	WSACleanup();
	return 0;


}