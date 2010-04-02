/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2006  EQEMu Development Team (http://eqemulator.net)

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
#ifndef __EQEmuConfig_H
#define __EQEmuConfig_H

#include "XMLParser.h"

class EQEmuConfig : public XMLParser {
public:
	virtual string GetByName(const string &var_name) const;

	// From <world/>
	string ShortName;
	string LongName;
	string WorldAddress;
	string LocalAddress;
	string LoginHost;
	string LoginAccount;
	string LoginPassword;
	uint16 LoginPort;
	bool Locked;
	uint16 WorldTCPPort;
	string WorldIP;
	bool TelnetEnabled;
	sint32 MaxClients;
	bool WorldHTTPEnabled;
	uint16 WorldHTTPPort;
	string WorldHTTPMimeFile;
	string SharedKey;
	
	// From <chatserver/>
	string ChatHost;
	uint16 ChatPort;
	bool UseIRC;
	string ChannelToOutput;
	string EQChannelToOutput;
	string ChatIRCHost;
	uint16 ChatIRCPort;
	string ChatIRCNick;

	// From <mailserver/>
	string MailHost;
	uint16 MailPort;

	// From <database/>
	string DatabaseHost;
	string DatabaseUsername;
	string DatabasePassword;
	string DatabaseDB;
	uint16 DatabasePort;

	// From <files/>
	string SpellsFile;
	string OpCodesFile;
	string EQTimeFile;
	string LogSettingsFile;

	// From <directories/>
	string MapDir;
	string QuestDir;
	string PluginDir;
	
	// From <launcher/>
	string LogPrefix;
	string LogSuffix;
	string ZoneExe;
	uint32 RestartWait;
	uint32 TerminateWait;
	uint32 InitialBootWait;
	uint32 ZoneBootInterval;

	// From <zones/>
	uint16 ZonePortLow;
	uint16 ZonePortHigh;
	uint8 DefaultStatus;

//	uint16 DynamicCount;

//	map<string,uint16> StaticZones;
	
protected:

	static EQEmuConfig *_config;

	static string ConfigFile;

#define ELEMENT(name) \
	void do_##name(TiXmlElement *ele);
	#include "EQEmuConfig_elements.h"


	EQEmuConfig() {
		// import the needed handler prototypes
#define ELEMENT(name) \
		Handlers[#name]=(ElementHandler)&EQEmuConfig::do_##name;
	#include "EQEmuConfig_elements.h"

		// Set sane defaults

		// Login server
		LoginHost="eqemulator.net";
		LoginPort=5998;

		// World
		//strcpy(lsi->name, Config->LongName.c_str());
		Locked=false;
		WorldTCPPort=9000;
		TelnetEnabled=false;
		WorldHTTPEnabled=false;
		WorldHTTPPort=9080;
		WorldHTTPMimeFile="mime.types";
		SharedKey = "";	//blank disables authentication

		// Mail
		ChatHost="eqchat.eqemulator.net";
		ChatPort=7778;
		UseIRC=false;
		ChannelToOutput="#InterServerChat";
		EQChannelToOutput="General";
		ChatIRCHost="eqnet.eqemulator.net";
		ChatIRCPort=6667;
		ChatIRCNick="EQEmuChatBot";


		// Mail
		MailHost="eqmail.eqemulator.net";
		MailPort=7779;

		// Mysql
		DatabaseHost="localhost";
		DatabasePort=3306;
		DatabaseUsername="eq";
		DatabasePassword="eq";
		DatabaseDB="eq";

		// Files
		SpellsFile="spells_us.txt";
		OpCodesFile="opcodes.conf";
		EQTimeFile="eqtime.cfg";
		LogSettingsFile="log.ini";

		// Dirs
		MapDir="Maps";
		QuestDir="quests";
		PluginDir="plugins";
		
		// Launcher
		LogPrefix = "logs/zone-";
		LogSuffix = ".log";
		RestartWait = 10000;		//milliseconds
		TerminateWait = 10000;		//milliseconds
		InitialBootWait = 20000;	//milliseconds
		ZoneBootInterval = 2000;	//milliseconds
#ifdef WIN32
		ZoneExe = "zone.exe";
#else
		ZoneExe = "./zone";
#endif
		
		// Zones
		ZonePortLow=7000;
		ZonePortHigh=7999;
		DefaultStatus=0;
		
		// For where zones need to connect to.
		WorldIP="127.0.0.1";
		
		// Dynamics to start
		//DynamicCount=5;
		
		MaxClients=-1;
		
	}
	virtual ~EQEmuConfig() {}

public:

	// Produce a const singleton
	static const EQEmuConfig *get() {
		if (_config == NULL) 
			LoadConfig();
		return(_config);
	}

	// Allow the use to set the conf file to be used.
	static void SetConfigFile(string file) { EQEmuConfig::ConfigFile = file; }

	// Load the config
	static bool LoadConfig() {
		if (_config != NULL)
			delete _config;
		_config=new EQEmuConfig;

		return _config->ParseFile(EQEmuConfig::ConfigFile.c_str(),"server");
	}

	void Dump() const;
};

#endif