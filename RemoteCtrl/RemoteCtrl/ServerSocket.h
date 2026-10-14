#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"





typedef void(*SOCKET_CALLBACK)(void* arg,int status,std::list<CPacket>& packets, CPacket& inPacket);//回调函数类型

class CServerSocket
{
public:
	static CServerSocket* GetInstance()
	{
		if (m_instance == nullptr)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket(short port)
	{
		if (serv_sock == -1)return false;	
		// 设置 SO_REUSEADDR 选项
		// 允许重用本地地址和端口
		int optval = 1;
		if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR,
			(const char*)&optval, sizeof(optval)) == -1) {
			MessageBox(NULL, _T("设置套接字选项失败！"), _T("错误！"), MB_OK | MB_ICONERROR);
			return false;
		}
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(serv_sock, (sockaddr*)&addr, sizeof(addr)) == -1)
		{
			MessageBox(NULL,_T("绑定套接字失败！"),_T("错误！"),MB_OK|MB_ICONERROR);
			return false;
		}
		if (listen(serv_sock, 1) == -1)
		{
			MessageBox(NULL,_T("监听套接字失败！"),_T("错误！"),MB_OK|MB_ICONERROR);
			return false;
		}
		
		return true;
	}

	//把业务逻辑当作回调函数传入
	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527)
	{
		bool ret = InitSocket(port);
		if (!ret)return -1;
		std::list<CPacket> lstpackets;
		//更新回调函数和参数
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (1)
		{
			if (AcceptClient() == false)
			{
				count++;
				if (count >= 3)
				{
					return -2;//接受客户端连接失败
				}
				continue;
			};
			//链接成功
			int ret = DealCommad();//接收消息,解析命令
			if (ret > 0)
			{
				//调用回调函数处理业务逻辑
				m_callback(m_arg, ret,lstpackets,m_packet); //ret进去作为命令，结束作为状态反映
				while (lstpackets.size() > 0)
				{
					Send(lstpackets.front());
					lstpackets.pop_front();
				}
			}
			CloseClient();//关闭客户端连接，等待下一个连接

		}
		return 0;
	}
	bool AcceptClient()
	{
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(serv_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1)
		{
			MessageBox(NULL,_T("接受客户端连接失败！"),_T("错误！"),MB_OK|MB_ICONERROR);
			return false;
		}
		TRACE(_T("客户端连接成功:m_client = %d\n"), m_client);
		return true;
		
	}

#define BUFFER_SIZE 4096
	//处理客户端命令
	int DealCommad()
	{
		if (m_client == -1)return -1;
		char* buffer = new (std::nothrow) char[BUFFER_SIZE];
		if (!buffer) { // 仅在 std::nothrow 时需检查 NULL
			TRACE("内存不足！\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0; // 缓冲区索引(始终表示缓冲区中内存结尾)
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				delete[]buffer;
				return -1;
			}
			index += len; // 更新索引
			len = index; // 处理数据的长度
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, index - len); // 清除已处理的数据)
				index -= len; // 更新索引
				delete[]buffer;
				return m_packet.sCmd; // 返回命令字
			}

		}
		delete[]buffer;
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;

	}
	bool Send(CPacket& packet)
	{
		if (m_client == -1)return false;
		TRACE(_T("服务端发送文件名：%s\n"), ((PFILEINFO)packet.strData.c_str())->szFileName);
		return send(m_client, packet.Data(), packet.Size(), 0) > 0;
	}

	void CloseClient()
	{
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client); // 关闭客户端套接字
			m_client = INVALID_SOCKET; // 重置客户端套接字为无效值
		}
	}

private:
	SOCKET_CALLBACK m_callback; // 套接字回调函数
	void* m_arg; // 套接字回调函数参数
	SOCKET serv_sock; // 服务器套接字
	SOCKET m_client; // 客户端套接字
	CPacket m_packet; //数据包对象
	CServerSocket& operator=(const CServerSocket& ss)
	{
		if (this == &ss) return *this; // 防止自赋值
		serv_sock = ss.serv_sock; // 复制服务器套接字
		m_client = ss.m_client; // 复制客户端套接字
		return *this;
	};
	CServerSocket(const CServerSocket& ss) 
	{
		serv_sock = ss.serv_sock; // 复制服务器套接字
		m_client = ss.m_client; // 复制客户端套接字
		// 注意：这里不需要初始化套接字环境，因为它已经在构造函数中完成了
		// InitSockEnv(); 不需要再次调用
	};
	CServerSocket()
	{
		m_client = -1; // 初始化客户端套接字为无效值
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL,_T("无法初始化套接字环境,请检查网络设置！"),_T("初始化错误！"),MB_OK|MB_ICONERROR);
			exit(0);
		}
		serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	
	}
	~CServerSocket()
	{
		// 释放套接字环境
		WSACleanup();
		closesocket(serv_sock); // 关闭服务器套接字
	}
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}

	
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			// 释放单例实例
			CServerSocket* tmp = m_instance; 
			m_instance = NULL;  // 将 m_instance 置为 NULL，防止重复释放
			delete tmp;
		}
	}
	
	static CServerSocket* m_instance;

	/*在 C++ 中，静态指针变量（如 static CServerSocket * m_instance; ）
	在程序结束时只会自动销毁指针本身（也就是释放指针变量占用的内存），
	不会自动析构和释放它所指向的对象
	（即不会自动调用 delete m_instance;，也不会调用 CServerSocket 的析构函数）*/
	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::GetInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};

    // 确保单例在程序运行期间存在,静态变量在程序结束时自动调用析构函数
	static CHelper m_helper; 
	
};

