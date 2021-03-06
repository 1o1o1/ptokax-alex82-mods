/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2015  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "HubCommands.h"
#include "IP2Country.h"
#include "LuaScript.h"
#include "RegThread.h"
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextFileManager.h"
//#include "TLSManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindow.h"
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
	#include "DB-SQLite.h"
#elif _WITH_POSTGRES
	#include "DB-PostgreSQL.h"
#elif _WITH_MYSQL
	#include "DB-MySQL.h"
#endif
//---------------------------------------------------------------------------
static ServerThread * pServersE = NULL;
//---------------------------------------------------------------------------

#ifdef _WIN32
	#ifndef _WIN_IOT
	    UINT_PTR clsServerManager::sectimer = 0;
	    UINT_PTR clsServerManager::regtimer = 0;
    #endif

	HANDLE clsServerManager::hConsole = NULL, clsServerManager::hLuaHeap = NULL, clsServerManager::hPtokaXHeap = NULL, clsServerManager::hRecvHeap = NULL, clsServerManager::hSendHeap = NULL;
	string clsServerManager::sLuaPath = "", clsServerManager::sOS = "";
#endif

#ifdef __MACH__
	clock_serv_t clsServerManager::csMachClock;
#endif

string clsServerManager::sPath = "", clsServerManager::sScriptPath = "";
size_t clsServerManager::szGlobalBufferSize = 0;
char * clsServerManager::pGlobalBuffer = NULL;
bool clsServerManager::bCmdAutoStart = false, clsServerManager::bCmdNoAutoStart = false, clsServerManager::bCmdNoTray = false, clsServerManager::bUseIPv4 = true,
    clsServerManager::bUseIPv6 = true, clsServerManager::bIPv6DualStack = false;

double clsServerManager::daCpuUsage[60];
double clsServerManager::dCpuUsage = 0;

uint64_t clsServerManager::ui64ActualTick = 0, clsServerManager::ui64TotalShare = 0;
uint64_t clsServerManager::ui64BytesRead = 0, clsServerManager::ui64BytesSent = 0, clsServerManager::ui64BytesSentSaved = 0;
uint64_t clsServerManager::ui64LastBytesRead = 0, clsServerManager::ui64LastBytesSent = 0;
uint64_t clsServerManager::ui64Mins = 0, clsServerManager::ui64Hours = 0, clsServerManager::ui64Days = 0;

#ifndef _WIN32
	uint32_t clsServerManager::ui32CpuCount = 0;
#endif

uint32_t clsServerManager::ui32aUploadSpeed[60], clsServerManager::ui32aDownloadSpeed[60];
uint32_t clsServerManager::ui32Joins = 0, clsServerManager::ui32Parts = 0, clsServerManager::ui32Logged = 0, clsServerManager::ui32Peak = 0;
uint32_t clsServerManager::ui32ActualBytesRead = 0, clsServerManager::ui32ActualBytesSent = 0;
uint32_t clsServerManager::ui32AverageBytesRead = 0, clsServerManager::ui32AverageBytesSent = 0;

ServerThread * clsServerManager::pServersS = NULL;

time_t clsServerManager::tStartTime = 0;

bool clsServerManager::bServerRunning = false, clsServerManager::bServerTerminated = false, clsServerManager::bIsRestart = false, clsServerManager::bIsClose = false;

#ifdef _WIN32
	#ifndef _BUILD_GUI
	    bool clsServerManager::bService = false;
	#else
        HINSTANCE clsServerManager::hInstance;
        HWND clsServerManager::hWndActiveDialog;
	#endif
#else
	bool clsServerManager::bDaemon = false;
#endif

char clsServerManager::sHubIP[16], clsServerManager::sHubIP6[40];

uint8_t clsServerManager::ui8SrCntr = 0, clsServerManager::ui8MinTick = 0;
//---------------------------------------------------------------------------

void clsServerManager::OnSecTimer() {
#ifdef _WIN32
	FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
	GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
	int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
	int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);
	double dcpuSec = double(kernelTime + userTime) / double(10000000I64);
	dCpuUsage = dcpuSec - daCpuUsage[ui8MinTick];
	daCpuUsage[ui8MinTick] = dcpuSec;
#else
    struct rusage rs;

    getrusage(RUSAGE_SELF, &rs);

	double dcpuSec = double(rs.ru_utime.tv_sec) + (double(rs.ru_utime.tv_usec)/1000000) + 
	double(rs.ru_stime.tv_sec) + (double(rs.ru_stime.tv_usec)/1000000);
	clsServerManager::dCpuUsage = dcpuSec - clsServerManager::daCpuUsage[clsServerManager::ui8MinTick];
	clsServerManager::daCpuUsage[clsServerManager::ui8MinTick] = dcpuSec;
#endif

	if(++clsServerManager::ui8MinTick == 60) {
		clsServerManager::ui8MinTick = 0;
    }

#ifdef _WIN32
	if(clsServerManager::bServerRunning == false) {
		return;
	}
#endif

    clsServerManager::ui64ActualTick++;

	clsServerManager::ui32ActualBytesRead = (uint32_t)(clsServerManager::ui64BytesRead - clsServerManager::ui64LastBytesRead);
	clsServerManager::ui32ActualBytesSent = (uint32_t)(clsServerManager::ui64BytesSent - clsServerManager::ui64LastBytesSent);
	clsServerManager::ui64LastBytesRead = clsServerManager::ui64BytesRead;
	clsServerManager::ui64LastBytesSent = clsServerManager::ui64BytesSent;

	clsServerManager::ui32AverageBytesSent -= clsServerManager::ui32aUploadSpeed[clsServerManager::ui8MinTick];
	clsServerManager::ui32AverageBytesRead -= clsServerManager::ui32aDownloadSpeed[clsServerManager::ui8MinTick];

	clsServerManager::ui32aUploadSpeed[clsServerManager::ui8MinTick] = clsServerManager::ui32ActualBytesSent;
	clsServerManager::ui32aDownloadSpeed[clsServerManager::ui8MinTick] = clsServerManager::ui32ActualBytesRead;

	clsServerManager::ui32AverageBytesSent += clsServerManager::ui32aUploadSpeed[clsServerManager::ui8MinTick];
	clsServerManager::ui32AverageBytesRead += clsServerManager::ui32aDownloadSpeed[clsServerManager::ui8MinTick];

#ifdef _BUILD_GUI
    clsMainWindow::mPtr->UpdateStats();
    clsMainWindowPageScripts::mPtr->UpdateMemUsage();
#endif
}
//---------------------------------------------------------------------------

void clsServerManager::OnRegTimer() {
	if(clsSettingManager::mPtr->bBools[SETBOOL_AUTO_REG] == true && clsSettingManager::mPtr->sTexts[SETTXT_REGISTER_SERVERS] != NULL) {
		// First destroy old hublist reg thread if any
	    if(clsRegisterThread::mPtr != NULL) {
	        clsRegisterThread::mPtr->Close();
	        clsRegisterThread::mPtr->WaitFor();
	        delete clsRegisterThread::mPtr;
	        clsRegisterThread::mPtr = NULL;
	    }
	        
	    // Create hublist reg thread
	    clsRegisterThread::mPtr = new (std::nothrow) clsRegisterThread();
	    if(clsRegisterThread::mPtr == NULL) {
	        AppendDebugLog("%s - [MEM] Cannot allocate clsRegisterThread::mPtr in ServerOnRegTimer\n");
	        return;
	    }
	        
	    // Setup hublist reg thread
	    clsRegisterThread::mPtr->Setup(clsSettingManager::mPtr->sTexts[SETTXT_REGISTER_SERVERS], clsSettingManager::mPtr->ui16TextsLens[SETTXT_REGISTER_SERVERS]);
	        
	    // Start the hublist reg thread
	    clsRegisterThread::mPtr->Resume();
	}
}
//---------------------------------------------------------------------------

void clsServerManager::Initialize() {
	// alex82 ... ��������� ���������������� TinyXML, ���������� ��������� �������� � �������
	TiXmlBase::SetCondenseWhiteSpace( false );

	time_t acctime;
	time(&acctime);
#ifdef _WIN32
	srand((uint32_t)acctime);

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

    hPtokaXHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x100000, 0);

	if(DirExist((clsServerManager::sPath+"\\cfg").c_str()) == false) {
		CreateDirectory((clsServerManager::sPath+"\\cfg").c_str(), NULL);
	}
	if(DirExist((clsServerManager::sPath+"\\logs").c_str()) == false) {
		CreateDirectory((clsServerManager::sPath+"\\logs").c_str(), NULL);
	}
	if(DirExist((clsServerManager::sPath+"\\scripts").c_str()) == false) {
		CreateDirectory((clsServerManager::sPath+"\\scripts").c_str(), NULL);
    }
	if(DirExist((clsServerManager::sPath+"\\texts").c_str()) == false) {
		CreateDirectory((clsServerManager::sPath+"\\texts").c_str(), NULL);
    }

	clsServerManager::sScriptPath = clsServerManager::sPath + "\\scripts\\";

	clsServerManager::sLuaPath = clsServerManager::sPath + "/";

	char * sTempLuaPath = clsServerManager::sLuaPath.c_str();
	for(size_t szi = 0; szi < clsServerManager::sPath.size(); szi++) {
		if(sTempLuaPath[szi] == '\\') {
			sTempLuaPath[szi] = '/';
		}
	}

    SetupOsVersion();
#else
    srandom(acctime);

	if(DirExist((clsServerManager::sPath+"/logs").c_str()) == false) {
		if(mkdir((clsServerManager::sPath+"/logs").c_str(), 0755) == -1) {
            if(bDaemon == true) {
                syslog(LOG_USER | LOG_ERR, "Creating  of logs directory failed!\n");
            } else {
                printf("Creating  of logs directory failed!");
            }
        }
	}
	if(DirExist((clsServerManager::sPath+"/cfg").c_str()) == false) {
		if(mkdir((clsServerManager::sPath+"/cfg").c_str(), 0755) == -1) {
            AppendLog("Creating of cfg directory failed!");
        }
	}
	if(DirExist((clsServerManager::sPath+"/scripts").c_str()) == false) {
		if(mkdir((clsServerManager::sPath+"/scripts").c_str(), 0755) == -1) {
            AppendLog("Creating of scripts directory failed!");
        }
    }
	if(DirExist((clsServerManager::sPath+"/texts").c_str()) == false) {
		if(mkdir((clsServerManager::sPath+"/texts").c_str(), 0755) == -1) {
            AppendLog("Creating of texts directory failed!");
        }
    }

	clsServerManager::sScriptPath = clsServerManager::sPath + "/scripts/";

    // get cpu count
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if(fp != NULL) {
        char buf[1024];
        while(fgets(buf, 1024, fp) != NULL) {
            if(strncasecmp (buf, "model name", 10) == 0 || strncmp (buf, "Processor", 9) == 0 || strncmp (buf, "cpu model", 9) == 0) {
                ui32CpuCount++;
            }
        }
    
        fclose(fp);
    }

    if(ui32CpuCount == 0) {
        ui32CpuCount = 1;
    }
#endif
    CreateGlobalBuffer();

    CheckForIPv6();

#ifdef __MACH__
	mach_port_t mpMachHost = mach_host_self();
	host_get_clock_service(mpMachHost, SYSTEM_CLOCK, &clsServerManager::csMachClock);
	mach_port_deallocate(mach_task_self(), mpMachHost);
#endif

    ui64ActualTick = ui64TotalShare = 0;
	ui64BytesRead = ui64BytesSent = ui64BytesSentSaved = 0;

	ui32ActualBytesRead = ui32ActualBytesSent = ui32AverageBytesRead = ui32AverageBytesSent = 0;

    ui32Joins = ui32Parts = ui32Logged = ui32Peak = 0;

    pServersS = NULL;
    pServersE = NULL;

    tStartTime = 0;

    ui64Mins = ui64Hours = ui64Days = 0;

    bServerRunning = bIsRestart = bIsClose = false;

    sHubIP[0] = '\0';
    sHubIP6[0] = '\0';

    ui8SrCntr = 0;

    clsZlibUtility::mPtr = new (std::nothrow) clsZlibUtility();
    if(clsZlibUtility::mPtr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate clsZlibUtility::mPtr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

    ui8MinTick = 0;

    ui64LastBytesRead = ui64LastBytesSent = 0;

	for(uint8_t ui8i = 0 ; ui8i < 60; ui8i++) {
		daCpuUsage[ui8i] = 0;
		ui32aUploadSpeed[ui8i] = 0;
		ui32aDownloadSpeed[ui8i] = 0;
	}

	dCpuUsage = 0.0;

	clsSettingManager::mPtr = new (std::nothrow) clsSettingManager();
    if(clsSettingManager::mPtr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate clsSettingManager::mPtr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

	// alex82 ... ������������� ������, ��������� � ����������
#ifdef _WIN32
    setlocale(LC_ALL, clsSettingManager::mPtr->clsSettingManager::mPtr->sTexts[SETTXT_LOCALE_WIN]);
#else
	setlocale(LC_ALL, clsSettingManager::mPtr->clsSettingManager::mPtr->sTexts[SETTXT_LOCALE_NIX]);
#endif
	
	// alex82 ... ������������� ������������� ������ ��� �����
	setlocale(LC_NUMERIC, "C");

	// alex82 ... ����� �������� ������� ��� ����� ������, �������� ����������������� ����� ��������� ����
	clsReservedNicksManager::mPtr = new (std::nothrow) clsReservedNicksManager();
	if(clsReservedNicksManager::mPtr == NULL) {
	    AppendDebugLog("%s - [MEM] Cannot allocate clsReservedNicksManager::mPtr in ServerInitialize\n");
	    exit(EXIT_FAILURE);
	}

	// alex82 ... �������� �������������� � ��������������

    clsLanguageManager::mPtr = new (std::nothrow) clsLanguageManager();
    if(clsLanguageManager::mPtr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate clsLanguageManager::mPtr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    clsLanguageManager::mPtr->Load();

    clsProfileManager::mPtr = new (std::nothrow) clsProfileManager();
    if(clsProfileManager::mPtr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate clsProfileManager::mPtr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

    clsRegManager::mPtr = new (std::nothrow) clsRegManager();
    if(clsRegManager::mPtr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate clsRegManager::mPtr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

    // Load registered users
	clsRegManager::mPtr->Load();

    clsBanManager::mPtr = new (std::nothrow) clsBanManager();
    if(clsBanManager::mPtr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate clsBanManager::mPtr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    // load banlist
	clsBanManager::mPtr->Load();

    clsTextFilesManager::mPtr = new (std::nothrow) clsTextFilesManager();
    if(clsTextFilesManager::mPtr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate clsTextFilesManager::mPtr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    clsUdpDebug::mPtr = new (std::nothrow) clsUdpDebug();
    if(clsUdpDebug::mPtr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate clsUdpDebug::mPtr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    clsScriptManager::mPtr = new (std::nothrow) clsScriptManager();
    if(clsScriptManager::mPtr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate clsScriptManager::mPtr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

#ifdef _BUILD_GUI
    clsMainWindow::mPtr = new (std::nothrow) clsMainWindow();

    if(clsMainWindow::mPtr == NULL || clsMainWindow::mPtr->CreateEx() == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate clsMainWindow::mPtr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }
#endif

	clsSettingManager::mPtr->UpdateAll();

#if defined(_WIN32) && !defined(_WIN_IOT)
    sectimer = SetTimer(NULL, 0, 1000, NULL);

	if(sectimer == 0) {
		AppendDebugLog("%s - [ERR] Cannot startsectimer in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    regtimer = 0;
#endif
}
//---------------------------------------------------------------------------

bool clsServerManager::Start() {
    time(&tStartTime);

    clsSettingManager::mPtr->UpdateAll();

    clsTextFilesManager::mPtr->RefreshTextFiles();

#ifdef _BUILD_GUI
    clsMainWindow::mPtr->EnableStartButton(FALSE);
#endif

    ui64ActualTick = ui64TotalShare = 0;

    ui64BytesRead = ui64BytesSent = ui64BytesSentSaved = 0;

	ui32ActualBytesRead = ui32ActualBytesSent = ui32AverageBytesRead = ui32AverageBytesSent = 0;

    ui32Joins = ui32Parts = ui32Logged = ui32Peak = 0;

    ui64Mins = ui64Hours = ui64Days = 0;

    ui8SrCntr = 0;

    sHubIP[0] = '\0';
    sHubIP6[0] = '\0';

#ifdef _BUILD_GUI
	if(ResolveHubAddress() == false) {
        clsMainWindow::mPtr->EnableStartButton(TRUE);
        return false;
    }
#else
	ResolveHubAddress();
#endif

    for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
        if(clsSettingManager::mPtr->ui16PortNumbers[ui8i] == 0) {
            break;
        }

        if(clsSettingManager::mPtr->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || (bUseIPv6 == true && bIPv6DualStack == false)) {
            if(bUseIPv6 == true) {
                CreateServerThread(AF_INET6, clsSettingManager::mPtr->ui16PortNumbers[ui8i]);
            }

            CreateServerThread(AF_INET, clsSettingManager::mPtr->ui16PortNumbers[ui8i]);
        } else {
            CreateServerThread(bUseIPv6 == true ? AF_INET6 : AF_INET, clsSettingManager::mPtr->ui16PortNumbers[ui8i]);
        }
    }

	if(pServersS == NULL) {
#ifdef _BUILD_GUI
		::MessageBox(clsMainWindow::mPtr->m_hWnd, clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED], clsLanguageManager::mPtr->sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
        clsMainWindow::mPtr->EnableStartButton(TRUE);
#else
		AppendLog(clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED]);
#endif
        return false;
    }

    AppendLog("Serving started");

//  if(tlsenabled == true) {
/*        TLSManager = new (std::nothrow) TLSMan();
        if(TLSManager == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot allocate TLSManager in ServerStart\n", 0);
        	exit(EXIT_FAILURE);
        }*/
//  }

#ifdef _WITH_SQLITE
    DBSQLite::mPtr = new (std::nothrow) DBSQLite();
    if(DBSQLite::mPtr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBSQLite::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }
#elif _WITH_POSTGRES
    DBPostgreSQL::mPtr = new (std::nothrow) DBPostgreSQL();
    if(DBPostgreSQL::mPtr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBPostgreSQL::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }
#elif _WITH_MYSQL
    DBMySQL::mPtr = new (std::nothrow) DBMySQL();
    if(DBMySQL::mPtr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBMySQL::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }
#endif

    clsIpP2Country::mPtr = new (std::nothrow) clsIpP2Country();
    if(clsIpP2Country::mPtr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate clsIpP2Country::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    clsEventQueue::mPtr = new (std::nothrow) clsEventQueue();
    if(clsEventQueue::mPtr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate clsEventQueue::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    clsHashManager::mPtr = new (std::nothrow) clsHashManager();
    if(clsHashManager::mPtr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate clsHashManager::mPtr in ServerStart\n");
        exit(EXIT_FAILURE);
    }

    clsUsers::mPtr = new (std::nothrow) clsUsers();
	if(clsUsers::mPtr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate clsUsers::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    clsGlobalDataQueue::mPtr = new (std::nothrow) clsGlobalDataQueue();
    if(clsGlobalDataQueue::mPtr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate clsGlobalDataQueue::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    clsDcCommands::mPtr = new (std::nothrow) clsDcCommands();
    if(clsDcCommands::mPtr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate clsDcCommands::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    // add botname to reserved nicks
    clsReservedNicksManager::mPtr->AddReservedNick(clsSettingManager::mPtr->sTexts[SETTXT_BOT_NICK]);
    clsSettingManager::mPtr->UpdateBot();

    // add opchat botname to reserved nicks
    clsReservedNicksManager::mPtr->AddReservedNick(clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
    clsSettingManager::mPtr->UpdateOpChat();

    clsReservedNicksManager::mPtr->AddReservedNick(clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);

    if((uint16_t)atoi(clsSettingManager::mPtr->sTexts[SETTXT_UDP_PORT]) != 0) {
        if(clsSettingManager::mPtr->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || (bUseIPv6 == true && bIPv6DualStack == false)) {
            if(bUseIPv6 == true) {
                UDPThread::mPtrIPv6 = UDPThread::Create(AF_INET6);
            }

            UDPThread::mPtrIPv4 = UDPThread::Create(AF_INET);
        } else {
            UDPThread::mPtrIPv6 = UDPThread::Create(bUseIPv6 == true ? AF_INET6 : AF_INET);
        }
    }
    
    if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		clsScriptManager::mPtr->Start();
    }

    clsServiceLoop::mPtr = new (std::nothrow) clsServiceLoop();
    if(clsServiceLoop::mPtr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate clsServiceLoop::mPtr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    // Start the server socket threads
    ServerThread * cur = NULL,
        * next = pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

		cur->Resume();
    }

    bServerRunning = true;

    // Call lua_Main
	clsScriptManager::mPtr->OnStartup();

#ifdef _BUILD_GUI
    clsMainWindow::mPtr->SetStatusValue((string(clsLanguageManager::mPtr->sTexts[LAN_RUNNING], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RUNNING])+"...").c_str());
    clsMainWindow::mPtr->SetStartButtonText(clsLanguageManager::mPtr->sTexts[LAN_STOP_HUB]);
    clsMainWindow::mPtr->EnableStartButton(TRUE);
    clsMainWindow::mPtr->EnableGuiItems(TRUE);
#endif

#if defined(_WIN32) && !defined(_WIN_IOT)
    //Start the HubRegistration timer
    if(clsSettingManager::mPtr->bBools[SETBOOL_AUTO_REG] == true) {
		regtimer = SetTimer(NULL, 0, 901000, NULL);

        if(regtimer == 0) {
			AppendDebugLog("%s - [ERR] Cannot start regtimer in ServerStart\n");
        	exit(EXIT_FAILURE);
        }
    }
#endif

    return true;
}
//---------------------------------------------------------------------------

void clsServerManager::Stop() {
#ifdef _BUILD_GUI
    clsMainWindow::mPtr->EnableStartButton(FALSE);
#endif

    int iret = sprintf(clsServerManager::pGlobalBuffer, "Serving stopped (UL: %" PRIu64 " [%" PRIu64 "], DL: %" PRIu64 ")", ui64BytesSent, ui64BytesSentSaved, ui64BytesRead);
    if(CheckSprintf(iret, clsServerManager::szGlobalBufferSize, "ServerMan::StopServer") == true) {
        AppendLog(clsServerManager::pGlobalBuffer);
    }

#if defined(_WIN32) && !defined(_WIN_IOT)
	//Stop the HubRegistration timer
	if(clsSettingManager::mPtr->bBools[SETBOOL_AUTO_REG] == true) {
        if(KillTimer(NULL, regtimer) == 0) {
    		AppendDebugLog("%s - [ERR] Cannot stop regtimer in ServerStop\n");
        	exit(EXIT_FAILURE);
        }
    }
#endif

    ServerThread * cur = NULL,
        * next = pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

		cur->Close();
		cur->WaitFor();

		delete cur;
    }

    pServersS = NULL;
    pServersE = NULL;

	// stop the main hub loop
	if(clsServiceLoop::mPtr != NULL) {
		bServerTerminated = true;
	} else {
		FinalStop(false);
    }
}
//---------------------------------------------------------------------------

void clsServerManager::FinalStop(const bool &bDeleteServiceLoop) {
    if(bDeleteServiceLoop == true) {
		delete clsServiceLoop::mPtr;
		clsServiceLoop::mPtr = NULL;
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		clsScriptManager::mPtr->Stop();
    }

    UDPThread::Destroy(UDPThread::mPtrIPv6);
    UDPThread::mPtrIPv6 = NULL;

    UDPThread::Destroy(UDPThread::mPtrIPv4);
    UDPThread::mPtrIPv4 = NULL;

	// delete userlist field
	if(clsUsers::mPtr != NULL) {
		clsUsers::mPtr->DisconnectAll();
		delete clsUsers::mPtr;
		clsUsers::mPtr = NULL;
    }

	delete clsDcCommands::mPtr;
    clsDcCommands::mPtr = NULL;

	// delete hashed userlist manager
    delete clsHashManager::mPtr;
    clsHashManager::mPtr = NULL;

	delete clsGlobalDataQueue::mPtr;
    clsGlobalDataQueue::mPtr = NULL;

    if(clsRegisterThread::mPtr != NULL) {
        clsRegisterThread::mPtr->Close();
        clsRegisterThread::mPtr->WaitFor();
        delete clsRegisterThread::mPtr;
        clsRegisterThread::mPtr = NULL;
    }

	delete clsEventQueue::mPtr;
    clsEventQueue::mPtr = NULL;

	delete clsIpP2Country::mPtr;
    clsIpP2Country::mPtr = NULL;

#ifdef _WITH_SQLITE
	delete DBSQLite::mPtr;
    DBSQLite::mPtr = NULL;
#elif _WITH_POSTGRES
	delete DBPostgreSQL::mPtr;
    DBPostgreSQL::mPtr = NULL;
#elif _WITH_MYSQL
	delete DBMySQL::mPtr;
    DBMySQL::mPtr = NULL;
#endif

/*	if(TLSManager != NULL) {
		delete TLSManager;
        TLSManager = NULL;
    }*/

	//userstat  // better here ;)
//    sqldb->FinalizeAllVisits();

#ifdef _BUILD_GUI
    clsMainWindow::mPtr->SetStatusValue((string(clsLanguageManager::mPtr->sTexts[LAN_STOPPED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_STOPPED])+".").c_str());
    clsMainWindow::mPtr->SetStartButtonText(clsLanguageManager::mPtr->sTexts[LAN_START_HUB]);
    clsMainWindow::mPtr->EnableStartButton(TRUE);
    clsMainWindow::mPtr->EnableGuiItems(FALSE);
#endif

    ui8SrCntr = 0;
    ui32Joins = ui32Parts = ui32Logged = 0;

    clsUdpDebug::mPtr->Cleanup();

#ifdef _WIN32
    HeapCompact(GetProcessHeap(), 0);
    HeapCompact(hPtokaXHeap, 0);
#endif

    bServerRunning = false;

    if(bIsRestart == true) {
        bIsRestart = false;

		// start hub
#ifdef _BUILD_GUI
        if(Start() == false) {
            clsMainWindow::mPtr->SetStatusValue((string(clsLanguageManager::mPtr->sTexts[LAN_READY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_READY])+".").c_str());
        }
#else
		if(Start() == false) {
            AppendLog("[ERR] Server start failed in ServerFinalStop");
            exit(EXIT_FAILURE);
        }
#endif
    } else if(bIsClose == true) {
		FinalClose();
    }
}
//---------------------------------------------------------------------------

void clsServerManager::FinalClose() {
#if defined(_WIN32) && !defined(_WIN_IOT)
    KillTimer(NULL, sectimer);
#endif

	clsBanManager::mPtr->Save(true);

    clsProfileManager::mPtr->SaveProfiles();

    clsRegManager::mPtr->Save();

	clsScriptManager::mPtr->SaveScripts();

	clsSettingManager::mPtr->Save();

    delete clsScriptManager::mPtr;
	clsScriptManager::mPtr = NULL;

    delete clsTextFilesManager::mPtr;
    clsTextFilesManager::mPtr = NULL;

    delete clsProfileManager::mPtr;
    clsProfileManager::mPtr = NULL;

    delete clsUdpDebug::mPtr;
    clsUdpDebug::mPtr = NULL;

    delete clsRegManager::mPtr;
    clsRegManager::mPtr = NULL;

    delete clsBanManager::mPtr;
    clsBanManager::mPtr = NULL;

    delete clsZlibUtility::mPtr;
    clsZlibUtility::mPtr = NULL;

    delete clsLanguageManager::mPtr;
    clsLanguageManager::mPtr = NULL;

	// alex82 ... �������� �������������� � ��������������

    delete clsSettingManager::mPtr;
    clsSettingManager::mPtr = NULL;

    delete clsReservedNicksManager::mPtr;
    clsReservedNicksManager::mPtr = NULL;

#ifdef _BUILD_GUI
    clsMainWindow::mPtr->SaveGuiSettings();
#endif

#ifdef __MACH__
	mach_port_deallocate(mach_task_self(), clsServerManager::csMachClock);
#endif

    DeleteGlobalBuffer();

#ifdef _WIN32
	HeapDestroy(hPtokaXHeap);
	
	WSACleanup();

	#ifndef _WIN_IOT
		::PostMessage(NULL, WM_USER+1, 0, 0);
	#endif
#endif
}
//---------------------------------------------------------------------------

void clsServerManager::UpdateServers() {
    bool bFound = false;

    // Remove servers for ports we don't want use anymore
    ServerThread * pCur = NULL,
        * pNext = pServersS;

    while(pNext != NULL) {
		pCur = pNext;
		pNext = pCur->pNext;

        bFound = false;

        for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
            if(clsSettingManager::mPtr->ui16PortNumbers[ui8i] == 0) {
                break;
            }

            if(pCur->ui16Port == clsSettingManager::mPtr->ui16PortNumbers[ui8i]) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
            if(pCur->pPrev == NULL) {
                if(pCur->pNext == NULL) {
                    pServersS = NULL;
                    pServersE = NULL;
                } else {
					pCur->pNext->pPrev = NULL;
                    pServersS = pCur->pNext;
                }
            } else if(pCur->pNext == NULL) {
				pCur->pPrev->pNext = NULL;
                pServersE = pCur->pPrev;
            } else {
				pCur->pPrev->pNext = pCur->pNext;
				pCur->pNext->pPrev = pCur->pPrev;
            }

			pCur->Close();
			pCur->WaitFor();

        	delete pCur;
        }
    }

    // Add servers for ports that not running
    for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
        if(clsSettingManager::mPtr->ui16PortNumbers[ui8i] == 0) {
            break;
        }

        bFound = false;

		pCur = NULL,
			pNext = pServersS;

        while(pNext != NULL) {
			pCur = pNext;
			pNext = pCur->pNext;

            if(pCur->ui16Port == clsSettingManager::mPtr->ui16PortNumbers[ui8i]) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
            if(clsSettingManager::mPtr->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || (bUseIPv6 == true && bIPv6DualStack == false)) {
                if(bUseIPv6 == true) {
                    CreateServerThread(AF_INET6, clsSettingManager::mPtr->ui16PortNumbers[ui8i], true);
                }
                CreateServerThread(AF_INET, clsSettingManager::mPtr->ui16PortNumbers[ui8i], true);
            } else {
                CreateServerThread(bUseIPv6 == true ? AF_INET6 : AF_INET, clsSettingManager::mPtr->ui16PortNumbers[ui8i], true);
            }
        }
    }
}
//---------------------------------------------------------------------------

void clsServerManager::ResumeAccepts() {
	if(bServerRunning == false) {
        return;
    }

    ServerThread * cur = NULL,
        * next = pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        cur->ResumeSck();
    }
}
//---------------------------------------------------------------------------

void clsServerManager::SuspendAccepts(const uint32_t &ui32Time) {
	if(bServerRunning == false) {
        return;
    }

    if(ui32Time != 0) {
        clsUdpDebug::mPtr->BroadcastFormat("[SYS] Suspending listening threads to %u seconds.", ui32Time);
    } else {
		const char sSuspendMsg[] = "[SYS] Suspending listening threads.";
        clsUdpDebug::mPtr->Broadcast(sSuspendMsg, sizeof(sSuspendMsg)-1);
    }

    ServerThread * cur = NULL,
        * next = pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        cur->SuspendSck(ui32Time);
    }
}
//---------------------------------------------------------------------------

void clsServerManager::UpdateAutoRegState() {
    if(bServerRunning == false) {
        return;
    }

    if(clsSettingManager::mPtr->bBools[SETBOOL_AUTO_REG] == true) {
#if defined(_WIN32) && !defined(_WIN_IOT)
        regtimer = SetTimer(NULL, 0, 901000, NULL);

        if(regtimer == 0) {
			AppendDebugLog("%s - [ERR] Cannot start regtimer in ServerUpdateAutoRegState\n");
            exit(EXIT_FAILURE);
        }
#else
	#ifdef __MACH__
		mach_timespec_t mts;
		clock_get_time(clsServerManager::csMachClock, &mts);
		clsServiceLoop::mPtr->ui64LastRegToHublist = mts.tv_sec;
	#elif _WIN32
		clsServiceLoop::mPtr->ui64LastRegToHublist = ::GetTickCount64() / 1000;
	#else
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		clsServiceLoop::mPtr->ui64LastRegToHublist = ts.tv_sec;
	#endif
#endif
#if defined(_WIN32) && !defined(_WIN_IOT)
    } else {
        if(KillTimer(NULL, regtimer) == 0) {
    		AppendDebugLog("%s - [ERR] Cannot stop regtimer in ServerUpdateAutoRegState\n");
        	exit(EXIT_FAILURE);
        }
#endif
    }
}
//---------------------------------------------------------------------------

void clsServerManager::CreateServerThread(const int &iAddrFamily, const uint16_t &ui16PortNumber, const bool &bResume/* = false*/) {
	ServerThread * pServer = new (std::nothrow) ServerThread(iAddrFamily, ui16PortNumber);
    if(pServer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pServer in ServerCreateServerThread\n");
        exit(EXIT_FAILURE);
    }

	if(pServer->Listen() == true) {
		if(pServersE == NULL) {
            pServersS = pServer;
            pServersE = pServer;
        } else {
            pServer->pPrev = pServersE;
            pServersE->pNext = pServer;
            pServersE = pServer;
        }
    } else {
        delete pServer;
    }

    if(bResume == true) {
        pServer->Resume();
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsServerManager::CommandLineSetup() {
	printf("%s built on %s %s\n\n", g_sPtokaXTitle, __DATE__, __TIME__);
	printf("Welcome to PtokaX configuration setup.\nDirectory for PtokaX configuration is: %s\nWhen this directory is wrong, then exit this setup.\nTo specify correct configuration directory start PtokaX with -c configdir parameter.", clsServerManager::sPath.c_str());

	const char sMenu[] = "\n\nAvailable options:\n"
		"1. Basic setup. Only few things required for PtokaX run.\n"
		"2. Complete setup. Long setup, where you can change all PtokaX setings.\n"
		"3. Add registered user.\n"
		"4. Exit this setup.\n\n"
		"Your choice: ";

	printf(sMenu);

	while(true) {
		int iChar = getchar();
	
		while(getchar() != '\n') {
			// boredom...
		};

		switch(iChar) {
			case '1':
				clsSettingManager::mPtr->CmdLineBasicSetup();
				printf(sMenu);
				continue;
			case '2':
				clsSettingManager::mPtr->CmdLineCompleteSetup();
				printf(sMenu);
				continue;
			case '3':
				clsRegManager::mPtr->AddRegCmdLine();
				printf(sMenu);
				continue;
			case '4':
				printf("%s ending...\n", g_sPtokaXTitle);
				break;
			default:
				printf("Unknown option: %c\nYour choice: ", iChar);
				continue;
		}

		break;
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool clsServerManager::ResolveHubAddress(const bool &bSilent/* = false*/) {
    if(clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP] == true) {
        if(isIP(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS]) == false) {
#ifdef _BUILD_GUI
            clsMainWindow::mPtr->SetStatusValue((string(clsLanguageManager::mPtr->sTexts[LAN_RESOLVING_HUB_ADDRESS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RESOLVING_HUB_ADDRESS])+"...").c_str());
#endif

            struct addrinfo hints;
            memset(&hints, 0, sizeof(addrinfo));

            if(bUseIPv6 == true) {
                hints.ai_family = AF_UNSPEC;
            } else {
                hints.ai_family = AF_INET;
            }

            struct addrinfo *res;

            if(::getaddrinfo(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS], NULL, &hints, &res) != 0 || (res->ai_family != AF_INET && res->ai_family != AF_INET6)) {
            	if(bSilent == false) {
#ifdef _WIN32
            		int err = WSAGetLastError();
	#ifdef _BUILD_GUI
					::MessageBox(clsMainWindow::mPtr->m_hWnd,(string(clsLanguageManager::mPtr->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+" '"+string(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS])+"' "+
						string(clsLanguageManager::mPtr->sTexts[LAN_HAS_FAILED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_HAS_FAILED])+".\n"+string(clsLanguageManager::mPtr->sTexts[LAN_ERROR_CODE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ERROR_CODE])+": "+string(WSErrorStr(err))+" ("+string(err)+")\n\n"+
						string(clsLanguageManager::mPtr->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".").c_str(), clsLanguageManager::mPtr->sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
	#else
                	AppendLog(string(clsLanguageManager::mPtr->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
						" '"+string(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(clsLanguageManager::mPtr->sTexts[LAN_HAS_FAILED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_HAS_FAILED])+".\n"+string(clsLanguageManager::mPtr->sTexts[LAN_ERROR_CODE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ERROR_CODE])+
						": "+string(WSErrorStr(err))+" ("+string(err)+")\n\n"+string(clsLanguageManager::mPtr->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".");
	#endif
#else
                	AppendLog(string(clsLanguageManager::mPtr->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
						" '"+string(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(clsLanguageManager::mPtr->sTexts[LAN_HAS_FAILED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_HAS_FAILED])+".\n"+string(clsLanguageManager::mPtr->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], 
						(size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".");
#endif
				}

                return false;
            } else {
				Memo("*** "+string(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS], (size_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_ADDRESS])+" "+string(clsLanguageManager::mPtr->sTexts[LAN_RESOLVED_SUCCESSFULLY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RESOLVED_SUCCESSFULLY])+".");

                if(bUseIPv6 == true) {
                    struct addrinfo *next = res;
                    while(next != NULL) {
                        if(next->ai_family == AF_INET) {
                            if(((sockaddr_in *)(next->ai_addr))->sin_addr.s_addr != INADDR_ANY) {
                                strcpy(sHubIP, inet_ntoa(((sockaddr_in *)(next->ai_addr))->sin_addr));
                            }
                        } else if(next->ai_family == AF_INET6) {
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
                            win_inet_ntop(&((struct sockaddr_in6 *)next->ai_addr)->sin6_addr, sHubIP6, 40);
#else
                            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)next->ai_addr)->sin6_addr, sHubIP6, 40);
#endif
                        }

                        next = next->ai_next;
                    }
                } else if(((sockaddr_in *)(res->ai_addr))->sin_addr.s_addr != INADDR_ANY) {
                    strcpy(sHubIP, inet_ntoa(((sockaddr_in *)(res->ai_addr))->sin_addr));
                }

                if(sHubIP[0] != '\0') {
                    string msg = "*** "+string(sHubIP);
                    if(IsPrivateIP(sHubIP) == true) {
                    	clsSettingManager::mPtr->SetBool(SETBOOL_AUTO_REG, false);
					}
                    if(sHubIP6[0] != '\0') {
                        msg += " / "+string(sHubIP6);
                        if(IsPrivateIP(sHubIP6) == true) {
	                    	clsSettingManager::mPtr->SetBool(SETBOOL_AUTO_REG, false);
						}
                    }

				    Memo(msg);
                } else if(sHubIP6[0] != '\0') {
				    Memo("*** "+string(sHubIP6));
				    if(IsPrivateIP(sHubIP6) == true) {
                    	clsSettingManager::mPtr->SetBool(SETBOOL_AUTO_REG, false);
					}
                }

				freeaddrinfo(res);
            }
        } else {
            strcpy(sHubIP, clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS]);
            if(IsPrivateIP(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS]) == true) {
                clsSettingManager::mPtr->SetBool(SETBOOL_AUTO_REG, false);
			}
        }
    } else {
        if(clsSettingManager::mPtr->sTexts[SETTXT_IPV4_ADDRESS] != NULL) {
            strcpy(sHubIP, clsSettingManager::mPtr->sTexts[SETTXT_IPV4_ADDRESS]);
        } else {
            sHubIP[0] = '\0';
        }

        if(clsSettingManager::mPtr->sTexts[SETTXT_IPV6_ADDRESS] != NULL) {
            strcpy(sHubIP6, clsSettingManager::mPtr->sTexts[SETTXT_IPV6_ADDRESS]);
        } else {
            sHubIP6[0] = '\0';
        }

        if(isIP(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS]) == true) {
        	if(IsPrivateIP(clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS]) == true) {
                clsSettingManager::mPtr->SetBool(SETBOOL_AUTO_REG, false);
			}
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
