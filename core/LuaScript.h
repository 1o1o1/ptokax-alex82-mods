/*
 * PtokaX - hub server for Direct Connect peer to peer network.

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
#ifndef LuaScriptH
#define LuaScriptH
//---------------------------------------------------------------------------
struct User;
struct Script;
//---------------------------------------------------------------------------

struct ScriptBot {
	ScriptBot * pPrev, * pNext;

    char *sNick;
    char *sMyINFO;

    bool bIsOP;

    ScriptBot();
    ~ScriptBot();

    ScriptBot(const ScriptBot&);
    const ScriptBot& operator=(const ScriptBot&);

    static ScriptBot * CreateScriptBot(char * sNick, const size_t &szNickLen, char * sDescription, const size_t &szDscrLen, char * sEmail, const size_t &szEmailLen, const bool &bOP);
	// alex82 ... �������� �������������� ������� ��� �������� ���� � ����������� $MyINFO
	static ScriptBot * CreateScriptBot(char * sNick, const size_t &szNickLen, char * sBotMyINFO, const size_t &szMyINFOLen, const bool &bOP);
};
//------------------------------------------------------------------------------

struct ScriptTimer {
#if defined(_WIN32) && !defined(_WIN_IOT)
    UINT_PTR uiTimerId;
#else
	uint64_t ui64Interval;
	uint64_t ui64LastTick;
#endif

	ScriptTimer * pPrev, * pNext;

	lua_State * pLua;

    char * sFunctionName;

    int iFunctionRef;

	static char sDefaultTimerFunc[];

    ScriptTimer();
    ~ScriptTimer();

    ScriptTimer(const ScriptTimer&);
    const ScriptTimer& operator=(const ScriptTimer&);

#if defined(_WIN32) && !defined(_WIN_IOT)
    static ScriptTimer * CreateScriptTimer(UINT_PTR uiTmrId, char * sFunctName, const size_t &szLen, const int &iRef, lua_State * pLuaState);
#else
	static ScriptTimer * CreateScriptTimer(char * sFunctName, const size_t &szLen, const int &iRef, lua_State * pLuaState);
#endif
};
//------------------------------------------------------------------------------

struct Script {
    Script * pPrev, * pNext;

    ScriptBot * pBotList;

    lua_State * pLUA;

    char * sName;

    uint32_t ui32DataArrivals;

	uint16_t ui16Functions;

    bool bEnabled, bRegUDP, bProcessed;

    enum LuaFunctions {
		ONSTARTUP         = 0x1,
		ONEXIT            = 0x2,
		ONERROR           = 0x4,
		USERCONNECTED     = 0x8,
		REGCONNECTED      = 0x10,
		OPCONNECTED       = 0x20,
		USERDISCONNECTED  = 0x40,
		REGDISCONNECTED   = 0x80,
		OPDISCONNECTED    = 0x100
	};

    Script();
    ~Script();

    Script(const Script&);
    const Script& operator=(const Script&);

    static Script * CreateScript(char *Name, const bool &enabled);
};
//------------------------------------------------------------------------------

bool ScriptStart(Script * cur);
void ScriptStop(Script * cur);

int ScriptGetGC(Script * cur);

void ScriptOnStartup(Script * cur);
void ScriptOnExit(Script * cur);

void ScriptPushUser(lua_State * L, User * u, const bool &bFullTable = false);
void ScriptPushUserExtended(lua_State * L, User * u, const int &iTable);

User * ScriptGetUser(lua_State * L, const int &iTop, const char * sFunction);

void ScriptError(Script * cur);

#if defined(_WIN32) && !defined(_WIN_IOT)
    void ScriptOnTimer(const UINT_PTR &uiTimerId);
#else
	void ScriptOnTimer(const uint64_t &ui64ActualMillis);
#endif

int ScriptTraceback(lua_State * L);
//------------------------------------------------------------------------------

#endif
