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

bool connectSocket (SOCKET &sock, u_short port)
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
		outStream << " [phPrinter] <- " << phPrinter << "\n";
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
		outStream << " [phPrinter] <- " << phPrinter << "\n";
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



std::wstring AnalyzeAndPrintPrinterInfo1Flags(DWORD Flags)
{
	std::wostringstream outStream;
	if ((Flags & PRINTER_ENUM_EXPAND) != 0)
		outStream << " PRINTER_ENUM_EXPAND";
	if ((Flags & PRINTER_ENUM_CONTAINER) != 0)
		outStream << " PRINTER_ENUM_CONTAINER";
	if ((Flags & PRINTER_ENUM_ICON1) != 0)
		outStream << " PRINTER_ENUM_ICON1";
	if ((Flags & PRINTER_ENUM_ICON2) != 0)
		outStream << " PRINTER_ENUM_ICON2";
	if ((Flags & PRINTER_ENUM_ICON3) != 0)
		outStream << " PRINTER_ENUM_ICON3";
	if ((Flags & PRINTER_ENUM_ICON4) != 0)
		outStream << " PRINTER_ENUM_ICON4";
	if ((Flags & PRINTER_ENUM_ICON5) != 0)
		outStream << " PRINTER_ENUM_ICON5";
	if ((Flags & PRINTER_ENUM_ICON6) != 0)
		outStream << " PRINTER_ENUM_ICON6";
	if ((Flags & PRINTER_ENUM_ICON7) != 0)
		outStream << " PRINTER_ENUM_ICON7";
	if ((Flags & PRINTER_ENUM_ICON8) != 0)
		outStream << " PRINTER_ENUM_ICON8";
	return outStream.str();
}

std::wstring AnalyzeAndPrintPrinterInfo4Attributes(DWORD Attributes)
{
	std::wostringstream outStream;
	if ((Attributes & PRINTER_ATTRIBUTE_LOCAL) != 0)
		outStream << " PRINTER_ATTRIBUTE_LOCAL";
	if ((Attributes & PRINTER_ATTRIBUTE_NETWORK) != 0)
		outStream << " PRINTER_ATTRIBUTE_NETWORK";
	return outStream.str();
}

std::wstring AnalyzeAndPrintPrinterInfo2_5Attributes(DWORD Attributes)
{
	std::wostringstream outStream;
	if ((Attributes & PRINTER_ATTRIBUTE_DIRECT) != 0)
		outStream << " PRINTER_ATTRIBUTE_DIRECT";
	if ((Attributes & PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST) != 0)
		outStream << " PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST";
	if ((Attributes & PRINTER_ATTRIBUTE_ENABLE_DEVQ) != 0)
		outStream << " PRINTER_ATTRIBUTE_ENABLE_DEVQ";
	if ((Attributes & PRINTER_ATTRIBUTE_HIDDEN) != 0)
		outStream << " PRINTER_ATTRIBUTE_HIDDEN";
	if ((Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS) != 0)
		outStream << " PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS";
	if ((Attributes & PRINTER_ATTRIBUTE_LOCAL) != 0)
		outStream << " PRINTER_ATTRIBUTE_NETWORK";
	if ((Attributes & PRINTER_ATTRIBUTE_PUBLISHED) != 0)
		outStream << " PRINTER_ATTRIBUTE_PUBLISHED";
	if ((Attributes & PRINTER_ATTRIBUTE_QUEUED) != 0)
		outStream << " PRINTER_ATTRIBUTE_QUEUED";
	if ((Attributes & PRINTER_ATTRIBUTE_RAW_ONLY) != 0)
		outStream << " PRINTER_ATTRIBUTE_RAW_ONLY";
	if ((Attributes & PRINTER_ATTRIBUTE_SHARED) != 0)
		outStream << " PRINTER_ATTRIBUTE_SHARED";
	#if (NTDDI_VERSION >= NTDDI_WINXP)
	if ((Attributes & PRINTER_ATTRIBUTE_FAX) != 0)
		outStream << " PRINTER_ATTRIBUTE_FAX";
	#endif // (NTDDI_VERSION >= NTDDI_WINXP)
	#if (NTDDI_VERSION >= NTDDI_VISTA)
	if ((Attributes & PRINTER_ATTRIBUTE_FRIENDLY_NAME) != 0)
		outStream << " PRINTER_ATTRIBUTE_FRIENDLY_NAME";
	if ((Attributes & PRINTER_ATTRIBUTE_PUSHED_MACHINE) != 0)
		outStream << " PRINTER_ATTRIBUTE_PUSHED_MACHINE";
	if ((Attributes & PRINTER_ATTRIBUTE_MACHINE) != 0)
		outStream << " PRINTER_ATTRIBUTE_MACHINE";
	if ((Attributes & PRINTER_ATTRIBUTE_FRIENDLY_NAME) != 0)
		outStream << " PRINTER_ATTRIBUTE_FRIENDLY_NAME";
	if ((Attributes & PRINTER_ATTRIBUTE_TS_GENERIC_DRIVER) != 0)
		outStream << " PRINTER_ATTRIBUTE_TS_GENERIC_DRIVER";
	#endif // (NTDDI_VERSION >= NTDDI_VISTA)
	return outStream.str();
}

std::wstring AnalyzeAndPrintPrinterInfo2Status(DWORD Status)
{
	std::wostringstream outStream;
	if ((Status & PRINTER_STATUS_BUSY) != 0)
		outStream << " PRINTER_STATUS_BUSY";
	if ((Status & PRINTER_STATUS_DOOR_OPEN) != 0)
		outStream << " PRINTER_STATUS_DOOR_OPEN";
	if ((Status & PRINTER_STATUS_ERROR) != 0)
		outStream << " PRINTER_STATUS_ERROR";
	if ((Status & PRINTER_STATUS_INITIALIZING) != 0)
		outStream << " PRINTER_STATUS_INITIALIZING";
	if ((Status & PRINTER_STATUS_IO_ACTIVE) != 0)
		outStream << " PRINTER_STATUS_IO_ACTIVE";
	if ((Status & PRINTER_STATUS_MANUAL_FEED) != 0)
		outStream << " PRINTER_STATUS_MANUAL_FEED";
	if ((Status & PRINTER_STATUS_NO_TONER) != 0)
		outStream << " PRINTER_STATUS_NO_TONER";
	if ((Status & PRINTER_STATUS_NOT_AVAILABLE) != 0)
		outStream << " PRINTER_STATUS_NOT_AVAILABLE";
	if ((Status & PRINTER_STATUS_OFFLINE) != 0)
		outStream << " PRINTER_STATUS_OFFLINE";
	if ((Status & PRINTER_STATUS_OUT_OF_MEMORY) != 0)
		outStream << " PRINTER_STATUS_OUT_OF_MEMORY";
	if ((Status & PRINTER_STATUS_OUTPUT_BIN_FULL) != 0)
		outStream << " PRINTER_STATUS_OUTPUT_BIN_FULL";
	if ((Status & PRINTER_STATUS_PAGE_PUNT) != 0)
		outStream << " PRINTER_STATUS_PAGE_PUNT";
	if ((Status & PRINTER_STATUS_PAPER_JAM) != 0)
		outStream << " PRINTER_STATUS_PAPER_JAM";
	if ((Status & PRINTER_STATUS_PAPER_OUT) != 0)
		outStream << " PRINTER_STATUS_PAPER_OUT";
	if ((Status & PRINTER_STATUS_PAPER_PROBLEM) != 0)
		outStream << " PRINTER_STATUS_PAPER_PROBLEM";
	if ((Status & PRINTER_STATUS_PAUSED) != 0)
		outStream << " PRINTER_STATUS_PAUSED";
	if ((Status & PRINTER_STATUS_PENDING_DELETION) != 0)
		outStream << " PRINTER_STATUS_PENDING_DELETION";
	if ((Status & PRINTER_STATUS_POWER_SAVE) != 0)
		outStream << " PRINTER_STATUS_POWER_SAVE";
	if ((Status & PRINTER_STATUS_PRINTING) != 0)
		outStream << " PRINTER_STATUS_PRINTING";
	if ((Status & PRINTER_STATUS_PROCESSING) != 0)
		outStream << " PRINTER_STATUS_PROCESSING";
	if ((Status & PRINTER_STATUS_SERVER_UNKNOWN) != 0)
		outStream << " PRINTER_STATUS_SERVER_UNKNOWN";
	if ((Status & PRINTER_STATUS_TONER_LOW) != 0)
		outStream << " PRINTER_STATUS_TONER_LOW";
	if ((Status & PRINTER_STATUS_USER_INTERVENTION) != 0)
		outStream << " PRINTER_STATUS_USER_INTERVENTION";
	if ((Status & PRINTER_STATUS_USER_INTERVENTION) != 0)
		outStream << " PRINTER_STATUS_USER_INTERVENTION";
	if ((Status & PRINTER_STATUS_WAITING) != 0)
		outStream << " PRINTER_STATUS_WAITING";
	return outStream.str();
}

std::wstring ParcePrinterInfo1W(PRINTER_INFO_1W* pPrinterEnum)
{
	if (pPrinterEnum == nullptr)
		return std::wstring();
	std::wstringstream outStream;
	outStream << " [Flags]: " << AnalyzeAndPrintPrinterInfo1Flags(pPrinterEnum->Flags) << "\n";
	outStream << " [pDescription]: " << (pPrinterEnum->pDescription == NULL ? L"NULL" : pPrinterEnum->pDescription)<< "\n";
	outStream << " [pName]: " << (pPrinterEnum->pName == NULL ? L"NULL" : pPrinterEnum->pName)<< "\n";
	outStream << " [pComment]: " << (pPrinterEnum->pComment == NULL ? L"NULL" : pPrinterEnum->pComment)<< "\n";
	return outStream.str();
}

std::wstring ParcePrinterInfo2W(PRINTER_INFO_2W* pPrinterEnum)
{
	if (pPrinterEnum == nullptr)
		return std::wstring();
	std::wstringstream outStream;
	outStream << " [pServerName]: " << (pPrinterEnum->pServerName == NULL ? L"NULL" : pPrinterEnum->pServerName)<< "\n";
	outStream << " [pPrinterName]: " << (pPrinterEnum->pPrinterName == NULL ? L"NULL" : pPrinterEnum->pPrinterName)<< "\n";
	outStream << " [pShareName]: " << (pPrinterEnum->pShareName == NULL ? L"NULL" : pPrinterEnum->pShareName)<< "\n";
	outStream << " [pPortName]: " << (pPrinterEnum->pPortName == NULL ? L"NULL" : pPrinterEnum->pPortName)<< "\n";
	outStream << " [pDriverName]: " << (pPrinterEnum->pDriverName == NULL ? L"NULL" : pPrinterEnum->pDriverName)<< "\n";
	outStream << " [pComment]: " << (pPrinterEnum->pComment == NULL ? L"NULL" : pPrinterEnum->pComment)<< "\n";
	outStream << " [pLocation]: " << (pPrinterEnum->pLocation == NULL ? L"NULL" : pPrinterEnum->pLocation)<< "\n";
	outStream << " [pSepFile]: " << (pPrinterEnum->pSepFile == NULL ? L"NULL" : pPrinterEnum->pSepFile)<< "\n";
	outStream << " [pPrintProcessor]: " << (pPrinterEnum->pPrintProcessor == NULL ? L"NULL" : pPrinterEnum->pPrintProcessor)<< "\n";
	outStream << " [pDatatype]: " << (pPrinterEnum->pDatatype == NULL ? L"NULL" : pPrinterEnum->pDatatype)<< "\n";
	outStream << " [pParameters]: " << (pPrinterEnum->pParameters == NULL ? L"NULL" : pPrinterEnum->pParameters)<< "\n";
	outStream << " [Attributes]: " << AnalyzeAndPrintPrinterInfo2_5Attributes(pPrinterEnum->Attributes) << "\n";
	outStream << " [Priority]: " << pPrinterEnum->Priority << "\n";
	outStream << " [DefaultPriority]: " << pPrinterEnum->DefaultPriority << "\n";
	outStream << " [StartTime]: " << pPrinterEnum->StartTime << "\n";
	outStream << " [UntilTime]: " << pPrinterEnum->UntilTime << "\n";
	outStream << " [Status]: " << AnalyzeAndPrintPrinterInfo2Status(pPrinterEnum->Status) << "\n";
	outStream << " [cJobs]: " << pPrinterEnum->cJobs << "\n";
	outStream << " [AveragePPM]: " << pPrinterEnum->AveragePPM << "\n";
	return outStream.str();
}

std::wstring ParcePrinterInfo4W(PRINTER_INFO_4W* pPrinterEnum)
{
	if (pPrinterEnum == nullptr)
		return std::wstring();
	std::wstringstream outStream;
	outStream << " [pPrinterName]: " << (pPrinterEnum->pPrinterName == NULL ? L"NULL" : pPrinterEnum->pPrinterName)<< "\n";
	outStream << " [pServerName]: " << (pPrinterEnum->pServerName == NULL ? L"NULL" : pPrinterEnum->pServerName)<< "\n";
	outStream << " [Attributes]: " << AnalyzeAndPrintPrinterInfo4Attributes(pPrinterEnum->Attributes) << "\n";
	return outStream.str();
}

std::wstring ParcePrinterInfo5W(PRINTER_INFO_5W* pPrinterEnum)
{
	if (pPrinterEnum == nullptr)
		return std::wstring();
	std::wstringstream outStream;
	outStream << " [pPrinterName]: " << (pPrinterEnum->pPrinterName == NULL ? L"NULL" : pPrinterEnum->pPrinterName)<< "\n";
	outStream << " [pPortName]: " << (pPrinterEnum->pPortName == NULL ? L"NULL" : pPrinterEnum->pPortName)<< "\n";
	outStream << " [Attributes]: " << AnalyzeAndPrintPrinterInfo2_5Attributes(pPrinterEnum->Attributes) << "\n";
	return outStream.str();
}

std::wstring ParcePrinterEnum(PRINTER_INFO_1W* pPrinterEnum, DWORD length)
{
	std::wostringstream outStream;
	for (size_t i = 0; i < length; i++)
	{
		outStream << "PRINTER_INFO_1W " << i + 1 << "\n";
		outStream << ParcePrinterInfo1W(pPrinterEnum + i);
	}
	return outStream.str();
}

std::wstring ParcePrinterEnum(PRINTER_INFO_2W* pPrinterEnum, DWORD length)
{
	std::wostringstream outStream;
	for (size_t i = 0; i < length; i++)
	{
		outStream << "PRINTER_INFO_2W " << i + 1 << "\n";
		outStream << ParcePrinterInfo2W(pPrinterEnum + i);
	}
	return outStream.str();
}

std::wstring ParcePrinterEnum(PRINTER_INFO_3* pPrinterEnum, DWORD length)
{
	return L"";
}

std::wstring ParcePrinterEnum(PRINTER_INFO_4W* pPrinterEnum, DWORD length)
{
	std::wostringstream outStream;
	for (size_t i = 0; i < length; i++)
	{
		outStream << "PRINTER_INFO_4W " << i + 1 << "\n";
		outStream << ParcePrinterInfo4W(pPrinterEnum + i);
	}
	return outStream.str();
}

std::wstring ParcePrinterEnum(PRINTER_INFO_5W* pPrinterEnum, DWORD length)
{
	std::wostringstream outStream;
	for (size_t i = 0; i < length; i++)
	{
		outStream << "PRINTER_INFO_5W " << i + 1 << "\n";
		outStream << ParcePrinterInfo5W(pPrinterEnum + i);
	}
	return outStream.str();
}

std::wstring GetPrinterEnumOutA(LPBYTE pPrinterEnum, DWORD level, DWORD length)
{
	return L"";
}

std::wstring GetPrinterEnumOutW(LPBYTE pPrinterEnum, DWORD level, DWORD length)
{
	std::wstring result;
	switch (level)
	{
	case 1:
	{
		auto printers = reinterpret_cast<PRINTER_INFO_1W*>(pPrinterEnum);
		result = ParcePrinterEnum(printers, length);
	}break;
	case 2:
	{
		auto printers = reinterpret_cast<PRINTER_INFO_2W*>(pPrinterEnum);
		result = ParcePrinterEnum(printers, length);
	}break;
	case 3:
	{
		auto printers = reinterpret_cast<PRINTER_INFO_3*>(pPrinterEnum);
		result = ParcePrinterEnum(printers, length);
	}break;
	case 4:
	{
		auto printers = reinterpret_cast<PRINTER_INFO_4W*>(pPrinterEnum);
		result = ParcePrinterEnum(printers, length);
	}break;
	case 5:
	{
		auto printers = reinterpret_cast<PRINTER_INFO_5W*>(pPrinterEnum);
		result = ParcePrinterEnum(printers, length);
	}break;
	
	default:
		break;
	}
	return result;
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
	{
		std::wstring printerEnumOut = GetPrinterEnumOutA(pPrinterEnum, Level, *pcReturned);
		outStream << "\n" << printerEnumOut << "\n";
	}
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
	{
		std::wstring printerEnumOut = GetPrinterEnumOutW(pPrinterEnum, Level, *pcReturned);
		outStream << "\n" << printerEnumOut << "\n";
	}
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
