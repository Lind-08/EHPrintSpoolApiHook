// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "framework.h"

std::wstring hookMessage;
std::wofstream outputFile;
std::string path = "D:\\hookLog.txt";
std::wstring processPath;

void writeToLog(std::wstring message)
{
	using namespace std::chrono;
	system_clock::time_point today = system_clock::now();
	time_t tt;
	tt = system_clock::to_time_t ( today );

	outputFile.open(path, std::fstream::app);
#pragma warning(suppress : 4996)	
	outputFile << ctime(&tt)  << " " << message << L" PID: " << GetCurrentProcessId() << L" EXE: " << processPath << std::endl;
	outputFile.close();
}

BOOL WINAPI myOpenPrinterA(
  _In_  LPSTR             pPrinterName,
  _Out_ LPHANDLE           phPrinter,
  _In_  LPPRINTER_DEFAULTSA pDefault
)
{
	writeToLog(L"OpenPrinterA called");
	return OpenPrinterA(pPrinterName, phPrinter, pDefault);
}

BOOL WINAPI myOpenPrinterW(
  _In_  LPTSTR             pPrinterName,
  _Out_ LPHANDLE           phPrinter,
  _In_  LPPRINTER_DEFAULTS pDefault
)
{
	writeToLog(L"OpenPrinterW called");
	return OpenPrinterW(pPrinterName, phPrinter, pDefault);
}


BOOL WINAPI myGetDefaultPrinterW(
  _In_    LPTSTR  pszBuffer,
  _Inout_ LPDWORD pcchBuffer
)
{
	writeToLog(L"GetDefaultPrinterW called");
	return GetDefaultPrinterW(pszBuffer, pcchBuffer);
}

BOOL WINAPI myGetDefaultPrinterA(
  _In_    LPSTR  pszBuffer,
  _Inout_ LPDWORD pcchBuffer
)
{
	writeToLog(L"GetDefaultPrinterA called");
	return GetDefaultPrinterA(pszBuffer, pcchBuffer);
}

void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	std::wcout << "\n\nNativeInjectionEntryPointt(REMOTE_ENTRY_INFO* inRemoteInfo)\n\n" <<
		"IIIII           jjj               tt                dd !!! \n"
		" III  nn nnn          eee    cccc tt      eee       dd !!! \n"
		" III  nnn  nn   jjj ee   e cc     tttt  ee   e  dddddd !!! \n"
		" III  nn   nn   jjj eeeee  cc     tt    eeeee  dd   dd     \n"
		"IIIII nn   nn   jjj  eeeee  ccccc  tttt  eeeee  dddddd !!! \n"
		"              jjjj                                         \n\n";

	std::wcout << "Injected by process Id: " << inRemoteInfo->HostPID << std::endl;

	if (inRemoteInfo->UserDataSize != NULL)
	{
		processPath = std::wstring(reinterpret_cast<wchar_t*>(inRemoteInfo->UserData));
	}

	// Perform hooking
	HOOK_TRACE_INFO hHook = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO hHook1 = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO hHook2 = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO hHook3 = { NULL }; // keep track of our hook

	auto winspl = LoadLibraryW(L"winspool.drv");

	if (winspl == NULL)
	{
		std::cerr << "Can't load winspool.drv. Error: " << GetLastError() << std::endl;
		return;
	}

	std::cout << std::endl << "Win32 GetDefaultPrinterW found at address: " << GetProcAddress(winspl, "GetDefaultPrinterW") << "\n";

	std::cout << std::endl << "Win32 GetDefaultPrinterA found at address: " << GetProcAddress(winspl, "GetDefaultPrinterA") << "\n";

	std::cout << std::endl << "Win32 OpenPrinterA found at address: " << GetProcAddress(winspl, "OpenPrinterA") << "\n";

	std::cout << std::endl << "Win32 OpenPrinterW found at address: " << GetProcAddress(winspl, "OpenPrinterW") << "\n";


	// Install the hook

	NTSTATUS result = LhInstallHook(
		GetProcAddress(winspl, "OpenPrinterA"),
		myOpenPrinterA,
		NULL,
		&hHook);
	if (FAILED(result))
	{
		std::wstring s(RtlGetLastErrorString());
		std::wcout << "Failed to install hook: ";
		std::wcout << s;
	}
	else 
	{
		std::cout << "Hook 'myOpenPrinterA installed successfully." << std::endl;
	}
		

	result = LhInstallHook(
		GetProcAddress(winspl, "OpenPrinterW"),
		myOpenPrinterW,
		NULL,
		&hHook1);
	if (FAILED(result))
	{
		std::wstring s(RtlGetLastErrorString());
		std::wcout << "Failed to install hook: ";
		std::wcout << s;
	}
	else 
	{
		std::cout << "Hook 'myOpenPrinterW installed successfully." << std::endl;
	}

	result = LhInstallHook(
		GetProcAddress(winspl, "GetDefaultPrinterW"),
		myGetDefaultPrinterW,
		NULL,
		&hHook2);
	if (FAILED(result))
	{
		std::wstring s(RtlGetLastErrorString());
		std::wcout << "Failed to install hook: ";
		std::wcout << s;
	}
	else 
	{
		std::cout << "Hook 'myGetDefaultPrinterW installed successfully." << std::endl;
	}

	result = LhInstallHook(
		GetProcAddress(winspl, "GetDefaultPrinterA"),
		myGetDefaultPrinterA,
		NULL,
		&hHook3);
	if (FAILED(result))
	{
		std::wstring s(RtlGetLastErrorString());
		std::wcout << "Failed to install hook: ";
		std::wcout << s;
	}
	else 
	{
		std::cout << "Hook 'GetDefaultPrinterA installed successfully." << std::endl;
	}

	// If the threadId in the ACL is set to 0,
	// then internally EasyHook uses GetCurrentThreadId()
	ULONG ACLEntries[1] = { 0 };

	// Disable the hook for the provided threadIds, enable for all others
	LhSetExclusiveACL(ACLEntries, 1, &hHook);
	LhSetExclusiveACL(ACLEntries, 1, &hHook1);
	LhSetExclusiveACL(ACLEntries, 1, &hHook2);
	LhSetExclusiveACL(ACLEntries, 1, &hHook3);
	FreeLibrary(winspl);
	return;
}
