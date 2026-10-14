
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#ifndef WM_SEND_PACK
#define WM_SEND_PACK_ACK (WM_USER + 2) //应答
#endif




// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数
	void LoadFileInfo(); //查看文件
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


	
private:
	void DealCommand(WORD nCmd,const std::string& strData,LPARAM lParam);//处理命令
	void InitUIData();//初始化界面数据
	//由选中的文件树控件获取文件路径
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildItem(HTREEITEM hTree);
	
	void LoadFileCurrent();//加载当前目录文件（刷新）

	void String2Tree(const std::string& driver, CTreeCtrl& tree);//字符串转树控件
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent);//更新文件信息到列表控件
	void UpdateDownloadFile(const std::string& strData,FILE* pFile);//下载文件

// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus; //状态对话框

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_serv_address;
	CString m_nPort;
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedBtnFileinfo();
	
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeletFile();
	afx_msg void OnRunFile();
	
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);
};
