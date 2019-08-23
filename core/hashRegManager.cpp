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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "hashRegManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "PXBReader.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/RegisteredUserDialog.h"
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------
#ifdef HASH_PASS
	#include "../sha2/sha2.h"
#endif
//---------------------------------------------------------------------------
clsRegManager * clsRegManager::mPtr = NULL;
//---------------------------------------------------------------------------
static const char sPtokaXRegiteredUsers[] = "PtokaX Registered Users";
static const size_t szPtokaXRegiteredUsersLen = sizeof(sPtokaXRegiteredUsers)-1;
//---------------------------------------------------------------------------

RegUser::RegUser() : tLastBadPass(0), sNick(NULL), pPrev(NULL), pNext(NULL), pHashTablePrev(NULL), pHashTableNext(NULL), ui32Hash(0), ui16Profile(0), tRegDate(0), tLastEnter(0), tOnlineTime(0), sCustom(NULL), ui8BadPassCount(0){
    sPass = NULL;
}
//---------------------------------------------------------------------------

RegUser::~RegUser() {
#ifdef _WIN32
    if(sNick != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sNick in RegUser::~RegUser\n");
    }
#else
	free(sNick);
#endif

#ifdef _WIN32
	if(sPass != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPass) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sPass in RegUser::~RegUser\n");
	}
#else
	free(sPass);
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RegUser * RegUser::CreateReg(char * sRegNick, size_t szRegNickLen, char * sRegPassword, size_t szRegPassLen, const uint16_t &ui16RegProfile, const time_t &tRegDate, const time_t &tLastEnter, const time_t &tOnlineTime) {
    RegUser * pReg = new (std::nothrow) RegUser();

    if(pReg == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new Reg in RegUser::CreateReg\n");

        return NULL;
    }

#ifdef _WIN32
    pReg->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szRegNickLen+1);
#else
	pReg->sNick = (char *)malloc(szRegNickLen+1);
#endif
    if(pReg->sNick == NULL) {
        AppendDebugLogFormat("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sNick in RegUser::RegUser\n");

        delete pReg;
        return NULL;
    }
    memcpy(pReg->sNick, sRegNick, szRegNickLen);
    pReg->sNick[szRegNickLen] = '\0';

	// alex82 ... Выпилили родное хеширование паролей...
#ifdef _WIN32
	pReg->sPass = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szRegPassLen+1);
#else
	pReg->sPass = (char *)malloc(szRegPassLen+1);
#endif
	if(pReg->sPass == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for sPass in RegUser::RegUser\n", (uint64_t)(szRegPassLen+1));

		delete pReg;
		return NULL;
	}
	memcpy(pReg->sPass, sRegPassword, szRegPassLen);
	pReg->sPass[szRegPassLen] = '\0';

    pReg->ui16Profile = ui16RegProfile;
	// alex82 ... Дата регистрации и последнего входа
	pReg->tRegDate = tRegDate;
	pReg->tLastEnter = tLastEnter;
	pReg->tOnlineTime = tOnlineTime;

	pReg->ui32Hash = HashNick(sRegNick, szRegNickLen);

    return pReg;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsRegManager::clsRegManager(void) : ui8SaveCalls(0), pRegListS(NULL), pRegListE(NULL) {
    memset(pTable, 0, sizeof(pTable));
}
//---------------------------------------------------------------------------

clsRegManager::~clsRegManager(void) {
    RegUser * curReg = NULL,
        * next = pRegListS;
        
    while(next != NULL) {
        curReg = next;
		next = curReg->pNext;

		delete curReg;
    }
}
//---------------------------------------------------------------------------

bool clsRegManager::AddNew(char * sNick, char * sPasswd, const uint16_t &iProfile) {
    if(Find(sNick, strlen(sNick)) != NULL) {
        return false;
    }

	// alex82 ... Дата регистрации и последнего входа
	time_t curtime = time(NULL);

    RegUser * pNewUser = NULL;

#ifdef HASH_PASS
	char buff[SHA256_DIGEST_BASE64_LENGTH];
	SHA256_Base64((unsigned char *)sPasswd, strlen(sPasswd), (unsigned char *)buff);
    pNewUser = RegUser::CreateReg(sNick, strlen(sNick), buff, SHA256_DIGEST_BASE64_LENGTH, iProfile, curtime, curtime, 0);
#else
    pNewUser = RegUser::CreateReg(sNick, strlen(sNick), sPasswd, strlen(sPasswd), iProfile, curtime, curtime, 0);
#endif

    if(pNewUser == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in clsRegManager::AddNew\n");
 
        return false;
    }

	Add(pNewUser);

#ifdef _BUILD_GUI
    if(clsRegisteredUsersDialog::mPtr != NULL) {
        clsRegisteredUsersDialog::mPtr->AddReg(pNewUser);
    }
#endif

    Save(true);

    if(clsServerManager::bServerRunning == false) {
        return true;
    }

	User * AddedUser = clsHashManager::mPtr->FindUser(pNewUser->sNick, strlen(pNewUser->sNick));

    if(AddedUser != NULL) {
        bool bAllowedOpChat = clsProfileManager::mPtr->IsAllowed(AddedUser, clsProfileManager::ALLOWEDOPCHAT);
        AddedUser->i32Profile = iProfile;

        if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            if(clsProfileManager::mPtr->IsAllowed(AddedUser, clsProfileManager::HASKEYICON) == true) {
                AddedUser->ui32BoolBits |= User::BIT_OPERATOR;
            } else {
                AddedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
            }

            if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
				// alex82 ... Прячем ключ юзера
				if(((AddedUser->ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY) == false) {
					clsUsers::mPtr->Add2OpList(AddedUser);
	                clsGlobalDataQueue::mPtr->OpListStore(AddedUser->sNick);
				}

                if(bAllowedOpChat != clsProfileManager::mPtr->IsAllowed(AddedUser, clsProfileManager::ALLOWEDOPCHAT)) {
					if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true &&
                        (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                        if(((AddedUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                            AddedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_HELLO],
                                clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_HELLO]);
                        }

                        AddedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                        AddedUser->SendFormat("clsRegManager::AddNew", true, "$OpList %s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                    }
                }
            }
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void clsRegManager::Add(RegUser * Reg) {
	Add2Table(Reg);
    
    if(pRegListE == NULL) {
    	pRegListS = Reg;
    	pRegListE = Reg;
    } else {
        Reg->pPrev = pRegListE;
    	pRegListE->pNext = Reg;
        pRegListE = Reg;
    }

	return;
}
//---------------------------------------------------------------------------

void clsRegManager::Add2Table(RegUser * Reg) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &Reg->ui32Hash, sizeof(uint16_t));

    if(pTable[ui16dx] != NULL) {
        pTable[ui16dx]->pHashTablePrev = Reg;
        Reg->pHashTableNext = pTable[ui16dx];
    }
    
    pTable[ui16dx] = Reg;
}
//---------------------------------------------------------------------------

void clsRegManager::ChangeReg(RegUser * pReg, char * sNewPasswd, const uint16_t &ui16NewProfile) {
	// alex82 ... Выпилили родное хеширование паролей
    if(sNewPasswd != NULL && strcmp(pReg->sPass, sNewPasswd) != 0) {
        size_t szPassLen = strlen(sNewPasswd);

#ifdef HASH_PASS
		SHA256_Base64((unsigned char *)sNewPasswd, szPassLen, (unsigned char *)pReg->sPass);
#else
        char * sOldPass = pReg->sPass;
#ifdef _WIN32
        pReg->sPass = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldPass, szPassLen+1);
#else
		pReg->sPass = (char *)realloc(sOldPass, szPassLen+1);
#endif
        if(pReg->sPass == NULL) {
            pReg->sPass = sOldPass;

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for sPass in hashRegMan::ChangeReg\n");

            return;
        }
        memcpy(pReg->sPass, sNewPasswd, szPassLen);
        pReg->sPass[szPassLen] = '\0';
#endif
    }

    pReg->ui16Profile = ui16NewProfile;

#ifdef _BUILD_GUI
    if(clsRegisteredUsersDialog::mPtr != NULL) {
        clsRegisteredUsersDialog::mPtr->RemoveReg(pReg);
        clsRegisteredUsersDialog::mPtr->AddReg(pReg);
    }
#endif

    clsRegManager::mPtr->Save(true);

    if(clsServerManager::bServerRunning == false) {
        return;
    }

    User *ChangedUser = clsHashManager::mPtr->FindUser(pReg->sNick, strlen(pReg->sNick));
    if(ChangedUser != NULL && ChangedUser->i32Profile != (int32_t)ui16NewProfile) {
        char msg[128];
        bool bAllowedOpChat = clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::ALLOWEDOPCHAT);

        ChangedUser->i32Profile = (int32_t)ui16NewProfile;

        if(((ChangedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) !=
            clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::HASKEYICON)) {
            if(clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::HASKEYICON) == true) {
                ChangedUser->ui32BoolBits |= User::BIT_OPERATOR;
				// alex82 ... Прячем ключ юзера
				if(((ChangedUser->ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY) == false) {
	                clsUsers::mPtr->Add2OpList(ChangedUser);
	                clsGlobalDataQueue::mPtr->OpListStore(ChangedUser->sNick);
				}
            } else {
                ChangedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
				// alex82 ... Прячем ключ юзера
				if(((ChangedUser->ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY) == false) {
 					// alex82 ... Исправили отправку OpList
                   int imsgLen = sprintf(msg, "$Quit %s|", ChangedUser->sNick);
                    if(CheckSprintf(imsgLen, 128, "clsRegManager::ChangeReg1") == true) {
						clsGlobalDataQueue::mPtr->AddQueueItem(msg, imsgLen, NULL, 0, clsGlobalDataQueue::CMD_QUIT);
                    }
					switch(clsSettingManager::mPtr->ui8FullMyINFOOption) {
						case 0:
							clsGlobalDataQueue::mPtr->AddQueueItem(ChangedUser->sMyInfoLong, ChangedUser->ui16MyInfoLongLen, NULL, 0, clsGlobalDataQueue::CMD_MYINFO);
							break;
						case 1:
							clsGlobalDataQueue::mPtr->AddQueueItem(ChangedUser->sMyInfoShort, ChangedUser->ui16MyInfoShortLen, ChangedUser->sMyInfoLong, ChangedUser->ui16MyInfoLongLen, clsGlobalDataQueue::CMD_MYINFO);
							break;
						case 2:
							clsGlobalDataQueue::mPtr->AddQueueItem(ChangedUser->sMyInfoShort, ChangedUser->ui16MyInfoShortLen, NULL, 0, clsGlobalDataQueue::CMD_MYINFO);
							break;
						default:
							break;
					}
					clsUsers::mPtr->DelFromOpList(ChangedUser->sNick);
				}
            }
        }

        if(bAllowedOpChat != clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::ALLOWEDOPCHAT)) {
            if(clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::ALLOWEDOPCHAT) == true) {
                if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true &&
                    (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                    if(((ChangedUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                        ChangedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_HELLO],
                        clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_HELLO]);
                    }

                    ChangedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                    ChangedUser->SendFormat("clsRegManager::ChangeReg1", true, "$OpList %s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                }
            } else {
                if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true && (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                    ChangedUser->SendFormat("clsRegManager::ChangeReg2", true, "$Quit %s|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                }
            }
        }
    }

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->RegChanged(pReg);
    }
#endif
}
//---------------------------------------------------------------------------
// alex82 ... Хранение произвольной строки в профиле юзера
void clsRegManager::SetCustom(RegUser * pReg, char * sCustom, const size_t &szCustomLen) {
	if (szCustomLen == 0) {
		if (pReg->sCustom != NULL) {
#ifdef _WIN32
			if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pReg->sCustom) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate sCustom in clsRegManager::SetCustom\n");
		    }
#else
			free(pReg->sCustom);
#endif
			pReg->sCustom = NULL;
		}
	} else {
		if (pReg->sCustom != NULL) {
			if (szCustomLen != strlen(pReg->sCustom)) {
				char * sOldCustom = pReg->sCustom;
#ifdef _WIN32
				pReg->sCustom = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldCustom, szCustomLen+1);
#else
				pReg->sCustom = (char *)realloc(sOldCustom, szCustomLen+1);
#endif
			}
		} else {
#ifdef _WIN32
			pReg->sCustom = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szCustomLen+1);
#else
			pReg->sCustom = (char *)malloc(szCustomLen+1);
#endif
		}
        memcpy(pReg->sCustom, sCustom, szCustomLen);
        pReg->sCustom[szCustomLen] = '\0';
	}
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void clsRegManager::Delete(RegUser * pReg, const bool &bFromGui/* = false*/) {
#else
void clsRegManager::Delete(RegUser * pReg, const bool &/*bFromGui = false*/) {
#endif
	if(clsServerManager::bServerRunning == true) {
        User * pRemovedUser = clsHashManager::mPtr->FindUser(pReg->sNick, strlen(pReg->sNick));

        if(pRemovedUser != NULL) {
            pRemovedUser->i32Profile = -1;
            if(((pRemovedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                clsUsers::mPtr->DelFromOpList(pRemovedUser->sNick);
                pRemovedUser->ui32BoolBits &= ~User::BIT_OPERATOR;

                if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true && (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                    pRemovedUser->SendFormat("clsRegManager::Delete", true, "$Quit %s|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                }
            }
        }
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && clsRegisteredUsersDialog::mPtr != NULL) {
        clsRegisteredUsersDialog::mPtr->RemoveReg(pReg);
    }
#endif

	Rem(pReg);

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->RegDeleted(pReg);
    }
#endif

    delete pReg;

    Save(true);
}
//---------------------------------------------------------------------------

void clsRegManager::Rem(RegUser * Reg) {
	RemFromTable(Reg);
    
    RegUser *prev, *next;
    prev = Reg->pPrev; next = Reg->pNext;

    if(prev == NULL) {
        if(next == NULL) {
            pRegListS = NULL;
            pRegListE = NULL;
        } else {
            next->pPrev = NULL;
            pRegListS = next;
        }
    } else if(next == NULL) {
        prev->pNext = NULL;
        pRegListE = prev;
    } else {
        prev->pNext = next;
        next->pPrev = prev;
    }
}
//---------------------------------------------------------------------------

void clsRegManager::RemFromTable(RegUser * Reg) {
    if(Reg->pHashTablePrev == NULL) {
        uint16_t ui16dx = 0;
        memcpy(&ui16dx, &Reg->ui32Hash, sizeof(uint16_t));

        if(Reg->pHashTableNext == NULL) {
            pTable[ui16dx] = NULL;
        } else {
            Reg->pHashTableNext->pHashTablePrev = NULL;
			pTable[ui16dx] = Reg->pHashTableNext;
        }
    } else if(Reg->pHashTableNext == NULL) {
        Reg->pHashTablePrev->pHashTableNext = NULL;
    } else {
        Reg->pHashTablePrev->pHashTableNext = Reg->pHashTableNext;
        Reg->pHashTableNext->pHashTablePrev = Reg->pHashTablePrev;
    }

	Reg->pHashTablePrev = NULL;
    Reg->pHashTableNext = NULL;
}
//---------------------------------------------------------------------------

RegUser* clsRegManager::Find(char * sNick, const size_t &szNickLen) {
    uint32_t ui32Hash = HashNick(sNick, szNickLen);

    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

    RegUser * cur = NULL,
        * next = pTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashTableNext;

		if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* clsRegManager::Find(User * u) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &u->ui32NickHash, sizeof(uint16_t));

	RegUser * cur = NULL,
        * next = pTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashTableNext;

		if(cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->sNick, u->sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* clsRegManager::Find(uint32_t ui32Hash, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

	RegUser * cur = NULL,
        * next = pTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashTableNext;

		if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}

//---------------------------------------------------------------------------

// alex82 ... Обновление времени последнего входа и времени онлайн для зарегистрированных юзеров
void clsRegManager::UpdateTimes(void) {
	User *next = clsUsers::mPtr->pListS;
	time_t curtime = time(NULL);

	while(next != NULL) {
		User *curUser = next;
		next = curUser->pNext;
		if(curUser->ui8State == User::STATE_ADDED && curUser->i32Profile != -1) {
			RegUser * pReg = clsRegManager::mPtr->Find(curUser);
			if(pReg != NULL) {
				// alex82 ... Чтобы избежать ошибок, связанных с переводом системного времени назад...
				if (curtime >= pReg->tLastEnter) {
					pReg->tOnlineTime = pReg->tOnlineTime+curtime-pReg->tLastEnter;
				}
				pReg->tLastEnter = curtime;
			}
		}
	}
}
//---------------------------------------------------------------------------

void clsRegManager::Load(void) {
	// alex82 ... вернули на место хранение аккаунтов в XML
    uint16_t iProfilesCount = (uint16_t)(clsProfileManager::mPtr->ui16ProfileCount-1);
    bool bIsBuggy = false;
	time_t curtime = time(NULL);

#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\RegisteredUsers.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/RegisteredUsers.xml").c_str());
#endif

    if(doc.LoadFile()) {
        TiXmlHandle cfg(&doc);
        TiXmlNode *registeredusers = cfg.FirstChild("RegisteredUsers").Node();
        if(registeredusers != NULL) {
            TiXmlNode *child = NULL;
            while((child = registeredusers->IterateChildren(child)) != NULL) {
				TiXmlNode *registereduser = child->FirstChild("Nick");

				if(registereduser == NULL || (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                char *nick = (char *)registereduser->Value();
                
				if(strlen(nick) > 64 || (registereduser = child->FirstChild("Password")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                char *pass = (char *)registereduser->Value();
                
				if(strlen(pass) > 64 || (registereduser = child->FirstChild("Profile")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

				uint16_t iProfile = (uint16_t)atoi(registereduser->Value());

				// alex82 ... Дата регистрации и последнего входа
				time_t regdate,lastenter,onlinetime;
				if ((registereduser = child->FirstChild("RegDate")) != NULL && (registereduser = registereduser->FirstChild()) != NULL)
					regdate = (time_t)atol(registereduser->Value());
				else {
					regdate = curtime;
					bIsBuggy = true;
				}
				if ((registereduser = child->FirstChild("LastEnter")) != NULL && (registereduser = registereduser->FirstChild()) != NULL)
					lastenter = (time_t)atol(registereduser->Value());
				else {
					lastenter = curtime;
					bIsBuggy = true;
				}
				if ((registereduser = child->FirstChild("OnlineTime")) != NULL && (registereduser = registereduser->FirstChild()) != NULL)
					onlinetime = (time_t)atol(registereduser->Value());
				else {
					onlinetime = 0;
					bIsBuggy = true;
				}

				if(iProfile > iProfilesCount) {
                    char msg[1024];
                    int imsgLen = sprintf(msg, "%s %s %s! %s %s.", clsLanguageManager::mPtr->sTexts[LAN_USER], nick, clsLanguageManager::mPtr->sTexts[LAN_HAVE_NOT_EXIST_PROFILE],
                        clsLanguageManager::mPtr->sTexts[LAN_CHANGED_PROFILE_TO], clsProfileManager::mPtr->ppProfilesTable[iProfilesCount]->sName);
					CheckSprintf(imsgLen, 1024, "clsRegManager::Load");

#ifdef _BUILD_GUI
					::MessageBox(NULL, msg, g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
#else
					AppendLog(msg);
#endif

                    iProfile = iProfilesCount;
                    bIsBuggy = true;
                }

                if(Find((char*)nick, strlen(nick)) == NULL) {
#ifdef HASH_PASS
					if (pass[0] != '|' || strlen(pass) != SHA256_DIGEST_BASE64_LENGTH)
					{
						char buff[SHA256_DIGEST_BASE64_LENGTH];
						SHA256_Base64((unsigned char *)pass, strlen(pass), (unsigned char *)buff);
						pass = buff;
						bIsBuggy = true;
					}
					else pass = pass+1;
#endif
					// alex82 ... Дата регистрации и последнего входа
					RegUser * newUser = RegUser::CreateReg(nick, strlen(nick), pass, strlen(pass), iProfile, regdate, lastenter, onlinetime);
                    if(newUser == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in clsRegManager::Load\n");
						
						exit(EXIT_FAILURE);
                    }
					if ((registereduser = child->FirstChild("Custom")) != NULL && (registereduser = registereduser->FirstChild()) != NULL) {
						char *custom = (char *)registereduser->Value();
						size_t custom_len = strlen(custom);
						if (custom_len > 0 && custom_len <= 256)
							SetCustom(newUser, custom, custom_len);
					}
					Add(newUser);
                } else {
                    char msg[1024];
                    int imsgLen = sprintf(msg, "%s %s %s! %s.", clsLanguageManager::mPtr->sTexts[LAN_USER], nick, clsLanguageManager::mPtr->sTexts[LAN_IS_ALREADY_IN_REGS], 
                        clsLanguageManager::mPtr->sTexts[LAN_USER_DELETED]);
					CheckSprintf(imsgLen, 1024, "hashRegMan::LoadXmlRegList2");

#ifdef _BUILD_GUI
					::MessageBox(NULL, msg, g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
#else
					AppendLog(msg);
#endif

                    bIsBuggy = true;
                }
            }
			if(bIsBuggy == true)
				Save();
        }
    }
	// alex82 ... Error processing
	else {
		if (doc.Error() && doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE){
			char msg[1024];
			int imsgLen = sprintf(msg, "Syntax error in cfg/RegisteredUsers.xml: %s Line: %d", doc.ErrorDesc(), doc.ErrorRow());
			CheckSprintf(imsgLen, 1024, "clsRegManager::Load");

#ifdef _BUILD_GUI
			::MessageBox(NULL, msg, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
			AppendLog(msg);
#endif
			exit(EXIT_FAILURE);
		} else {
			Save();
		}
	}
}
//---------------------------------------------------------------------------

void clsRegManager::Save(const bool &bSaveOnChange/* = false*/, const bool &bSaveOnTime/* = false*/) {
    if(bSaveOnTime == true && ui8SaveCalls == 0) {
        return;
    }

    ui8SaveCalls++;

    if(bSaveOnChange == true && ui8SaveCalls < 100) {
        return;
    }

    ui8SaveCalls = 0;

// alex82 ... вернули на место хранение аккаунтов в XML
#ifdef HASH_PASS
	char buff[SHA256_DIGEST_BASE64_LENGTH+1];
	buff[0] = '|';
	buff[SHA256_DIGEST_BASE64_LENGTH] = '\0';
#endif

#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\RegisteredUsers.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/RegisteredUsers.xml").c_str());
#endif
	// alex82 ... Charset changed
    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1251", "yes"));
    TiXmlElement registeredusers("RegisteredUsers");
    RegUser *next = pRegListS;
    while(next != NULL) {
        RegUser *curReg = next;
		next = curReg->pNext;
		
        TiXmlElement nick("Nick");
        nick.InsertEndChild(TiXmlText(curReg->sNick));
        
        TiXmlElement pass("Password");
#ifdef HASH_PASS
		memcpy(buff+1, curReg->sPass, SHA256_DIGEST_BASE64_LENGTH-1);
        pass.InsertEndChild(TiXmlText(buff));
#else
        pass.InsertEndChild(TiXmlText(curReg->sPass));
#endif
        
        TiXmlElement profile("Profile");
        profile.InsertEndChild(TiXmlText(string(curReg->ui16Profile).c_str()));

		// alex82 ... Дата регистрации и последнего входа
        TiXmlElement regdate("RegDate");
        regdate.InsertEndChild(TiXmlText(string(curReg->tRegDate).c_str()));

        TiXmlElement lastenter("LastEnter");
        lastenter.InsertEndChild(TiXmlText(string(curReg->tLastEnter).c_str()));

        TiXmlElement onlinetime("OnlineTime");
        onlinetime.InsertEndChild(TiXmlText(string(curReg->tOnlineTime).c_str()));

	    TiXmlElement custom("Custom");
		if (curReg->sCustom != NULL) {
	        custom.InsertEndChild(TiXmlText(curReg->sCustom));
		}
		
        TiXmlElement registereduser("RegisteredUser");
        registereduser.InsertEndChild(nick);
        registereduser.InsertEndChild(pass);
        registereduser.InsertEndChild(profile);
		// alex82 ... Дата регистрации и последнего входа
        registereduser.InsertEndChild(regdate);
        registereduser.InsertEndChild(lastenter);
		registereduser.InsertEndChild(onlinetime);
		// alex82 ... Хранение произвольной строки в профиле юзера
		if (curReg->sCustom != NULL) {
			registereduser.InsertEndChild(custom);
		}
        
        registeredusers.InsertEndChild(registereduser);
    }
    doc.InsertEndChild(registeredusers);
    doc.SaveFile();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsRegManager::AddRegCmdLine() {
	char sNick[66];

nick:
	printf("Please enter Nick for new Registered User (Maximal length 64 characters. Characters |, $ and space are not allowed): ");
	if(fgets(sNick, 66, stdin) != NULL) {
		char * sMatch = strchr(sNick, '\n');
		if(sMatch != NULL) {
			sMatch[0] = '\0';
		}

		sMatch = strpbrk(sNick, " $|");
		if(sMatch != NULL) {
			printf("Character '%c' is not allowed in Nick!\n", sMatch[0]);

			if(WantAgain() == false) {
				return;
			}

			goto nick;
		}

		size_t szLen = strlen(sNick);

		if(szLen == 0) {
			printf("No Nick specified!\n");

			if(WantAgain() == false) {
				return;
			}

			goto nick;
		}

		RegUser * pReg = Find(sNick, strlen(sNick));
		if(pReg != NULL) {
			printf("Registered user with nick '%s' already exist!\n", sNick);

			if(WantAgain() == false) {
				return;
			}

			goto nick;
		}
	} else {
		printf("Error reading Nick... ending.\n");
		exit(EXIT_FAILURE);
	}

	char sPassword[66];

password:
	printf("Please enter Password for new Registered User (Maximal length 64 characters. Character | is not allowed): ");
	if(fgets(sPassword, 66, stdin) != NULL) {
		char * sMatch = strchr(sPassword, '\n');
		if(sMatch != NULL) {
			sMatch[0] = '\0';
		}

		sMatch = strchr(sPassword, '|');
		if(sMatch != NULL) {
			printf("Character | is not allowed in Password!\n");

			if(WantAgain() == false) {
				return;
			}

			goto password;
		}

		size_t szLen = strlen(sPassword);

		if(szLen == 0) {
			printf("No Password specified!\n");

			if(WantAgain() == false) {
				return;
			}

			goto password;
		}
	} else {
		printf("Error reading Password... ending.\n");
		exit(EXIT_FAILURE);
	}

	printf("\nAvailable profiles: \n");
    for(uint16_t ui16i = 0; ui16i < clsProfileManager::mPtr->ui16ProfileCount; ui16i++) {
    	printf("%hu - %s\n", ui16i, clsProfileManager::mPtr->ppProfilesTable[ui16i]->sName);
    }

	uint16_t ui16Profile = 0;
	char sProfile[7];

profile:

	printf("Please enter Profile number for new Registered User: ");
	if(fgets(sProfile, 7, stdin) != NULL) {
		char * sMatch = strchr(sProfile, '\n');
		if(sMatch != NULL) {
			sMatch[0] = '\0';
		}

		uint8_t ui8Len = (uint8_t)strlen(sProfile);

		if(ui8Len == 0) {
			printf("No Profile specified!\n");

			if(WantAgain() == false) {
				return;
			}

			goto profile;
		}

		for(uint8_t ui8i = 0; ui8i < ui8Len; ui8i++) {
			if(isdigit(sProfile[ui8i]) == 0) {
				printf("Character '%c' is not valid number!\n", sProfile[ui8i]);

				if(WantAgain() == false) {
					return;
				}

				goto profile;
			}
		}

		ui16Profile = (uint16_t)atoi(sProfile);
		if(ui16Profile >= clsProfileManager::mPtr->ui16ProfileCount) {
			printf("Profile number %hu not exist!\n", ui16Profile);

			if(WantAgain() == false) {
				return;
			}

			goto profile;
		}
	} else {
		printf("Error reading Profile... ending.\n");
		exit(EXIT_FAILURE);
	}

	if(AddNew(sNick, sPassword, ui16Profile) == false) {
		printf("Error adding new Registered User... ending.\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Registered User with Nick '%s' Password '%s' and Profile '%hu' was added.", sNick, sPassword, ui16Profile);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
