/*
	EQEMu:  Everquest Server Emulator

	Copyright (C) 2001-2008 EQEMu Development Team (http://eqemulator.net)

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

#include "../common/debug.h"
#include "../common/opcodemgr.h"
#include "../common/EQStreamFactory.h"
#include "../common/rulesys.h"
#include "../common/servertalk.h"
#include "database.h"
#include "queryservconfig.h"
#include "worldserver.h"
#include <list>
#include <signal.h>

volatile bool RunLoops = true;

uint32 MailMessagesSent = 0;
uint32 ChatMessagesSent = 0;

TimeoutManager          timeout_manager;

Database database;

string WorldShortName;

RuleManager *rules = new RuleManager();

const queryservconfig *Config;

WorldServer *worldserver = 0;


void CatchSignal(int sig_num) {

	RunLoops = false;

	if(worldserver)
		worldserver->Disconnect();
}

int main() {

	Timer InterserverTimer(INTERSERVER_TIMER); // does auto-reconnect

	_log(QUERYSERV__INIT, "Starting EQEmu Universal Chat Server.");

	if (!queryservconfig::LoadConfig()) {

		_log(QUERYSERV__INIT, "Loading server configuration failed.");

		return(1);
	}

	Config = queryservconfig::get();

	if(!load_log_settings(Config->LogSettingsFile.c_str()))
		_log(QUERYSERV__INIT, "Warning: Unable to read %s", Config->LogSettingsFile.c_str());
	else
		_log(QUERYSERV__INIT, "Log settings loaded from %s", Config->LogSettingsFile.c_str());

	WorldShortName = Config->ShortName;

	_log(QUERYSERV__INIT, "Connecting to MySQL...");

	if (!database.Connect(
		Config->DatabaseHost.c_str(),
		Config->DatabaseUsername.c_str(),
		Config->DatabasePassword.c_str(),
		Config->DatabaseDB.c_str(),
		Config->DatabasePort)) {
		_log(WORLD__INIT_ERR, "Cannot continue without a database connection.");
		return(1);
	}

	char tmp[64];

	if (database.GetVariable("RuleSet", tmp, sizeof(tmp)-1)) {
		_log(WORLD__INIT, "Loading rule set '%s'", tmp);
		if(!rules->LoadRules(&database, tmp)) {
			_log(QUERYSERV__ERROR, "Failed to load ruleset '%s', falling back to defaults.", tmp);
		}
	} else {
		if(!rules->LoadRules(&database, "default")) {
			_log(QUERYSERV__INIT, "No rule set configured, using default rules");
		} else {
			_log(QUERYSERV__INIT, "Loaded default rule set 'default'", tmp);
		}
	}

	if (signal(SIGINT, CatchSignal) == SIG_ERR)	{
		_log(QUERYSERV__ERROR, "Could not set signal handler");
		return 0;
	}
	if (signal(SIGTERM, CatchSignal) == SIG_ERR)	{
		_log(QUERYSERV__ERROR, "Could not set signal handler");
		return 0;
	}

	worldserver = new WorldServer;

	worldserver->Connect();

	while(RunLoops) {

		Timer::SetCurrentTime();

		if (InterserverTimer.Check()) {
			if (worldserver->TryReconnect() && (!worldserver->Connected()))
				worldserver->AsyncConnect();
		}
		worldserver->Process();

		timeout_manager.CheckTimeouts();

		Sleep(100);
	}
}

void UpdateWindowTitle(char* iNewTitle) {
#ifdef _WINDOWS
        char tmp[500];
        if (iNewTitle) {
                snprintf(tmp, sizeof(tmp), "QueryServ: %s", iNewTitle);
        }
        else {
                snprintf(tmp, sizeof(tmp), "QueryServ");
        }
        SetConsoleTitle(tmp);
#endif
}