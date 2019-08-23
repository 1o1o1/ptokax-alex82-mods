#*******************************************************************************
#
# Makefile for PtokaX.
#
#*******************************************************************************

#*******************************************************************************
# Compiler
#*******************************************************************************
CXX = g++

#*******************************************************************************
# Lua
#*******************************************************************************

# Lua in Debian is lua5.1. Other known names are lua-5.1, lua51 and lua.
# If you have Lua lib with another name than lua5.1 change it.

LUA = lua5.3

#*******************************************************************************
# Debug flags
#*******************************************************************************
#CXXFLAGS = -g -Wall -Wextra

#*******************************************************************************
# Release flags
#*******************************************************************************
ptokax: CXXFLAGS = -Wall -Wno-unknown-pragmas -Wno-format -O2 -D_FORTIFY_SOURCE=2 -fomit-frame-pointer -pipe -fstack-protector -DLUA_COMPAT_5_2
ptokax_hashpass: CXXFLAGS = -Wall -Wno-unknown-pragmas -Wno-format -O2 -D_FORTIFY_SOURCE=2 -fomit-frame-pointer -pipe -fstack-protector -D HASH_PASS -DLUA_COMPAT_5_2

#*******************************************************************************
# Include
#*******************************************************************************
INCLUDE = -Itinyxml -I/usr/include -I/usr/include/$(LUA)

#*******************************************************************************
# Files to compile and link
#*******************************************************************************

OBJS = $(CURDIR)/obj/colUsers.o $(CURDIR)/obj/DcCommands.o $(CURDIR)/obj/DeFlood.o $(CURDIR)/obj/eventqueue.o $(CURDIR)/obj/GlobalDataQueue.o $(CURDIR)/obj/hashBanManager.o $(CURDIR)/obj/hashUsrManager.o \
        $(CURDIR)/obj/hashRegManager.o $(CURDIR)/obj/HubCommands.o $(CURDIR)/obj/IP2Country.o $(CURDIR)/obj/LanguageManager.o $(CURDIR)/obj/LuaBanManLib.o $(CURDIR)/obj/LuaCoreLib.o $(CURDIR)/obj/LuaIP2CountryLib.o \
        $(CURDIR)/obj/LuaProfManLib.o $(CURDIR)/obj/LuaRegManLib.o $(CURDIR)/obj/LuaScript.o $(CURDIR)/obj/LuaScriptManager.o $(CURDIR)/obj/LuaScriptManLib.o $(CURDIR)/obj/LuaSetManLib.o $(CURDIR)/obj/LuaTmrManLib.o \
        $(CURDIR)/obj/LuaUDPDbgLib.o $(CURDIR)/obj/ProfileManager.o $(CURDIR)/obj/PtokaX-nix.o $(CURDIR)/obj/PXBReader.o $(CURDIR)/obj/pxstring.o $(CURDIR)/obj/RegThread.o $(CURDIR)/obj/ResNickManager.o $(CURDIR)/obj/ServerManager.o \
        $(CURDIR)/obj/ServerThread.o $(CURDIR)/obj/serviceLoop.o $(CURDIR)/obj/SettingManager.o $(CURDIR)/obj/TextFileManager.o $(CURDIR)/obj/UdpDebug.o $(CURDIR)/obj/UDPThread.o $(CURDIR)/obj/User.o $(CURDIR)/obj/utility.o \
        $(CURDIR)/obj/ZlibUtility.o $(CURDIR)/obj/LuaPrint.o

OBJS_HASHPASS = $(CURDIR)/obj/sha2.o $(CURDIR)/obj/base64.o

#*******************************************************************************
# Binary to create
#*******************************************************************************

all: ptokax

ptokax: $(OBJS)
	cd tinyxml; $(MAKE)
	$(CXX) $(OBJS) $(CURDIR)/tinyxml/tinyxml.a -o PtokaX_mod2 -lstdc++ -lpthread -l$(LUA) -lrt -lz

ptokax_hashpass: SHA2_H = $(CURDIR)/sha2/sha2.h
ptokax_hashpass: $(OBJS) $(OBJS_HASHPASS)
	cd tinyxml; $(MAKE)
	$(CXX) $(OBJS) $(OBJS_HASHPASS) $(CURDIR)/tinyxml/tinyxml.a -o PtokaX_mod2 -lstdc++ -lpthread -l$(LUA) -lrt -lz

#*******************************************************************************
# Files to compile
#*******************************************************************************
$(CURDIR)/obj/colUsers.o: $(CURDIR)/core/colUsers.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/colUsers.h $(CURDIR)/core/GlobalDataQueue.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/ServerManager.h \
  $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h
	mkdir -p $(CURDIR)/obj
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/colUsers.cpp -o $(CURDIR)/obj/colUsers.o

$(CURDIR)/obj/DcCommands.o: $(CURDIR)/core/DcCommands.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/DcCommands.h $(CURDIR)/core/colUsers.h \
  $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashBanManager.h $(CURDIR)/core/hashRegManager.h $(CURDIR)/core/hashUsrManager.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ProfileManager.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h \
  $(CURDIR)/core/utility.h $(CURDIR)/core/ZlibUtility.h $(CURDIR)/core/DeFlood.h $(CURDIR)/core/HubCommands.h $(CURDIR)/core/IP2Country.h \
  $(CURDIR)/core/ResNickManager.h $(CURDIR)/core/TextFileManager.h $(SHA2_H)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/DcCommands.cpp -o $(CURDIR)/obj/DcCommands.o

$(CURDIR)/obj/DeFlood.o: $(CURDIR)/core/DeFlood.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashBanManager.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/DeFlood.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/DeFlood.cpp -o $(CURDIR)/obj/DeFlood.o

$(CURDIR)/obj/eventqueue.o: $(CURDIR)/core/eventqueue.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/eventqueue.h \
  $(CURDIR)/core/DcCommands.h $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ServerManager.h \
  $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/LuaScript.h \
  $(CURDIR)/core/RegThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/eventqueue.cpp -o $(CURDIR)/obj/eventqueue.o

$(CURDIR)/obj/GlobalDataQueue.o: $(CURDIR)/core/GlobalDataQueue.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/GlobalDataQueue.h \
  $(CURDIR)/core/colUsers.h $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h \
  $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/ZlibUtility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/GlobalDataQueue.cpp -o $(CURDIR)/obj/GlobalDataQueue.o

$(CURDIR)/obj/hashBanManager.o: $(CURDIR)/core/hashBanManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/hashBanManager.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h \
  $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/hashBanManager.cpp -o $(CURDIR)/obj/hashBanManager.o

$(CURDIR)/obj/hashRegManager.o: $(CURDIR)/core/hashRegManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/hashRegManager.h \
  $(CURDIR)/core/colUsers.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/LanguageManager.h \
  $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/PXBReader.h $(SHA2_H)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/hashRegManager.cpp -o $(CURDIR)/obj/hashRegManager.o

$(CURDIR)/obj/hashUsrManager.o: $(CURDIR)/core/hashUsrManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/hashUsrManager.h \
  $(CURDIR)/core/User.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/hashUsrManager.cpp -o $(CURDIR)/obj/hashUsrManager.o

$(CURDIR)/obj/HubCommands.o: $(CURDIR)/core/HubCommands.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/colUsers.h \
  $(CURDIR)/core/DcCommands.h $(CURDIR)/core/eventqueue.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashBanManager.h \
  $(CURDIR)/core/hashRegManager.h $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h \
  $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/serviceLoop.h \
  $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/HubCommands.h \
  $(CURDIR)/core/IP2Country.h $(CURDIR)/core/LuaScript.h $(CURDIR)/core/TextFileManager.h $(SHA2_H)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/HubCommands.cpp -o $(CURDIR)/obj/HubCommands.o

$(CURDIR)/obj/IP2Country.o: $(CURDIR)/core/IP2Country.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/IP2Country.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/IP2Country.cpp -o $(CURDIR)/obj/IP2Country.o

$(CURDIR)/obj/LanguageManager.o: $(CURDIR)/core/LanguageManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LanguageXml.h \
  $(CURDIR)/core/LanguageStrings.h $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LanguageManager.cpp -o $(CURDIR)/obj/LanguageManager.o

$(CURDIR)/obj/LuaBanManLib.o: $(CURDIR)/core/LuaBanManLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaBanManLib.h $(CURDIR)/core/hashBanManager.h $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/LuaScriptManager.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaBanManLib.cpp -o $(CURDIR)/obj/LuaBanManLib.o

$(CURDIR)/obj/LuaCoreLib.o: $(CURDIR)/core/LuaCoreLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h $(CURDIR)/core/LuaCoreLib.h \
  $(CURDIR)/core/colUsers.h $(CURDIR)/core/eventqueue.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashBanManager.h $(CURDIR)/core/hashUsrManager.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ServerManager.h \
  $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/IP2Country.h \
  $(CURDIR)/core/ResNickManager.h $(CURDIR)/core/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaCoreLib.cpp -o $(CURDIR)/obj/LuaCoreLib.o

$(CURDIR)/obj/LuaIP2CountryLib.o: $(CURDIR)/core/LuaIP2CountryLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaIP2CountryLib.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/IP2Country.h \
  $(CURDIR)/core/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaIP2CountryLib.cpp -o $(CURDIR)/obj/LuaIP2CountryLib.o

$(CURDIR)/obj/LuaProfManLib.o: $(CURDIR)/core/LuaProfManLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaProfManLib.h $(CURDIR)/core/ProfileManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaProfManLib.cpp -o $(CURDIR)/obj/LuaProfManLib.o

$(CURDIR)/obj/LuaRegManLib.o: $(CURDIR)/core/LuaRegManLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaRegManLib.h $(CURDIR)/core/colUsers.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashRegManager.h $(CURDIR)/core/hashUsrManager.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaRegManLib.cpp -o $(CURDIR)/obj/LuaRegManLib.o

$(CURDIR)/obj/LuaScript.o: $(CURDIR)/core/LuaScript.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h $(CURDIR)/core/colUsers.h \
  $(CURDIR)/core/eventqueue.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/LanguageManager.h \
  $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/LuaScript.h $(CURDIR)/core/IP2Country.h \
  $(CURDIR)/core/LuaCoreLib.h $(CURDIR)/core/LuaBanManLib.h $(CURDIR)/core/LuaIP2CountryLib.h $(CURDIR)/core/LuaProfManLib.h \
  $(CURDIR)/core/LuaRegManLib.h $(CURDIR)/core/LuaScriptManLib.h $(CURDIR)/core/LuaSetManLib.h $(CURDIR)/core/LuaTmrManLib.h \
  $(CURDIR)/core/LuaUDPDbgLib.h $(CURDIR)/core/ResNickManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaScript.cpp -o $(CURDIR)/obj/LuaScript.o

$(CURDIR)/obj/LuaScriptManager.o: $(CURDIR)/core/LuaScriptManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/User.h \
  $(CURDIR)/core/utility.h $(CURDIR)/core/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaScriptManager.cpp -o $(CURDIR)/obj/LuaScriptManager.o

$(CURDIR)/obj/LuaScriptManLib.o: $(CURDIR)/core/LuaScriptManLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaScriptManLib.h $(CURDIR)/core/eventqueue.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/utility.h $(CURDIR)/core/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaScriptManLib.cpp -o $(CURDIR)/obj/LuaScriptManLib.o

$(CURDIR)/obj/LuaSetManLib.o: $(CURDIR)/core/LuaSetManLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaSetManLib.h $(CURDIR)/core/eventqueue.h $(CURDIR)/core/hashRegManager.h $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/LuaScriptManager.h \
  $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaSetManLib.cpp -o $(CURDIR)/obj/LuaSetManLib.o

$(CURDIR)/obj/LuaTmrManLib.o: $(CURDIR)/core/LuaTmrManLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaTmrManLib.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/utility.h $(CURDIR)/core/LuaScript.h 
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaTmrManLib.cpp -o $(CURDIR)/obj/LuaTmrManLib.o

$(CURDIR)/obj/LuaUDPDbgLib.o: $(CURDIR)/core/LuaUDPDbgLib.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/LuaInc.h \
  $(CURDIR)/core/LuaUDPDbgLib.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/utility.h $(CURDIR)/core/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaUDPDbgLib.cpp -o $(CURDIR)/obj/LuaUDPDbgLib.o

$(CURDIR)/obj/ProfileManager.o: $(CURDIR)/core/ProfileManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/ProfileManager.h \
  $(CURDIR)/core/colUsers.h $(CURDIR)/core/hashRegManager.h $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/ProfileManager.cpp -o $(CURDIR)/obj/ProfileManager.o

$(CURDIR)/obj/PtokaX-nix.o: $(CURDIR)/core/PtokaX-nix.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/eventqueue.h $(CURDIR)/core/GlobalDataQueue.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/serviceLoop.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/PtokaX-nix.cpp -o $(CURDIR)/obj/PtokaX-nix.o

$(CURDIR)/obj/PXBReader.o: $(CURDIR)/core/PXBReader.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/PXBReader.h $(CURDIR)/core/ServerManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/PXBReader.cpp -o $(CURDIR)/obj/PXBReader.o

$(CURDIR)/obj/pxstring.o: $(CURDIR)/core/pxstring.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/pxstring.cpp -o $(CURDIR)/obj/pxstring.o

$(CURDIR)/obj/RegThread.o: $(CURDIR)/core/RegThread.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/eventqueue.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/utility.h $(CURDIR)/core/RegThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/RegThread.cpp -o $(CURDIR)/obj/RegThread.o

$(CURDIR)/obj/ResNickManager.o: $(CURDIR)/core/ResNickManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/ResNickManager.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/ResNickManager.cpp -o $(CURDIR)/obj/ResNickManager.o

$(CURDIR)/obj/ServerManager.o: $(CURDIR)/core/ServerManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/ServerManager.h \
  $(CURDIR)/core/colUsers.h $(CURDIR)/core/DcCommands.h $(CURDIR)/core/eventqueue.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashBanManager.h \
  $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/hashRegManager.h $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h \
  $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/serviceLoop.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/utility.h $(CURDIR)/core/ZlibUtility.h $(CURDIR)/core/HubCommands.h \
  $(CURDIR)/core/IP2Country.h $(CURDIR)/core/LuaScript.h $(CURDIR)/core/RegThread.h $(CURDIR)/core/ResNickManager.h $(CURDIR)/core/ServerThread.h \
  $(CURDIR)/core/TextFileManager.h $(CURDIR)/core/UDPThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/ServerManager.cpp -o $(CURDIR)/obj/ServerManager.o

$(CURDIR)/obj/ServerThread.o: $(CURDIR)/core/ServerThread.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/eventqueue.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/serviceLoop.h \
  $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/utility.h $(CURDIR)/core/ServerThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/ServerThread.cpp -o $(CURDIR)/obj/ServerThread.o

$(CURDIR)/obj/serviceLoop.o: $(CURDIR)/core/serviceLoop.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/serviceLoop.h \
  $(CURDIR)/core/colUsers.h $(CURDIR)/core/eventqueue.h $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashBanManager.h $(CURDIR)/core/hashRegManager.h $(CURDIR)/core/hashUsrManager.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ProfileManager.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/User.h \
  $(CURDIR)/core/utility.h $(CURDIR)/core/ZlibUtility.h $(CURDIR)/core/LuaScript.h $(CURDIR)/core/RegThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/serviceLoop.cpp -o $(CURDIR)/obj/serviceLoop.o

$(CURDIR)/obj/SettingManager.o: $(CURDIR)/core/SettingManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/SettingStr.h \
  $(CURDIR)/core/SettingCom.h $(CURDIR)/core/SettingDefaults.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/colUsers.h \
  $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/LuaScriptManager.h \
  $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h $(CURDIR)/core/ResNickManager.h \
  $(CURDIR)/core/ServerThread.h $(CURDIR)/core/TextFileManager.h $(CURDIR)/core/UDPThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/SettingManager.cpp -o $(CURDIR)/obj/SettingManager.o

$(CURDIR)/obj/TextFileManager.o: $(CURDIR)/core/TextFileManager.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h \
  $(CURDIR)/core/TextFileManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/User.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/TextFileManager.cpp -o $(CURDIR)/obj/TextFileManager.o

$(CURDIR)/obj/UdpDebug.o: $(CURDIR)/core/UdpDebug.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/LanguageManager.h \
  $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/User.h \
  $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/UdpDebug.cpp -o $(CURDIR)/obj/UdpDebug.o

$(CURDIR)/obj/UDPThread.o: $(CURDIR)/core/UDPThread.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/eventqueue.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/SettingIds.h $(CURDIR)/core/utility.h $(CURDIR)/core/UDPThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/UDPThread.cpp -o $(CURDIR)/obj/UDPThread.o

$(CURDIR)/obj/User.o: $(CURDIR)/core/User.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/User.h $(CURDIR)/core/colUsers.h $(CURDIR)/core/DcCommands.h \
  $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/hashUsrManager.h $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h \
  $(CURDIR)/core/LuaScriptManager.h $(CURDIR)/core/ProfileManager.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/utility.h $(CURDIR)/core/UdpDebug.h $(CURDIR)/core/ZlibUtility.h $(CURDIR)/core/DeFlood.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/User.cpp -o $(CURDIR)/obj/User.o

$(CURDIR)/obj/utility.o: $(CURDIR)/core/utility.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/utility.h $(CURDIR)/core/hashBanManager.h \
  $(CURDIR)/core/LanguageManager.h $(CURDIR)/core/LanguageIds.h $(CURDIR)/core/ServerManager.h $(CURDIR)/core/SettingManager.h \
  $(CURDIR)/core/SettingIds.h $(CURDIR)/core/UdpDebug.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/utility.cpp -o $(CURDIR)/obj/utility.o

$(CURDIR)/obj/ZlibUtility.o: $(CURDIR)/core/ZlibUtility.cpp $(CURDIR)/core/stdinc.h $(CURDIR)/core/pxstring.h $(CURDIR)/core/ZlibUtility.h \
  $(CURDIR)/core/ServerManager.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/ZlibUtility.cpp -o $(CURDIR)/obj/ZlibUtility.o

$(CURDIR)/obj/LuaPrint.o: $(CURDIR)/core/LuaPrint.cpp $(CURDIR)/core/stdinc.h  $(CURDIR)/core/LuaInc.h $(CURDIR)/core/LuaPrint.h \
  $(CURDIR)/core/GlobalDataQueue.h $(CURDIR)/core/SettingManager.h $(CURDIR)/core/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/core/LuaPrint.cpp -o $(CURDIR)/obj/LuaPrint.o

$(CURDIR)/obj/sha2.o: $(CURDIR)/sha2/sha2.c $(CURDIR)/sha2/sha2.h
	$(CXX) $(CXXFLAGS) -fno-strict-aliasing $(INCLUDE) -c $(CURDIR)/sha2/sha2.c -o $(CURDIR)/obj/sha2.o

$(CURDIR)/obj/base64.o: $(CURDIR)/base64/base64.c $(CURDIR)/base64/base64.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/base64/base64.c -o $(CURDIR)/obj/base64.o

	
#*******************************************************************************
# Install
#*******************************************************************************
install:
	install -s ptokax /usr/bin
	-mkdir -p /etc/ptokax/language
	-cp bin/language/Russian.xml /etc/ptokax/language/Russian.xml
	-mkdir -p /etc/ptokax/scripts
	-cp bin/scripts/locale.lua /etc/ptokax/scripts/locale.lua
	cp control_script /etc/init.d/ptokax
	chmod 755 /etc/init.d/ptokax
	update-rc.d ptokax defaults


#*******************************************************************************
# Autorun on
#*******************************************************************************
on:
	update-rc.d ptokax enable


#*******************************************************************************
# Autorun off
#*******************************************************************************
off:
	update-rc.d ptokax disable


#*******************************************************************************
# Uninstall
#*******************************************************************************
uninstall:
	update-rc.d -f ptokax remove
	-rm /etc/init.d/ptokax
	-rm /usr/bin/ptokax


#*******************************************************************************
# Cleanup
#*******************************************************************************
clean:
	cd tinyxml; $(MAKE) clean
	-rm $(OBJS) $(OBJS_HASHPASS) ptokax