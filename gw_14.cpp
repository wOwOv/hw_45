#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "RingBuffer.h"
#include "Profiler.h"


#define SERVERPORT 9000

struct SOCKETINFO;


struct myOverlapped
{
	WSAOVERLAPPED overlapped;
	bool type;				//0이면 recv 1이면 send

};


//소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	myOverlapped sendio;
	myOverlapped recvio;
	RingBuffer RecvQ;
	RingBuffer SendQ;
	SOCKET sock;
	long sendflag;
};


//작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg);

DWORD WINAPI MonitorThread(LPVOID arg);

//오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);

int main(int argc, char* argv[])
{
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return 1;
	}

	//입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL)
	{
		return 1;
	}

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(CPU 개수 * 2)개의 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL)
		{
			return 1;
		}
		CloseHandle(hThread);
	}
	hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);

	//socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		err_quit((char*)"socket()");
	}


	//bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit((char*)"bind()");
	}

	//SO_SNDBUF
	int optval = 0;
	retval = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		err_quit((char*)"SNDBUF()");
	}

	//listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		err_quit((char*)"listen()");
	}

	//데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes, flags;

	while (1)
	{
		//accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display((char*)"accept()");
			break;
		}
		printf("[TCP server] client connected : IP address=%s, Port=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));


		//소켓 정보 구조체 할당
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == NULL)
		{
			break;
		}
		ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
		ZeroMemory(&ptr->recvio.overlapped, sizeof(ptr->recvio.overlapped));
		ptr->sendio.type = 1;
		ptr->recvio.type = 0;
		ptr->sock = client_sock;


		//소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, (ULONG_PTR)ptr, 0);

		//비동기 입출력 시작
		WSABUF wsabuf;
		wsabuf.buf = ptr->RecvQ.GetRearBufferPtr();
		wsabuf.len = ptr->RecvQ.DirectEnqueueSize();
		flags = 0;
		

		retval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, &ptr->recvio.overlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				err_display((char*)"WSARecv()");
			}
			else
			{
				printf("[MAIN] WSARecv - retval : WSA_IO_PENDING, recvbytes : %d\n", recvbytes);
			}
		}
	}


	//윈속 종료
	WSACleanup();
	return 0;


}


//작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1)
	{
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		myOverlapped* myoverlapped;
		SOCKETINFO* ptr;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)& ptr, (LPOVERLAPPED*)&myoverlapped, INFINITE);

		//클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (retval == 0 || cbTransferred == 0)
		{
			if (retval == 0)
			{
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &myoverlapped->overlapped, &temp1, FALSE, &temp2);
				err_display((char*)"WSAGetOverlappedResult()");
			}
			closesocket(ptr->sock);
			printf("[TCP server] client ended: IP address=%s, Port=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			delete ptr;
			continue;
		}

		//recvio overlapped임
		if (myoverlapped->type == 0)
		{

			ptr->RecvQ.MoveRear(cbTransferred);
			char* recvtemp = new char[cbTransferred + 1];
			//받은 데이터 출력
			int deqret = ptr->RecvQ.Dequeue(recvtemp, cbTransferred);
			recvtemp[cbTransferred] = '\0';
			printf("[TCP/%s:%d <<%s>>\n\ntransferred: %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), recvtemp,cbTransferred);

			//받은 것 SendQ로 옮기기
			int enqret = ptr->SendQ.Enqueue(recvtemp, cbTransferred);
			if (enqret != cbTransferred)
			{
				continue;
			}
			delete[] recvtemp;

			//WSASend걸기
			if (ptr->SendQ.GetUsedSize() > 0 && InterlockedExchange(&ptr->sendflag, 1) == 0)
			{
				if (ptr->SendQ.GetUsedSize() == ptr->SendQ.DirectDequeueSize())
				{
					WSABUF wsabuf;
					DWORD sendbytes;
					DWORD sendflags = 0;
					wsabuf.buf = ptr->SendQ.GetFrontBufferPtr();
					wsabuf.len = ptr->SendQ.DirectDequeueSize();
					ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
					retval = WSASend(ptr->sock, &wsabuf, 1, &sendbytes, sendflags, &ptr->sendio.overlapped, NULL);
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display((char*)"WSASend()");
							//continue;
						}

					}

				}
				else
				{
					WSABUF wsabuf[2];
					DWORD sendbytes = 0;
					DWORD sendflags = 0;
					wsabuf[0].buf = ptr->SendQ.GetFrontBufferPtr();
					wsabuf[0].len = ptr->SendQ.DirectDequeueSize();
					wsabuf[1].buf = ptr->SendQ.GetStartBufferPtr();
					wsabuf[1].len = ptr->SendQ.GetUsedSize() - wsabuf[0].len;
					ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
					retval = WSASend(ptr->sock, wsabuf, 2, &sendbytes, sendflags, &ptr->sendio.overlapped, NULL);
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display((char*)"WSASend()");
							//continue;
						}
					}

				}

			}

			//WSARecv 걸기
			//비동기 입출력 시작
			if (ptr->RecvQ.GetUsedSize() == ptr->RecvQ.DirectEnqueueSize())
			{
				WSABUF wsabuf;
				wsabuf.buf = ptr->RecvQ.GetRearBufferPtr();
				wsabuf.len = ptr->RecvQ.DirectEnqueueSize();
				DWORD recvbytes, flags = 0;
				ZeroMemory(&ptr->recvio.overlapped, sizeof(ptr->recvio.overlapped));
				retval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, &ptr->recvio.overlapped, NULL);
				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != ERROR_IO_PENDING)
					{
						err_display((char*)"WSARecv()");
					}
				}
			}
			else
			{
				WSABUF wsabuf[2];
				DWORD recvbytes = 0;
				DWORD recvflags = 0;
				wsabuf[0].buf = ptr->RecvQ.GetRearBufferPtr();
				wsabuf[0].len = ptr->RecvQ.DirectEnqueueSize();
				wsabuf[1].buf = ptr->RecvQ.GetStartBufferPtr();
				wsabuf[1].len = ptr->RecvQ.GetFreeSize() - wsabuf[0].len;
				ZeroMemory(&ptr->recvio.overlapped, sizeof(ptr->recvio.overlapped));
				retval = WSARecv(ptr->sock, wsabuf, 2, &recvbytes, &recvflags, &ptr->recvio.overlapped, NULL);
				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						err_display((char*)"WSARecv()");

					}
				}
			}

		}

		if (myoverlapped->type == 1)
		{

			ptr->SendQ.MoveFront(cbTransferred);
			InterlockedExchange(&ptr->sendflag, 0);
			if (ptr->SendQ.GetUsedSize() > 0 && InterlockedExchange(&ptr->sendflag, 1) == 0)
			{
				if (ptr->SendQ.GetUsedSize() == ptr->SendQ.DirectDequeueSize())
				{
					WSABUF wsabuf;
					DWORD sendbytes;
					DWORD sendflags = 0;
					wsabuf.buf = ptr->SendQ.GetFrontBufferPtr();
					wsabuf.len = ptr->SendQ.DirectDequeueSize();
					ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
					retval = WSASend(ptr->sock, &wsabuf, 1, &sendbytes, sendflags, &ptr->sendio.overlapped, NULL);
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display((char*)"WSASend()");
						}

					}

				}
				else
				{
					WSABUF wsabuf[2];
					DWORD sendbytes = 0;
					DWORD sendflags = 0;
					wsabuf[0].buf = ptr->SendQ.GetFrontBufferPtr();
					wsabuf[0].len = ptr->SendQ.DirectDequeueSize();
					wsabuf[1].buf = ptr->SendQ.GetStartBufferPtr();
					wsabuf[1].len = ptr->SendQ.GetUsedSize() - wsabuf[0].len;
					ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
					retval = WSASend(ptr->sock, wsabuf, 2, &sendbytes, sendflags, &ptr->sendio.overlapped, NULL);
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display((char*)"WSASend()");
							
						}

					}
				}

			}
		}
	}



	return 0;

}

DWORD WINAPI MonitorThread(LPVOID arg)
{
	while (1)
	{
		if (GetAsyncKeyState(0x51) & 0x01)
		{ 
			ProfileDataOutText();
		}
		if (GetAsyncKeyState(0x57) & 0x01)
		{
			ProfileReset();
		}
	
	}
}


void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (LPCSTR)lpMsgBuf, (char*)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}