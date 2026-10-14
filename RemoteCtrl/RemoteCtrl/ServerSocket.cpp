#include "pch.h"  
#include "ServerSocket.h"  

//类的静态成员变量初始化（必须在类外，且要带上变量类型）
CServerSocket* CServerSocket::m_instance = NULL;

CServerSocket::CHelper CServerSocket::m_helper; //静态成员变量初始化

CServerSocket* pserver = CServerSocket::GetInstance(); //获取服务器实例