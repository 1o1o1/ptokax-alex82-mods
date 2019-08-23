/*
* PtokaX mod

 * Copyright (C) 2013-2016  alex82 aka Caddish Hedgehog

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
#include "LuaInc.h"
//---------------------------------------------------------------------------
#include "LuaPrint.h"
//---------------------------------------------------------------------------
#include "GlobalDataQueue.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------

int LuaPrint (lua_State *L) {
	int n = lua_gettop(L);
	int i;
	size_t b = 0;
#ifndef _BUILD_GUI
	static const char dollar[6] = "&#36;";
	static const char endpipe[7] = "&#124;";
	int j;
	b = sprintf(clsServerManager::pGlobalBuffer, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
#endif
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		size_t szDataLen;
		s = lua_tolstring(L, -1, &szDataLen);
		if (s == NULL)
		  return luaL_error(L, LUA_QL("tostring") " must return a string to "
							   LUA_QL("print"));
		if (b+szDataLen <= 128000)
		{
			if (i>1)
			{
				clsServerManager::pGlobalBuffer[b] = '\t';
				b++;
			}
#ifdef _BUILD_GUI
			memcpy(clsServerManager::pGlobalBuffer+b, s, szDataLen);
			b += szDataLen;
#else
			for (j=0; j<(int)szDataLen; j++)
			{
				if (s[j] == '$')
				{
					memcpy(clsServerManager::pGlobalBuffer+b, dollar, 5);
					b += 5;
				}
				else if (s[j] == '|')
				{
					memcpy(clsServerManager::pGlobalBuffer+b, endpipe, 6);
					b += 6;
				}
				else
				{
					clsServerManager::pGlobalBuffer[b] = s[j];
					b++;
				}
			}
#endif
			lua_pop(L, 1);  /* pop result */
		}
	}
#ifdef _BUILD_GUI
	clsServerManager::pGlobalBuffer[b] = '\0';
	RichEditAppendText(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS], clsServerManager::pGlobalBuffer);
#else
	clsServerManager::pGlobalBuffer[b] = '|';
	clsServerManager::pGlobalBuffer[b+1] = '\0';
	clsGlobalDataQueue::mPtr->SingleItemStore(clsServerManager::pGlobalBuffer, b+1, NULL, 0, clsGlobalDataQueue::SI_TOPROFILE);
#endif
	return 0;
}
