#pragma once
#include <MSWSock.h>
#include "EdoyunThread.h"
#include "cEdoyunQueue.h"
#include <map>
#include <WS2tcpip.h>
#include <mutex>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
enum EdoyunOperator
{
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class EdoyunServer;
class EdoyunClient;

typedef std::shared_ptr<EdoyunClient> PCLIENT;//客户端对象智能指针，命名简化为PCLIENT，方便后续使用

//自定义重叠结构体，继承自原生overlapped结构体，增加操作类型、缓冲区、处理函数、服务器对象等成员
class EdoyunOverlapped
{
public:
	OVERLAPPED  m_overlapped;        //原生overlapped结构体
	DWORD       m_operator;          // 操作类型，参见 EdoyunOperator
	std::vector<char> m_buffer;      // 缓冲区
	ThreadWorker m_worker;           // 处理函数
	EdoyunServer* m_server;          // 服务器对象
};

template<EdoyunOperator>
class AcceptOverlapped;

typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

class EdoyunClient
{
public:
	EdoyunClient();
	~EdoyunClient()
	{
		closesocket(m_sock);
	}

	void SetOverlapped(PCLIENT& ptr);

	operator SOCKET()
	{
		return m_sock;
	}
	operator PVOID()
	{
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();
	operator LPDWORD()
	{
		return &m_received;
	}

	sockaddr_in* GetLocalAddr()
	{
		return &m_laddr;
	}
	sockaddr_in* GetRemoteAddr()
	{
		return &m_raddr;
	}

private:
	SOCKET m_sock;
	DWORD  m_received;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};



template<EdoyunOperator>
class AcceptOverlapped : public EdoyunOverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped();
	int AcceptWorker();

	PCLIENT m_client;
};



template<EdoyunOperator>
class RecvOverlapped : public EdoyunOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped()
		: m_operator(ERecv)
		, m_worker(this, &RecvOverlapped::RecvWorker)
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}

	int RecvWorker()
	{
		// TODO:
		return 0;
	}
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;


template<EdoyunOperator>
class SendOverlapped : public EdoyunOverlapped, ThreadFuncBase
{
public:
	SendOverlapped()
		: m_operator(ESend)
		, m_worker(this, &SendOverlapped::SendWorker)
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}

	int SendWorker()
	{
		// TODO:
		return 0;
	}
};

typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EdoyunOperator>
class ErrorOverlapped : public EdoyunOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped()
		: m_operator(EError)
		, m_worker(this, &ErrorOverlapped::ErrorWorker)
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}

	int ErrorWorker()
	{
		// TODO:
		return 0;
	}
};

typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class EdoyunServer : public ThreadFuncBase
{
public:
	EdoyunServer(const std::string& ip = "0.0.0.0", short port = 9527)
		: m_pool(10)
	{
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		InetPtonA(AF_INET, ip.c_str(), &m_addr.sin_addr);
	}

	~EdoyunServer()
	{}

	bool StartService()
	{
		CreateSocket();
		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1)
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}

		if (listen(m_sock, 3) == -1)
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}

		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);//创建完成端口 

		if (m_hIOCP == NULL)
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}

		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);//将套接字与完成端口关联

		m_pool.Invoke();

		m_pool.DispatchWorker(
			ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp)
		);

		if (!NewAccept())
			return false;

		return true;



	}

	bool NewAccept()
	{
		PCLIENT pClient(new EdoyunClient());
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));

		if (!AcceptEx(
			m_sock,
			*pClient,
			*pClient,
			0,
			sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16,
			nullptr,
			(LPOVERLAPPED)*pClient))
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}

private:
	void CreateSocket()
	{
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}

	int threadIocp()
	{
		DWORD       transferred = 0;
		ULONG_PTR   CompletionKey = 0;
		OVERLAPPED* lpOverlapped = nullptr;

		while (true)
		{
			BOOL ret = GetQueuedCompletionStatus(
				m_hIOCP,
				&transferred,
				&CompletionKey,
				&lpOverlapped,
				INFINITE);

			if (ret && transferred > 0 && CompletionKey != 0)
			{
				EdoyunOverlapped* pOverlapped =
					CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);

				switch (pOverlapped->m_operator)
				{
				case EAccept:
				{
					ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				case ERecv:
				{
					RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				case ESend:
				{
					SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				case EError:
				{
					ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				default:
					break;
				}
			}
			else
			{
				// 出错或连接关闭，退出线程
				return -1;
			}
		}

		return 0;
	}

private:
	EdoyunThreadPool m_pool;
	HANDLE           m_hIOCP;
	SOCKET           m_sock;
	sockaddr_in      m_addr;
	std::map<SOCKET, std::shared_ptr<EdoyunClient>> m_client;
};

