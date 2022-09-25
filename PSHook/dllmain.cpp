// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "framework.h"

std::wstring hookMessage;
std::wofstream outputFile;
std::wstring processPath;

WSADATA wsData;
SOCKET sock;

bool initWSA(WSADATA *data)
{
	auto err = WSAStartup(MAKEWORD(2,2), data);
	if (err != 0)
	{ 
		std::cerr << "Error WinSock version initializaion #";
		std::cerr << WSAGetLastError();
		return false;
	}
	else
		return true;
}

bool initSocket(SOCKET &sock)
{
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET) {
		std::cerr << "Error initialization socket # " << WSAGetLastError() << std::endl; 
		closesocket(sock);
		return false;
	}
	else
	{
		std::cerr << "Server socket initialization is OK" << std::endl;
		return true;
	}
}

bool connectSocket (SOCKET &sock, DWORD port)
{
	sockaddr_in servInfo;
	ZeroMemory(&servInfo, sizeof(servInfo));	
				
	servInfo.sin_family = AF_INET;
#pragma warning(suppress : 4996)	
	servInfo.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	servInfo.sin_port = htons(port);

	auto err = connect(sock, (sockaddr*)&servInfo, sizeof(servInfo));
	if ( err != 0 ) {
		std::cerr << "Connection to Server is FAILED. Error # " << WSAGetLastError() << std::endl;
		return false;
	}
	else 
	{
		std::cerr << "Connection established SUCCESSFULLY. Ready to send a message to Server" << std::endl;
		return true;
	}
}

void writeToLog(std::wstring message, SOCKET &sock)
{
	std::wstring outMessage;
	std::wostringstream outStream;
	auto tt = time(nullptr);
#pragma warning(suppress : 4996)	
	auto* ti = localtime(&tt);
	outStream << std::put_time<wchar_t>(ti, L"%c") << " " << message << L" PID: " << GetCurrentProcessId() << L" EXE: " << processPath << std::endl;
	outMessage = outStream.str();
	if (sock != INVALID_SOCKET)
	{
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;
		std::string converted_str = converter.to_bytes( outMessage );
		auto bytes = converted_str.c_str();
		int length = converted_str.length();
		int sended = 0;
		do
		{
			sended += send(sock, bytes + sended, length - sended,0);
		}while (sended != length);
	}
}

BOOL WINAPI myOpenPrinterA(
  _In_  LPSTR             pPrinterName,
  _Out_ LPHANDLE           phPrinter,
  _In_  LPPRINTER_DEFAULTSA pDefault
)
{
	std::wostringstream outStream;
	outStream << L"OpenPrinterA called.";
	writeToLog(outStream.str(), sock);
	return OpenPrinterA(pPrinterName, phPrinter, pDefault);
}

BOOL WINAPI myOpenPrinterW(
  _In_  LPTSTR             pPrinterName,
  _Out_ LPHANDLE           phPrinter,
  _In_  LPPRINTER_DEFAULTS pDefault
)
{
	std::wostringstream outStream;
	outStream << L"OpenPrinterA called.";
	writeToLog(outStream.str(), sock);
	return OpenPrinterW(pPrinterName, phPrinter, pDefault);
}


BOOL WINAPI myGetDefaultPrinterW(
  _In_    LPTSTR  pszBuffer,
  _Inout_ LPDWORD pcchBuffer
)
{
	std::wostringstream outStream;
	outStream << L"GetDefaultPrinterW called.";
	writeToLog(outStream.str(), sock);
	return GetDefaultPrinterW(pszBuffer, pcchBuffer);
}

BOOL WINAPI myGetDefaultPrinterA(
  _In_    LPSTR  pszBuffer,
  _Inout_ LPDWORD pcchBuffer
)
{
	std::wostringstream outStream;
	outStream << L"GetDefaultPrinterA called.";
	writeToLog(outStream.str(), sock);
	return GetDefaultPrinterA(pszBuffer, pcchBuffer);
}

void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	std::cout << "\n\nNativeInjectionEntryPointt(REMOTE_ENTRY_INFO* inRemoteInfo)\n\n" <<
		"IIIII           jjj               tt                dd !!! \n"
		" III  nn nnn          eee    cccc tt      eee       dd !!! \n"
		" III  nnn  nn   jjj ee   e cc     tttt  ee   e  dddddd !!! \n"
		" III  nn   nn   jjj eeeee  cc     tt    eeeee  dd   dd     \n"
		"IIIII nn   nn   jjj  eeeee  ccccc  tttt  eeeee  dddddd !!! \n"
		"              jjjj                                         \n\n";

	std::cout << "Injected by process Id: " << inRemoteInfo->HostPID << std::endl;

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
	initWSA(&wsData);
    initSocket(sock);
	connectSocket(sock, 31313);
	FreeLibrary(winspl);
	return;
}
