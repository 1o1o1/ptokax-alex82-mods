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
#include "ProfileManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "PXBReader.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/ProfilesDialog.h"
    #include "../gui.win/RegisteredUserDialog.h"
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------
clsProfileManager * clsProfileManager::mPtr = NULL;
//---------------------------------------------------------------------------
static const char sPtokaXProfiles[] = "PtokaX Profiles";
static const size_t szPtokaXProfilesLen = sizeof(sPtokaXProfiles)-1;
//---------------------------------------------------------------------------

ProfileItem::ProfileItem() : sName(NULL) {
	for(uint16_t ui16i = 0; ui16i < PERMISSIONS_COUNT; ui16i++) {
        bPermissions[ui16i] = false;
    }
}
//---------------------------------------------------------------------------

ProfileItem::~ProfileItem() {
#ifdef _WIN32
    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sName) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate sName in ProfileItem::~ProfileItem\n");
    }
#else
	free(sName);
#endif
}
//---------------------------------------------------------------------------

void clsProfileManager::Load() {
#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\Profiles.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/Profiles.xml").c_str());
#endif
	if(doc.LoadFile() == false) {
         if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file Profiles.xml: %s Line: %d", doc.ErrorDesc(), doc.ErrorRow());
			CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsProfileManager::Load");
#ifdef _BUILD_GUI
			::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
			AppendLog(clsServerManager::pGlobalBuffer);
#endif
            exit(EXIT_FAILURE);
        }
	}

	TiXmlHandle cfg(&doc);
	TiXmlNode *profiles = cfg.FirstChild("Profiles").Node();
	if(profiles != NULL) {
		TiXmlNode *child = NULL;
		while((child = profiles->IterateChildren(child)) != NULL) {
			TiXmlNode *profile = child->FirstChild("Name");

			if(profile == NULL || (profile = profile->FirstChild()) == NULL) {
				continue;
			}

			const char *sName = profile->Value();

			if((profile = child->FirstChildElement("Permissions")) == NULL ||
				(profile = profile->FirstChild()) == NULL) {
				continue;
			}

			const char *sRights = profile->Value();

			ProfileItem * pNewProfile = CreateProfile(sName);

			// alex82 ... WTF o_O?
			for(uint8_t ui8i = 0; ui8i < strlen(sRights); ui8i++) {
				if (ui8i == PERMISSIONS_COUNT) {
					break;
				} else if (sRights[ui8i] == '1') {
					pNewProfile->bPermissions[ui8i] = true;
				} else {
					pNewProfile->bPermissions[ui8i] = false;
				}
			}
		}
	} else {
#ifdef _BUILD_GUI
		::MessageBox(NULL, clsLanguageManager::mPtr->sTexts[LAN_PROFILES_LOAD_FAIL], g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
		AppendLog(clsLanguageManager::mPtr->sTexts[LAN_PROFILES_LOAD_FAIL]);
#endif
		exit(EXIT_FAILURE);
	}
}
//---------------------------------------------------------------------------

clsProfileManager::clsProfileManager() : ppProfilesTable(NULL), ui16ProfileCount(0) {
#ifdef _WIN32
    if(FileExist((clsServerManager::sPath + "\\cfg\\Profiles.xml").c_str()) == true) {
#else
    if(FileExist((clsServerManager::sPath + "/cfg/Profiles.xml").c_str()) == true) {
#endif
        Load();
        return;
    } else {
	    const char * sProfileNames[] = { "Master", "Operator", "VIP", "Reg" };
	    const char * sProfilePermisions[] = {
	        "10011111111111111111111111111111111111111111101000111111",
	        "10011111101111111110011000111111111000000011101000111111",
	        "00000000000000011110000000000001100000000000000000000111",
	        "00000000000000000000000000000001100000000000000000000000"
	    };

		for(uint8_t ui8i = 0; ui8i < 4; ui8i++) {
			ProfileItem * pNewProfile = CreateProfile(sProfileNames[ui8i]);

			for(uint8_t ui8j = 0; ui8j < strlen(sProfilePermisions[ui8i]); ui8j++) {
				if(sProfilePermisions[ui8i][ui8j] == '1') {
					pNewProfile->bPermissions[ui8j] = true;
				} else {
					pNewProfile->bPermissions[ui8j] = false;
				}
			}
		}

	    SaveProfiles();
	}

}
//---------------------------------------------------------------------------

clsProfileManager::~clsProfileManager() {
    SaveProfiles();

    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
        delete ppProfilesTable[ui16i];
    }

#ifdef _WIN32
    if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ppProfilesTable) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate ProfilesTable in clsProfileManager::~clsProfileManager\n");
    }
#else
	free(ppProfilesTable);
#endif
    ppProfilesTable = NULL;
}
//---------------------------------------------------------------------------

void clsProfileManager::SaveProfiles() {
    char permisionsbits[PERMISSIONS_COUNT+1];

#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\Profiles.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/Profiles.xml").c_str());
#endif
	// alex82 ... Charset changed
    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1251", "yes"));
    TiXmlElement profiles("Profiles");

    for(uint16_t ui16j = 0; ui16j < ui16ProfileCount; ui16j++) {
        TiXmlElement name("Name");
        name.InsertEndChild(TiXmlText(ppProfilesTable[ui16j]->sName));
        
        for(uint16_t ui16i = 0; ui16i < sizeof(permisionsbits)-1; ui16i++) {
            if(ppProfilesTable[ui16j]->bPermissions[ui16i] == false) {
                permisionsbits[ui16i] = '0';
            } else {
                permisionsbits[ui16i] = '1';
            }
        }
        permisionsbits[PERMISSIONS_COUNT] = '\0';
        
        TiXmlElement permisions("Permissions");
        permisions.InsertEndChild(TiXmlText(permisionsbits));
        
        TiXmlElement profile("Profile");
        profile.InsertEndChild(name);
        profile.InsertEndChild(permisions);
        
        profiles.InsertEndChild(profile);
    }

    doc.InsertEndChild(profiles);
    doc.SaveFile();
}
//---------------------------------------------------------------------------

bool clsProfileManager::IsAllowed(User * u, const uint32_t &iOption) const {
    // profile number -1 = normal user/no profile assigned
    if(u->i32Profile == -1)
        return false;
        
    // return right of the profile
    return ppProfilesTable[u->i32Profile]->bPermissions[iOption];
}
//---------------------------------------------------------------------------

bool clsProfileManager::IsProfileAllowed(const int32_t &iProfile, const uint32_t &iOption) const {
    // profile number -1 = normal user/no profile assigned
    if(iProfile == -1)
        return false;
        
    // return right of the profile
    return ppProfilesTable[iProfile]->bPermissions[iOption];
}
//---------------------------------------------------------------------------

int32_t clsProfileManager::AddProfile(char * name) {
    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
		if(strcasecmp(ppProfilesTable[ui16i]->sName, name) == 0) {
            return -1;
        }
    }

    uint32_t ui32j = 0;
    while(true) {
        switch(name[ui32j]) {
            case '\0':
                break;
            case '|':
                return -2;
            default:
                if(name[ui32j] < 33) {
                    return -2;
                }
                
                ui32j++;
                continue;
        }

        break;
    }

    CreateProfile(name);

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->AddProfile();
    }
#endif

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }
#endif

    return (int32_t)(ui16ProfileCount-1);
}
//---------------------------------------------------------------------------

int32_t clsProfileManager::GetProfileIndex(const char * name) {
    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
		if(strcasecmp(ppProfilesTable[ui16i]->sName, name) == 0) {
            return ui16i;
        }
    }
    
    return -1;
}
//---------------------------------------------------------------------------

// RemoveProfileByName(name)
// returns: 0 if the name doesnot exists or is a default profile idx 0-3
//          -1 if the profile is in use
//          1 on success
int32_t clsProfileManager::RemoveProfileByName(char * name) {
    for(uint16_t ui16i = 0; ui16i < ui16ProfileCount; ui16i++) {
		if(strcasecmp(ppProfilesTable[ui16i]->sName, name) == 0) {
            return (RemoveProfile(ui16i) == true ? 1 : -1);
        }
    }
    
    return 0;
}

//---------------------------------------------------------------------------

bool clsProfileManager::RemoveProfile(const uint16_t &iProfile) {
    RegUser * curReg = NULL,
        * next = clsRegManager::mPtr->pRegListS;

    while(next != NULL) {
        curReg = next;
		next = curReg->pNext;

		if(curReg->ui16Profile == iProfile) {
            //Profile in use can't be deleted!
            return false;
        }
    }
    
    ui16ProfileCount--;

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->RemoveProfile(iProfile);
    }
#endif
    
    delete ppProfilesTable[iProfile];
    
	for(uint16_t ui16i = iProfile; ui16i < ui16ProfileCount; ui16i++) {
        ppProfilesTable[ui16i] = ppProfilesTable[ui16i+1];
    }

    // Update profiles for online users
    if(clsServerManager::bServerRunning == true) {
        User * curUser = NULL,
            * nextUser = clsUsers::mPtr->pListS;

        while(nextUser != NULL) {
            curUser = nextUser;
            nextUser = curUser->pNext;
            
            if(curUser->i32Profile > iProfile) {
                curUser->i32Profile--;
            }
        }
    }

    // Update profiles for registered users
    next = clsRegManager::mPtr->pRegListS;
    while(next != NULL) {
        curReg = next;
		next = curReg->pNext;
        if(curReg->ui16Profile > iProfile) {
            curReg->ui16Profile--;
        }
    }

    ProfileItem ** pOldTable = ppProfilesTable;
#ifdef _WIN32
    ppProfilesTable = (ProfileItem **)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable, ui16ProfileCount*sizeof(ProfileItem *));
#else
	ppProfilesTable = (ProfileItem **)realloc(pOldTable, ui16ProfileCount*sizeof(ProfileItem *));
#endif
    if(ppProfilesTable == NULL) {
        ppProfilesTable = pOldTable;

		AppendDebugLog("%s - [MEM] Cannot reallocate ProfilesTable in clsProfileManager::RemoveProfile\n");
    }

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif

    return true;
}
//---------------------------------------------------------------------------

ProfileItem * clsProfileManager::CreateProfile(const char * name) {
    ProfileItem ** pOldTable = ppProfilesTable;
#ifdef _WIN32
    if(ppProfilesTable == NULL) {
        ppProfilesTable = (ProfileItem **)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (ui16ProfileCount+1)*sizeof(ProfileItem *));
    } else {
        ppProfilesTable = (ProfileItem **)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable, (ui16ProfileCount+1)*sizeof(ProfileItem *));
    }
#else
	ppProfilesTable = (ProfileItem **)realloc(pOldTable, (ui16ProfileCount+1)*sizeof(ProfileItem *));
#endif
    if(ppProfilesTable == NULL) {
#ifdef _WIN32
        HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable);
#else
        free(pOldTable);
#endif
		AppendDebugLog("%s - [MEM] Cannot (re)allocate ProfilesTable in clsProfileManager::CreateProfile\n");
        exit(EXIT_FAILURE);
    }

    ProfileItem * pNewProfile = new (std::nothrow) ProfileItem();
    if(pNewProfile == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewProfile in clsProfileManager::CreateProfile\n");
        exit(EXIT_FAILURE);
    }
 
    size_t szLen = strlen(name);
#ifdef _WIN32
    pNewProfile->sName = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	pNewProfile->sName = (char *)malloc(szLen+1);
#endif
    if(pNewProfile->sName == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes in clsProfileManager::CreateProfile for pNewProfile->sName\n", (uint64_t)szLen);

        exit(EXIT_FAILURE);
    } 
    memcpy(pNewProfile->sName, name, szLen);
    pNewProfile->sName[szLen] = '\0';

    for(uint16_t ui16i = 0; ui16i < PERMISSIONS_COUNT; ui16i++) {
        pNewProfile->bPermissions[ui16i] = false;
    }
    
    ui16ProfileCount++;

    ppProfilesTable[ui16ProfileCount-1] = pNewProfile;

    return pNewProfile;
}
//---------------------------------------------------------------------------

void clsProfileManager::MoveProfileDown(const uint16_t &iProfile) {
    ProfileItem *first = ppProfilesTable[iProfile];
    ProfileItem *second = ppProfilesTable[iProfile+1];
    
    ppProfilesTable[iProfile+1] = first;
    ppProfilesTable[iProfile] = second;

    RegUser * curReg = NULL,
        * nextReg = clsRegManager::mPtr->pRegListS;

	while(nextReg != NULL) {
        curReg = nextReg;
		nextReg = curReg->pNext;

		if(curReg->ui16Profile == iProfile) {
			curReg->ui16Profile++;
		} else if(curReg->ui16Profile == iProfile+1) {
			curReg->ui16Profile--;
		}
	}

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->MoveDown(iProfile);
    }

    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif

    if(clsUsers::mPtr == NULL) {
        return;
    }

    User * curUser = NULL,
        * nextUser = clsUsers::mPtr->pListS;

	while(nextUser != NULL) {
        curUser = nextUser;
		nextUser = curUser->pNext;

		if(curUser->i32Profile == (int32_t)iProfile) {
			curUser->i32Profile++;
		} else if(curUser->i32Profile == (int32_t)(iProfile+1)) {
			curUser->i32Profile--;
		}
    }
}
//---------------------------------------------------------------------------

void clsProfileManager::MoveProfileUp(const uint16_t &iProfile) {
    ProfileItem *first = ppProfilesTable[iProfile];
    ProfileItem *second = ppProfilesTable[iProfile-1];
    
    ppProfilesTable[iProfile-1] = first;
    ppProfilesTable[iProfile] = second;

	RegUser * curReg = NULL,
        * nextReg = clsRegManager::mPtr->pRegListS;

	while(nextReg != NULL) {
		curReg = nextReg;
		nextReg = curReg->pNext;

        if(curReg->ui16Profile == iProfile) {
			curReg->ui16Profile--;
		} else if(curReg->ui16Profile == iProfile-1) {
			curReg->ui16Profile++;
		}
	}

#ifdef _BUILD_GUI
    if(clsProfilesDialog::mPtr != NULL) {
        clsProfilesDialog::mPtr->MoveUp(iProfile);
    }

    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif

    if(clsUsers::mPtr == NULL) {
        return;
    }

    User * curUser = NULL,
        * nextUser = clsUsers::mPtr->pListS;

    while(nextUser != NULL) {
        curUser = nextUser;
		nextUser = curUser->pNext;

		if(curUser->i32Profile == (int32_t)iProfile) {
			curUser->i32Profile--;
		} else if(curUser->i32Profile == (int32_t)(iProfile-1)) {
			curUser->i32Profile++;
		}
    }
}
//---------------------------------------------------------------------------

void clsProfileManager::ChangeProfileName(const uint16_t &iProfile, char * sName, const size_t &szLen) {
    char * sOldName = ppProfilesTable[iProfile]->sName;

#ifdef _WIN32
    ppProfilesTable[iProfile]->sName = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldName, szLen+1);
#else
	ppProfilesTable[iProfile]->sName = (char *)realloc(sOldName, szLen+1);
#endif
    if(ppProfilesTable[iProfile]->sName == NULL) {
        ppProfilesTable[iProfile]->sName = sOldName;

		AppendDebugLogFormat("[MEM] Cannot reallocate %" PRIu64 " bytes in clsProfileManager::ChangeProfileName for ProfilesTable[iProfile]->sName\n", (uint64_t)szLen);

        return;
    } 
	memcpy(ppProfilesTable[iProfile]->sName, sName, szLen);
    ppProfilesTable[iProfile]->sName[szLen] = '\0';

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->UpdateProfiles();
    }

	if(clsRegisteredUsersDialog::mPtr != NULL) {
		clsRegisteredUsersDialog::mPtr->UpdateProfiles();
	}
#endif
}
//---------------------------------------------------------------------------

void clsProfileManager::ChangeProfilePermission(const uint16_t &iProfile, const size_t &szId, const bool &bValue) {
    ppProfilesTable[iProfile]->bPermissions[szId] = bValue;
}
//---------------------------------------------------------------------------
