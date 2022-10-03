#include <iostream>
#include <string>
#include <easyhook.h>
#include <cstring>
#include <tlhelp32.h>

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS2) (HANDLE, USHORT*, USHORT*);

LPFN_ISWOW64PROCESS2 fnIsWow64Process2;

BOOL IsWow64(HANDLE hProcess)
{
    USHORT bIsWow64 = 0;

    //IsWow64Process2 is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process2 = (LPFN_ISWOW64PROCESS2) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process2");

    if(fnIsWow64Process2 != NULL)
    {
        if (!fnIsWow64Process2(hProcess, &bIsWow64, NULL))
        {
            //handle error
        }
        if (bIsWow64 == IMAGE_FILE_MACHINE_UNKNOWN)
            return TRUE;
    }
    return FALSE;
}

std::wstring getProcessInformation(DWORD id) 
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnapshot) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if(Process32First(hSnapshot, &pe32)) {
            do {
               if (pe32.th32ProcessID == id)
               {
                   return std::wstring(pe32.szExeFile);
               }
            } while(Process32Next(hSnapshot, &pe32));
         }
         CloseHandle(hSnapshot);
    }
    return std::wstring();
}


int wmain(int argc, wchar_t* argv[])
{
    DWORD process_id;
    std::wcout << "Enter the target process Id: ";
    std::cin >> process_id;

    
    auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, process_id);
    if (!hProcess)
    {
        std::cout << "Can't find process with id: " << process_id;
        return GetLastError();
    }


    wchar_t* dllToInject32 = (wchar_t*)L".\\PSHook32.dll";
    wchar_t* dllToInject64 = (wchar_t*)L".\\PSHook64.dll";
    std::wstring processPath = getProcessInformation(process_id);
    int length = processPath.length() + 1; 
    wchar_t *buff = new wchar_t[length];
    processPath.copy(buff,length);
    buff[length-1] = L'\0';
    std::wcout << "Exe: " << buff << std::endl;
    std::wcout << "Attemping to inject x32: " << dllToInject32 << std::endl << std::endl;
    std::wcout << "Attemping to inject x64: " << dllToInject64 << std::endl << std::endl;
    NTSTATUS nt;
    if (IsWow64(hProcess))
    {
         nt = RhInjectLibrary(
            process_id,
            0,
            EASYHOOK_INJECT_DEFAULT,
            NULL,
            dllToInject64,
            buff,
            sizeof(wchar_t) * length
        );

    }
    else
    {
        nt = RhInjectLibrary(
            process_id,
            0,
            EASYHOOK_INJECT_DEFAULT,
            dllToInject32,
            NULL,
            buff,
            sizeof(wchar_t) * length
        );
    }

    if (nt != 0)
    {
        std::wcout << "RhInjectLibrary failed with error code = " << nt << std::endl;
        PWCHAR err = RtlGetLastErrorString();
		std::wcout << err << std::endl;
    }
    else 
	{
		std::wcout << L"Library injected successfully.\n";
	}

	std::wcout << "Press Enter to exit";
	std::wstring input;
	std::getline(std::wcin, input);
    std::getline(std::wcin, input);
    CloseHandle(hProcess);
    delete [] buff;
	return 0;
}