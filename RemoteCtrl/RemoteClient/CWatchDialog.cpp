// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false; // 初始化监控缓存状态为空
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序
// 客户端坐标转化为远程坐标
CPoint CWatchDialog::UserPoint2RemotePoint(CPoint& point,bool isScreen)
{
	CRect clientRect;
	if(isScreen)ScreenToClient(&point);// 屏幕坐标转化为窗口客户区坐标
	TRACE("窗口客户区：x=%d,y=%d\n", point.x, point.y);
	
	////误差处理：窗口坐标转换为控件坐标
	
	// 将对话框的客户区坐标转换为控件的客户区坐标
	::MapWindowPoints(GetSafeHwnd(), GetDlgItem(IDC_WATCH)->GetSafeHwnd(), &point, 1);
	//TRACE("控件坐标：remote x=%d,y=%d\n", point.x, point.y);
	//本地坐标到远程坐标的转换/
	//获取本地控件的屏幕坐标
	m_picture.GetWindowRect(&clientRect);
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	//获取远程屏幕分辨率（此处为本地实验所以直接获取本机系统参数）
	int screenWidth = 2358;
	int screenHeight = 1360;
	//比例转换:远程长宽/本地长宽=远程坐标/本地坐标
	int x = point.x * screenWidth / width0;
	int y = point.y * screenHeight / height0;

	return CPoint(x,y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false; // 初始化监控缓存状态为空
	//SetTimer(0, 50, nullptr);//设置定时器，50ms刷新一次
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//if (nIDEvent == 0) 
	//{
	//	if (m_isFull)//缓存是否满了
	//	{
	//		TRACE("更新图片控件");
	//		//更新
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);
	//		m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(NULL);
	//		m_image.Destroy();
	//		m_isFull = false;//重置缓存状态
	//	}


	//}

	//CDialog::OnTimer(nIDEvent);
}


LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2)
	{
		//错误处理
	}
	else if (lParam == 1)
	{
		//对方关闭了套接字
	}
	else
	{
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL)
		{
			switch (pPacket->sCmd)
			{
			case 6:
			{
				CEdoyunTool::Bytes2Image(m_image, pPacket->strData);
				//更新
				CRect rect;
				m_picture.GetWindowRect(rect);
				m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
				m_picture.InvalidateRect(NULL);
				m_image.Destroy();
				TRACE("更新图片控件");
			}
			break;
			case 5:

				break;
			case 7:

				break;
			case 8:

				break;
			default:
				break;
			}
			delete pPacket;
		}

	}
	return 0;
}
//所有鼠标消息获取的都是客户区坐标，即鼠标相对于窗口客户区左上角的坐标

// 左键双击消息
void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point);//客户区坐标转化为远程坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 1;//双击
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	CDialog::OnLButtonDblClk(nFlags, point);
}

// 左键按下消息
void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	TRACE("窗口客户区x=%d,y=%d\n", point.x, point.y);
	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point);
	//封装
	MOUSEEV event;
	TRACE("封装前x=%d,y=%d\n", remote.x, remote.y);
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 2;//按下
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	CDialog::OnLButtonDown(nFlags, point);
}

// 左键抬起消息
void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 3;//弹起
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	CDialog::OnLButtonUp(nFlags, point);
}

// 右键双击消息
void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;//右键
	event.nAction = 1;//双击
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	CDialog::OnRButtonDblClk(nFlags, point);
}

// 右键按下消息
void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;//右键
	event.nAction = 2;//按下
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	CDialog::OnRButtonDown(nFlags, point);
}

// 右键抬起消息
void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;//右键
	event.nAction = 3;//弹起
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	CDialog::OnRButtonUp(nFlags, point);
}

// 鼠标移动消息
void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 8;//无键
	event.nAction = 0;//移动
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	CDialog::OnMouseMove(nFlags, point);
}

// 静态控件点击消息
void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point);

	//坐标转换
	CPoint remote = UserPoint2RemotePoint(point,true);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 1;//单击
	//发送
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
}

//
void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}


//锁机
void CWatchDialog::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), 7);

}

//解锁
void CWatchDialog::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), 8);

}
