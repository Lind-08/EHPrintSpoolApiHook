#pragma once

#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#include <windows.h>
#include <easyhook.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <winspool.h>
#include <fstream>
#include <chrono>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <sstream>
#include <locale>
#include <codecvt>
#pragma comment(lib, "Ws2_32.lib")
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo);
