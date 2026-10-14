#pragma once

#include "pch.h"
#include "framework.h"
#include <list>



class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), strData(""), sSum(0)
	{}
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
			i += nLength - 4;
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

	~CPacket()
	{}
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
	const char* Data()
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2; // 包头
		*(DWORD*)pData = nLength; pData += 4; // 包长度
		*(WORD*)pData = sCmd; pData += 2; // 命令字
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size(); // 数据内容
		*(WORD*)pData = sSum; pData += 2; // 校验和
		return strOut.c_str(); // 返回数据内容的指针
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

#include <io.h>
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

