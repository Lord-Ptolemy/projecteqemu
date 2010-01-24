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
#include "../common/MiscFunctions.h"

#include "ucsconfig.h"
#include "clientlist.h"
#include "database.h"
#include "chatchannel.h"

#include "../common/EQStreamFactory.h"
#include "../common/EmuTCPConnection.h"
#include "../common/EmuTCPServer.h"
#include <list>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>

using namespace std;

extern Database database;
extern string WorldShortName;
extern string GetMailPrefix();
extern ChatChannelList *ChannelList;
extern Clientlist *CL;
extern uint32 ChatMessagesSent;
extern uint32 MailMessagesSent;
extern ucsconfig *Config;
const bool UseIRC = ucsconfig::get()->UseIRC;
extern IRC conn;

typedef int (Client::*pt2Member)(char * params, irc_reply_data * hostd, void * conn);

/* maintains the loop for processing the other two IRC commands */
#ifdef WIN32
void IRCConnect(void* irc_conn) {
#else	//not Windows
void *IRCConnect(void* irc_conn) {
#endif
		_log(IRC__TRACE, "IRCConnect(): Casting irc_conn to conn.");
		IRC* conn = (IRC*)irc_conn;	//lets us use the connection passed in the arguments of the thread
		_log(IRC__TRACE, "IRCConnect(): irc_conn cast successfully, starting IRC::message_loop().");
		conn->message_loop();
}

int end_of_motd(char* params, irc_reply_data* hostd, void* conn) /* hooks END OF MOTD message from IRC */
	{
		IRC* irc_conn=(IRC*)conn;
	
		const ucsconfig *Config=ucsconfig::get();

		char name[128];

		strcpy(name,Config->ChannelToOutput.c_str());
			

		irc_conn->join(name);

		return 0;
	}

std::string GetCleanMessageFromIRC(std::string in) {
	std::string out = in;	//our return string

	if (in.empty())	//if there's nothing there, we don't really need to do the rest of this
		return in;

	if (in.at(0) == ':')	//get rid of that annoying colon at the beginning
		out = in.substr(1);

	//TODO: convert ACTION to an actual emote, or something similar
	//

	return out;
}

/*int triggers(char*params,irc_reply_data*hostd,void*conn) //hooks privmsg to your specified channel
{

	IRC* irc_conn=(IRC*)conn;
	

	//const ucsconfig *Config=ucsconfig::get();

	string ChannelName = Config->EQChannelToOutput;

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	char name[128];
	strcpy(name,Config->ChannelToOutput.c_str());

	if(!strcmp(params,":!rejoin"))
	{
		irc_conn->join(name);
	}	

	string parame = params;
	string IRCName = hostd->nick;

	if(!strcmp(hostd->target,name))
	{
		RequiredChannel->SendMessageToChannelFromIRC(GetCleanMessageFromIRC(parame), IRCName); // I got an IRC command, now let's send it
		
	}
	return 0;
	
}*/


int LookupCommand(const char *ChatCommand) {

	if(!ChatCommand) return -1;

	for(int i = 0; i < CommandEndOfList; i++) {

		if(!strcasecmp(Commands[i].CommandString, ChatCommand))
			return Commands[i].CommandCode;
	}

	return -1;
}

void Client::SendUptime() {

	int32 ms = Timer::GetCurrentTime();
	int32 d = ms / 86400000;
	ms -= d * 86400000;
	int32 h = ms / 3600000;
	ms -= h * 3600000;
	int32 m = ms / 60000;
	ms -= m * 60000;
	int32 s = ms / 1000;

	char *Buffer = NULL;

	MakeAnyLenString(&Buffer, "UCS has been up for %02id %02ih %02im %02is", d, h, m, s);
	GeneralChannelMessage(Buffer);
	safe_delete_array(Buffer);
	MakeAnyLenString(&Buffer, "Chat Messages Sent: %i, Mail Messages Sent: %i", ChatMessagesSent, MailMessagesSent);
	GeneralChannelMessage(Buffer);
	safe_delete_array(Buffer);
}

vector<string> ParseRecipients(string RecipientString) {

	// This method parses the Recipient List in the mailto command, which can look like this example:
	//
	// "Baalinor <SOE.EQ.BTG2.Baalinor>, 
	// -Friends <SOE.EQ.BTG2.Playedtest SOE.EQ.BTG2.Dyetest>, 
	// Guild <SOE.EQ.BTG2.Dsfvxcbcx SOE.EQ.BTG2.Necronor>, SOE.EQ.BTG2.luccerathe, SOE.EQ.BTG2.codsas
	//
	// First, it splits it up at commas, so it looks like this:
	//
	// Baalinor <SOE.EQ.BTG2.Baalinor>
	// -Friends <SOE.EQ.BTG2.Playedtest SOE.EQ.BTG2.Dyetest>
	// Guild <SOE.EQ.BTG2.Dsfvxcbcx SOE.EQ.BTG2.Necronor>
	// SOE.EQ.BTG2.luccerathe
	// SOE.EQ.BTG2
	//
	// Then, if an entry has a '<' in it, it extracts the text between the < and >
	// If the text between the < and > has a space in it, then there are multiple addresses in there, so those are extracted.
	//
	// The prefix (SOE.EQ.<Shortname>) is discarded, the names are normalised so they begin with a single upper case character
	// followed by lower case.
	//
	// The vector is sorted and any duplicates discarded, so the vector we return, in our example, looks like this:
	//
	// Baalinor
	// -Playedtest
	// -Dyetest
	// Dsfvxcbcx
	// Necronor
	// Luccerathe
	// Codsas
	//
	// The '-' prefix indicates 'Secret To' (like BCC:)
	//
	vector<string> RecipientList;

	string Secret;

	string::size_type CurrentPos, Comma, FirstLT, LastGT, Space, LastPeriod;

	CurrentPos = 0;

	while(CurrentPos != string::npos) {

		Comma = RecipientString.find_first_of(",", CurrentPos);

		if(Comma == string::npos) {

			RecipientList.push_back(RecipientString.substr(CurrentPos));

			break;
		}
		RecipientList.push_back(RecipientString.substr(CurrentPos, Comma - CurrentPos));

		CurrentPos = Comma + 2;
	}

	vector<string>::iterator Iterator;

	Iterator = RecipientList.begin();

	while(Iterator != RecipientList.end()) {

		if((*Iterator)[0] == '-') {

			Secret = "-";

			while((*Iterator)[0] == '-')
				(*Iterator) = (*Iterator).substr(1);
		}
		else
			Secret = "";

		FirstLT = (*Iterator).find_first_of("<");

		if(FirstLT != string::npos) {

			LastGT = (*Iterator).find_last_of(">");

			if(LastGT != string::npos) {

				(*Iterator) = (*Iterator).substr(FirstLT + 1, LastGT - FirstLT - 1);

				if((*Iterator).find_first_of(" ") != string::npos) {

					string Recips = (*Iterator);

					RecipientList.erase(Iterator);

					CurrentPos = 0;

					while(CurrentPos != string::npos) {

						Space = Recips.find_first_of(" ", CurrentPos);

						if(Space == string::npos) {

							RecipientList.push_back(Secret + Recips.substr(CurrentPos));

							break;
						}
						RecipientList.push_back(Secret + Recips.substr(CurrentPos, 
									Space - CurrentPos));
						CurrentPos = Space + 1;
					}
					Iterator = RecipientList.begin();

					continue;
				}
			}
		}

		
		(*Iterator) = Secret + (*Iterator);
				
		Iterator++;

	}

	for(Iterator = RecipientList.begin(); Iterator != RecipientList.end(); Iterator++) {

		if((*Iterator).length() > 0) {

			if((*Iterator)[0] == '-')
				Secret = "-";
			else
				Secret = "";

			LastPeriod = (*Iterator).find_last_of(".");

			if(LastPeriod != string::npos)  {

				(*Iterator) = (*Iterator).substr(LastPeriod + 1);
			
				for(unsigned int i = 0; i < (*Iterator).length(); i++) {

					if(i == 0)
						(*Iterator)[i] = toupper((*Iterator)[i]);
					else
						(*Iterator)[i] = tolower((*Iterator)[i]);
				}

				(*Iterator) = Secret + (*Iterator);
			}
		}

	}

	sort(RecipientList.begin(), RecipientList.end());

	vector<string>::iterator new_end_pos = unique(RecipientList.begin(), RecipientList.end());

	RecipientList.erase(new_end_pos, RecipientList.end());

	return RecipientList;

}

static void ProcessMailTo(Client *c, string MailMessage) {

	_log(UCS__TRACE, "MAILTO: From %s, %s", c->MailBoxName().c_str(), MailMessage.c_str());
		
	vector<string> Recipients;

	string::size_type FirstQuote = MailMessage.find_first_of("\"", 0);

	string::size_type NextQuote = MailMessage.find_first_of("\"", FirstQuote + 1);

	string RecipientsString = MailMessage.substr(FirstQuote+1, NextQuote-FirstQuote - 1);

	Recipients = ParseRecipients(RecipientsString);

	// Now extract the subject field. This is in quotes if it is more than one word
	//
	string Subject;

	string::size_type SubjectStart = NextQuote + 2;

	string::size_type SubjectEnd;

	if(MailMessage.substr(SubjectStart, 1) == "\"") {

		SubjectEnd = MailMessage.find_first_of("\"", SubjectStart + 1);

		Subject = MailMessage.substr(SubjectStart + 1, SubjectEnd - SubjectStart - 1);

		SubjectEnd += 2;

	}
	else {
		SubjectEnd = MailMessage.find_first_of(" ", SubjectStart);

		Subject = MailMessage.substr(SubjectStart, SubjectEnd - SubjectStart);

		SubjectEnd++;

	}
	string Body = MailMessage.substr(SubjectEnd);

	bool Success = true;

	RecipientsString.clear();

	int VisibleRecipients = 0;

	for(unsigned int i = 0; i<Recipients.size(); i++) {

		if(Recipients[i][0] == '-') {

			Recipients[i] = Recipients[i].substr(1);

		}
		else {
			if(VisibleRecipients > 0)

				RecipientsString += ", ";

			VisibleRecipients++;

			RecipientsString = RecipientsString + GetMailPrefix() + Recipients[i];
		}
	}
	if(VisibleRecipients == 0)
		RecipientsString = "<UNDISCLOSED RECIPIENTS>";

	for(unsigned int i=0; i<Recipients.size(); i++) {

		if(!database.SendMail(Recipients[i], c->MailBoxName(), Subject, Body, RecipientsString)) {

			_log(UCS__ERROR, "Failed in SendMail(%s, %s, %s, %s)", Recipients[i].c_str(),
					  c->MailBoxName().c_str(), Subject.c_str(), RecipientsString.c_str());

			int PacketLength = 10 + Recipients[i].length() + Subject.length();

			// Failure
			EQApplicationPacket *outapp = new EQApplicationPacket(OP_MailDeliveryStatus, PacketLength);

			char *PacketBuffer = (char *)outapp->pBuffer;

			VARSTRUCT_ENCODE_STRING(PacketBuffer, "1");
			VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x20);
			VARSTRUCT_ENCODE_STRING(PacketBuffer, Recipients[i].c_str());
			VARSTRUCT_ENCODE_STRING(PacketBuffer, Subject.c_str());
			VARSTRUCT_ENCODE_STRING(PacketBuffer, "0");
			VARSTRUCT_ENCODE_TYPE(uint16, PacketBuffer, 0x3237);
			VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x0);

			_pkt(UCS__PACKETS, outapp);

			c->QueuePacket(outapp);

			safe_delete(outapp);

			Success = false;
		}
	}
						
	if(Success) {
		// Success
		EQApplicationPacket *outapp = new EQApplicationPacket(OP_MailDeliveryStatus, 10);

		char *PacketBuffer = (char *)outapp->pBuffer;

		VARSTRUCT_ENCODE_STRING(PacketBuffer, "1");
		VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0);
		VARSTRUCT_ENCODE_STRING(PacketBuffer, "test"); // Doesn't matter what we send in this text field.
		VARSTRUCT_ENCODE_STRING(PacketBuffer, "1");

		_pkt(UCS__PACKETS, outapp);

		c->QueuePacket(outapp);

		safe_delete(outapp);
	}


}

static void ProcessSetMessageStatus(string SetMessageCommand) {

	int MessageNumber;

	int Status;

	switch(SetMessageCommand[0]) {

		case 'R': // READ
			Status = 3;
			break;

		case 'T': // TRASH
			Status = 4;
			break;

		default: // DELETE
			Status = 0;

	}
	string::size_type NumStart = SetMessageCommand.find_first_of("123456789");

	while(NumStart != string::npos) {

		string::size_type NumEnd = SetMessageCommand.find_first_of(" ", NumStart);

		if(NumEnd == string::npos) {

			MessageNumber = atoi(SetMessageCommand.substr(NumStart).c_str());

			database.SetMessageStatus(MessageNumber, Status);

			break;
		}

		MessageNumber = atoi(SetMessageCommand.substr(NumStart, NumEnd-NumStart).c_str());

		database.SetMessageStatus(MessageNumber, Status);

		NumStart = SetMessageCommand.find_first_of("123456789", NumEnd);
	}
}

static void ProcessCommandBuddy(Client *c, string Buddy) {

	_log(UCS__TRACE, "Received buddy command with parameters %s", Buddy.c_str());
	c->GeneralChannelMessage("Buddy list modified");

	uint8 SubAction = 1;

	if(Buddy.substr(0, 1) == "-")
		SubAction = 0;

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_Buddy, Buddy.length() + 2);
	char *PacketBuffer = (char *)outapp->pBuffer;
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, SubAction);

	if(SubAction == 1) {
		VARSTRUCT_ENCODE_STRING(PacketBuffer, Buddy.c_str());
		database.AddFriendOrIgnore(c->GetCharID(), 1, Buddy);
	}
	else {
		VARSTRUCT_ENCODE_STRING(PacketBuffer, Buddy.substr(1).c_str());
		database.RemoveFriendOrIgnore(c->GetCharID(), 1, Buddy.substr(1));
	}

	_pkt(UCS__PACKETS, outapp);
	c->QueuePacket(outapp);

	safe_delete(outapp);

}

static void ProcessCommandIgnore(Client *c, string Ignoree) {

	_log(UCS__TRACE, "Received ignore command with parameters %s", Ignoree.c_str());
	c->GeneralChannelMessage("Ignore list modified");

	uint8 SubAction = 0;

	if(Ignoree.substr(0, 1) == "-") {
		SubAction = 1;
		Ignoree = Ignoree.substr(1);

		// Strip off the SOE.EQ.<shortname>.
		//
		string CharacterName;

		string::size_type LastPeriod = Ignoree.find_last_of(".");

		if(LastPeriod == string::npos)
			CharacterName = Ignoree;
		else
			CharacterName = Ignoree.substr(LastPeriod + 1);

		database.RemoveFriendOrIgnore(c->GetCharID(), 0, CharacterName);

	}
	else
	{
		database.AddFriendOrIgnore(c->GetCharID(), 0, Ignoree);
		Ignoree = "SOE.EQ." + WorldShortName + "." + Ignoree;
	}

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_Ignore, Ignoree.length() + 2);
	char *PacketBuffer = (char *)outapp->pBuffer;
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, SubAction);

	VARSTRUCT_ENCODE_STRING(PacketBuffer, Ignoree.c_str());

	_pkt(UCS__PACKETS, outapp);
	c->QueuePacket(outapp);

	safe_delete(outapp);

}
Clientlist::Clientlist(int ChatPort) {

	chatsf = new EQStreamFactory(ChatStream, ChatPort);

	ChatOpMgr = new RegularOpcodeManager;

	if(!ChatOpMgr->LoadOpcodes("mail_opcodes.conf"))
		exit(1);

	if (chatsf->Open())
		_log(UCS__INIT,"Client (UDP) Chat listener started on port %i.", ChatPort);
	else {
		_log(UCS__ERROR,"Failed to start client (UDP) listener (port %-4i)", ChatPort);

		exit(1);
	}
}

Client::Client(EQStream *eqs) {

	ClientStream = eqs; 

	CurrentMailBox = 0;

	Announce = false;

	Status = 0;

	HideMe = 0;

	AccountID = 0;

	AllowInvites = true;
	Revoked = false;

	for(int i = 0; i < MAX_JOINED_CHANNELS ; i++)
		JoinedChannels[i] = NULL;

	TotalKarma = 0;
	AttemptedMessages = 0;
	ForceDisconnect = false;

	AccountGrabUpdateTimer = new Timer(60000); //check every minute
	GlobalChatLimiterTimer = new Timer(RuleI(Chat, IntervalDurationMS));

	TypeOfConnection = ConnectionTypeUnknown;

	//IRC
	//we need to initialize this, otherwise we're just pulling random values from memory
	IRCThreadPtr = 0;
}

Client::~Client() {

	CloseConnection();

	LeaveAllChannels(false);

	if(AccountGrabUpdateTimer)
	{
		delete AccountGrabUpdateTimer;
		AccountGrabUpdateTimer = NULL;
	}

	if(GlobalChatLimiterTimer)
	{
		delete GlobalChatLimiterTimer;
		GlobalChatLimiterTimer = NULL;
	}
}

void Client::CloseConnection() {

	ClientStream->RemoveData();

	ClientStream->Close();

	ClientStream->ReleaseFromUse();
}

void Clientlist::CheckForStaleConnections(Client *c) {

	if(!c) return;

	list<Client*>::iterator Iterator;

	for(Iterator = ClientChatConnections.begin(); Iterator != ClientChatConnections.end(); Iterator++) {

		if(((*Iterator) != c) && ((c->GetName() == (*Iterator)->GetName())
		   && (c->GetConnectionType() == (*Iterator)->GetConnectionType()))) {

			_log(UCS__CLIENT, "Removing old connection for %s", c->GetName().c_str());

			struct in_addr  in;

			in.s_addr = (*Iterator)->ClientStream->GetRemoteIP();

			_log(UCS__CLIENT, "Client connection from %s:%d closed.", inet_ntoa(in),
										   ntohs((*Iterator)->ClientStream->GetRemotePort()));
			
			safe_delete((*Iterator));

			Iterator = ClientChatConnections.erase(Iterator);
		}
	}
}

int Client::triggers(char *params, irc_reply_data *hostd, void *conn) {	//hooks privmsg to your specified channel

	IRC* irc_conn=(IRC*)conn;
	

	//const ucsconfig *Config=ucsconfig::get();

	string ChannelName = Config->EQChannelToOutput;

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	char name[128];
	strcpy(name,Config->ChannelToOutput.c_str());

	if(!strcmp(params,":!rejoin"))
	{
		irc_conn->join(name);
	}	

	string parame = params;
	string IRCName = hostd->nick;

	if(!strcmp(hostd->target,name))
	{
		//RequiredChannel->SendMessageToChannelFromIRC(GetCleanMessageFromIRC(parame), IRCName); // I got an IRC command, now let's send it
		this->SendChannelMessageFromIRC(GetCleanMessageFromIRC(parame), IRCName);	// I got an IRC command, now let's send it
	}
	return 0;
}

bool Client::CreateIRCThread() {

	pt2Member MemberPointer = &Client::triggers;
	this->IRCConnection.hook_irc_command("PRIVMSG", &(this->*MemberPointer));
	this->IRCConnection.hook_irc_command("376", &end_of_motd);
	this->IRCConnection.hook_irc_command("422", &end_of_motd);

	//this allows us to go from const to non-const
	char irc_host[64];
	char irc_nick[64];
	strcpy(irc_host, Config->ChatIRCHost.c_str());
	strcpy(irc_nick, this->GetName().c_str());

	this->IRCConnection.start(irc_host, Config->ChatIRCPort, irc_nick, "EQEMu IRC Bot", "EQEMu IRC Bot", 0);
	if (!this->IRCThreadPtr) {
		_log(IRC__TRACE, "No existing IRC thread found for %s, starting IRC thread.", irc_nick);
		#ifdef WIN32
		this->IRCThreadPtr = _beginthread(IRCConnect,0,(void*)&this->IRCConnection);
		#else
		pthread_create(&this->IRCThreadPtr,NULL,IRCConnect,(void*)&this->IRCConnection);
		#endif
		_log(IRC__TRACE, "Thread %i started for %s", this->IRCThreadPtr, irc_nick);
		return true;
	} else {
		_log(IRC__TRACE, "Found existing thread %i for %s", this->IRCThreadPtr, irc_nick);
		return false;
	}
}

void Clientlist::Process() {

	EQStream *eqs;

	while((eqs = chatsf->Pop())) {

		struct in_addr  in;

		in.s_addr = eqs->GetRemoteIP();

		_log(UCS__CLIENT, "New Client UDP connection from %s:%d", inet_ntoa(in), ntohs(eqs->GetRemotePort()));

		eqs->SetOpcodeManager(&ChatOpMgr);

		Client *c = new Client(eqs);

		ClientChatConnections.push_back(c);
	}

	list<Client*>::iterator Iterator;

	for(Iterator = ClientChatConnections.begin(); Iterator != ClientChatConnections.end(); Iterator++) {

		(*Iterator)->AccountUpdate();
		if((*Iterator)->ClientStream->CheckClosed()) {

			struct in_addr  in;

			in.s_addr = (*Iterator)->ClientStream->GetRemoteIP();

			_log(UCS__CLIENT, "Client connection from %s:%d closed.", inet_ntoa(in),
										   ntohs((*Iterator)->ClientStream->GetRemotePort()));
			
			safe_delete((*Iterator));

			Iterator = ClientChatConnections.erase(Iterator);

			if(Iterator == ClientChatConnections.end())
				break;

			continue;
		}

		EQApplicationPacket *app = 0;

		bool KeyValid = true;

		while( KeyValid && !(*Iterator)->GetForceDisconnect() &&
		       (app = (EQApplicationPacket *)(*Iterator)->ClientStream->PopPacket())) {

			_pkt(UCS__PACKETS, app);

			EmuOpcode opcode = app->GetOpcode();

			switch(opcode) {

				case OP_MailLogin: {

					char *PacketBuffer = (char *)app->pBuffer;

					char MailBox[64];

					char Key[64];

					char ConnectionTypeIndicator;

					VARSTRUCT_DECODE_STRING(MailBox, PacketBuffer);

					if(strlen(PacketBuffer) != 9)
					{
						_log(UCS__ERROR, "Mail key is the wrong size. Version of world incompatible with UCS.");
						KeyValid = false;
						break;
					}
					ConnectionTypeIndicator = VARSTRUCT_DECODE_TYPE(char, PacketBuffer);

					(*Iterator)->SetConnectionType(ConnectionTypeIndicator);

					VARSTRUCT_DECODE_STRING(Key, PacketBuffer);

					string MailBoxString = MailBox, CharacterName;

					// Strip off the SOE.EQ.<shortname>.
					//
					string::size_type LastPeriod = MailBoxString.find_last_of(".");

					if(LastPeriod == string::npos)
						CharacterName = MailBoxString;
					else
						CharacterName = MailBoxString.substr(LastPeriod + 1);

					_log(UCS__TRACE, "Received login for user %s with key %s", MailBox, Key);

					if(!database.VerifyMailKey(CharacterName, (*Iterator)->ClientStream->GetRemoteIP(), Key)) {

						_log(UCS__ERROR, "Chat Key for %s does not match, closing connection.", MailBox);

						KeyValid = false;

						break;
					}

					(*Iterator)->SetAccountID(database.FindAccount(CharacterName.c_str(), (*Iterator)));

					database.GetAccountStatus((*Iterator));

					if((*Iterator)->GetConnectionType() == ConnectionTypeCombined)
						(*Iterator)->SendFriends();

					(*Iterator)->SendMailBoxes();

					CheckForStaleConnections((*Iterator));

					//start irc thread
					_log(IRC__TRACE, "Checking to see if we should use IRC: %s", UseIRC ? "TRUE" : "FALSE");
					if(UseIRC) {
						(*Iterator)->CreateIRCThread();
//						IRC &IRCconn = (*Iterator)->IRCConnection;
//						char irc_host[64];
//						char irc_nick[64];
//						strcpy(irc_host, Config->ChatIRCHost.c_str());
//						strcpy(irc_nick, (*Iterator)->GetName().c_str());
//						//int (Client::*pt2Member)(char*, irc_reply_data*, void*) = NULL;
//						pt2Member MemberPointer = &Client::triggers;
//						IRCconn.hook_irc_command("PRIVMSG",&MemberPointer);
//						IRCconn.hook_irc_command("376", &end_of_motd); //hook the end of MOTD message
//						IRCconn.hook_irc_command("422", &end_of_motd);	//MOTD File is missing
//						IRCconn.start(irc_host,Config->ChatIRCPort,irc_nick,"EQEMu IRC Bot","EQEMu IRC Bot",0);
//
//						//_log(IRC__TRACE, "Checking for an existing IRC thread: %s", (*Iterator)->IRCThreadPtr ? "TRUE" : "FALSE");
//						/*_log(IRC__TRACE, "Requesting Mutex lock for IRCThreadPtr.");
//						(*Iterator)->MIRCThreadPtr.lock();
//						_log(IRC__TRACE, "Got Mutex lock for IRCThreadPtr.");*/
//						if (!(*Iterator)->IRCThreadPtr) {
//							_log(IRC__TRACE, "No existing IRC thread found for %s, starting IRC thread.", irc_nick);
//#ifdef WIN32
//							(*Iterator)->IRCThreadPtr = _beginthread(IRCConnect,0,(void*)&IRCconn);
//#else
//							//pthread_t th1;
//							pthread_create(&(*Iterator)->IRCThreadPtr,NULL,IRCConnect,(void*)&IRCconn);
//#endif
//							_log(IRC__TRACE, "Thread %i started for %s", (*Iterator)->IRCThreadPtr, irc_nick);
//						} else {
//							_log(IRC__TRACE, "Found existing thread %i for %s", (*Iterator)->IRCThreadPtr, irc_nick);
//						}
//						/*_log(IRC__TRACE, "Unlocking Mutex for IRCThreadPtr.");
//						(*Iterator)->MIRCThreadPtr.unlock();
//						_log(IRC__TRACE, "Mutex for IRCThreadPtr unlocked.");*/
					}

					break;
				}

				case OP_Mail: {

					const char *PacketBuffer = (const char*)app->pBuffer;
					
					string CommandString = (const char*)app->pBuffer;

					if(CommandString.length() == 0)
						break;

					if(isdigit(CommandString[0])) {

						(*Iterator)->SendChannelMessageByNumber(CommandString);

						break;
					}

					if(CommandString[0] == '#') {

						(*Iterator)->SendChannelMessage(CommandString);

						break;
					}

					string Command, Parameters;

					string::size_type Space = CommandString.find_first_of(" ");

					if(Space != string::npos) { 

						Command = CommandString.substr(0, Space);

						string::size_type ParametersStart = CommandString.find_first_not_of(" ", Space);

						if(ParametersStart != string::npos)
							Parameters = CommandString.substr(ParametersStart);
					}
					else
						Command = CommandString;

					int CommandCode = LookupCommand(Command.c_str());

					switch(CommandCode) {

						case CommandJoin:
							(*Iterator)->JoinChannels(Parameters);
							break;

						case CommandLeaveAll:
							(*Iterator)->LeaveAllChannels();
							break;

						case CommandLeave:
							(*Iterator)->LeaveChannels(Parameters);
							break;

						case CommandListAll:
							ChannelList->SendAllChannels((*Iterator));
							break;
						
						case CommandList:
							(*Iterator)->ProcessChannelList(Parameters);
							break;

						case CommandSet:
							(*Iterator)->LeaveAllChannels(false);
							(*Iterator)->JoinChannels(Parameters);
							break;

						case CommandAnnounce:
							(*Iterator)->ToggleAnnounce();
							break;

						case CommandSetOwner:
							(*Iterator)->SetChannelOwner(Parameters);
							break;

						case CommandOPList:
							(*Iterator)->OPList(Parameters);
							break;

						case CommandInvite:
							(*Iterator)->ChannelInvite(Parameters);
							break;

						case CommandGrant:
							(*Iterator)->ChannelGrantModerator(Parameters);
							break;

						case CommandModerate:
							(*Iterator)->ChannelModerate(Parameters);
							break;

						case CommandVoice:
							(*Iterator)->ChannelGrantVoice(Parameters);
							break;

						case CommandKick:
							(*Iterator)->ChannelKick(Parameters);
							break;

						case CommandPassword:
							(*Iterator)->SetChannelPassword(Parameters);
							break;
				
						case CommandToggleInvites:
							(*Iterator)->ToggleInvites();
							break;

						case CommandAFK:
							break;

						case CommandUptime:
							(*Iterator)->SendUptime();
							break;

						case CommandGetHeaders:
							database.SendHeaders((*Iterator));
							break;

						case CommandGetBody:
							database.SendBody((*Iterator), atoi(Parameters.c_str()));
							break;

						case CommandMailTo:
							ProcessMailTo((*Iterator), Parameters);
							break;

						case CommandSetMessageStatus:
							_log(UCS__TRACE, "Set Message Status, Params: %s", Parameters.c_str());
							ProcessSetMessageStatus(Parameters);
							break;

						case CommandSelectMailBox:
						{
							string::size_type NumStart = Parameters.find_first_of("0123456789");
						 	(*Iterator)->ChangeMailBox(atoi(Parameters.substr(NumStart).c_str()));
							break;
						}
						case CommandSetMailForwarding:
							break;
						
						case CommandBuddy:
							RemoveApostrophes(Parameters);
							ProcessCommandBuddy((*Iterator), Parameters);
							break;

						case CommandIgnorePlayer:
							RemoveApostrophes(Parameters);
							ProcessCommandIgnore((*Iterator), Parameters);
							break;
				
						default:
							(*Iterator)->SendHelp();
							_log(UCS__ERROR, "Unhandled OP_Mail command: %s", PacketBuffer);
					}

					break;
				}

				default: {

					_log(UCS__ERROR, "Unhandled chat opcode %8X", opcode);
					break;
				}
			}
			safe_delete(app);

		}
		if(!KeyValid || (*Iterator)->GetForceDisconnect()) {

			struct in_addr  in;

			in.s_addr = (*Iterator)->ClientStream->GetRemoteIP();

			_log(UCS__TRACE, "Force disconnecting client: %s:%d, KeyValid=%i, GetForceDisconnect()=%i",
					      inet_ntoa(in), ntohs((*Iterator)->ClientStream->GetRemotePort()),
					      KeyValid, (*Iterator)->GetForceDisconnect());

			(*Iterator)->ClientStream->Close();

			safe_delete((*Iterator));

			Iterator = ClientChatConnections.erase(Iterator);

			if(Iterator == ClientChatConnections.end())
				break;
		}

	}

}

void Clientlist::CloseAllConnections() {


	list<Client*>::iterator Iterator;

	for(Iterator = ClientChatConnections.begin(); Iterator != ClientChatConnections.end(); Iterator++) {

		_log(UCS__TRACE, "Removing client %s", (*Iterator)->GetName().c_str());

		(*Iterator)->CloseConnection();
	}
}

void Client::AddCharacter(int CharID, const char *CharacterName) {

	if(!CharacterName) return;
	_log(UCS__TRACE, "Adding character %s with ID %i for %s", CharacterName, CharID, GetName().c_str());
	CharacterEntry NewCharacter;
	NewCharacter.CharID = CharID;
	NewCharacter.Name = CharacterName;

	Characters.push_back(NewCharacter);
}

void Client::SendMailBoxes() {

	int Count = Characters.size();

	int PacketLength = 10;

	string s;
	
	for(int i = 0; i < Count; i++) {

		s += GetMailPrefix() + Characters[i].Name;

		if(i != (Count - 1))
			s = s + ",";
	}

	PacketLength += s.length() + 1;

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_MailLogin, PacketLength);

	char *PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 1);
	VARSTRUCT_ENCODE_TYPE(uint32, PacketBuffer, Count);
	VARSTRUCT_ENCODE_TYPE(uint32, PacketBuffer, 0);

	VARSTRUCT_ENCODE_STRING(PacketBuffer, s.c_str());
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0);

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

Client *Clientlist::FindCharacter(string CharacterName) {

	list<Client*>::iterator Iterator;

	for(Iterator = ClientChatConnections.begin(); Iterator != ClientChatConnections.end(); Iterator++) {

		if((*Iterator)->GetName() == CharacterName)
			return ((*Iterator));

	}

	return NULL;
}

void Client::AddToChannelList(ChatChannel *JoinedChannel) {

	if(!JoinedChannel) return;

	for(int i = 0; i < MAX_JOINED_CHANNELS; i++)
		if(JoinedChannels[i] == NULL) {
			JoinedChannels[i] = JoinedChannel;
			_log(UCS__TRACE, "Added Channel %s to slot %i for %s", JoinedChannel->GetName().c_str(), i + 1, GetName().c_str());
			return;
		}
}

void Client::RemoveFromChannelList(ChatChannel *JoinedChannel) {

	for(int i = 0; i < MAX_JOINED_CHANNELS; i++)
		if(JoinedChannels[i] == JoinedChannel) {

			// Shuffle all the channels down. Client likes them all nice and consecutive.
			//
			for(int j = i; j < (MAX_JOINED_CHANNELS - 1); j++)
				JoinedChannels[j] = JoinedChannels[j + 1];

			JoinedChannels[MAX_JOINED_CHANNELS - 1] = NULL;

			break;
		}
}

int Client::ChannelCount() {

	int NumberOfChannels = 0;

	for(int i = 0; i < MAX_JOINED_CHANNELS; i++)
		if(JoinedChannels[i])
			NumberOfChannels++;

	return NumberOfChannels;

}

void Client::JoinChannels(string ChannelNameList) {

	_log(UCS__TRACE, "Client: %s joining channels %s", GetName().c_str(), ChannelNameList.c_str());

	int NumberOfChannels = ChannelCount();

	string::size_type CurrentPos = ChannelNameList.find_first_not_of(" ");

	while(CurrentPos != string::npos) {

		if(NumberOfChannels == MAX_JOINED_CHANNELS) {

			GeneralChannelMessage("You have joined the maximum number of channels. /leave one before trying to join another.");
			
			break;
		}

		string::size_type Comma = ChannelNameList.find_first_of(", ", CurrentPos);

		if(Comma == string::npos) {

			ChatChannel* JoinedChannel = ChannelList->AddClientToChannel(ChannelNameList.substr(CurrentPos), this);

			if(JoinedChannel)
				AddToChannelList(JoinedChannel);

			break;
		}

		ChatChannel* JoinedChannel = ChannelList->AddClientToChannel(ChannelNameList.substr(CurrentPos, Comma-CurrentPos), this);

		if(JoinedChannel) {

			AddToChannelList(JoinedChannel);

			NumberOfChannels++;
		}

		CurrentPos = ChannelNameList.find_first_not_of(", ", Comma);
	}

	string JoinedChannelsList, ChannelMessage;

	ChannelMessage = "Channels: ";

	char tmp[200];

	int ChannelCount = 0;

	for(int i = 0; i < MAX_JOINED_CHANNELS ; i++) {

		if(JoinedChannels[i] != NULL) {

			if(ChannelCount) {

				JoinedChannelsList = JoinedChannelsList + ",";

				ChannelMessage = ChannelMessage + ",";

			}

			JoinedChannelsList = JoinedChannelsList + JoinedChannels[i]->GetName();

			sprintf(tmp, "%i=%s(%i)", i + 1, JoinedChannels[i]->GetName().c_str(), JoinedChannels[i]->MemberCount(Status));

			ChannelMessage += tmp;

			ChannelCount++;
		}
	}


	EQApplicationPacket *outapp = new EQApplicationPacket(OP_Mail, JoinedChannelsList.length() + 1);

	char *PacketBuffer = (char *)outapp->pBuffer;

	sprintf(PacketBuffer, "%s", JoinedChannelsList.c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);

	if(ChannelCount == 0)
		ChannelMessage = "You are not on any channels.";

	outapp = new EQApplicationPacket(OP_ChannelMessage, ChannelMessage.length() + 3);

	PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_STRING(PacketBuffer, ChannelMessage.c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::LeaveChannels(string ChannelNameList) {

	_log(UCS__TRACE, "Client: %s leaving channels %s", GetName().c_str(), ChannelNameList.c_str());

	string::size_type CurrentPos = 0;

	while(CurrentPos != string::npos) {

		string::size_type Comma = ChannelNameList.find_first_of(", ", CurrentPos);

		if(Comma == string::npos) {

			ChatChannel* JoinedChannel = ChannelList->RemoveClientFromChannel(ChannelNameList.substr(CurrentPos), this);

			if(JoinedChannel)
				RemoveFromChannelList(JoinedChannel);

			break;
		}

		ChatChannel* JoinedChannel = ChannelList->RemoveClientFromChannel(ChannelNameList.substr(CurrentPos, Comma-CurrentPos), this);

		if(JoinedChannel)
			RemoveFromChannelList(JoinedChannel);

		CurrentPos = ChannelNameList.find_first_not_of(", ", Comma);
	}

	string JoinedChannelsList, ChannelMessage;

	ChannelMessage = "Channels: ";

	char tmp[200];

	int ChannelCount = 0;

	for(int i = 0; i < MAX_JOINED_CHANNELS ; i++) {

		if(JoinedChannels[i] != NULL) {

			if(ChannelCount) {

				JoinedChannelsList = JoinedChannelsList + ",";

				ChannelMessage = ChannelMessage + ",";
			}

			JoinedChannelsList = JoinedChannelsList + JoinedChannels[i]->GetName();

			sprintf(tmp, "%i=%s(%i)", i + 1, JoinedChannels[i]->GetName().c_str(), JoinedChannels[i]->MemberCount(Status));

			ChannelMessage += tmp;

			ChannelCount++;
		}
	}


	EQApplicationPacket *outapp = new EQApplicationPacket(OP_Mail, JoinedChannelsList.length() + 1);

	char *PacketBuffer = (char *)outapp->pBuffer;

	sprintf(PacketBuffer, "%s", JoinedChannelsList.c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);

	if(ChannelCount == 0)
		ChannelMessage = "You are not on any channels.";

	outapp = new EQApplicationPacket(OP_ChannelMessage, ChannelMessage.length() + 3);

	PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_STRING(PacketBuffer, ChannelMessage.c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::LeaveAllChannels(bool SendUpdatedChannelList) {

	for(int i = 0; i < MAX_JOINED_CHANNELS; i++) {

		if(JoinedChannels[i]) {

			ChannelList->RemoveClientFromChannel(JoinedChannels[i]->GetName(), this);

			JoinedChannels[i] = NULL;
		}
	}

	if(SendUpdatedChannelList)
		SendChannelList();
}


void Client::ProcessChannelList(string Input) {

	if(Input.length() == 0) {

		SendChannelList();

		return;
	}

	string ChannelName = Input;

	if(isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(RequiredChannel) 
		RequiredChannel->SendChannelMembers(this);
	else
		GeneralChannelMessage("Channel " + Input + " not found.");
}



void Client::SendChannelList() {

	string ChannelMessage;

	ChannelMessage = "Channels: ";

	char tmp[200];

	int ChannelCount = 0;

	for(int i = 0; i < MAX_JOINED_CHANNELS ; i++) {

		if(JoinedChannels[i] != NULL) {

			if(ChannelCount)
				ChannelMessage = ChannelMessage + ",";

			sprintf(tmp, "%i=%s(%i)", i + 1, JoinedChannels[i]->GetName().c_str(), JoinedChannels[i]->MemberCount(Status));

			ChannelMessage += tmp;

			ChannelCount++;
		}
	}

	if(ChannelCount == 0)
		ChannelMessage = "You are not on any channels.";

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChannelMessage, ChannelMessage.length() + 3);

	char *PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_STRING(PacketBuffer, ChannelMessage.c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::SendChannelMessage(string Message) 
{

	string::size_type MessageStart = Message.find_first_of(" ");

	if(MessageStart == string::npos)
		return;

	string ChannelName = Message.substr(1, MessageStart-1);

	_log(UCS__TRACE, "%s tells %s, [%s]", GetName().c_str(), ChannelName.c_str(), Message.substr(MessageStart + 1).c_str());

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(IsRevoked())
	{
		GeneralChannelMessage("You are Revoked, you cannot chat in global channels.");
		return;
	}

	if(RequiredChannel)
		if(RuleB(Chat, EnableAntiSpam))
		{
			if(!RequiredChannel->IsModerated() || RequiredChannel->HasVoice(GetName()) || RequiredChannel->IsOwner(GetName()) ||
				RequiredChannel->IsModerator(GetName()) || IsChannelAdmin())
			{
				if(GlobalChatLimiterTimer)
				{
					if(GlobalChatLimiterTimer->Check(false))
					{
						GlobalChatLimiterTimer->Start(RuleI(Chat, IntervalDurationMS));
						AttemptedMessages = 0;
					}
				}
				int AllowedMessages = RuleI(Chat, MinimumMessagesPerInterval) + GetKarma();
				AllowedMessages = AllowedMessages > RuleI(Chat, MaximumMessagesPerInterval) ? RuleI(Chat, MaximumMessagesPerInterval) : AllowedMessages; 
				
				if(RuleI(Chat, MinStatusToBypassAntiSpam) <= Status)
					AllowedMessages = 10000;
				
				AttemptedMessages++;
				if(AttemptedMessages > AllowedMessages)
				{
					if(AttemptedMessages > RuleI(Chat, MaxMessagesBeforeKick))
					{
						ForceDisconnect = true;
					}
					if(GlobalChatLimiterTimer)
					{
						char TimeLeft[256];
						sprintf(TimeLeft, "You are currently rate limited, you cannot send more messages for %i seconds.", 
							(GlobalChatLimiterTimer->GetRemainingTime() / 1000));
						GeneralChannelMessage(TimeLeft);
					}
					else
					{
						GeneralChannelMessage("You are currently rate limited, you cannot send more messages for up to 60 seconds.");
					}
				}
				else
				{
					RequiredChannel->SendMessageToChannel(Message.substr(MessageStart+1), this);
				}
			}
			else
				GeneralChannelMessage("Channel " + ChannelName + " is moderated and you have not been granted a voice.");
		}
		else
		{
			if(!RequiredChannel->IsModerated() || RequiredChannel->HasVoice(GetName()) || RequiredChannel->IsOwner(GetName()) ||
				RequiredChannel->IsModerator(GetName()) || IsChannelAdmin())
				RequiredChannel->SendMessageToChannel(Message.substr(MessageStart+1), this);
			else
				GeneralChannelMessage("Channel " + ChannelName + " is moderated and you have not been granted a voice.");
		}

}

void Client::SendChannelMessageByNumber(string Message) {

	string::size_type MessageStart = Message.find_first_of(" ");

	if(MessageStart == string::npos)
		return;

	int ChannelNumber = atoi(Message.substr(0, MessageStart).c_str());

	if((ChannelNumber < 1) || (ChannelNumber > MAX_JOINED_CHANNELS)) {

		GeneralChannelMessage("Invalid channel name/number specified.");

		return;
	}

	ChatChannel *RequiredChannel = JoinedChannels[ChannelNumber-1];

	if(!RequiredChannel) {

		GeneralChannelMessage("Invalid channel name/number specified.");

		return;
	}

	if(IsRevoked())
	{
		GeneralChannelMessage("You are Revoked, you cannot chat in global channels.");
		return;
	}

	_log(UCS__TRACE, "%s tells %s, [%s]", GetName().c_str(), RequiredChannel->GetName().c_str(), 
						   Message.substr(MessageStart + 1).c_str());

	if(RuleB(Chat, EnableAntiSpam))
	{
		if(!RequiredChannel->IsModerated() || RequiredChannel->HasVoice(GetName()) || RequiredChannel->IsOwner(GetName()) ||
			RequiredChannel->IsModerator(GetName()))
		{
				if(GlobalChatLimiterTimer)
				{
					if(GlobalChatLimiterTimer->Check(false))
					{
						GlobalChatLimiterTimer->Start(RuleI(Chat, IntervalDurationMS));
						AttemptedMessages = 0;
					}
				}
				int AllowedMessages = RuleI(Chat, MinimumMessagesPerInterval) + GetKarma();
				AllowedMessages = AllowedMessages > RuleI(Chat, MaximumMessagesPerInterval) ? RuleI(Chat, MaximumMessagesPerInterval) : AllowedMessages; 
				if(RuleI(Chat, MinStatusToBypassAntiSpam) <= Status)
					AllowedMessages = 10000;

				AttemptedMessages++;
				if(AttemptedMessages > AllowedMessages)
				{
					if(AttemptedMessages > RuleI(Chat, MaxMessagesBeforeKick))
					{
						ForceDisconnect = true;
					}
					if(GlobalChatLimiterTimer)
					{
						char TimeLeft[256];
						sprintf(TimeLeft, "You are currently rate limited, you cannot send more messages for %i seconds.", 
							(GlobalChatLimiterTimer->GetRemainingTime() / 1000));
						GeneralChannelMessage(TimeLeft);
					}
					else
					{
						GeneralChannelMessage("You are currently rate limited, you cannot send more messages for up to 60 seconds.");
					}
				}
				else
				{
					RequiredChannel->SendMessageToChannel(Message.substr(MessageStart+1), this);
				}
		}
		else
			GeneralChannelMessage("Channel " + RequiredChannel->GetName() + " is moderated and you have not been granted a voice.");
	}
	else
	{
		if(!RequiredChannel->IsModerated() || RequiredChannel->HasVoice(GetName()) || RequiredChannel->IsOwner(GetName()) ||
			RequiredChannel->IsModerator(GetName()))
			RequiredChannel->SendMessageToChannel(Message.substr(MessageStart+1), this);
		else
			GeneralChannelMessage("Channel " + RequiredChannel->GetName() + " is moderated and you have not been granted a voice.");
	}

}

void Client::SendChannelMessage(string ChannelName, string Message, Client *Sender) {

	if(!Sender) return;

	string FQSenderName = WorldShortName + "." + Sender->GetName();

	int PacketLength = ChannelName.length() + Message.length() + FQSenderName.length() + 3;

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChannelMessage, PacketLength);

	char *PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_STRING(PacketBuffer, ChannelName.c_str());
	VARSTRUCT_ENCODE_STRING(PacketBuffer, FQSenderName.c_str());
	VARSTRUCT_ENCODE_STRING(PacketBuffer, Message.c_str());

	_pkt(UCS__PACKETS, outapp);
	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::SendChannelMessageFromIRC(string Message, string IRCName) { /* Packet handler for IRC Messages */

	const ucsconfig *Config=ucsconfig::get();

	string FQSenderName = "IRC." + IRCName;

	string ChannelName = Config->EQChannelToOutput.c_str();

	int PacketLength = ChannelName.length() + Message.length() + FQSenderName.length() + 3;

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChannelMessage, PacketLength);

	char *PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_STRING(PacketBuffer, ChannelName.c_str());
	VARSTRUCT_ENCODE_STRING(PacketBuffer, FQSenderName.c_str());
	VARSTRUCT_ENCODE_STRING(PacketBuffer, Message.c_str());

	_pkt(CHANNELS__PACKETS, outapp);
	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::ToggleAnnounce() {

	string Message = "Announcing now ";

	if(Announce) 
		Message += "off";
	else
		Message += "on";

	Announce = !Announce;

	GeneralChannelMessage(Message);
}

void Client::AnnounceJoin(ChatChannel *Channel, Client *c) {

	if(!Channel || !c) return;

	int PacketLength = Channel->GetName().length() + c->GetName().length() + 2;

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChannelAnnounceJoin, PacketLength);

	char *PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_STRING(PacketBuffer, Channel->GetName().c_str());
	VARSTRUCT_ENCODE_STRING(PacketBuffer, c->GetName().c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::AnnounceLeave(ChatChannel *Channel, Client *c) {

	if(!Channel || !c) return;

	int PacketLength = Channel->GetName().length() + c->GetName().length() + 2;

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChannelAnnounceLeave, PacketLength);

	char *PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_STRING(PacketBuffer, Channel->GetName().c_str());
	VARSTRUCT_ENCODE_STRING(PacketBuffer, c->GetName().c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::GeneralChannelMessage(const char *Characters) {

	if(!Characters) return;

	string Message = Characters;

	GeneralChannelMessage(Message);

}

void Client::GeneralChannelMessage(string Message) {

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChannelMessage, Message.length() + 3);
	char *PacketBuffer = (char *)outapp->pBuffer;
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0x00);
	VARSTRUCT_ENCODE_STRING(PacketBuffer, Message.c_str());

	_pkt(UCS__PACKETS, outapp);
	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::SetChannelPassword(string ChannelPassword) {

	string::size_type PasswordStart = ChannelPassword.find_first_not_of(" ");

	if(PasswordStart == string::npos) {
		string Message = "Incorrect syntax: /chat password <new password> <channel name>";
		GeneralChannelMessage(Message);
		return;
	}

	string::size_type Space = ChannelPassword.find_first_of(" ", PasswordStart);

	if(Space == string::npos) {
		string Message = "Incorrect syntax: /chat password <new password> <channel name>";
		GeneralChannelMessage(Message);
		return;
	}

	string Password = ChannelPassword.substr(PasswordStart, Space - PasswordStart);

	string::size_type ChannelStart = ChannelPassword.find_first_not_of(" ", Space);

	if(ChannelStart == string::npos) {
		string Message = "Incorrect syntax: /chat password <new password> <channel name>";
		GeneralChannelMessage(Message);
		return;
	}

	string ChannelName = ChannelPassword.substr(ChannelStart);

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	string Message;

	if(!strcasecmp(Password.c_str(), "remove")) {
		Password.clear();
		Message = "Password REMOVED on channel " + ChannelName;
	}
	else
		Message = "Password change on channel " + ChannelName;

	_log(UCS__TRACE, "Set password of channel [%s] to [%s] by %s", ChannelName.c_str(), Password.c_str(), GetName().c_str());

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {
		string Message = "Channel not found.";
		GeneralChannelMessage(Message);
		return;
	}

	if(!RequiredChannel->IsOwner(GetName()) && !RequiredChannel->IsModerator(GetName()) && !IsChannelAdmin()) {
		string Message = "You do not own or have moderator rights on channel " + ChannelName;
		GeneralChannelMessage(Message);
		return;
	}

	RequiredChannel->SetPassword(Password);

	GeneralChannelMessage(Message);

}

void Client::SetChannelOwner(string CommandString) {

	string::size_type PlayerStart = CommandString.find_first_not_of(" ");

	if(PlayerStart == string::npos) {
		string Message = "Incorrect syntax: /chat setowner <player> <channel>";
		GeneralChannelMessage(Message);
		return;
	}

	string::size_type Space = CommandString.find_first_of(" ", PlayerStart);

	if(Space == string::npos) {
		string Message = "Incorrect syntax: /chat setowner <player> <channel>";
		GeneralChannelMessage(Message);
		return;
	}

	string NewOwner = CapitaliseName(CommandString.substr(PlayerStart, Space - PlayerStart));

	string::size_type ChannelStart = CommandString.find_first_not_of(" ", Space);

	if(ChannelStart == string::npos) {
		string Message = "Incorrect syntax: /chat setowner <player> <channel>";
		GeneralChannelMessage(Message);
		return;
	}

	string ChannelName = CapitaliseName(CommandString.substr(ChannelStart));

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	_log(UCS__TRACE, "Set owner of channel [%s] to [%s]", ChannelName.c_str(), NewOwner.c_str());

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {
		GeneralChannelMessage("Channel " + ChannelName + " not found.");
		return;
	}

	if(!RequiredChannel->IsOwner(GetName()) && !IsChannelAdmin()) {
		string Message = "You do not own channel " + ChannelName;
		GeneralChannelMessage(Message);
		return;
	}

	if(database.FindCharacter(NewOwner.c_str()) < 0) {

		GeneralChannelMessage("Player " + NewOwner + " does not exist.");
		return;
	}

	RequiredChannel->SetOwner(NewOwner);

	if(RequiredChannel->IsModerator(NewOwner))
		RequiredChannel->RemoveModerator(NewOwner);

	GeneralChannelMessage("Channel owner changed.");

}

void Client::OPList(string CommandString) {

	string::size_type ChannelStart = CommandString.find_first_not_of(" ");
 
	if(ChannelStart == string::npos) {
		string Message = "Incorrect syntax: /chat oplist <channel>";
		GeneralChannelMessage(Message);
		return;
	}

	string ChannelName = CapitaliseName(CommandString.substr(ChannelStart));

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {
		GeneralChannelMessage("Channel " + ChannelName + " not found.");
		return;
	}

	RequiredChannel->SendOPList(this);
}

void Client::ChannelInvite(string CommandString) {

	string::size_type PlayerStart = CommandString.find_first_not_of(" ");

	if(PlayerStart == string::npos) {
		string Message = "Incorrect syntax: /chat invite <player> <channel>";
		GeneralChannelMessage(Message);
		return;
	}

	string::size_type Space = CommandString.find_first_of(" ", PlayerStart);

	if(Space == string::npos) {
		string Message = "Incorrect syntax: /chat invite <player> <channel>";
		GeneralChannelMessage(Message);
		return;
	}

	string Invitee = CapitaliseName(CommandString.substr(PlayerStart, Space - PlayerStart));

	string::size_type ChannelStart = CommandString.find_first_not_of(" ", Space);

	if(ChannelStart == string::npos) {
		string Message = "Incorrect syntax: /chat invite <player> <channel>";
		GeneralChannelMessage(Message);
		return;
	}

	string ChannelName = CapitaliseName(CommandString.substr(ChannelStart));

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	_log(UCS__TRACE, "[%s] invites [%s] to channel [%s]", GetName().c_str(), Invitee.c_str(), ChannelName.c_str());

	Client *RequiredClient = CL->FindCharacter(Invitee);

	if(!RequiredClient) {

		GeneralChannelMessage(Invitee + " is not online.");
		return;
	}

	if(RequiredClient == this) {

		GeneralChannelMessage("You cannot invite yourself to a channel.");
		return;
	}

	if(!RequiredClient->InvitesAllowed()) {

		GeneralChannelMessage("That player is not currently accepting channel invitations.");
		return;
	}

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {

		GeneralChannelMessage("Channel " + ChannelName + " not found.");
		return;
	}

	if(!RequiredChannel->IsOwner(GetName()) && !RequiredChannel->IsModerator(GetName())) {

		string Message = "You do not own or have moderator rights to channel " + ChannelName;

		GeneralChannelMessage(Message);
		return;
	}

	if(RequiredChannel->IsClientInChannel(RequiredClient)) {

		GeneralChannelMessage(Invitee + " is already in that channel");

		return;
	}

	RequiredChannel->AddInvitee(Invitee);

	RequiredClient->GeneralChannelMessage(GetName() + " has invited you to join channel " + ChannelName);

	GeneralChannelMessage("Invitation sent to " + Invitee + " to join channel " + ChannelName);

}

void Client::ChannelModerate(string CommandString) {

	string::size_type ChannelStart = CommandString.find_first_not_of(" ");

	if(ChannelStart == string::npos) {

		string Message = "Incorrect syntax: /chat moderate <channel>";

		GeneralChannelMessage(Message);
		return;
	}

	string ChannelName = CapitaliseName(CommandString.substr(ChannelStart));

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {

		GeneralChannelMessage("Channel " + ChannelName + " not found.");
		return;
	}

	if(!RequiredChannel->IsOwner(GetName()) && !RequiredChannel->IsModerator(GetName()) && !IsChannelAdmin()) {

		GeneralChannelMessage("You do not own or have moderator rights to channel " + ChannelName);
		return;
	}

	RequiredChannel->SetModerated(!RequiredChannel->IsModerated());

	if(!RequiredChannel->IsClientInChannel(this))
		if(RequiredChannel->IsModerated())
			GeneralChannelMessage("Channel " + ChannelName + " is now moderated.");
		else
			GeneralChannelMessage("Channel " + ChannelName + " is no longer moderated.");

}

void Client::ChannelGrantModerator(string CommandString) {

	string::size_type PlayerStart = CommandString.find_first_not_of(" ");

	if(PlayerStart == string::npos) {

		GeneralChannelMessage("Incorrect syntax: /chat grant <player> <channel>");
		return;
	}

	string::size_type Space = CommandString.find_first_of(" ", PlayerStart);

	if(Space == string::npos) {

		GeneralChannelMessage("Incorrect syntax: /chat grant <player> <channel>");
		return;
	}

	string Moderator = CapitaliseName(CommandString.substr(PlayerStart, Space - PlayerStart));

	string::size_type ChannelStart = CommandString.find_first_not_of(" ", Space);

	if(ChannelStart == string::npos) {

		GeneralChannelMessage("Incorrect syntax: /chat grant <player> <channel>");
		return;
	}

	string ChannelName = CapitaliseName(CommandString.substr(ChannelStart));

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	_log(UCS__TRACE, "[%s] gives [%s] moderator rights to channel [%s]", GetName().c_str(), Moderator.c_str(), ChannelName.c_str());

	Client *RequiredClient = CL->FindCharacter(Moderator);

	if(!RequiredClient && (database.FindCharacter(Moderator.c_str()) < 0)) {

		GeneralChannelMessage("Player " + Moderator + " does not exist.");
		return;
	}

	if(RequiredClient == this) {

		GeneralChannelMessage("You cannot grant yourself moderator rights to a channel.");
		return;
	}

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {

		GeneralChannelMessage("Channel " + ChannelName + " not found.");
		return;
	}

	if(!RequiredChannel->IsOwner(GetName()) && !IsChannelAdmin()) {

		GeneralChannelMessage("You do not own channel " + ChannelName);
		return;
	}

	if(RequiredChannel->IsModerator(Moderator)) {

		RequiredChannel->RemoveModerator(Moderator);

		if(RequiredClient)
			RequiredClient->GeneralChannelMessage(GetName() + " has removed your moderator rights to channel " + ChannelName);

		GeneralChannelMessage("Removing moderator rights from " + Moderator + " to channel " + ChannelName);
	}
	else {
		RequiredChannel->AddModerator(Moderator);
	
		if(RequiredClient)
			RequiredClient->GeneralChannelMessage(GetName() + " has made you a moderator of channel " + ChannelName);

		GeneralChannelMessage(Moderator + " is now a moderator on channel " + ChannelName);
	}

}

void Client::ChannelGrantVoice(string CommandString) {

	string::size_type PlayerStart = CommandString.find_first_not_of(" ");

	if(PlayerStart == string::npos) {

		GeneralChannelMessage("Incorrect syntax: /chat voice <player> <channel>");
		return;
	}

	string::size_type Space = CommandString.find_first_of(" ", PlayerStart);

	if(Space == string::npos) {
		GeneralChannelMessage("Incorrect syntax: /chat voice <player> <channel>");
		return;
	}

	string Voicee = CapitaliseName(CommandString.substr(PlayerStart, Space - PlayerStart));

	string::size_type ChannelStart = CommandString.find_first_not_of(" ", Space);

	if(ChannelStart == string::npos) {
		GeneralChannelMessage("Incorrect syntax: /chat voice <player> <channel>");
		return;
	}

	string ChannelName = CapitaliseName(CommandString.substr(ChannelStart));

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	_log(UCS__TRACE, "[%s] gives [%s] voice to channel [%s]", GetName().c_str(), Voicee.c_str(), ChannelName.c_str());

	Client *RequiredClient = CL->FindCharacter(Voicee);

	if(!RequiredClient && (database.FindCharacter(Voicee.c_str()) < 0)) {

		GeneralChannelMessage("Player " + Voicee + " does not exist.");
		return;
	}

	if(RequiredClient == this) {

		GeneralChannelMessage("You cannot grant yourself voice to a channel.");
		return;
	}

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {

		GeneralChannelMessage("Channel " + ChannelName + " not found.");
		return;
	}

	if(!RequiredChannel->IsOwner(GetName()) && !RequiredChannel->IsModerator(GetName()) && !IsChannelAdmin()) {

		GeneralChannelMessage("You do not own or have moderator rights to channel " + ChannelName);
		return;
	}

	if(RequiredChannel->IsOwner(RequiredClient->GetName()) || RequiredChannel->IsModerator(RequiredClient->GetName())) {

		GeneralChannelMessage("The channel owner and moderators automatically have voice.");
		return;
	}

	if(RequiredChannel->HasVoice(Voicee)) {

		RequiredChannel->RemoveVoice(Voicee);

		if(RequiredClient)
			RequiredClient->GeneralChannelMessage(GetName() + " has removed your voice rights to channel " + ChannelName);

		GeneralChannelMessage("Removing voice from " + Voicee + " in channel " + ChannelName);
	}
	else {
		RequiredChannel->AddVoice(Voicee);
	
		if(RequiredClient)
			RequiredClient->GeneralChannelMessage(GetName() + " has given you voice in channel " + ChannelName);

		GeneralChannelMessage(Voicee + " now has voice in channel " + ChannelName);
	}

}

void Client::ChannelKick(string CommandString) {

	string::size_type PlayerStart = CommandString.find_first_not_of(" ");

	if(PlayerStart == string::npos) {

		GeneralChannelMessage("Incorrect syntax: /chat kick <player> <channel>");
		return;
	}

	string::size_type Space = CommandString.find_first_of(" ", PlayerStart);

	if(Space == string::npos) {

		GeneralChannelMessage("Incorrect syntax: /chat kick <player> <channel>");
		return;
	}

	string Kickee = CapitaliseName(CommandString.substr(PlayerStart, Space - PlayerStart));

	string::size_type ChannelStart = CommandString.find_first_not_of(" ", Space);

	if(ChannelStart == string::npos) {

		GeneralChannelMessage("Incorrect syntax: /chat kick <player> <channel>");
		return;
	}
	string ChannelName = CapitaliseName(CommandString.substr(ChannelStart));

	if((ChannelName.length() > 0) && isdigit(ChannelName[0]))
		ChannelName = ChannelSlotName(atoi(ChannelName.c_str()));

	_log(UCS__TRACE, "[%s] kicks [%s] from channel [%s]", GetName().c_str(), Kickee.c_str(), ChannelName.c_str());

	Client *RequiredClient = CL->FindCharacter(Kickee);

	if(!RequiredClient) {

		GeneralChannelMessage("Player " + Kickee + " is not online.");
		return;
	}

	if(RequiredClient == this) {

		GeneralChannelMessage("You cannot kick yourself out of a channel.");
		return;
	}

	ChatChannel *RequiredChannel = ChannelList->FindChannel(ChannelName);

	if(!RequiredChannel) {

		GeneralChannelMessage("Channel " + ChannelName + " not found.");
		return;
	}

	if(!RequiredChannel->IsOwner(GetName()) && !RequiredChannel->IsModerator(GetName()) && !IsChannelAdmin()) {

		GeneralChannelMessage("You do not own or have moderator rights to channel " + ChannelName);
		return;
	}

	if(RequiredChannel->IsOwner(RequiredClient->GetName())) {

		GeneralChannelMessage("You cannot kick the owner out of the channel.");
		return;
	}

	if(RequiredChannel->IsModerator(Kickee) && !RequiredChannel->IsOwner(GetName())) {

		GeneralChannelMessage("Only the channel owner can kick a moderator out of the channel.");
		return;
	}

	if(RequiredChannel->IsModerator(Kickee))  {

		RequiredChannel->RemoveModerator(Kickee);

		RequiredClient->GeneralChannelMessage(GetName() + " has removed your moderator rights to channel " + ChannelName);

		GeneralChannelMessage("Removing moderator rights from " + Kickee + " to channel " + ChannelName);
	}

	RequiredClient->GeneralChannelMessage(GetName() + " has kicked you from channel " + ChannelName);

	GeneralChannelMessage("Kicked " + Kickee + " from channel " + ChannelName);

	RequiredClient->LeaveChannels(ChannelName);
}

void Client::ToggleInvites() {

	AllowInvites = !AllowInvites;

	if(AllowInvites)
		GeneralChannelMessage("You will now receive channel invitations.");
	else
		GeneralChannelMessage("You will no longer receive channel invitations.");

}

string Client::ChannelSlotName(int SlotNumber) {

	if((SlotNumber < 1 ) || (SlotNumber > MAX_JOINED_CHANNELS))
		return "";

	if(JoinedChannels[SlotNumber - 1] == NULL)
		return "";

	return JoinedChannels[SlotNumber - 1]->GetName();

}

void Client::SendHelp() {

	GeneralChannelMessage("Chat Channel Commands:");

	GeneralChannelMessage("/join, /leave, /leaveall, /list, /announce, /autojoin, ;set");
	GeneralChannelMessage(";oplist, ;grant, ;invite, ;kick, ;moderate, ;password, ;voice");
	GeneralChannelMessage(";setowner, ;toggleinvites");
}

void Client::AccountUpdate()
{
	if(AccountGrabUpdateTimer)
	{
		if(AccountGrabUpdateTimer->Check(false))
		{
			AccountGrabUpdateTimer->Start(60000);
			database.GetAccountStatus(this);
		}
	}
}

void Client::SetConnectionType(char c) {

	switch(c)
	{
		case 'S':
		{
			TypeOfConnection = ConnectionTypeCombined;
			_log(UCS__TRACE, "Connection type is Combined (SoF or later client)");
			break;
		}
		case 'M':
		{
			TypeOfConnection = ConnectionTypeMail;
			_log(UCS__TRACE, "Connection type is Mail (6.2 or Titanium client)");
			break;
		}
		case 'C':
		{
			TypeOfConnection = ConnectionTypeChat;
			_log(UCS__TRACE, "Connection type is Chat (6.2 or Titanium client)");
			break;
		}
		default:
		{
			TypeOfConnection = ConnectionTypeUnknown;
			_log(UCS__TRACE, "Connection type is unknown.");
		}
	}
}

Client *Clientlist::IsCharacterOnline(string CharacterName) {

	// This method is used to determine if the character we are a sending an email to is connected to the mailserver,
	// so we can send them a new email notification.
	//
	// The way live works is that it sends a notification if a player receives an email for their 'primary' mailbox,
	// i.e. for the character they are logged in as, or for the character whose mailbox they have selected in the
	// mail window.
	//
	list<Client*>::iterator Iterator;

	for(Iterator = ClientChatConnections.begin(); Iterator != ClientChatConnections.end(); Iterator++) {

		if(!(*Iterator)->IsMailConnection())
			continue;

		int MailBoxNumber = (*Iterator)->GetMailBoxNumber(CharacterName);

		// If the mail is destined for the primary mailbox for this character, or the one they have selected
		//
		if((MailBoxNumber == 0) || (MailBoxNumber == (*Iterator)->GetMailBoxNumber())) 
				return (*Iterator);

	}

	return NULL;
}

int Client::GetMailBoxNumber(string CharacterName) {

	for(unsigned int i = 0; i < Characters.size(); i++)
		if(Characters[i].Name == CharacterName)
			return i;

	return -1;
}

void Client::SendNotification(int MailBoxNumber, string Subject, string From, int MessageID) {

	char TimeStamp[100];

	char sMessageID[100];

	char Sequence[100];
	
	sprintf(TimeStamp, "%i", (int)time(NULL));

	sprintf(sMessageID, "%i", MessageID);

	sprintf(Sequence, "%i", 1);

	int PacketLength = 8 + strlen(sMessageID) + strlen(TimeStamp)+ From.length() + Subject.length();

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_MailNew, PacketLength);

	char *PacketBuffer = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_INTSTRING(PacketBuffer, MailBoxNumber);
	VARSTRUCT_ENCODE_STRING(PacketBuffer, sMessageID);
	VARSTRUCT_ENCODE_STRING(PacketBuffer, TimeStamp);
	VARSTRUCT_ENCODE_STRING(PacketBuffer, "1"); 
	VARSTRUCT_ENCODE_STRING(PacketBuffer, From.c_str()); 
	VARSTRUCT_ENCODE_STRING(PacketBuffer, Subject.c_str());

	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::ChangeMailBox(int NewMailBox) {

	_log(UCS__TRACE, "%s Change to mailbox %i", MailBoxName().c_str(), NewMailBox);

	SetMailBox(NewMailBox);

	_log(UCS__TRACE, "New mailbox is %s", MailBoxName().c_str());
						
	EQApplicationPacket *outapp = new EQApplicationPacket(OP_MailboxChange, 2);

	char *buf = (char *)outapp->pBuffer;

	VARSTRUCT_ENCODE_INTSTRING(buf, NewMailBox);
						
	_pkt(UCS__PACKETS, outapp);

	QueuePacket(outapp);

	safe_delete(outapp);
}

void Client::SendFriends() {

	vector<string> Friends, Ignorees;

	database.GetFriendsAndIgnore(GetCharID(), Friends, Ignorees);

	EQApplicationPacket *outapp;

	vector<string>::iterator Iterator;

	Iterator = Friends.begin();

	while(Iterator != Friends.end()) {

		outapp = new EQApplicationPacket(OP_Buddy, (*Iterator).length() + 2);

		char *PacketBuffer = (char *)outapp->pBuffer;

		VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 1);

		VARSTRUCT_ENCODE_STRING(PacketBuffer, (*Iterator).c_str());

		_pkt(UCS__PACKETS, outapp);

		QueuePacket(outapp);

		safe_delete(outapp);

		Iterator++;
	}

	Iterator = Ignorees.begin();

	while(Iterator != Ignorees.end()) {

		string Ignoree = "SOE.EQ." + WorldShortName + "." + (*Iterator);

		outapp = new EQApplicationPacket(OP_Ignore, Ignoree.length() + 2);

		char *PacketBuffer = (char *)outapp->pBuffer;

		VARSTRUCT_ENCODE_TYPE(uint8, PacketBuffer, 0);

		VARSTRUCT_ENCODE_STRING(PacketBuffer, Ignoree.c_str());

		_pkt(UCS__PACKETS, outapp);

		QueuePacket(outapp);

		safe_delete(outapp);

		Iterator++;
	}
}

string Client::MailBoxName() {

	if((Characters.size() == 0) || (CurrentMailBox > (Characters.size() - 1)))
	{
		_log(UCS__ERROR, "MailBoxName() called with CurrentMailBox set to %i and Characters.size() is %i",
		     CurrentMailBox, Characters.size());

		return "";
	}

	_log(UCS__TRACE, "MailBoxName() called with CurrentMailBox set to %i and Characters.size() is %i",
	     CurrentMailBox, Characters.size());

	return Characters[CurrentMailBox].Name;

}

int Client::GetCharID() {

	if(Characters.size() == 0)
		return 0;

	return Characters[0].CharID;
}