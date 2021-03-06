// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#ifndef _WIN32_WINNT		// 允许使用特定于 Windows XP 或更高版本的功能。
#define _WIN32_WINNT 0x0501	// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif						

#include <stdio.h>
#include <tchar.h>
#include <iostream>

#include <WinSock2.h>
#pragma comment( lib, "ws2_32.lib" )

#define SERVER_PORT		20000
#define SERVER_IP		127.0.0.1
#define SEND_STRING		"服务器端回执,收到消息后您即将被服务器断开!"
#define BUFFER_SIZE		255 

class WinSocketSystem
{
public:
	WinSocketSystem()
	{
		int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
		if (iResult != NO_ERROR)
		{
			exit(-1);
		}
	}

	~WinSocketSystem()
	{
		WSACleanup();
	}

protected:
	WSADATA wsaData;
};

static WinSocketSystem g_winsocketsystem;