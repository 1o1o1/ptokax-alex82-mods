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
#ifndef hashRegManagerH
#define hashRegManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct RegUser {
	time_t tLastBadPass;

    char * sNick;

    union {
        char * sPass;
        uint8_t * ui8PassHash;
    };

    RegUser * pPrev, * pNext;
    RegUser * pHashTablePrev, * pHashTableNext;

    uint32_t ui32Hash;

    uint16_t ui16Profile;
	
	// alex82 ... ���� ����������� � ���������� �����
	time_t tRegDate;
	time_t tLastEnter;
	time_t tOnlineTime;
	// alex82 ... �������� ������������ ������ � ������� �����
	char *sCustom;

    uint8_t ui8BadPassCount;

    RegUser();
    ~RegUser();

    RegUser(const RegUser&);
    const RegUser& operator=(const RegUser&);

    static RegUser * CreateReg(char * sRegNick, size_t szRegNickLen, char * sRegPassword, size_t szRegPassLen, const uint16_t &ui16RegProfile, const time_t &tRegDate, const time_t &tLastEnter, const time_t &tOnlineTime);
}; 
//---------------------------------------------------------------------------

class clsRegManager {
private:
    RegUser * pTable[65536];

    uint8_t ui8SaveCalls;

    clsRegManager(const clsRegManager&);
    const clsRegManager& operator=(const clsRegManager&);

    void LoadXML();
public:
    static clsRegManager * mPtr;

    RegUser * pRegListS, * pRegListE;

    clsRegManager(void);
    ~clsRegManager(void);

    bool AddNew(char * sNick, char * sPasswd, const uint16_t &iProfile);

    void Add(RegUser * Reg);
    void Add2Table(RegUser * Reg);
    static void ChangeReg(RegUser * pReg, char * sNewPasswd, const uint16_t &ui16NewProfile);
	// alex82 ... �������� ������������ ������ � ������� �����
	void SetCustom(RegUser * pReg, char * sCustom, const size_t &szCustomLen);
    void Delete(RegUser * pReg, const bool &bFromGui = false);
    void Rem(RegUser * Reg);
    void RemFromTable(RegUser * Reg);

    RegUser * Find(char * sNick, const size_t &szNickLen);
    RegUser * Find(User * u);
    RegUser * Find(uint32_t hash, char * sNick);

	// alex82 ... ���������� ������� ���������� ����� � ������� ������ ��� ������������������ ������
	void UpdateTimes(void);

    void Load(void);
    void Save(const bool &bSaveOnChange = false, const bool &bSaveOnTime = false);

	void AddRegCmdLine();
};
//---------------------------------------------------------------------------

#endif
