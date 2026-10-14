// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "EdoyunTool.h"
#include <conio.h>
#include "CEdoyunQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define INVOKE_PATH _T("C:\\Users\\LENOVO\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;



bool ChooseAutoInvoke(const CString& strPath)
{
	TCHAR wcsSystem[MAX_PATH] = _T("");
	GetSystemDirectory(wcsSystem, MAX_PATH);
	//目前是本机用户的启动目录（远程调试记得换）
     if (PathFileExists(strPath))
    {
		// 已经设置自动启动
		return true;
    }
	CString strInfo = _T("该程序只允许用于合法的用途！\n");
	strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
	strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
		if (CEdoyunTool::WriteStartupDir(strPath) == false)
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
		}
		//WriteRegisterTable(strPath);
        
    }
	else if (ret == IDCANCEL) {
        return false;
	}
    return true;
}


enum
{
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};

typedef struct IocpParam
{
    int nOperator; // 操作
    std::string strData; // 数据
    _beginthread_proc_type cbFunc; // 回调

    IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL)
    {
        nOperator = op;
        strData = sData;
        cbFunc = cb;
    }

    IocpParam()
    {
        nOperator = -1;
    }
} IOCP_PARAM;


////工作线程threadQueueEntry通过GetQueuedCompletionStatus监听 IOCP 事件，处理队列操作
//void threadQueueEntry(HANDLE hIOCP)
//{
//	threadmain(hIOCP);
//	_endthread();
//}
//
//
//void threadmain(HANDLE hIOCP)
//{
//    std::list<std::string> lstString;  // 核心队列（存储字符串）
//    DWORD dwTransferred = 0;           // 传输字节数（IOCP参数，这里仅占位）
//    ULONG_PTR CompletionKey = 0;       // 自定义键（指向IocpParam对象）
//    OVERLAPPED* pOverlapped = NULL;    // 重叠结构（IOCP异步IO用，这里仅占位）
//    //循环监听完成端口事件（ 阻塞等待IOCP事件：INFINITE表示无限等待）
//    while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
//    {
//        // 退出条件：传输字节数为0 或 CompletionKey为空（主线程投递退出信号）
//        if ((dwTransferred == 0) || (CompletionKey == NULL))
//        {
//            printf("thread is prepare to exit!\r\n");
//            break;
//        }
//        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
//        if (pParam->nOperator == IocpListPush) {
//            lstString.push_back(pParam->strData);
//        }
//        else if (pParam->nOperator == IocpListPop)
//        {
//            std::string* pStr = NULL;
//            if (lstString.size() > 0) {
//                pStr = new std::string(lstString.front());
//                lstString.pop_front();
//            }
//            if (pParam->cbFunc) {
//                pParam->cbFunc(pStr);
//            }
//        }
//        else if (pParam->nOperator == IocpListEmpty)
//        {
//            lstString.clear();
//        }
//        delete pParam;
//    }
//}


// 回调函数，处理弹出数据
void func(void* arg)
{
    std::string* pStr = (std::string*)arg;
    if (pStr != NULL) {
        printf("pop from list:%s\r\n", pStr->c_str());
        delete pStr;
    }
    else {
        printf("list is empty, no data!\r\n");
    }
}

int main()
{
    //if (!CEdoyunTool::Init())return 1;

	//CEdoyunQueue<std::string> lstStrings;//创建基于IOCP的队列对象
	//ULONGLONG tick0 = GetTickCount64(), tick = GetTickCount64();

 //  
	//while (_kbhit() == 0) {//完成端口把请求和响应解耦开了
	//	//每隔一段时间，向完成端口投递不同的请求（模拟真实情况）
 //       if (GetTickCount64() - tick0 > 1300) 
 //       {
 //           lstStrings.PushBack("hello world\n");
	//		tick0 = GetTickCount64();
 //        
 //       }
 //       if (GetTickCount64() - tick > 2000) {
 //           std::string ans{};
	//		lstStrings.PopFront(ans);
 //           if (!ans.empty()) 
 //           {
 //               printf(ans.c_str());
 //           }
 //           tick = GetTickCount64();
 //       }
 //       Sleep(1);
 //   }
    if (CEdoyunTool::IsAdmin())
    {
        if (!CEdoyunTool::Init())return 1;
        OutputDebugString(L"当前进程具有管理员权限。\n");
        if (ChooseAutoInvoke(INVOKE_PATH))
        {
            CCommand cmd;
            int ret = CServerSocket::GetInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret)
            {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功创建套接字，请检查网咯状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                break;
            case -2:
                MessageBox(NULL, _T("已超过最大重试次数,程序结束！"), _T("接受客户端连接失败！"), MB_OK | MB_ICONERROR);
                break;
            default:
                break;
            }
        }
       
    }
    else
    {
        OutputDebugString(L"当前进程不具有管理员权限。\n");
        
        if (CEdoyunTool::RunAsAdmin() == false)
        {
            CEdoyunTool::ShowError();
            return 1;
        }
    } 
    return 0;
}
