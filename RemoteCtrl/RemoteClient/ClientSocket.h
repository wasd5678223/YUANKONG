#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <ws2tcpip.h>  // 包含 inet_pton() 的定义
#include <winsock2.h>
#include <vector>
#include <list>
#include <io.h>
#include <map>
#include <iostream>
#include <mutex>

#define WM_SEND_PACK (WM_USER + 1)//发送数据包消息
#define WM_SEND_PACK_ACK (WM_USER + 2) //应答
//文件信息结构体
typedef struct file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录0否1是
	BOOL HasNext;//是否还有后续0 没有1有
	char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;

typedef struct PacketData
{
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PacketData(BYTE* pData, size_t nLen, UINT mode, WPARAM nParam)
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data)
	{
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;	
	}
	PacketData& operator=(const PacketData& data)
	{
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;	
		}
		return *this;
	}
}PACKET_DATA;

enum
{
	CSM_AUTOCLOSE = 1,//自动关闭连接
};


class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), strData(""), sSum(0) {}
	//用于发送数据转换为包格式(打包)
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF; // 包头
		nLength = nSize + 2 + 2; // 包长度 = 数据长度 + 命令字2字节 + 校验和2字节
		sCmd = nCmd; // 命令字
		// 如果数据内容不为空，则设置数据内容
		if (nSize > 0)
		{
			strData.resize(nSize); // 设置数据内容大小
			memcpy((void*)strData.c_str(), pData, nSize); // 复制数据内容
		}
		else
		{
			strData.clear(); // 如果数据长度为0，则清空数据内容
		}
		//计算校验和
		sSum = 0;
		for (size_t i = 0; i < strData.size(); i++)
		{
			sSum += BYTE(strData[i]) & 0xFF;
		}
		
	}
	CPacket(const CPacket& packet)
	{
		sHead = packet.sHead; // 复制包头
		nLength = packet.nLength; // 复制包长度
		sCmd = packet.sCmd; // 复制命令字
		strData = packet.strData; // 复制数据内容
		sSum = packet.sSum; // 复制校验和
		
	}
	//用于接收数据转换为包格式
	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			//检查当前i开始的字节是否为包头0xFEFF
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;

			}
		}
		//如果剩余数据小于最小包数据，直接返回
		//最小包数据为包头2字节 + 包长度4字节 + 命令字2字节 + 校验和2字节 = 10字节
		if (i + 4 + 2 + 2 > nSize)
		{
			//包数据可能不全,或者包头未能全部接收到
			nSize = 0;
			return;
		}

		//获取包长度
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize)
		{//包未完全接收到,就返回,解析失败
			nSize = 0;
			return;
		}
		//获取命令字
		sCmd = *(WORD*)(pData + i); i += 2;

		//检查包长度是否正确
		//包长度至少为命令字2字节 + 校验和2字节
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			TRACE(_T("客户端解析文件名：%s\n"), ((PFILEINFO)strData.c_str())->szFileName);
			i += (nLength - 4);
		}


		//读取校验和
		sSum = *(WORD*)(pData + i); i += 2;
		//验证校验和是否正确
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;//解析成功返回解析的长度
			return;
		}
		//不正确，利用大小返回零（解析失败）
		nSize = 0;

	}

	~CPacket() {}
	CPacket& operator=(const CPacket& packet)
	{
		if (this == &packet) return *this; // 防止自赋值
		sHead = packet.sHead; // 复制包头
		nLength = packet.nLength; // 复制包长度
		sCmd = packet.sCmd; // 复制命令字
		strData = packet.strData; // 复制数据内容
		sSum = packet.sSum; // 复制校验和
		return *this;
	}
	int Size()
	{
		return nLength + 2 + 2 + 2; // 包长度 + 命令字2字节 + 校验和2字节 + 包头2字节
	}

	//
	const char* Data(std::string& strOut) const
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2; // 包头
		*(DWORD*)pData = nLength; pData += 4; // 包长度
		*(WORD*)pData = sCmd; pData += 2; // 命令字
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size(); // 数据内容
		*(WORD*)pData = sSum; pData += 2; // 校验和
		return strOut.c_str();
	}
public:
	WORD sHead; //包头
	DWORD nLength;//包长度
	WORD sCmd; //命令字
	std::string strData; //包内数据内容
	WORD sSum; //校验和
	std::string strOut; //输出整个包的数据内容

};

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //点击，移动，双击
	WORD nButton; //左键，右键，滚轮
	POINT ptXY;  //坐标

}MOUSEEV, * PMOUSEEV;

class CClientSocket
{
public:
	static CClientSocket* GetInstance()
	{
		if (m_instance == nullptr)
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket();
    

#define BUFFER_SIZE 40960000
	//处理服务端命令
	int DealCommad()
	{
		if (m_sock == -1)return false;
		char* buffer = m_buffer.data();
		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if ((int)len <= 0 && index == 0)
			{
				return -1;
			}
			index += len; // 更新索引
			len = index; // 处理数据的长度
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len); // 清除已处理的数据（将未处理的数据移动到缓冲区前面）
				index -= len; // 更新索引
				return m_packet.sCmd; // 返回命令字
			}

		}
	}

	

	bool SendPacket(HWND hWnd,const CPacket& pack, bool isAutoClosed = true, WPARAM nParam = 0);
	



	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 3))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}

		return false;
	}

	CPacket& GetPacket()
	{
		return m_packet; // 返回当前数据包对象
	}

	void CloseSocket()
	{
		if (m_sock != -1)
		{
			closesocket(m_sock); // 关闭客户端套接字
			m_sock = -1; // 设置套接字为无效状态
		}
	}	

	// 更新服务器地址
	void UpdateAddress(int nip, int nport)
	{
		if (m_nIP != nip || m_nPort != nport)
		{
			m_nIP = nip;
			m_nPort = nport;
		}		
	}

private:
	HANDLE m_eventInvoke;//启动事件
	UINT m_nThreadID; // 线程ID
	// 消息处理函数指针类型定义
	typedef void (CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	// 消息映射表
	std::map<UINT, MSGFUNC>m_mapFunc;
	HANDLE m_hThread; // 线程句柄
	bool m_bAutoClosed;
	std::mutex m_lock; // 互斥锁，保护共享资源
	std::map<HANDLE, bool>m_mapAutoClose;
	std::list<CPacket> m_listSend; // 存储接收数据包的列表
	std::map<HANDLE, std::list<CPacket>*> m_mapAck; // 存储命令响应的映射表
	int m_nIP; // 服务器IP地址
	int m_nPort; // 服务器端口号

	std::vector<char>m_buffer;
	size_t index = 0; // 缓冲区索引(始终表示缓冲区中内存结尾)
	SOCKET m_sock; // 服务器套接字
	CPacket m_packet; //数据包对象
	CClientSocket& operator=(const CClientSocket& ss)
	{
		if (this == &ss) return *this; // 防止自赋值
		m_sock = ss.m_sock; // 复制服务器套接字
		return *this;
	};
	// 复制构造函数
	CClientSocket(const CClientSocket& ss);


	CClientSocket();
	
	~CClientSocket()
	{
		// 释放套接字环境
		WSACleanup();
		m_sock = INVALID_SOCKET;
		closesocket(m_sock); // 关闭客户端套接字
	}

	static unsigned __stdcall threadEntry(void* arg);

	//void threadFunc();

	void threadFunc2();

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
			CClientSocket* tmp = m_instance;
			m_instance = NULL;  // 将 m_instance 置为 NULL，防止重复释放
			delete tmp;
		}
	}

	static CClientSocket* m_instance;

	bool Send(const char* pData, int nSize)
	{
		if (m_sock == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;

	}
	bool Send(CPacket& packet)
	{
		if (m_sock == -1)return false;
		std::string strOut;
		return send(m_sock, packet.Data(strOut), packet.Size(), 0) > 0;
	}

	// 消息处理函数           缓冲区的值     缓冲区的长度
	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);

	/*在 C++ 中，静态指针变量（如 static CClientSocket * m_instance; ）
	在程序结束时只会自动销毁指针本身（也就是释放指针变量占用的内存），
	不会自动析构和释放它所指向的对象
	（即不会自动调用 delete m_instance;，也不会调用 CServerSocket 的析构函数）*/
	class CHelper
	{
	public:
		CHelper()
		{
			CClientSocket::GetInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};

	// 确保单例在程序运行期间存在,静态变量在程序结束时自动调用析构函数
	static CHelper m_helper;

};
