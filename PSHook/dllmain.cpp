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
	outStream << std::put_time<wchar_t>(ti, L"%c") << " " << message << L"\n PID: " << GetCurrentProcessId() << L"\n EXE: " << processPath << std::endl;
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

std::wstring AnalyzeAndPrintDesiredAccessFlags(DWORD Flags)
{
	std::wostringstream outStream;
	if ((Flags & PRINTER_ACCESS_ADMINISTER) != 0)
	outStream << " PRINTER_ACCESS_ADMINISTER";
	if ((Flags & PRINTER_ACCESS_USE) != 0)
		outStream << " PRINTER_ACCESS_USE";
	#if (NTDDI_VERSION >= NTDDI_WINBLUE)
	if ((Flags & PRINTER_ACCESS_MANAGE_LIMITED) != 0)
		outStream << " PRINTER_ACCESS_MANAGE_LIMITED";
	#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
	if ((Flags & PRINTER_ALL_ACCESS) != 0)
		outStream << " PRINTER_ALL_ACCESS";
	if ((Flags & DELETE) != 0)
		outStream << " DELETE";
	if ((Flags & READ_CONTROL) != 0)
		outStream << " READ_CONTROL";
	if ((Flags & SYNCHRONIZE) != 0)
		outStream << " SYNCHRONIZE";
	if ((Flags & WRITE_DAC) != 0)
		outStream << " WRITE_DAC";
	if ((Flags & WRITE_OWNER) != 0)
		outStream << " WRITE_OWNER";
	return outStream.str();
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
		outStream << " [pPrinterName]: NULL\n";
	else
		outStream << " [pPrinterName]: " << "\"" << pPrinterName << "\"" << "\n";
	if (phPrinter == NULL)
		outStream << " [phPrinter] <- NULL\n";
	else
		outStream << " [phPrinter] <- " << (unsigned long) phPrinter << "\n";
	if (pDefault == NULL)
		outStream << " [pDefault]: NULL\n";
	else
	{
		if (pDefault->DesiredAccess != NULL)
		{
			outStream << " [pDefault.DesiredAccess]: ";
			outStream << AnalyzeAndPrintDesiredAccessFlags(pDefault->DesiredAccess) << "\n";
		}
		else 
			outStream << " [pDefault.DesiredAccess]: NULL\n";
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
	outStream << L"OpenPrinterA called.\n";
	auto res = OpenPrinterW(pPrinterName, phPrinter, pDefault);
	auto err = GetLastError();
	if (pPrinterName == NULL)
		outStream << " [pPrinterName]: NULL\n";
	else
		outStream << " [pPrinterName]: " << "\"" << pPrinterName << "\"" << "\n";
	if (phPrinter == NULL)
		outStream << " [phPrinter] <- NULL\n";
	else
		outStream << " [phPrinter] <- " << (unsigned long) phPrinter << "\n";
	if (pDefault == NULL)
		outStream << " [pDefault]: NULL\n";
	else
	{
		if (pDefault->DesiredAccess != NULL)
		{
			outStream << " [pDefault.DesiredAccess]: ";
			outStream << AnalyzeAndPrintDesiredAccessFlags(pDefault->DesiredAccess) << "\n";
		}
		else 
			outStream << " [pDefault.DesiredAccess]: NULL\n";
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
	outStream << L"GetDefaultPrinterW called.\n";
	auto res = GetDefaultPrinterW(pszBuffer, pcchBuffer);
	auto err = GetLastError();
	if (pszBuffer == NULL)
	{
		outStream << L" [pszBuffer]: NULL \n [pcchBuffer] <- " << *pcchBuffer << "\n"; 	
		outStream << " [return] <- FALSE\n [error] " << err << " |";
	}
	else 
	{
		outStream << L" [pszBuffer] <- \"" << pszBuffer << "\"\n [pcchBuffer] <- " << *pcchBuffer << " \n [return] <- TRUE |"; 
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
	outStream << L"GetDefaultPrinterA called.\n";
	auto res = GetDefaultPrinterA(pszBuffer, pcchBuffer);
	auto err = GetLastError();
	if (pszBuffer == NULL)
	{
		outStream << L" [pszBuffer]: NULL \n [pcchBuffer] <- " << *pcchBuffer << "\n"; 	
		outStream << " [return] <- FALSE\n [error] " << err << " |";
	}
	else 
	{
		outStream << L" [pszBuffer] <- \"" << pszBuffer << "\"\n [pcchBuffer] <- " << *pcchBuffer << " \n [return] <- TRUE |"; 
	}
	writeToLog(outStream.str(), sock);
	return res;
}

std::wstring AnalyzeAndPrintEnumPrintersFlags(DWORD Flags)
{
	std::wostringstream outStream;
	if ((Flags & PRINTER_ENUM_LOCAL) != 0)
	outStream << " PRINTER_ENUM_LOCAL";
	if ((Flags & PRINTER_ENUM_NAME) != 0)
		outStream << " PRINTER_ENUM_NAME";
	if ((Flags & PRINTER_ENUM_SHARED) != 0)
		outStream << " PRINTER_ENUM_SHARED";
	if ((Flags & PRINTER_ENUM_CONNECTIONS) != 0)
		outStream << " PRINTER_ENUM_CONNECTIONS";
	if ((Flags & PRINTER_ENUM_NETWORK) != 0)
		outStream << " PRINTER_ENUM_NETWORK";
	if ((Flags & PRINTER_ENUM_REMOTE) != 0)
		outStream << " PRINTER_ENUM_REMOTE";
	#if (NTDDI_VERSION >= NTDDI_WINBLUE)
	if ((Flags & PRINTER_ENUM_CATEGORY_3D) != 0)
		outStream << " PRINTER_ENUM_CATEGORY_3D";
	if ((Flags & PRINTER_ENUM_CATEGORY_ALL) != 0)
		outStream << " PRINTER_ENUM_CATEGORY_ALL";
	#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
	return outStream.str();
}

BOOL WINAPI myEnumPrintersA(
  _In_  DWORD   Flags,
  _In_  LPSTR  Name,
  _In_  DWORD   Level,
  _Out_ LPBYTE  pPrinterEnum,
  _In_  DWORD   cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned)
{
	std::wostringstream outStream;
	outStream << L"EnumPrintersA called.\n";
	auto res = EnumPrintersA(Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
	auto err = GetLastError();
	outStream << " [Flags]: ";
	outStream << AnalyzeAndPrintEnumPrintersFlags(Flags) << "\n";
	outStream << " [Name]: ";
	if (Name == NULL)
		outStream << "NULL\n";
	else
		outStream << Name << "\n";
	outStream << " [Level]: " << Level << "\n";
	outStream << " [pPrinterEnum]: "; 
	if (pPrinterEnum == NULL)
		outStream << "NULL\n";
	else
		outStream << *pPrinterEnum << "\n";
	outStream << " [cbBuf]: " << cbBuf << "\n"; 
	outStream << " [pcbNeeded] <- " << *pcbNeeded << "\n"; 
	outStream << " [pcReturned] <- " << *pcReturned << "\n";
	if (res == NULL)
	{
		outStream << " [return] <- FALSE [error] " << err << " |";
	}
	else 
		outStream << " [return] <- TRUE |";
	writeToLog(outStream.str(), sock);
	return res;
}

BOOL WINAPI myEnumPrintersW(
  _In_  DWORD   Flags,
  _In_  LPTSTR  Name,
  _In_  DWORD   Level,
  _Out_ LPBYTE  pPrinterEnum,
  _In_  DWORD   cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned)
{
	std::wostringstream outStream;
	outStream << L"EnumPrintersW called.\n";
	auto res = EnumPrintersW(Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
	auto err = GetLastError();
	outStream << " [Flags]: ";
	outStream << AnalyzeAndPrintEnumPrintersFlags(Flags) << "\n";
	outStream << " [Name]: ";
	if (Name == NULL)
		outStream << "NULL\n";
	else
		outStream << Name << "\n";
	outStream << " [Level]: " << Level << "\n";
	outStream << " [pPrinterEnum]: "; 
	if (pPrinterEnum == NULL)
		outStream << "NULL\n";
	else
		outStream << *pPrinterEnum << "\n";
	outStream << " [cbBuf]: " << cbBuf << "\n"; 
	outStream << " [pcbNeeded] <- " << *pcbNeeded << "\n"; 
	outStream << " [pcReturned] <- " << *pcReturned << "\n";
	if (res == NULL)
	{
		outStream << " [return] <- FALSE [error] " << err << " |";
	}
	else 
		outStream << " [return] <- TRUE |";
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
	HOOK_TRACE_INFO OpenPrinterAHook = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO OpenPrinterWHook = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO GetDefaultPrinterWHook = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO GetDefaultPrinterAHook = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO EnumPrintersAHook = { NULL }; // keep track of our hook
	HOOK_TRACE_INFO EnumPrintersWHook = { NULL }; // keep track of our hook

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

	std::cout << std::endl << "Win32 EnumPrintersA found at address: " << GetProcAddress(winspl, "EnumPrintersA") << "\n";

	std::cout << std::endl << "Win32 EnumPrintersW found at address: " << GetProcAddress(winspl, "EnumPrintersW") << "\n";

	// Install the hook

	NTSTATUS result = LhInstallHook(
		GetProcAddress(winspl, "OpenPrinterA"),
		myOpenPrinterA,
		NULL,
		&OpenPrinterAHook);
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
		&OpenPrinterWHook);
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
		&GetDefaultPrinterWHook);
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
		&GetDefaultPrinterAHook);
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

	result = LhInstallHook(
		GetProcAddress(winspl, "EnumPrintersA"),
		myEnumPrintersA,
		NULL,
		&EnumPrintersAHook);
	if (FAILED(result))
	{
		std::wstring s(RtlGetLastErrorString());
		std::wcout << "Failed to install hook: ";
		std::wcout << s;
	}
	else 
	{
		std::cout << "Hook 'EnumPrintersA installed successfully." << std::endl;
	}

	result = LhInstallHook(
		GetProcAddress(winspl, "EnumPrintersW"),
		myEnumPrintersW,
		NULL,
		&EnumPrintersWHook);
	if (FAILED(result))
	{
		std::wstring s(RtlGetLastErrorString());
		std::wcout << "Failed to install hook: ";
		std::wcout << s;
	}
	else 
	{
		std::cout << "Hook 'EnumPrintersW installed successfully." << std::endl;
	}


	// If the threadId in the ACL is set to 0,
	// then internally EasyHook uses GetCurrentThreadId()
	ULONG ACLEntries[1] = { 0 };

	// Disable the hook for the provided threadIds, enable for all others
	LhSetExclusiveACL(ACLEntries, 1, &OpenPrinterAHook);
	LhSetExclusiveACL(ACLEntries, 1, &OpenPrinterWHook);
	LhSetExclusiveACL(ACLEntries, 1, &GetDefaultPrinterWHook);
	LhSetExclusiveACL(ACLEntries, 1, &GetDefaultPrinterAHook);
	LhSetExclusiveACL(ACLEntries, 1, &EnumPrintersAHook);
	LhSetExclusiveACL(ACLEntries, 1, &EnumPrintersWHook);
	initWSA(&wsData);
    initSocket(sock);
	connectSocket(sock, 31313);
	FreeLibrary(winspl);
	return;
}
