#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>
class CEdoyunTool
{
public:
	// 将字节数组转换为CImage图像对象
	static int Bytes2Image(CImage& image, std::string& strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, strBuffer.size()); // 分配全局内存
		if (hGlobal == NULL)
		{
			AfxMessageBox(_T("内存分配失败！"));
			Sleep(1); // 等待后继续监控
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hGlobal, TRUE, &pStream); // 创建内存流
		if (hRet == S_OK)
		{
			// 将数据写入内存流
			ULONG nWritten = 0;
			pStream->Write(pData, strBuffer.size(), &nWritten);
			LARGE_INTEGER bg = { 0 }; // 设置流的起始位置
			pStream->Seek(bg, STREAM_SEEK_SET, NULL); // 将流的指针移动到起始位置
			if ((HBITMAP)image != NULL)
			{
				image.Destroy(); // 销毁之前的图像
			}
			image.Load(pStream); // 从内存流加载图像
		}
		return hRet;
	}
};

