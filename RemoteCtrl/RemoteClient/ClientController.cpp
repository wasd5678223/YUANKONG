#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"
std::map<UINT, CClientController::MSGFUNC>
CClientController::m_mapFunc{};
CClientController* CClientController::m_instance = nullptr;
CClientController::CHelper CClientController::m_helper; // 静态成员变量定义，确保单例在程序结束时被释放
int CClientController::InitController()
{
	//创建监听线程  
	m_hThread = (HANDLE)_beginthreadex(NULL, 0,
		&CClientController::threadEntry,
		this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	//设置主窗口指针
	pMainWnd = &m_remoteDlg;
	//启动主对话框
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessageToController(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // 创建事件对象用于同步
	if (hEvent == NULL)
	{
		return -2;
	}
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
	WaitForSingleObject(hEvent, INFINITE);//发送消息后等待处理完成（阻塞）
	CloseHandle(hEvent);// 关闭事件句柄
	return info.result;
}


bool CClientController::SendCommandPacket
(HWND hWnd,int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::GetInstance();
	CPacket packet(nCmd, pData, nLength);
	pClient->SendPacket(hWnd, packet, bAutoClose, wParam);
	return true;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(
		FALSE, NULL,
		strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);

		/*m_hThreadDownload =
			(HANDLE)_beginthread
			(&CClientController::threadDownloadEntry, 0, this);*/
		//等待下载线程启动
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		//{
		//	//下载线程启动失败
		//	return -1;
		//}
		m_remoteDlg.BeginWaitCursor();// 显示等待光标
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));// 设置状态对话框的提示信息
		m_statusDlg.ShowWindow(SW_SHOW);// 显示状态对话框
		m_statusDlg.CenterWindow(&m_remoteDlg);// 将状态对话框居中显示
		m_statusDlg.SetActiveWindow();// 激活状态对话框
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClose = false;
	m_hThreadWatch = (HANDLE)_beginthread
	(&CClientController::threadWatchEntry, 0, this); // 创建监控线程
	m_watchDlg.DoModal(); // 弹出监控对话框(模态)
	m_isClose = true;
	WaitForSingleObject(m_hThreadWatch, 500); //  等待监控线程结束

}

// 远程监控对话框线程函数
void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();// 获取当前时间戳
	while (!m_isClose) {
		if (m_watchDlg.isFull() == false) {
			if (GetTickCount64() - nTick < 400)
			{
				Sleep(400 - (DWORD)(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			if (ret == 1) 
			{
				TRACE("成功发送图片请求！");
			}
			else
			{
				TRACE("发送  是请求图片操作  失败！ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}

}

// 远程监控对话框线程入口
void CClientController::threadWatchEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthreadex(0);// 结束线程
	return;
}


// 线程主函数逻辑
void CClientController::threadFunc()
{
	
	MSG msg;// 消息结构体
	//没有消息就阻塞等待，有消息就处理消息（正常消息非零进入循环，0退出）
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);// 翻译消息
		DispatchMessage(&msg);// 分发消息
		// 处理自定义消息
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
		
	}


}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	// 将传入的参数转换为 CClientController 对象指针
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);// 结束线程
	return 0;
}




LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//非模态，非阻塞显示
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//返回阻塞的，模态对话框
	return m_watchDlg.DoModal();
}
