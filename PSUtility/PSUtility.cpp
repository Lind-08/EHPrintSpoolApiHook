#include <iostream>
#include <string>
#include <Windows.h>
#include <winspool.h>

int wmain(int argc, wchar_t* argv)
{
    std::wstring value;
    HMODULE res = NULL;
    DWORD length = 0;
    wchar_t *buffer;
    while (true)
    {
        HANDLE currentThread = GetCurrentThread();
        std::wcout << L"PSUtility.exe process id: ";
        std::wcout << GetProcessIdOfThread(currentThread) << std:: endl;
        CloseHandle(currentThread);
        if (res == NULL) 
            CloseHandle(res);

        std::wcout << L"Press <enter> to LL (Ctrl-C to exit): ";
        std::getline(std::wcin, value);
        res = LoadLibrary(L"winspool.drv");
        if (res)
            std::wcout << L"winspool.drv loaded!" << std::endl;
        auto resStr = GetDefaultPrinterW(NULL,&length);
        buffer = new wchar_t[length];
        resStr = GetDefaultPrinterW(buffer, &length);
        std::wcout << std::wstring(buffer) << std::endl;
        delete [] buffer;
    }
    return 0;
}
