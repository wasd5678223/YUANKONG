#pragma once
class CEdoyunTool
{
	public:
        // 检查是否有管理员权限
        static bool IsAdmin()
        {
            HANDLE hToken = NULL;// 访问令牌句柄
            // 打开当前进程的访问令牌
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            {
                ShowError();
                return false;
            }
            // 获取令牌的提升信息
            TOKEN_ELEVATION eve;
            DWORD len = 0;
            // 获取提升信息
            if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
                ShowError();
                return false;
            }
            CloseHandle(hToken);// 关闭句柄
            // 判断是否有提升的权限
            if (len == sizeof(eve))
            {
                return eve.TokenIsElevated;
            }
            printf("GetTokenInformation length error: %d \n", len);
        }

        static bool RunAsAdmin()
        {
            TCHAR sPath[MAX_PATH] = _T("");
            GetModuleFileName(NULL, sPath, MAX_PATH);
        
            SHELLEXECUTEINFO sei = { sizeof(sei) };
            sei.lpVerb = _T("runas");
            sei.lpFile = sPath;
            sei.nShow = SW_SHOWNORMAL;
        
            if (!ShellExecuteEx(&sei))
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_CANCELLED)
                {
                    ShowError();
                }
                return false;
            }
            return true;
        }

        // 显示错误信息
        static void ShowError()
        {
            LPWSTR lpMessageBuf = NULL;
            //strerror(errno);//标准C语言库
            FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                NULL, GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&lpMessageBuf, 0, NULL);
            OutputDebugString(lpMessageBuf);
            LocalFree(lpMessageBuf);
        }

		// 写入启动目录
        static bool WriteStartupDir(const CString& strPath)
        {
            TCHAR sPath[MAX_PATH] = _T("");
            GetModuleFileName(NULL, sPath, MAX_PATH);//直接获取当前可执行文件路径（包括文件名)

             return CopyFile(sPath, strPath, FALSE); // 复制文件到启动目录
            //fopen CFile system(copy) CopyFile OpenFile
            
        }

        // 设置自动启动
//开机启动的时候，程序的权限是跟随启动用户的
//如果两者权限不一致，可能会导致无法正常启动
//开机启动对环境变量有影响，如果程序依赖dll（动态库），可能会启动失败

		// 写入注册表启动项
        static bool WriteRegisterTable(const CString& strPath)
        {
            CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");

            TCHAR sPath[MAX_PATH] = _T("");
            GetModuleFileName(NULL, sPath, MAX_PATH);//直接获取当前可执行文件路径（包括文件名)   
            BOOL ret = CopyFile(sPath, strPath, FALSE); // 复制文件到启动目录
            //fopen CFile system(copy) CopyFile OpenFile
            if (ret == FALSE)
            {
                MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
                return false;
            }

            HKEY hKey = NULL;
            ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
            if (ret != ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"),
                    _T("错误"), MB_ICONERROR | MB_TOPMOST);
                return false;
            }

            ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
            if (ret != ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"),
                    _T("错误"), MB_ICONERROR | MB_TOPMOST);
                return false;
            }
            RegCloseKey(hKey);
            return true;
        }

        // 初始化应用程序（程序简化）
        static bool Init()
        {
            HMODULE hModule = ::GetModuleHandle(nullptr);
            if (hModule == nullptr)
            {
                wprintf(L"错误:GetModuleHandle 失败\n");
                return false;
            }
            if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
            {
                // TODO: 在此处为应用程序的行为编写代码。
                wprintf(L"错误: MFC 初始化失败\n");
                return false;
            }
            return true;
        }




};
