#include "pch.h"
#include "Command.h"

CCommand::CCommand()
{
	struct
	{
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1, &CCommand::MakeDriverInfo },
		{2, &CCommand::MakeDirectoryInfo },
		{3, &CCommand::RunFile },
		{4, &CCommand::DownloadFile },
		{5, &CCommand::MouseEvent },
		{6, &CCommand::SendScreen },
		{7, &CCommand::LockMachine },
		{8, &CCommand::UnLockMachine },
		{9, &CCommand::DeleteLocalFile },
		{1981, &CCommand::Test },
		{-1, nullptr } //结束标志
	};

	for (int i = 0;data[i].nCmd != -1; i++)
	{
		m_mapFunction.insert(std::make_pair(data[i].nCmd, data[i].func));
	}

}

int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& packets, CPacket& inPacket)
{
	auto it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end())
	{
		return -1;
	}
	return (this->*(it->second))(packets, inPacket); // 直接当场调用找到的对应成员函数

}
