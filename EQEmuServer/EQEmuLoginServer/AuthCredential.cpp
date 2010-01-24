/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2009  EQEMu Development Team (http://eqemulator.net)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "AuthCredential.h"
#include "../common/MiscFunctions.h"

AuthCredential::AuthCredential() {
	SetAccountID(0);
	SetIPAddress(0);
	SetPort(0);
	time(&this->_createTime);
	play_server_id = 0;
	play_sequence_id = 0;
}

void AuthCredential::AddCredential(std::string AccountUserName, uint32 AccountID, uint32 IPAddress, uint16 Port) {
	SetAccountUserName(AccountUserName);
	SetAccountID(AccountID);
	SetIPAddress(IPAddress);
	SetPort(Port);
}

void AuthCredential::GenerateKey()
{
	_key.clear();
	int count = 0;
	while(count < 10)
	{
		int pre_sel = MakeRandomInt(0, 1);
		if(pre_sel == 1) //number
		{
			char outC = MakeRandomInt(0x30, 0x39);
			_key.append((const char*)&outC, sizeof(char));
		}
		else //letter
		{
			char outC = MakeRandomInt(0x41, 0x5A);
			_key.append((const char*)&outC, sizeof(char));
		}
		count++;
	}
}