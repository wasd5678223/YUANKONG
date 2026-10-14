
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include "ClientController.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_serv_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_serv_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
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
//返回值是命令号，小于零为错误

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELET_FILE, &CRemoteClientDlg::OnDeletFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// TODO: 在此添加额外的初始化代码
	InitUIData(); // 初始化界面数据
	
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


// 测试连接按钮的处理函数
void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE); // 更新控件数据到成员变量

	if (m_serv_address == 0 || m_nPort.IsEmpty())
	{
		AfxMessageBox(_T("请先输入服务器地址和端口号！"));
		return;
	}
	// 发送测试命令数据包
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),1981);
}

// 查看服务器文件信息按钮的处理函数
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);
	if (ret == -1)
	{
		AfxMessageBox(_T("发送文件信息命令失败！"));
		return;
	}

	
}
void CRemoteClientDlg::DealCommand(WORD nCmd,const std::string& strData, LPARAM lParam)
{
		switch (nCmd) 
		{
		case 1://获取磁盘分区信息
			String2Tree(strData, m_Tree);
			break;
		case 2://获取指定目录下的文件信息
			UpdateFileInfo(*((PFILEINFO)strData.c_str()), (HTREEITEM)lParam);
			break;
		case 3:
			TRACE("打开文件命令响应\r\n");
			MessageBox("打开文件完成！", "操作完成", MB_ICONINFORMATION);
			break;
		case 4:
			UpdateDownloadFile(strData, (FILE*)lParam);
			break;
		case 9:
			TRACE("删除文件命令响应\r\n");
			MessageBox("删除文件完成！", "操作完成", MB_ICONINFORMATION);

			break;
		case 1981:
			TRACE("测试命令响应\r\n");
			MessageBox("链接测试成功！", "链接成功", MB_ICONINFORMATION);
			break;
		default:
			TRACE("未知命令响应:%d\r\n", nCmd);
			break;
		}
}

void CRemoteClientDlg::InitUIData()
{
	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	UpdateData();

	m_nPort = _T("9527"); // 设置默认端口号
	m_serv_address = 0x7F000001; // 设置默认服务器地址为0x7F000001   //  0xAC138EF6

	UpdateData(FALSE); // 更新控件数据到界面

	//把默认值传递给控制器
	CClientController* pController = CClientController::GetInstance();
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)m_nPort));

	m_dlgStatus.Create(IDD_DLG_STATUS, this); // 创建状态对话框
	m_dlgStatus.ShowWindow(SW_HIDE); // 隐藏状态对话框
}
CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	// 获取选中节点的路径（往上追溯）
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree); // 获取父节点
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildItem(HTREEITEM hTree)
{
	HTREEITEM hSub = m_Tree.GetChildItem(hTree); // 获取第一个子节点
	while (hSub != NULL)
	{
		HTREEITEM hNext = m_Tree.GetNextSiblingItem(hSub); // 先获取下一个兄弟节点
		m_Tree.DeleteItem(hSub); // 再删除当前节点
		hSub = hNext; // 移动到下一个兄弟节点
	}
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;// 鼠标位置
	GetCursorPos(&ptMouse);// 获取鼠标位置
	m_Tree.ScreenToClient(&ptMouse);// 将鼠标位置转换为树控件的坐标
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);// 获取鼠标点击的树控件项
	if (hTreeSelected == NULL)// 如果没有选中项，直接返回
		return;
	DeleteTreeChildItem(hTreeSelected);// 删除选中节点的所有子节点
	m_List.DeleteAllItems();// 清空列表控件（防止每次点击树控件累加）	
	CString strPath = GetPath(hTreeSelected);// 获取选中节点的路径
	CClientController::GetInstance()->SendCommandPacket
	(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(),(WPARAM)hTreeSelected);// 发送查看指定目录下的文件命令数据包
}


// 加载当前目录下的文件信息
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTreeSelected = m_Tree.GetSelectedItem();
    m_List.DeleteAllItems();// 清空列表控件（防止每次点击树控件累加）	
	CString strPath = GetPath(hTreeSelected);// 获取选中节点的路径
	std::list<CPacket> listPackets;
	int nCmd = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());// 发送查看指定目录下的文件命令数据包
	PFILEINFO pInfo = (PFILEINFO)listPackets.front().strData.c_str();// 获取服务器返回的文件信息数据包
	CClientSocket* pClient = CClientSocket::GetInstance();
	while (pInfo->HasNext)
	{
		if (!pInfo->IsDirectory)
		{
			// 如果是文件，则在列表控件中插入一行
			m_List.InsertItem(0, pInfo->szFileName);
		}
		 
		int cmd = pClient->DealCommad();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::GetInstance()->GetPacket().strData.c_str();
	}
	//pClient->CloseSocket();
}

void CRemoteClientDlg::String2Tree(const std::string& driver, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems(); // 清空树控件（防止每次点击树控件累加）
	for (size_t i = 0; i < driver.size(); i++)// 遍历驱动器信息字符串
	{
		// 如果遇到逗号，则表示一个驱动器信息结束，插入到树控件中
		if (driver[i] == ',')
		{
			dr += ":"; // 添加冒号，表示驱动器的格式
			HTREEITEM hTmp = tree.InsertItem(CString(dr.c_str()), TVI_ROOT, TVI_LAST); // 插入驱动器节点
			tree.InsertItem(NULL, hTmp, TVI_LAST);// 第二行：在主节点下插入一个空子节点
			dr.clear();
			continue;

		}
		//如果没有遇到逗号，则继续拼接驱动器信息
		dr += driver[i];
	}
	// 处理最后一个驱动器信息（如果有的话）
	if (dr.size() > 0)
	{
		dr += ":"; // 添加冒号，表示驱动器的格式
		HTREEITEM hTmp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST); // 插入驱动器节点
		tree.InsertItem(NULL, hTmp, TVI_LAST); // 第二行：在主节点下插入一个空子节点
	}


}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& pInfo,HTREEITEM hParent)
{
	if (pInfo.HasNext == FALSE)
		return;
	if (pInfo.IsDirectory)// 如果是目录
	{
		// 如果是目录，则在树控件中插入一个节点
		if (CString(pInfo.szFileName) == "." || (CString(pInfo.szFileName) == ".."))
		{
			// 如果是当前目录或者上级目录，则不插入节点
			return;
		}
		// 插入目录节点到树控件
		HTREEITEM hTemp = m_Tree.InsertItem(pInfo.szFileName, (HTREEITEM)hParent, TVI_LAST);
		m_Tree.InsertItem("", hTemp, TVI_LAST);// 第二行：在主节点下插入一个空子节点
		m_Tree.Expand((HTREEITEM)hParent, TVE_EXPAND); // 展开父节点

	}
	else
	{
		// 如果是文件，则在列表控件中插入一行
		m_List.InsertItem(0, pInfo.szFileName);
	}


}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	if (length == 0) {
		length = *(long long*)strData.c_str();
		if (length == 0) {
			AfxMessageBox("文件长度为零或者无法读取文件！！！");
			CClientController::GetInstance()->DownloadEnd();
		}
	}
	else if (length > 0 && (index >= length))
	{
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::GetInstance()->DownloadEnd();
	}
	else
	{
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		if (index >= length)
		{
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::GetInstance()->DownloadEnd();
		}

	}


}

// 双击树控件的处理函数->加载文件信息
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}
// 单击树控件的处理函数->加载文件信息
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}
// 右键点击文件列表的处理函数->弹出菜单
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, PtList; // 鼠标位置和列表控件位置
	GetCursorPos(&ptMouse); // 获取鼠标位置
	PtList = ptMouse; // 将鼠标位置赋值给列表控件位置
	m_List.ScreenToClient(&PtList); // 转换为列表控件的坐标
	int ListSelected = m_List.HitTest(PtList);// 获取列表控件选中项的索引
	if (ListSelected == -1) // 如果没有选中项，直接返回
		return;
	CMenu menu; // 创建菜单对象
	menu.LoadMenu(IDR_MENU_RCLICK); // 加载菜单资源
	CMenu* pPupup = menu.GetSubMenu(0); // 获取子菜单
	if (pPupup != NULL)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this); // 弹出菜单
	}
	else
	{
		AfxMessageBox(_T("菜单加载失败！")); // 如果菜单加载失败，弹出提示框
	}
}
// 下载文件按钮的处理函数
void CRemoteClientDlg::OnDownloadFile()
{
	CClientController::GetInstance()->DownFile(
		GetPath(m_Tree.GetSelectedItem()) +
		m_List.GetItemText(m_List.GetSelectionMark(), 0));
}
// 删除文件按钮的处理函数
void CRemoteClientDlg::OnDeletFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		MessageBox("删除文件命令执行失败！！！");
	}
	LoadFileCurrent();
}
// 运行文件按钮的处理函数
void CRemoteClientDlg::OnRunFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		MessageBox("打开文件命令执行失败！！！");
	}
}


// 开始监控按钮的处理函数
void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::GetInstance()->StartWatchScreen(); // 启动监控屏幕线程
	return;

}

void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}

// IP地址控件内容变化的处理函数
void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	//更新成员变量中的IP地址(把ip数据交给控制类)
	UpdateData(TRUE); // 更新控件数据到成员变量
	CClientController* pController = CClientController::GetInstance();
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)m_nPort));

}

void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	//更新成员变量中的IP地址(把ip数据交给控制类)
	UpdateData(TRUE); // 更新控件数据到成员变量
	CClientController* pController = CClientController::GetInstance();
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)m_nPort));

}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2)
	{
		//错误处理
		TRACE("套接字错误，错误码:%d\r\n", lParam);
	}
	else if (lParam == 1)
	{
		//对方关闭了套接字
		TRACE("对方关闭了连接\r\n");
	}
	else
	{
		CPacket head = *(CPacket*)wParam;//复制过来（局部，离开后自动释放）
		delete (CPacket*)wParam;//释放内存
		DealCommand(head.sCmd, head.strData, lParam);
	}
	return 0;
}