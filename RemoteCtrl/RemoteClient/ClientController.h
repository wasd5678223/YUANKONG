#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "EdoyunTool.h"

#define WM_SHOW_STATUS (WM_USER + 3)//展示状态对话框
#define WM_SHOW_WATCH (WM_USER + 4)//展示远程监控对话框
#define WM_SEND_MESSAGE (WM_USER + 0x1000)//发送消息给控制器(自定义消息处理)



class CClientController
{
public:
	//单例模式(获取全局唯一对象)
    static CClientController* GetInstance()
    {
		if (m_instance == nullptr)
		{
			m_instance = new CClientController();
			//定义消息和对应的处理函数
			struct
			{
				UINT nMsg;
				MSGFUNC func;
			}MsgFuncs[] =
			{
				{ WM_SHOW_STATUS, &CClientController::OnShowStatus },
				{ WM_SHOW_WATCH, &CClientController::OnShowWatch },
				{(UINT) - 1,NULL}
			};
			//循环将消息和对应的处理函数加入映射表，以便后续查找调用
			for (int i = 0; MsgFuncs[i].func != NULL; i++)
			{
				m_mapFunc.insert(std::make_pair(MsgFuncs[i].nMsg, MsgFuncs[i].func));
			}
		}
		return m_instance;
        
	}
	//初始化
    int InitController();
    //启动
    int Invoke(CWnd*& pMainWnd);
	//发送消息
	LRESULT SendMessageToController(MSG msg);
	//更新网络服务器地址和端口
	void UpdateAddress(int nIP, int nPort)
	{
		CClientSocket::GetInstance()->UpdateAddress(nIP, nPort);
	}

	//处理命令
	int DealCommand()
	{
		return CClientSocket::GetInstance()->DealCommad();
	}
	//关闭套接字
	void CloseSocket()
	{
		CClientSocket::GetInstance()->CloseSocket();
	}

	// 发送命令数据包
//1：查看磁盘分区
//2：查看指定目录下的文件
//3：打开文件
//4：下载文件
//5：鼠标操作
//6：发送屏幕内容（发送屏幕截图到客户端）
//7：锁机
//8：解锁
//9: 删除文件
//1981：测试
//返回值是状态，true成功，false失败
	bool SendCommandPacket(
		HWND hWnd,//需要应答的窗口
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		WPARAM wParam = 0);

	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::GetInstance();
		
		// TODO: 存入CIImage
		return CEdoyunTool::Bytes2Image(image,pClient->GetPacket().strData);

	}

	void DownloadEnd();


	

	//下载文件
	int DownFile(CString strPath);
	

	void StartWatchScreen();

	
protected:
	
	void threadWatchScreen();//远程监控对话框线程函数
	static void  threadWatchEntry(void* arg); //远程监控对话框线程入口
	CClientController():
		m_statusDlg(&m_remoteDlg),
		m_watchDlg(&m_remoteDlg)
	{
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_isClose = true;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;

	}
	~CClientController()
	{
		WaitForSingleObject(m_hThread, 100);

	}
	void threadFunc();
	////静态函数不能使用this指针，所以需要传入this指针，然后调用成员函数（而且这样可以分割业务逻辑）
	static unsigned __stdcall threadEntry(void* arg);
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			// 释放单例实例
			CClientController* tmp = m_instance;
			m_instance = NULL;  // 将 m_instance 置为 NULL，防止重复释放
			delete tmp;
		}
	}

	
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo
	{
		MSG msg;
		LRESULT result;

		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}

		MsgInfo(const MsgInfo& m)
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}

		MsgInfo& operator=(const MsgInfo& m)
		{
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	} MSGINFO;
	// 消息处理函数指针类型定义
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg,WPARAM wParam,LPARAM lParam);
	// 消息映射表
	static std::map<UINT,MSGFUNC>m_mapFunc;
	
	/*在 C++ 中，静态指针变量（如 static CClientSocket * m_instance; ）
	在程序结束时只会自动销毁指针本身（也就是释放指针变量占用的内存），
	不会自动析构和释放它所指向的对象
	（即不会自动调用 delete m_instance;，也不会调用 CServerSocket 的析构函数,所以我们需要手动释放）*/
	CWatchDialog m_watchDlg; //远程监控对话框对象
	CRemoteClientDlg m_remoteDlg; //主对话框对象
	CStatusDlg m_statusDlg; //状态对话框对象
	HANDLE m_hThread;//监听线程句柄
	HANDLE m_hThreadWatch;//远程监控线程句柄
	bool m_isClose;//是否关闭连接
	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地路径
	CString m_strLocal;
	unsigned m_nThreadID;//监听线程ID

    static CClientController* m_instance;
	class CHelper
	{
	public:
		CHelper()
		{
		}
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};

	static CHelper m_helper;



};

