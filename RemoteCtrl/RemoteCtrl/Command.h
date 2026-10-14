#pragma once
#include "Resource.h"
#include <map>
#include <atlimage.h>
#include "ServerSocket.h"
#include <direct.h>
#include "EdoyunTool.h"
#include "Packet.h"
#include "LockDialog.h"
class CCommand
{
public:
	CCommand();
    ~CCommand(){};
	int ExcuteCommand(int nCmd, std::list<CPacket>& packets,CPacket& inPacket);
    static void RunCommand(void* arg, int status,std::list<CPacket>& packets, CPacket& inPacket)
    {
		CCommand* pThis = (CCommand*)arg;//转换回对象指针
		if (status) //存在命令
        {
            //执行命令
			int ret = pThis->ExcuteCommand(status,packets,inPacket);
            if (ret != 0)
            {
				TRACE("命令执行失败，输入命令：%d,错误状态：%d\n", status,ret);
            }

        }
       
		
    }
protected:
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>& packets, CPacket& inPacket);//定义成员函数指针
	std::map<int, CMDFUNC> m_mapFunction;//命令映射表（把命令号和成员函数用哈希表关联起来）

    CLockDialog dlg;
    unsigned threadid = 0; //线程ID

protected:
    //查看磁盘分区信息
    int MakeDriverInfo(std::list<CPacket>& packets, CPacket& inPacket)
    {
        std::string result;
        for (int i = 1; i <= 26; i++)
        {
            if (_chdrive(i) == 0)
            {
                if (result.size() > 0)
                {
                    result += ",";
                }
                result += 'A' + i - 1;
            }
        }
        result += ',';

        CPacket packet(1, (BYTE*)result.c_str(), result.size());
        packets.push_back(packet);
        
        return 0;
    }


    //查看指定目录下的文件信息
    int MakeDirectoryInfo(std::list<CPacket>& packets, CPacket& inPacket)
    {
        //获取需要打开的目录路径
        std::string strPath = inPacket.strData;
        //尝试切换
        if (_chdir(strPath.c_str()) != 0)
        {
            FILEINFO finfo;
            finfo.IsInvalid = TRUE;  //标记无效
            finfo.IsDirectory = TRUE;  //是目录
            finfo.HasNext = FALSE;  //没有后续文件
            memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            packets.push_back(pack);
            OutputDebugString(L"没有权限，切换目录失败！\n");
            return -2;
        }
        //切换目录成功，开始查找文件
        //找当前目录下第一个文件
        _finddata_t fdata;
        long long hfind = _findfirst("*", &fdata);
        if (hfind == -1)
        {
            OutputDebugString(L"当前目录下没有文件！\n");
            return -3;
        }
        //开始循环查找文件，并发送文件信息给客户端
        do
        {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; //是否为目录
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            packets.push_back(pack);
        } while (_findnext(hfind, &fdata) == 0);
        //查找完毕，发送一个结束标志
        FILEINFO finfo;
        finfo.HasNext = FALSE;  //没有后续文件
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        packets.push_back(pack);

        return 0;
    }

    int RunFile(std::list<CPacket>& packets, CPacket& inPacket)
    {
        //获取需要打开的文件路径
        std::string strPath = inPacket.strData;
        //尝试打开文件
        if (_access(strPath.c_str(), 0) != 0)
        {
            OutputDebugString(L"没有权限，打开文件失败！\n");
            return -2;
        }
        //打开文件成功，执行打开操作
        ShellExecuteW(nullptr, L"open", (LPCWSTR)strPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        CPacket pack(3, NULL, 0);
        packets.push_back(pack);

        return 0;
    }

    int DownloadFile(std::list<CPacket>& packets, CPacket& inPacket)
    {
        //获取需要下载的文件路径
        std::string strPath = inPacket.strData;
        //尝试打开文件
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        long long fileSize = 0;
        if (err != 0 || pFile == NULL)
        {
            CPacket pack(4, (BYTE*)&fileSize, 8);
            packets.push_back(pack);
            OutputDebugString(L"打开文件失败！\n");
            return -3;
        }
        //打开文件成功
        //获取文件大小并发送给客户端
        fseek(pFile, 0, SEEK_END);
        fileSize = _ftelli64(pFile);
        fseek(pFile, 0, SEEK_SET);
        CPacket head(4, (BYTE*)&fileSize, 8);
        packets.push_back(head);

        //开始发送文件内容
        char buffer[1024] = "";
        size_t rlen = 0;
        do
        {
            rlen = fread(buffer, sizeof(char), sizeof(buffer), pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            packets.push_back(pack);
        } while (rlen >= 1024);
        //发送完毕，发送一个结束标志
        CPacket pack(4, NULL, 0);
        packets.push_back(pack);
        fclose(pFile);
        return 0;
    }

    int MouseEvent(std::list<CPacket>& packets, CPacket& inPacket)
    {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
		DWORD nFlags = 0;
		//根据鼠标事件类型设置相应的标志位
		switch (mouse.nButton)
		{
		case 0://左键
			nFlags = 1;
			break;
		case 1://右键
			nFlags = 2;
			break;
		case 2://中键
			nFlags = 4;
			break;
		case 4://没有按键
			nFlags = 8;
			break;
		}
		//如果鼠标事件类型为移动，则不需要设置鼠标位置
		if (nFlags != 8)
		{
			SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); //设置鼠标位置
		}
		//根据鼠标动作类型设置相应的标志位
		switch (mouse.nAction)
		{
		case 0://单击
			nFlags |= 0X10;
			break;
		case 1://双击
			nFlags |= 0X20;
			break;
		case 2://按下
			nFlags |= 0X40;
			break;
		case 3://释放
			nFlags |= 0X80;
			break;
		default:
			break;
		}
		TRACE("鼠标事件：nFlags = %08X, x = %d, y = %d\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		//根据鼠标事件类型执行相应的鼠标操作
		switch (nFlags)
		{
		case 0x21://左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			//不跳出，继续执行下一个单击，这样就实现了双击
		case 0X11://左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://左键释放
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22://右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12://右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42://右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://右键释放
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24://中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14://中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44://中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84://中键释放
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08://单纯鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, 0);
			break;
		default:
			break;
		}
		//发送鼠标事件成功，发送一个空包给客户端
		CPacket pack(5, NULL, 0);
		packets.push_back(pack);
		return 0;
	}

    int SendScreen(std::list<CPacket>& packets, CPacket& inPacket)
    {
        CImage screen; //创建一个CImage对象用于存储屏幕截图
        HDC hScreen = ::GetDC(NULL); //获取屏幕的DC
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); //获取屏幕的位深
        int nWidth = GetDeviceCaps(hScreen, HORZRES);  //获取屏幕宽度
        int nHeight = GetDeviceCaps(hScreen, VERTRES);  //获取屏幕高度
        screen.Create(nWidth, nHeight, nBitPerPixel); //创建一个适合屏幕属性的位图
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  //把屏幕像素信息拷贝上去
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK)
        {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            PBYTE pData = (PBYTE)GlobalLock(hMem);
            SIZE_T nSize = GlobalSize(hMem);
            CPacket pack(6, pData, nSize);
            packets.push_back(pack);
            GlobalUnlock(hMem);
        }
        pStream->Release();
        screen.ReleaseDC();
        ReleaseDC(NULL, hScreen);
        GlobalFree(hMem);
        return 0;

    }

    //创建一个线程来处理锁屏对话框,防止锁屏对话框阻塞主线程
    static unsigned __stdcall threadLockDlg(void* arg)
    {
        CCommand* pThis = (CCommand*)(arg);
        pThis->threadDlgMain();
        _endthreadex(0); //结束线程

        return 0; //线程结束返回0 
    }

    void threadDlgMain()
    {
        //创建一个锁屏对话框
        dlg.Create(IDD_DIALOG_INFO, NULL);
        //模态必须要父窗口，我们没有，所以用非模态
        dlg.ShowWindow(SW_SHOW);
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//置顶，不可改变大小，不可移动
        ShowCursor(FALSE); //隐藏鼠标指针
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE); //隐藏任务栏
        //遮蔽屏幕，防止用户操作其他窗口
        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXSCREEN); //获取屏幕宽度
        rect.bottom = GetSystemMetrics(SM_CYSCREEN); //获取屏幕高度
        dlg.MoveWindow(&rect); //移动对话框到屏幕中心 
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText)
        {
            CRect rtText;
            pText->GetWindowRect(&rtText); // 获取控件在屏幕上的位置

            // 将屏幕坐标转换为对话框客户区坐标
            CRect dialogClientRect;
            dlg.GetClientRect(&dialogClientRect); // 获取对话框客户区大小

            // 计算控件在对话框客户区中央的位置
            int nWidth = rtText.Width();
            int nHeight = rtText.Height();
            int x = (dialogClientRect.Width() - nWidth) / 2;
            int y = (dialogClientRect.Height() - nHeight) / 2;

            // 将文本控件移动到对话框中心
            pText->MoveWindow(x, y, nWidth, nHeight);

            // 重点：强制重绘！！！
            pText->Invalidate();           // 标记需要重绘
            pText->UpdateWindow();         // 立即重绘

            // 可选：同时重绘整个对话框以确保清除残留
            dlg.Invalidate();
            dlg.UpdateWindow();

        }
        //限制鼠标移动范围
        rect.left = 0;
        rect.top = 0;
        rect.right = 1;
        rect.bottom = 1;
        ClipCursor(&rect); //限制鼠标在对话框区域内移动

        //消息循环，等待用户输入密码
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            //如果是键盘消息,退出
            if (msg.message == WM_KEYDOWN)
            {
                if (msg.wParam == VK_ESCAPE) //按下ESC键退出锁屏
                {
                    break; //退出消息循环
                }
            }
        }
        dlg.DestroyWindow(); //销毁对话框
        ShowCursor(TRUE); //显示鼠标指针
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); //显示任务栏 
        ClipCursor(NULL); //取消鼠标限制
    }
    int LockMachine(std::list<CPacket>& packets, CPacket& inPacket)
    {
        //如果锁屏对话框已经存在，则不再创建新的对话框
        if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE)
        {
            //_beginthread(threadLockDlg, 0, NULL); //创建一个线程来处理锁屏对话框
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
        }
        CPacket pack(7, NULL, 0);
        packets.push_back(pack); //发送一个锁屏命令给客户端
        return 0;
    }

    int UnLockMachine(std::list<CPacket>& packets, CPacket& inPacket)
    {
        //向锁屏对话框所在线程发送一个解锁消息
        PostThreadMessage(threadid, WM_KEYDOWN, VK_ESCAPE, 0);
        CPacket pack(8, NULL, 0);
        packets.push_back(pack);
        //发送一个锁屏命令给客户端,表示解锁成功

        return 0;
    }

    int Test(std::list<CPacket>& packets, CPacket& inPacket)
    {
        TRACE("调用测试");
        CPacket pack(1981, NULL, 0);
        packets.push_back(pack);
        return 0;
    }

    int DeleteLocalFile(std::list<CPacket>& packets, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;
        DeleteFileA(strPath.c_str());
        CPacket pack(9, NULL, 0);
        packets.push_back(pack);
        return 0;
    }

};

