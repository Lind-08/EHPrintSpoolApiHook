// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "framework.h"

std::wstring hookMessage;
std::wofstream outputFile;
std::wstring processPath;

WSADATA wsData;
SOCKET sock;

std::string WINAPI GetLastErrorAsString(DWORD errorMessageID)
{
    //Get the error message ID, if any.
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }
    
    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}

std::wstring WINAPI GetLastErrorAsStringW(DWORD errorMessageID)
{
    //Get the error message ID, if any.
    if(errorMessageID == 0) {
        return std::wstring(); //No error message has been recorded
    }
    
    LPWSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::wstring message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}

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

void WINAPI writeToLog(std::wstring message, SOCKET &sock)
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
	auto res = OpenPrinterA(pPrinterName, phPrinter, pDefault);
	auto err = GetLastError();
	if (pPrinterName == NULL)
		outStream << " [pPrinterName]: NULL";
	else
		outStream << " [pPrinterName]: " << pPrinterName;
	if (phPrinter == NULL)
		outStream << " [phPrinter] <- NULL";
	else
		outStream << " [phPrinter] <- " << (unsigned long) phPrinter;
	if (pDefault == NULL)
		outStream << " [pDefault]: NULL";
	else
	{
		if (pDefault->DesiredAccess != NULL)
		{
			outStream << " [pDefault.DesiredAccess]: ";
			if ((pDefault->DesiredAccess & PRINTER_ACCESS_ADMINISTER) != 0)
			outStream << " PRINTER_ACCESS_ADMINISTER";
			if ((pDefault->DesiredAccess & PRINTER_ACCESS_USE) != 0)
				outStream << " PRINTER_ACCESS_USE";
			#if (NTDDI_VERSION >= NTDDI_WINBLUE)
			if ((pDefault->DesiredAccess & PRINTER_ACCESS_MANAGE_LIMITED) != 0)
				outStream << " PRINTER_ACCESS_MANAGE_LIMITED";
			#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
			if ((pDefault->DesiredAccess & PRINTER_ALL_ACCESS) != 0)
				outStream << " PRINTER_ALL_ACCESS";
			if ((pDefault->DesiredAccess & DELETE) != 0)
				outStream << " DELETE";
			if ((pDefault->DesiredAccess & READ_CONTROL) != 0)
				outStream << " READ_CONTROL";
			if ((pDefault->DesiredAccess & SYNCHRONIZE) != 0)
				outStream << " SYNCHRONIZE";
			if ((pDefault->DesiredAccess & WRITE_DAC) != 0)
				outStream << " WRITE_DAC";
			if ((pDefault->DesiredAccess & WRITE_OWNER) != 0)
				outStream << " WRITE_OWNER";
		}
		else 
			outStream << " [pDefault.DesiredAccess]: NULL";
	}
	if (res == FALSE)
		outStream << " [return] <- FALSE [error] " << err << " |";
	else
		outStream << " [return] <- TRUE |";
	writeToLog(outStream.str(), sock);
	return res;
}

BOOL WINAPI myOpenPrinterW(
  _In_  LPTSTR             pPrinterName,
  _Out_ LPHANDLE           phPrinter,
  _In_  LPPRINTER_DEFAULTS pDefault
)
{
	std::wostringstream outStream;
	outStream << L"OpenPrinterA called.";
	auto res = OpenPrinterW(pPrinterName, phPrinter, pDefault);
	auto err = GetLastError();
	if (pPrinterName == NULL)
		outStream << " [pPrinterName]: NULL";
	else
		outStream << " [pPrinterName]: " << pPrinterName;
	if (phPrinter == NULL)
		outStream << " [phPrinter] <- NULL";
	else
		outStream << " [phPrinter] <- " << (unsigned long) phPrinter;
	if (pDefault == NULL)
		outStream << " [pDefault]: NULL";
	else
	{
		if (pDefault->DesiredAccess != NULL)
		{
			outStream << " [pDefault.DesiredAccess]: ";
			if ((pDefault->DesiredAccess & PRINTER_ACCESS_ADMINISTER) != 0)
			outStream << " PRINTER_ACCESS_ADMINISTER";
			if ((pDefault->DesiredAccess & PRINTER_ACCESS_USE) != 0)
				outStream << " PRINTER_ACCESS_USE";
			#if (NTDDI_VERSION >= NTDDI_WINBLUE)
			if ((pDefault->DesiredAccess & PRINTER_ACCESS_MANAGE_LIMITED) != 0)
				outStream << " PRINTER_ACCESS_MANAGE_LIMITED";
			#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
			if ((pDefault->DesiredAccess & PRINTER_ALL_ACCESS) != 0)
				outStream << " PRINTER_ALL_ACCESS";
			if ((pDefault->DesiredAccess & DELETE) != 0)
				outStream << " DELETE";
			if ((pDefault->DesiredAccess & READ_CONTROL) != 0)
				outStream << " READ_CONTROL";
			if ((pDefault->DesiredAccess & SYNCHRONIZE) != 0)
				outStream << " SYNCHRONIZE";
			if ((pDefault->DesiredAccess & WRITE_DAC) != 0)
				outStream << " WRITE_DAC";
			if ((pDefault->DesiredAccess & WRITE_OWNER) != 0)
				outStream << " WRITE_OWNER";
		}
		else 
			outStream << " [pDefault.DesiredAccess]: NULL";
	}
	if (res == FALSE)
		outStream << " [return] <- FALSE [error] " << err << " |";
	else
		outStream << " [return] <- TRUE |";
	writeToLog(outStream.str(), sock);
	return res;
}


BOOL WINAPI myGetDefaultPrinterW(
  _In_    LPTSTR  pszBuffer,
  _Inout_ LPDWORD pcchBuffer
)
{
	std::wostringstream outStream;
	outStream << L"GetDefaultPrinterW called.";
	auto res = GetDefaultPrinterW(pszBuffer, pcchBuffer);
	auto err = GetLastError();
	if (pszBuffer == NULL)
	{
		outStream << L" [pszBuffer]: NULL [pcchBuffer] <- " << *pcchBuffer; 	
		outStream << " [return] <- FALSE [error] " << err << " |";
	}
	else 
	{
		outStream << L" [pszBuffer] <- \"" << pszBuffer << "\" [pcchBuffer] <- " << *pcchBuffer << " [return] TRUE |"; 
	}
	writeToLog(outStream.str(), sock);
	return res;
}

BOOL WINAPI myGetDefaultPrinterA(
  _In_    LPSTR  pszBuffer,
  _Inout_ LPDWORD pcchBuffer
)
{
	std::wostringstream outStream;
	outStream << L"GetDefaultPrinterA called.";
	auto res = GetDefaultPrinterA(pszBuffer, pcchBuffer);
	auto err = GetLastError();
	if (pszBuffer == NULL)
	{
		outStream << L" [pszBuffer]: NULL [pcchBuffer] <- " << *pcchBuffer; 	
		outStream << " [return] <- FALSE [error] " << err << " |";
	}
	else 
	{
		outStream << L" [pszBuffer] <- \"" << pszBuffer << "\" [pcchBuffer] <- " << *pcchBuffer << " [return] TRUE |"; 
	}
	writeToLog(outStream.str(), sock);
	return res;
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
