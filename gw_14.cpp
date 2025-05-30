#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32")
#include "Struct.h"
#include <winsock2.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "RingBuffer.h"
#include "SerialBuffer.h"
#include <unordered_map>
//#include "Profiler.h"

using namespace std;

#define SERVERPORT 6000

__int64 SessionKey;

struct myOverlapped
{
	WSAOVERLAPPED overlapped;
	bool type;				//0이면 recv 1이면 send
};


//소켓 정보 저장을 위한 구조체와 변수
struct SESSION
{
	myOverlapped sendio;
	myOverlapped recvio;
	RingBuffer RecvQ;
	RingBuffer SendQ;

	SOCKET sock;
	IN_ADDR ip;
	u_short port;

	__int64 sessionkey;

	long sendflag = 0;
	long iocount = 0;
	//SRWLOCK lock;
	CRITICAL_SECTION lock;
};


//세션맵
unordered_map<__int64, SESSION*> SessionMap;

//세션맵 락
SRWLOCK sessionLock;

//listen socket
SOCKET ListenSock;

//IOCP
HANDLE hcp;

//작업자 스레드 함수
unsigned __stdcall WorkerThread(LPVOID arg);

//accept 스레드 함수
unsigned __stdcall AcceptThread(LPVOID arg);

//세션 삭제 함수
void Delete(SESSION* ptr)
{
	AcquireSRWLockExclusive(&sessionLock);
	closesocket(ptr->sock);
	SessionMap.erase(ptr->sessionkey);
	LeaveCriticalSection(&ptr->lock);
	delete ptr;
	ReleaseSRWLockExclusive(&sessionLock);
}



int main(int argc, char* argv[])
{
	printf("START\n");
	//retval
	int scretval;//startup,cleanup retval
	int cpretval;//CreateIoCompletionProt retval
	int lscretval;//listensock retval
	int sbretval;//setsockopt sndbuf0 retval
	int bdretval;//bind retval
	int lnretval;//listen retval
	int csretval;//closesocket retval
	int lgretval;//linger retval
	int ndretval;//nodelay retval

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		scretval = WSAGetLastError();
		printf("WSAStartup error: %d\n", scretval);
		return -1;
	}

	//입출력 완료 포트 생성
	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL)
	{
		cpretval = GetLastError();
		printf("CreateIoCompletionPort create error: %d\n", cpretval);
		return -1;
	}

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(CPU 개수 * 2)개의 작업자 스레드 생성
	HANDLE hThread[9];
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread, hcp, 0, NULL);
		if (hThread == NULL)
		{
			return 1;
		}
	}

	//socket()
	ListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSock == INVALID_SOCKET)
	{
		lscretval = WSAGetLastError();
		printf("socket() error: %d\n", lscretval);
		return -1;
	}


	//bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	bdretval = bind(ListenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bdretval == SOCKET_ERROR)
	{
		bdretval = WSAGetLastError();
		printf("bind error: %d\n", bdretval);
		return -1;
	}

	//SO_SNDBUF
	int bfoptval = 0;
	sbretval = setsockopt(ListenSock, SOL_SOCKET, SO_SNDBUF, (char*)&bfoptval, sizeof(bfoptval));
	if (sbretval == SOCKET_ERROR)
	{
		sbretval = WSAGetLastError();
		printf("nonblocking error: %d\n", sbretval);
		return -1;
	}

	//SO_LINGER
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	lgretval = setsockopt(ListenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (lgretval == SOCKET_ERROR)
	{
		lgretval = WSAGetLastError();
		printf("linger error: %d\n", lgretval);
		return -1;
	}

	//nodelay
	BOOL ndoptval = TRUE;
	ndretval = setsockopt(ListenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&ndoptval, sizeof(ndoptval));
	if (ndretval == SOCKET_ERROR)
	{
		ndretval = WSAGetLastError();
		printf("nodelay error: %d\n", ndretval);
		return -1;
	}

	//listen()
	lnretval = listen(ListenSock, SOMAXCONN_HINT(7000));
	if (lnretval == SOCKET_ERROR)
	{
		lnretval = WSAGetLastError();
		printf("listen error: %d\n", lnretval);
		return -1;
	}

	hThread[8] = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread, NULL, 0, NULL);
	InitializeSRWLock(&sessionLock);

	while (1)
	{
		if (_kbhit())
		{
			if (_getch() == 'e')
			{
				printf("~Server End~\n");
				//closesocket()
				csretval = closesocket(ListenSock);
				if (csretval == SOCKET_ERROR)
				{
					csretval = WSAGetLastError();
					printf("closesocket error: %d\n", csretval);
				}

				ULONG_PTR p = 0;
				LPOVERLAPPED po = nullptr;
				for (int i = 0; i < 8; i++)
				{
					PostQueuedCompletionStatus(hcp, 0, p, po);
				}

				break;
			}
		}


	}
	WaitForMultipleObjects(9, hThread, TRUE, INFINITE);
	printf("All thread ended!\n");




	//윈속종료
	scretval = WSACleanup();
	if (scretval == SOCKET_ERROR)
	{
		scretval = WSAGetLastError();
		printf("cleanup error: %d\n", scretval);
	}
	return 0;


}

/*
통지가 온 것에 대한 iocount 감소는 맨 마지막에서 하자
send나 recv걸 때 증가시켜주고 실패하면 감소시켜주기
*/
//작업자 스레드 함수
unsigned __stdcall WorkerThread(LPVOID arg)
{
	int gqcsretval;//GetQueuedCompletionStatus retval;
	int sdretval;//send retval
	int rvretval;//recv retval
	HANDLE iocp = (HANDLE)arg;
	SBuffer msgbuf;

	while (1)
	{
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred = 0;
		myOverlapped* myoverlapped = 0;
		SESSION* ptr = 0;
		gqcsretval = GetQueuedCompletionStatus(iocp, &cbTransferred, (PULONG_PTR)&ptr, (LPOVERLAPPED*)&myoverlapped, INFINITE);

		//__int64 sessionid = ptr->sessionkey;
		do
		{
			//비동기 입출력 결과 확인
			//스레드 종료
			if (cbTransferred == 0 && ptr == 0 && myoverlapped == 0)
			{
				return 0;
			}
			//
			if (gqcsretval == 0 || cbTransferred == 0)
			{
				if (gqcsretval == 0)
				{
					gqcsretval = WSAGetLastError();
					DWORD temp1, temp2;
					WSAGetOverlappedResult(ptr->sock, &myoverlapped->overlapped, &temp1, FALSE, &temp2);
					//printf("GetQueuedCompletionStatus error : %d\n", gqcsretval);
				}

			}
			else
			{
				//EnterCriticalSection(&ptr->lock);
				//recvio overlapped임
				if (myoverlapped->type == 0)
				{
					EnterCriticalSection(&ptr->lock);
					ptr->RecvQ.MoveRear(cbTransferred);
					LeaveCriticalSection(&ptr->lock);

					//printf("WSARecv completion\n");

					EnterCriticalSection(&ptr->lock);
					while (1)
					{
						//recv 후 처리
						//header만큼 들어왔는지 확인

						if (ptr->RecvQ.GetUsedSize() < sizeof(HEADER))
						{
							break;
						}
						HEADER header;
						int pkret = ptr->RecvQ.Peek((char*)&header, sizeof(HEADER));
						if (pkret != sizeof(HEADER))
						{
							DebugBreak();
						}

						//헤더+데이터 만큼 들어있는지 확인
						if (ptr->RecvQ.GetUsedSize() < header.size + sizeof(HEADER))
						{
							break;
						}
						msgbuf.Clear();
						int deqret = ptr->RecvQ.Dequeue(msgbuf.GetBufferPtr(), header.size + sizeof(HEADER));
						if (deqret != header.size + sizeof(HEADER))
						{
							DebugBreak();
						}
						int movret = msgbuf.MoveWritePos(deqret);
						if (movret != deqret)
						{
							DebugBreak();
						}

						//~~~뭔가의 처리~~~
						//받은 것 SendQ로 옮기기
						int size = msgbuf.GetDataSize();
						int enqret = ptr->SendQ.Enqueue(msgbuf.GetBufferPtr(), size);
						if (enqret != size)
						{
							DebugBreak();
						}

					}
					LeaveCriticalSection(&ptr->lock);

					//WSASend걸기

					EnterCriticalSection(&ptr->lock);
					if (ptr->SendQ.GetUsedSize() > 0&& InterlockedExchange(&ptr->sendflag, 1) == 0 )
					{

						InterlockedIncrement(&ptr->iocount);
						//버퍼 1개 꽂는 경우
						if (ptr->SendQ.GetUsedSize() == ptr->SendQ.DirectDequeueSize())
						{
							WSABUF wsabuf;
							DWORD sendbytes;
							DWORD sendflags = 0;
							wsabuf.buf = ptr->SendQ.GetFrontBufferPtr();
							wsabuf.len = ptr->SendQ.DirectDequeueSize();//directsize;
							ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
							sdretval = WSASend(ptr->sock, &wsabuf, 1, &sendbytes, sendflags, &ptr->sendio.overlapped, NULL);
							if (sdretval == SOCKET_ERROR)
							{
								sdretval = WSAGetLastError();
								if (sdretval != WSA_IO_PENDING)
								{
									//printf("WSASend error : %d\n", sdretval);
									InterlockedDecrement(&ptr->iocount);
								}
							}
						}
						else
						{
							WSABUF wsabuf[2];
							DWORD sendbytes;
							DWORD sendflags = 0;
							wsabuf[0].buf = ptr->SendQ.GetFrontBufferPtr();
							wsabuf[0].len = ptr->SendQ.DirectDequeueSize();
							wsabuf[1].buf = ptr->SendQ.GetStartBufferPtr();
							wsabuf[1].len = ptr->SendQ.GetUsedSize() - wsabuf[0].len;
							ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
							sdretval = WSASend(ptr->sock, wsabuf, 2, &sendbytes, sendflags, &ptr->sendio.overlapped, NULL);
							if (sdretval == SOCKET_ERROR)
							{
								sdretval = WSAGetLastError();
								if (sdretval != WSA_IO_PENDING)
								{
									//printf("WSASend error : %d\n", sdretval);
									InterlockedDecrement(&ptr->iocount);
								}
							}
						}




					}
					LeaveCriticalSection(&ptr->lock);
					//printf("Recv completion WSASend SessionID: %d\n", ptr->sessionkey);



				//WSARecv 걸기
				//비동기 입출력 시작
					EnterCriticalSection(&ptr->lock);
					InterlockedIncrement(&ptr->iocount);
					if (ptr->RecvQ.GetUsedSize() == ptr->RecvQ.DirectEnqueueSize())
					{
						WSABUF wsabuf;
						wsabuf.buf = ptr->RecvQ.GetRearBufferPtr();
						wsabuf.len = ptr->RecvQ.DirectEnqueueSize();
						DWORD recvbytes, flags = 0;
						ZeroMemory(&ptr->recvio.overlapped, sizeof(ptr->recvio.overlapped));


						rvretval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, &ptr->recvio.overlapped, NULL);
						if (rvretval == SOCKET_ERROR)
						{
							rvretval = WSAGetLastError();
							if (rvretval != ERROR_IO_PENDING)
							{
								//printf("WSARecv error : %d\n", rvretval);

								InterlockedDecrement(&ptr->iocount);

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

						rvretval = WSARecv(ptr->sock, wsabuf, 2, &recvbytes, &recvflags, &ptr->recvio.overlapped, NULL);
						if (rvretval == SOCKET_ERROR)
						{
							rvretval = WSAGetLastError();
							if (rvretval != ERROR_IO_PENDING)
							{
								//printf("WSARecv error : %d\n", rvretval);

								InterlockedDecrement(&ptr->iocount);
							}
						}
					}
					//printf("Recv completion WSARecv SessionID: %d\n", ptr->sessionkey);
					LeaveCriticalSection(&ptr->lock);

				}
				if (myoverlapped->type == 1)
				{
					EnterCriticalSection(&ptr->lock);
					//printf("WSASend completion\n");
					ptr->SendQ.MoveFront(cbTransferred);

					if (ptr->SendQ.GetUsedSize() > 0)
					{
						InterlockedIncrement(&ptr->iocount);
						if (ptr->SendQ.GetUsedSize() == ptr->SendQ.DirectDequeueSize())
						{
							WSABUF wsabuf;
							DWORD sendbytes;
							DWORD sendflags = 0;
							wsabuf.buf = ptr->SendQ.GetFrontBufferPtr();
							wsabuf.len = ptr->SendQ.DirectDequeueSize();//directsize;
							ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
							sdretval = WSASend(ptr->sock, &wsabuf, 1, &sendbytes, sendflags, &ptr->sendio.overlapped, NULL);
							if (sdretval == SOCKET_ERROR)
							{
								sdretval = WSAGetLastError();
								if (sdretval != WSA_IO_PENDING)
								{
									//printf("WSASend error : %d\n", sdretval);

									InterlockedDecrement(&ptr->iocount);

								}

							}
						}
						else
						{
							WSABUF wsabuf[2];
							DWORD recvbytes = 0;
							DWORD recvflags = 0;
							wsabuf[0].buf = ptr->RecvQ.GetRearBufferPtr();
							wsabuf[0].len = ptr->RecvQ.DirectDequeueSize();
							wsabuf[1].buf = ptr->RecvQ.GetStartBufferPtr();
							wsabuf[1].len = ptr->RecvQ.GetUsedSize() - wsabuf[0].len;
							ZeroMemory(&ptr->recvio.overlapped, sizeof(ptr->recvio.overlapped));

							rvretval = WSARecv(ptr->sock, wsabuf, 2, &recvbytes, &recvflags, &ptr->recvio.overlapped, NULL);
							if (rvretval == SOCKET_ERROR)
							{
								rvretval = WSAGetLastError();
								if (rvretval != ERROR_IO_PENDING)
								{
									//printf("WSARecv error : %d\n", rvretval);

									InterlockedDecrement(&ptr->iocount);
								}
							}
						}

						LeaveCriticalSection(&ptr->lock);
					}
					else
					{
						InterlockedExchange(&ptr->sendflag, 0);
						LeaveCriticalSection(&ptr->lock);
					}

				}

				//LeaveCriticalSection(&ptr->lock);

			}

			//완료통지 온 것에 대한 iocount감소
			EnterCriticalSection(&ptr->lock);
			if (InterlockedDecrement(&ptr->iocount) == 0)
			{
				Delete(ptr);
				break;
			}
			LeaveCriticalSection(&ptr->lock);


		} while (0);

	}
	return 0;
}



//accept 스레드 함수
unsigned __stdcall AcceptThread(LPVOID arg)
{
	int atretval;//accept retval
	int rvretval;//recv retval

	//데이터 통신에 사용할 변수
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes;
	DWORD flags;

	while (1)
	{

		//accept()
		SOCKET sock;
		addrlen = sizeof(clientaddr);
		sock = accept(ListenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (sock == INVALID_SOCKET)
		{
			atretval = WSAGetLastError();
			printf("accept error: %d\n", atretval);

			return -1;
		}

		else
		{
			//소켓 정보 구조체 할당
			SESSION* ptr = new SESSION;
			if (ptr == NULL)
			{
				printf("new session error");
			}
			ZeroMemory(&ptr->sendio.overlapped, sizeof(ptr->sendio.overlapped));
			ZeroMemory(&ptr->recvio.overlapped, sizeof(ptr->recvio.overlapped));
			ptr->sendio.type = 1;
			ptr->recvio.type = 0;
			ptr->sock = sock;
			ptr->ip = clientaddr.sin_addr;
			ptr->port = clientaddr.sin_port;
			ptr->sessionkey = SessionKey;
			InterlockedIncrement(&ptr->iocount);
			//InitializeSRWLock(&ptr->lock);
			InitializeCriticalSection(&ptr->lock);

			AcquireSRWLockExclusive(&sessionLock);
			SessionMap.insert(make_pair(ptr->sessionkey, ptr));
			ReleaseSRWLockExclusive(&sessionLock);

			//printf("[TCP server] client connected : IP address=%s, Port=%d, SessionID: %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), ptr->sessionkey);

			InterlockedIncrement((unsigned long long*) & SessionKey);

			//소켓과 입출력 완료 포트 연결
			CreateIoCompletionPort((HANDLE)ptr->sock, hcp, (ULONG_PTR)ptr, 0);

			//비동기 입출력 시작
			WSABUF wsabuf;
			wsabuf.buf = ptr->RecvQ.GetRearBufferPtr();
			wsabuf.len = ptr->RecvQ.DirectEnqueueSize();
			flags = 0;

			rvretval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, &ptr->recvio.overlapped, NULL);
			if (rvretval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != ERROR_IO_PENDING)
				{
					printf("WSARecv error: %d\n", rvretval);

					closesocket(ptr->sock);
					//printf("[TCP server] client ended: IP address=%s, Port=%d, SessionID: %d\n", inet_ntoa(ptr->ip), ntohs(ptr->port), ptr->sessionkey);
					AcquireSRWLockExclusive(&sessionLock);
					SessionMap.erase(ptr->sessionkey);
					ReleaseSRWLockExclusive(&sessionLock);
					delete ptr;
				}

			}


		}
	}
}

bool SendPost(SESSION* session)
{
	int sdretval;
	if (session->SendQ.GetUsedSize() > 0 && InterlockedExchange(&session->sendflag, 1) == 0)
	{
		InterlockedIncrement(&session->iocount);
		//버퍼 1개 꽂는 경우
		if (session->SendQ.GetUsedSize() == session->SendQ.DirectDequeueSize())
		{
			WSABUF wsabuf;
			DWORD sendbytes;
			DWORD sendflags = 0;
			wsabuf.buf = session->SendQ.GetFrontBufferPtr();
			wsabuf.len = session->SendQ.DirectDequeueSize();//directsize;
			ZeroMemory(&session->sendio.overlapped, sizeof(session->sendio.overlapped));
			sdretval = WSASend(session->sock, &wsabuf, 1, &sendbytes, sendflags, &session->sendio.overlapped, NULL);
			if (sdretval == SOCKET_ERROR)
			{
				sdretval = WSAGetLastError();
				if (sdretval != WSA_IO_PENDING)
				{
					//printf("WSASend error : %d\n", sdretval);
					InterlockedDecrement(&session->iocount);
				}
			}
		}
		else
		{
			WSABUF wsabuf[2];
			DWORD sendbytes;
			DWORD sendflags = 0;
			wsabuf[0].buf = session->SendQ.GetFrontBufferPtr();
			wsabuf[0].len = session->SendQ.DirectDequeueSize();
			wsabuf[1].buf = session->SendQ.GetStartBufferPtr();
			wsabuf[1].len = session->SendQ.GetUsedSize() - wsabuf[0].len;
			ZeroMemory(&session->sendio.overlapped, sizeof(session->sendio.overlapped));
			sdretval = WSASend(session->sock, wsabuf, 2, &sendbytes, sendflags, &session->sendio.overlapped, NULL);
			if (sdretval == SOCKET_ERROR)
			{
				sdretval = WSAGetLastError();
				if (sdretval != WSA_IO_PENDING)
				{
					//printf("WSASend error : %d\n", sdretval);
					InterlockedDecrement(&session->iocount);
				}
			}
		}




	}
}

bool SendPacket(__int64 sessionID, SBuffer* msgbuf)
{
	AcquireSRWLockShared(&sessionLock);
	SESSION* tgtsession = SessionMap.find(sessionID)->second;
	EnterCriticalSection(&tgtsession->lock);
	ReleaseSRWLockShared(&sessionLock);
	tgtsession->SendQ.Enqueue(msgbuf->GetBufferPtr(), msgbuf->GetDataSize());
	SendPost(tgtsession);
	LeaveCriticalSection(&tgtsession->lock);
}

void Release(__int64 sessionID)
{
	AcquireSRWLockExclusive(&sessionLock);
	SESSION* tgtsession = SessionMap.find(sessionID)->second;
	if (tgtsession != SessionMap.end()->second)
	{
		//세션맵에서 삭제
		SessionMap.erase(sessionID);
		
		//세션 포인터 삭제 준비
		EnterCriticalSection(&tgtsession->lock);
		ReleaseSRWLockExclusive(&sessionLock);
		LeaveCriticalSection(&tgtsession->lock);
		closesocket(tgtsession->sock);
		delete tgtsession;

		return;
	}
	ReleaseSRWLockExclusive(&sessionLock);
}