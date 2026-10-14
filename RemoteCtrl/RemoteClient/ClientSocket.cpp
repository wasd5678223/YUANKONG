#include "pch.h"
#include "ClientSocket.h"

//类的静态成员变量初始化（必须在类外，且要带上变量类型）
CClientSocket* CClientSocket::m_instance = NULL;

CClientSocket::CHelper CClientSocket::m_helper; //静态成员变量初始化

CClientSocket* pclient = CClientSocket::GetInstance(); //获取客户端实例

bool CClientSocket::InitSocket()
{
	{
		if (m_sock != -1)
		{
			CloseSocket(); // 如果套接字已存在，先关闭它
		}
		// 1. 必须初始化Winsock
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0) {
			std::cout << "WSAStartup failed with error: " << result << std::endl;
			return 1;
		}
		std::cout << "WSAStartup successful" << std::endl;

		// 2. 创建socket
		m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		std::cout << "socket() raw return value: " << m_sock << std::endl;
		std::cout << "INVALID_SOCKET defined as: " << INVALID_SOCKET << std::endl;

		if (m_sock == INVALID_SOCKET) {
			int error = WSAGetLastError();
			std::cout << "Socket creation FAILED!" << std::endl;
			std::cout << "WSAGetLastError(): " << error << std::endl;

			// 常见错误码解析
			switch (error) {
			case WSANOTINITIALISED:
				std::cout << "- WSAStartup was not called or failed" << std::endl;
				break;
			case WSAENETDOWN:
				std::cout << "- Network subsystem failed" << std::endl;
				break;
			case WSAEAFNOSUPPORT:
				std::cout << "- Address family not supported" << std::endl;
				break;
			case WSAEMFILE:
				std::cout << "- No more socket descriptors available" << std::endl;
				break;
			case WSAEINPROGRESS:
				std::cout << "- A blocking Winsock call is in progress" << std::endl;
				break;
			case WSAEPROTONOSUPPORT:
				std::cout << "- Specified protocol is not supported" << std::endl;
				break;
			case WSAEPROTOTYPE:
				std::cout << "- Protocol type is wrong for this socket" << std::endl;
				break;
			case WSAESOCKTNOSUPPORT:
				std::cout << "- Socket type not supported in this address family" << std::endl;
				break;
			default:
				std::cout << "- Unknown error" << std::endl;
			}
		}
		else {
			std::cout << "Socket creation SUCCESS!" << std::endl;
			std::cout << "Valid socket handle: " << m_sock << std::endl;
		}

		if (m_sock == -1)
		{
			return false;
		}
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(m_nPort);
		// 将int类型的地址转换为网络字节序
		addr.sin_addr.S_un.S_addr = htonl(m_nIP);     // 设置服务器IP地址(本机测试时直接使用127.0.0.1)
		int ret = connect(m_sock, (sockaddr*)&addr, sizeof(addr)); //连接服务器  
		if (ret == -1)
		{
			int error = WSAGetLastError();
			MessageBox(NULL, _T("连接服务器失败！"), _T("错误！"), MB_OK | MB_ICONERROR);
			TRACE("连接服务器失败: %d！\n", error); // 使用WSAGetLastError获取错误代码并打印  
			return false;
		}
		return true;
	}
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM nParam)
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	 PACKET_DATA* packetData = new PACKET_DATA((BYTE*)strOut.c_str(), strOut.size(), nMode, nParam);
	bool ret = PostThreadMessage
	(m_nThreadID, WM_SEND_PACK, (WPARAM)packetData, (LPARAM)hWnd);
	if (!ret)
	{
		delete packetData; // 如果消息发送失败，释放分配的内存
	}
	return ret;
}



//bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& listPacks, bool isAutoClose)
//{
//	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
//	{
//		if (InitSocket() == false)
//		{
//			return false;
//		}
//		m_hThread =(HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this); // 启动线程
//		TRACE("客户端线程启动成功！\n");
//	}
//
//	m_lock.lock();
//	auto pr = std::pair<HANDLE, std::list<CPacket>*>(pack.hEvent, &listPacks);
//	m_mapAck.insert(pr);
//	m_mapAutoClose[pack.hEvent] = isAutoClose;
//	m_listSend.push_back(pack);
//	m_lock.unlock();
//	//等待响应（通过事件实现同步）
//	WaitForSingleObject(pack.hEvent, INFINITE);
//	//获取响应数据包
//	auto it = m_mapAck.find(pack.hEvent);
//
//	if (it != m_mapAck.end())
//	{
//		m_lock.lock();
//		m_mapAck.erase(it);
//		m_lock.unlock();
//		return true;
//	}
//	
//	return false;
//}

unsigned  CClientSocket::threadEntry(void* arg)
{
	// 将传入的参数转换为 CClientSocket 对象指针
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);// 结束线程
	return 0;
}
// 线程主函数逻辑
//void CClientSocket::threadFunc()
//{
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE);
//	char* pBuffer = (char*)strBuffer.c_str();
//	int index = 0; // 缓冲区索引(始终表示缓冲区中内存结尾)
//	InitSocket(); // 确保套接字已初始化
//	while (m_sock != INVALID_SOCKET)
//	{
//		if (m_listSend.size() > 0)
//		{
//			m_lock.lock();
//			TRACE("数据包个数: %d\n", m_listSend.size());
//			CPacket& head = m_listSend.front();//别名
//			m_lock.unlock();
//			if (Send(head)== false)
//			{
//				InitSocket(); // 重新初始化套接字
//				TRACE("发送数据失败！\n");
//				continue;
//			}
//			//获取响应数据包
//			auto it = m_mapAck.find(head.hEvent);
//			if (it != m_mapAck.end())
//			{
//				auto it2 = m_mapAutoClose.find(head.hEvent);
//				do
//				{
//					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
//					if ((length > 0) || (index > 0))
//					{
//						// 更新索引（缓冲区新索引 = 缓冲区原索引 + 新接收到的字节数）
//						index += length; 
//						size_t size =   index;
//						//把接收过来的数据打包
//						// size进去前表示缓冲区中数据的总长度
//						// size出来表示已经处理的数据大小
//						CPacket pack((BYTE*)pBuffer, size); 
//						if (size > 0)
//						{
//							//通知对应事件
//							pack.hEvent = head.hEvent; // 设置事件句柄（因为事件只在客户端包类中存在）
//							it->second->push_back(pack);//存储响应数据包
//
//							// 清除已处理的数据（将未处理的数据移动到缓冲区前面）
//							memmove(pBuffer, pBuffer + size, index - size);
//							index -= size; // 更新索引
//							if (it2->second)
//							{
//								SetEvent(head.hEvent);// 通知等待的线程
//								break;
//							}
//						}
//					}
//					else if (length <= 0 && index <= 0)
//					{
//						CloseSocket();
//						SetEvent(head.hEvent);// 通知等待的线程
//						break;
//					}
//
//				} while (it2->second == false);
//				m_lock.lock();
//				m_listSend.pop_front(); // 移除已发送的数据包
//				m_mapAutoClose.erase(head.hEvent);// 移除自动关闭标志
//				m_lock.unlock();
//				InitSocket();
//
//
//			}
//		}
//		else
//		{
//			Sleep(1);//
//		}
//	}
//	CloseSocket();
//}


void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg; // 消息结构体
	while (::GetMessage(&msg, NULL, 0, 0)) // 处理消息循环
	{
		TranslateMessage(&msg);// 翻译消息
		DispatchMessage(&msg);// 分发消息
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}

	}
	
}



// 复制构造函数
CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = ss.m_hThread;
	m_bAutoClosed = ss.m_bAutoClosed;
	m_sock = ss.m_sock; // 复制服务器套接字
	// 注意：这里不需要初始化套接字环境，因为它已经在构造函数中完成了
	// InitSockEnv(); 不需要再次调用
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	m_mapFunc = ss.m_mapFunc;
}

CClientSocket::CClientSocket() :
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClosed(true),
	m_hThread(INVALID_HANDLE_VALUE)
{
	/*if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}*/
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT)
	{
		TRACE("网络消息处理线程开启失败！！！");
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	/*serv_sock = socket(PF_INET, SOCK_STREAM, 0);*/
	struct
	{
		UINT nMsg;
		MSGFUNC func;
	}funcs[] =
	{
		{ WM_SEND_PACK, &CClientSocket::SendPack },
		{WM_SEND_PACK_ACK,},
		{ (UINT)-1,NULL }
	};
	//循环将消息和对应的处理函数加入映射表，以便后续查找调用
	for (int i = 0; funcs[i].func != NULL; i++)
	{
		if (m_mapFunc.insert(std::make_pair(funcs[i].nMsg, funcs[i].func)).second == false)
		{
			TRACE("消息映射插入失败！消息值: %d 函数值: %p\n", funcs[i].nMsg, funcs[i].func);
		}

	}

}




// 发送数据包的消息 对应的处理函数
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = (HWND)lParam;
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket())
	{
		int ret = send(m_sock, data.strData.c_str(), data.strData.size(), 0);
		if (ret > 0) 
		{
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) 
			{
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if ((length > 0) || (index > 0)) 
				{
					index += (size_t)length;
					size_t nLen = index;//nLen进去之前表示缓冲区中数据的总长度
					CPacket pack((BYTE*)pBuffer, nLen);//nLen出来表示已经处理的数据大小
					if (nLen > 0) 
					{
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						//自动关闭连接
						if(data.nMode & CSM_AUTOCLOSE)
						{
							
							CloseSocket();
							return;
						}
						index -= nLen;// 缓冲区更新索引(减去本次解析掉的)
						// 清除已处理的数据（将未处理的数据移动到缓冲区前面）
						memmove(pBuffer, pBuffer + nLen, index);
					}
					
				}
				//连接断开（服务端关闭套接字，或者网络设备异常）
				else
				{
					CloseSocket();
					//::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd,NULL,0), 1);
				}
			}
		}
		else
		{
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}

	}
	else
	{
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}