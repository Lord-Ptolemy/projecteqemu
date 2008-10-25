/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

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
#include <stdlib.h>
#include <math.h>
#include "masterentity.h"
#include "faction.h"
#include "map.h"
#include "spdat.h"
#include "../common/skills.h"
#include "../common/MiscFunctions.h"
#include "../common/rulesys.h"
#include "StringIDs.h"
#include <iostream>

extern Zone* zone;
//#define LOSDEBUG 6

//look around a client for things which might aggro the client.
void EntityList::CheckClientAggro(Client *around) {
	_ZP(EntityList_CheckClientAggro);

	LinkedListIterator<Mob*> iterator(mob_list);
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance()) {
		_ZP(EntityList_CheckClientAggro_Loop);
		Mob* mob = iterator.GetData();
		if(mob->IsClient())	//also ensures that mob != around
			continue;
		
		if(mob->CheckWillAggro(around)) {
			mob->AddToHateList(around);
		}
	}
}

void EntityList::DescribeAggro(Client *towho, NPC *from_who, float d, bool verbose) {
	float d2 = d*d;
	
	towho->Message(0, "Describing aggro for %s", from_who->GetName());
	
	bool engaged = from_who->IsEngaged();
	if(engaged) {
		Mob *top = from_who->GetHateTop();
		towho->Message(0, ".. I am currently fighting with %s", top == NULL?"(NULL)":top->GetName());
	}
	bool check_npcs = from_who->WillAggroNPCs();
	
	if(verbose) {
		char namebuf[256];
		
		int my_primary = from_who->GetPrimaryFaction();
		Mob *own = from_who->GetOwner();
		if(own != NULL)
			my_primary = own->GetPrimaryFaction();
		
		if(my_primary == 0) {
			strcpy(namebuf, "(No faction)");
		} else if(my_primary < 0) {
			strcpy(namebuf, "(Special faction)");
		} else {
			if(!database.GetFactionName(my_primary, namebuf, sizeof(namebuf)))
				strcpy(namebuf, "(Unknown)");
		}
		towho->Message(0, ".. I am on faction %s (%d)\n", namebuf, my_primary);
	}
	
	LinkedListIterator<Mob*> iterator(mob_list);
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance()) {
		Mob* mob = iterator.GetData();
		if(mob->IsClient())	//also ensures that mob != around
			continue;
		
		if(mob->DistNoRoot(*from_who) > d2)
			continue;
		
		if(engaged) {
			int32 amm = from_who->GetHateAmount(mob);
			if(amm == 0) {
				towho->Message(0, "... %s is not on my hate list.", mob->GetName());
			} else {
				towho->Message(0, "... %s is on my hate list with value %lu", mob->GetName(), amm);
			}
		} else if(!check_npcs && mob->IsNPC()) {
				towho->Message(0, "... %s is an NPC and my npc_aggro is disabled.", mob->GetName());
		} else {
			from_who->DescribeAggro(towho, mob, verbose);
		}
	}
}

void NPC::DescribeAggro(Client *towho, Mob *mob, bool verbose) {
	//this logic is duplicated from below, try to keep it up to date.
	float iAggroRange = GetAggroRange();
	
	float t1, t2, t3;
	t1 = mob->GetX() - GetX();
	t2 = mob->GetY() - GetY();
	t3 = mob->GetZ() - GetZ();
	//Cheap ABS()
	if(t1 < 0)
		t1 = 0 - t1;
	if(t2 < 0)
		t2 = 0 - t2;
	if(t3 < 0)
		t3 = 0 - t3;
	if(   ( t1 > iAggroRange)
	   || ( t2 > iAggroRange)
	   || ( t3 > iAggroRange) ) {
	   towho->Message(0, "...%s is out of range (fast). distances (%.3f,%.3f,%.3f), range %.3f", mob->GetName(),
	   t1, t2, t3, iAggroRange);
	   return;
	}
	
	if(mob->IsInvisible(this)) {
	   towho->Message(0, "...%s is invisible to me. ", mob->GetName());
	   return;
	}
	if((mob->IsClient() &&
	       (!mob->CastToClient()->Connected()
	  	    || mob->CastToClient()->IsLD()
	        || mob->CastToClient()->IsBecomeNPC()
	        || mob->CastToClient()->GetGM()
	       )   
	   ))
	{
	   towho->Message(0, "...%s is my owner. ", mob->GetName());
	   return;
	}
	
	
	if(mob == GetOwner()) {
	   towho->Message(0, "...%s a GM or is not connected. ", mob->GetName());
	   return;
	}

	float dist2  = mob->DistNoRoot(*this);
	float iAggroRange2 = iAggroRange*iAggroRange;
	if( dist2 > iAggroRange2 ) {
	   towho->Message(0, "...%s is out of range. %.3f > %.3f ", mob->GetName(),
	   dist2, iAggroRange2);
	   return;
	}
	
	if(GetINT() > 75 && mob->GetLevelCon(GetLevel()) == CON_GREEN ) {
	   towho->Message(0, "...%s is red to me (basically)", mob->GetName(),
	   dist2, iAggroRange2);
	   return;
	}
	
	if(verbose) {
		int my_primary = GetPrimaryFaction();
		int mob_primary = mob->GetPrimaryFaction();
		Mob *own = GetOwner();
		if(own != NULL)
			my_primary = own->GetPrimaryFaction();
		own = mob->GetOwner();
		if(mob_primary > 0 && own != NULL)
			mob_primary = own->GetPrimaryFaction();
		
		if(mob_primary == 0) {
			towho->Message(0, "...%s has no primary faction", mob->GetName());
		} else if(mob_primary < 0) {
			towho->Message(0, "...%s is on special faction %d", mob->GetName(), mob_primary);
		} else {
			char namebuf[256];
			if(!database.GetFactionName(mob_primary, namebuf, sizeof(namebuf)))
				strcpy(namebuf, "(Unknown)");
			list<struct NPCFaction*>::iterator cur,end;
			cur = faction_list.begin();
			end = faction_list.end();
			bool res = false;
			for(; cur != end; cur++) {
				struct NPCFaction* fac = *cur;
				if ((sint32)fac->factionID == mob_primary) {
					if (fac->npc_value > 0) {
						towho->Message(0, "...%s is on ALLY faction %s (%d) with %d", mob->GetName(), namebuf, mob_primary, fac->npc_value);
						res = true;
						break;
					} else if (fac->npc_value < 0) {
						towho->Message(0, "...%s is on ENEMY faction %s (%d) with %d", mob->GetName(), namebuf, mob_primary, fac->npc_value);
						res = true;
						break;
					} else {
						towho->Message(0, "...%s is on NEUTRAL faction %s (%d) with 0", mob->GetName(), namebuf, mob_primary);
						res = true;
						break;
					}
				}
			}
			if(!res) {
				towho->Message(0, "...%s is on faction %s (%d), which I have no entry for.", mob->GetName(), namebuf, mob_primary);
			}
		}
	}
	
	FACTION_VALUE fv = mob->GetReverseFactionCon(this);
	
	if(!(
			fv == FACTION_SCOWLS
			||
			(mob->GetPrimaryFaction() != GetPrimaryFaction() && mob->GetPrimaryFaction() == -4 && GetOwner() == NULL)
			||
			fv == FACTION_THREATENLY
		)) {
	   towho->Message(0, "...%s faction not low enough. value='%s'", mob->GetName(), FactionValueToString(fv));
	   return;
	}
	if(fv == FACTION_THREATENLY) {
	   towho->Message(0, "...%s threatening to me, so they only have a %d chance per check of attacking.", mob->GetName());
	}
	
	if(!CheckLosFN(mob)) {
	   towho->Message(0, "...%s is out of sight.", mob->GetName());		
	}
	
	towho->Message(0, "...%s meets all conditions, I should be attacking them.", mob->GetName());	
}

/*
	If you change this function, you should update the above function
	to keep the #aggro command accurate.
*/
bool Mob::CheckWillAggro(Mob *mob) {
	if(!mob)
		return false;
	_ZP(Mob_CheckWillAggro);

	//sometimes if a client has some lag while zoning into a dangerous place while either invis or a GM
	//they will aggro mobs even though it's supposed to be impossible, to lets make sure we've finished connecting
	if(mob->IsClient() && !mob->CastToClient()->ClientFinishedLoading())
		return false;
	
	float iAggroRange = GetAggroRange();
	
	// Check If it's invisible and if we can see invis
	// Check if it's a client, and that the client is connected and not linkdead,
	//   and that the client isn't Playing an NPC, with thier gm flag on
	// Check if it's not a Interactive NPC
	// Trumpcard: The 1st 3 checks are low cost calcs to filter out unnessecary distance checks. Leave them at the beginning, they are the most likely occurence.
	// Image: I moved this up by itself above faction and distance checks because if one of these return true, theres no reason to go through the other information
	
	float t1, t2, t3;
	t1 = mob->GetX() - GetX();
	t2 = mob->GetY() - GetY();
	t3 = mob->GetZ() - GetZ();
	//Cheap ABS()
	if(t1 < 0)
		t1 = 0 - t1;
	if(t2 < 0)
		t2 = 0 - t2;
	if(t3 < 0)
		t3 = 0 - t3;
	if(   ( t1 > iAggroRange)
	   || ( t2 > iAggroRange)
	   || ( t3 > iAggroRange)
	   ||(mob->IsInvisible(this))
	   || (mob->IsClient() &&
	       (!mob->CastToClient()->Connected()
	  	    || mob->CastToClient()->IsLD()
	        || mob->CastToClient()->IsBecomeNPC()
	        || mob->CastToClient()->GetGM()
	       )   
	   ))
	{
		return(false);
	}

	//im not sure I understand this..
	//if I have an owner and it is not this mob, then I cannot
	//aggro this mob...???
	//changed to be 'if I have an owner and this is it'
	if(mob == GetOwner()) {
		return(false);
	}

	float dist2  = mob->DistNoRoot(*this);
	float iAggroRange2 = iAggroRange*iAggroRange;

	if( dist2 > iAggroRange2 ) {
		// Skip it, out of range
		return(false);
	}
	
	//Image: Get their current target and faction value now that its required
	//this function call should seem backwards
	FACTION_VALUE fv = mob->GetReverseFactionCon(this);
	
	// Make sure they're still in the zone
	// Are they in range?
	// Are they kos?
	// Are we stupid or are they green
	// and they don't have thier gm flag on
	if
	(
	//old InZone check taken care of above by !mob->CastToClient()->Connected()
	(
		( GetINT() <= 75 )
		||( mob->GetLevelCon(GetLevel()) != CON_GREEN )
	)
	&&
	(
		(
			fv == FACTION_SCOWLS
			||
			(mob->GetPrimaryFaction() != GetPrimaryFaction() && mob->GetPrimaryFaction() == -4 && GetOwner() == NULL)
			||
			(
				fv == FACTION_THREATENLY
				&& MakeRandomInt(0,99) < THREATENLY_ARRGO_CHANCE
			)
		)
	)
	)
	{
		//FatherNiwtit: make sure we can see them. last since it is very expensive
		if(CheckLosFN(mob)) {

			// Aggro
			#if EQDEBUG>=6
				LogFile->write(EQEMuLog::Debug, "Check aggro for %s target %s.", GetName(), mob->GetName());
			#endif
			return(true);
		}
	  }
#if EQDEBUG >= 6
	  printf("Is In zone?:%d\n", mob->InZone());
	  printf("Dist^2: %f\n", dist2);
	  printf("Range^2: %f\n", iAggroRange2);
	  printf("Faction: %d\n", fv);
	  printf("Int: %d\n", GetINT());
	  printf("Con: %d\n", GetLevelCon(mob->GetLevel()));
#endif		
	return(false);
}

Mob* EntityList::AICheckCloseAggro(Mob* sender, float iAggroRange, float iAssistRange) {
	if (!sender || !sender->IsNPC())
		return(NULL);
	_ZP(EntityList_AICheckCloseAggro);

#ifdef REVERSE_AGGRO
	//with reverse aggro, npc->client is checked elsewhere, no need to check again
	LinkedListIterator<NPC*> iterator(npc_list);
#else
	LinkedListIterator<Mob*> iterator(mob_list);
#endif
	iterator.Reset();
	//float distZ;
	while(iterator.MoreElements()) {
		Mob* mob = iterator.GetData();
		
		if(sender->CheckWillAggro(mob)) {
			return(mob);
		}
		
		iterator.Advance();
	}
	//LogFile->write(EQEMuLog::Debug, "Check aggro for %s no target.", sender->GetName());
	return(NULL);
}

int EntityList::GetHatedCount(Mob *attacker, Mob *exclude) {

	// Return a list of how many non-feared, non-mezzed, non-green mobs, within aggro range, hate *attacker

	if(!attacker) return 0;

	int Count = 0;

	LinkedListIterator<NPC*> iterator(npc_list);

	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance()) {

		NPC* mob = iterator.GetData();

		if(!mob || (mob == exclude)) continue;
		
		if(!mob->IsEngaged()) continue;

		if(mob->IsFeared() || mob->IsMezzed()) continue;

		if(attacker->GetLevelCon(mob->GetLevel()) == CON_GREEN) continue;

		if(!mob->CheckAggro(attacker)) continue;

		float AggroRange = mob->GetAggroRange();

		// Square it because we will be using DistNoRoot
			
		AggroRange = AggroRange * AggroRange;

		if(mob->DistNoRoot(*attacker) > AggroRange) continue;

		Count++;

	}

	return Count;

}

void EntityList::AIYellForHelp(Mob* sender, Mob* attacker) {
	_ZP(EntityList_AIYellForHelp);
	if(!sender || !attacker)
		return;
	if (sender->GetPrimaryFaction() == 0 )
		return; // well, if we dont have a faction set, we're gonna be indiff to everybody
	
	LinkedListIterator<NPC*> iterator(npc_list);
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance()) {
		NPC* mob = iterator.GetData();
		if(!mob){
			continue;
		}
		float r = mob->GetAssistRange();
		r = r * r;

		if (
			mob != sender
			&& mob != attacker
//			&& !mob->IsCorpse()
//			&& mob->IsAIControlled()
			&& mob->GetPrimaryFaction() != 0
			&& mob->DistNoRoot(*sender) <= r
			&& !mob->IsEngaged()
			)
		{
			//if they are in range, make sure we are not green...
			//then jump in if they are our friend
			if(attacker->GetLevelCon(mob->GetLevel()) != CON_GREEN)
			{
				bool useprimfaction = false;
				if(mob->GetPrimaryFaction() == sender->CastToNPC()->GetPrimaryFaction())
				{
					const NPCFactionList *cf = database.GetNPCFactionEntry(mob->GetNPCFactionID());
					if(cf){
						if(cf->assistprimaryfaction != 0)
							useprimfaction = true;
					}
				}

				if(useprimfaction || sender->GetReverseFactionCon(mob) <= FACTION_AMIABLE )
				{
					//attacking someone on same faction, or a friend
					//Father Nitwit:  make sure we can see them.
					if(mob->CheckLosFN(sender)) {
#if (EQDEBUG>=5) 
						LogFile->write(EQEMuLog::Debug, "AIYellForHelp(\"%s\",\"%s\") %s attacking %s Dist %f Z %f", 
						sender->GetName(), attacker->GetName(), mob->GetName(), attacker->GetName(), mob->DistNoRoot(*sender), fabs(sender->GetZ()+mob->GetZ()));
#endif
						mob->AddToHateList(attacker, 1, 0, false);
					}
				}
			}
		}
	}
}

/*
solar: returns false if attack should not be allowed
I try to list every type of conflict that's possible here, so it's easy
to see how the decision is made.  Yea, it could be condensed and made
faster, but I'm doing it this way to make it readable and easy to modify
*/

bool Mob::IsAttackAllowed(Mob *target)
{
	Mob *mob1, *mob2, *tempmob;
	Client *c1, *c2, *becomenpc;
//	NPC *npc1, *npc2;
	int reverse;

#ifdef EQBOTS

    //franck-add: a bot can't attack a bot. A bot can't attack its leader(client). You(client) can't attack your bots.
	if((IsBot() && (target->IsBot() || target->IsClient())) || (IsClient() && target->IsBot()))
		return false;    

#endif //EQBOTS

	if(!zone->CanDoCombat())
		return false;

	// some special cases
	if(!target)
		return false;

	if(this == target)	// you can attack yourself
		return true;

	// can't damage own pet (applies to everthing)
	Mob *target_owner = target->GetOwner();
	Mob *our_owner = GetOwner();
	if(target_owner && target_owner == this)
		return false;
	else if(our_owner && our_owner == target)
		return false;
	
	//cannot hurt untargetable mobs
	bodyType bt = target->GetBodyType();

	if(bt == BT_NoTarget || bt == BT_NoTarget2)
		return(false);
	
	// solar: the format here is a matrix of mob type vs mob type.
	// redundant ones are omitted and the reverse is tried if it falls through.

	// cb: LDoN Treasure chests dont fight
	if(this->GetClass() == LDON_TREASURE)
		return(false);

	
	// first figure out if we're pets.  we always look at the master's flags.
	// no need to compare pets to anything
	mob1 = our_owner ? our_owner : this;
	mob2 = target_owner ? target_owner : target;

#ifdef EQBOTS

    // franck-add: Bots pet can't attack others bots and there pets. Clients and their pet can't attack bot pets.
	if(mob1->IsClient()) {
		if(mob2->IsBot())
			return false;
	}
	
	else if(mob1->IsBot()){
		if(mob2->IsBot())
			return false;
		else if(mob2->IsClient())
			return false;
	}

#endif //EQBOTS

	reverse = 0;
	do
	{
		if(_CLIENT(mob1))
		{
			if(_CLIENT(mob2))					// client vs client
			{
				c1 = mob1->CastToClient();
				c2 = mob2->CastToClient();
	
				if	// if both are pvp they can fight
				(
					c1->GetPVP() &&
					c2->GetPVP()
				)
					return true;
				else if	// if they're dueling they can go at it
				(
					c1->IsDueling() &&
					c2->IsDueling() &&
					c1->GetDuelTarget() == c2->GetID() &&
					c2->GetDuelTarget() == c1->GetID()
				)
					return true;
				else
					return false;
			}
			else if(_NPC(mob2))				// client vs npc
			{	
				return true;
			}
			else if(_BECOMENPC(mob2))	// client vs becomenpc
			{
				c1 = mob1->CastToClient();
				becomenpc = mob2->CastToClient();
	
				if(c1->GetLevel() > becomenpc->GetBecomeNPCLevel())
					return false;
				else
					return true;
			}	
			else if(_CLIENTCORPSE(mob2))	// client vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client vs npc corpse
			{
				return false;
			}
		}
		else if(_NPC(mob1))
		{
			if(_NPC(mob2))						// npc vs npc
			{
/*
this says that an NPC can NEVER attack a faction ally...
this is stupid... somebody else should check this rule if they want to
enforce it, this just says 'can they possibly fight based on their
type', in which case, the answer is yes.
*/
/*				npc1 = mob1->CastToNPC();
				npc2 = mob2->CastToNPC();
				if
				(
					npc1->GetPrimaryFaction() != 0 &&
					npc2->GetPrimaryFaction() != 0 &&
					(
						npc1->GetPrimaryFaction() == npc2->GetPrimaryFaction() ||
						npc1->IsFactionListAlly(npc2->GetPrimaryFaction())
					)
				)
					return false;
				else
*/
					return true;
			}
			else if(_BECOMENPC(mob2))	// npc vs becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// npc vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// npc vs npc corpse
			{
				return false;
			}
		}
		else if(_BECOMENPC(mob1))
		{
			if(_BECOMENPC(mob2))			// becomenpc vs becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// becomenpc vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// becomenpc vs npc corpse
			{
				return false;
			}
		}
		else if(_CLIENTCORPSE(mob1))
		{
			if(_CLIENTCORPSE(mob2))		// client corpse vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client corpse vs npc corpse
			{
				return false;
			}
		}
		else if(_NPCCORPSE(mob1))
		{
			if(_NPCCORPSE(mob2))			// npc corpse vs npc corpse
			{
				return false;
			}
		}

		// we fell through, now we swap the 2 mobs and run through again once more
		tempmob = mob1;
		mob1 = mob2;
		mob2 = tempmob;
	}
	while( reverse++ == 0 );

	LogFile->write(EQEMuLog::Debug, "Mob::IsAttackAllowed: don't have a rule for this - %s vs %s\n", this->GetName(), target->GetName());
	return false;
}


// solar: this is to check if non detrimental things are allowed to be done
// to the target.  clients cannot affect npcs and vice versa, and clients
// cannot affect other clients that are not of the same pvp flag as them.
// also goes for their pets
bool Mob::IsBeneficialAllowed(Mob *target)
{
	Mob *mob1, *mob2, *tempmob;
	Client *c1, *c2;
	int reverse;

	if(!target)
		return false;

#ifdef EQBOTS

    //franck-add: Eqoffline.
	if(IsClient() && target->IsBot())
		return true;
	else if(IsBot() && target->IsClient())
		return true;
	else if(IsBot() && target->IsBot())
		return true;

#endif //EQBOTS

	// solar: see IsAttackAllowed for notes
	
	// first figure out if we're pets.  we always look at the master's flags.
	// no need to compare pets to anything
	mob1 = this->GetOwnerID() ? this->GetOwner() : this;
	mob2 = target->GetOwnerID() ? target->GetOwner() : target;

	// if it's self target or our own pet it's ok
	if(mob1 == mob2)
		return true;

	reverse = 0;
	do
	{
		if(_CLIENT(mob1))
		{
			if(_CLIENT(mob2))					// client to client
			{
				c1 = mob1->CastToClient();
				c2 = mob2->CastToClient();

				if(c1->GetPVP() == c2->GetPVP())
					return true;
				else if	// if they're dueling they can heal each other too
				(
					c1->IsDueling() &&
					c2->IsDueling() &&
					c1->GetDuelTarget() == c2->GetID() &&
					c2->GetDuelTarget() == c1->GetID()
				)
					return true;
				else
					return false;
			}
			else if(_NPC(mob2))				// client to npc
			{
                /* fall through and swap positions */
			}
			else if(_BECOMENPC(mob2))	// client to becomenpc
			{
				return false;
			}
			else if(_CLIENTCORPSE(mob2))	// client to client corpse
			{
				return true;
			}
			else if(_NPCCORPSE(mob2))	// client to npc corpse
			{
				return false;
			}
		}
		else if(_NPC(mob1))
		{
			if(_CLIENT(mob2))
			{
#ifdef GUILDWARS
				return true;
#else
				return false;
#endif
			}
			if(_NPC(mob2))						// npc to npc
			{
				return true;
			}
			else if(_BECOMENPC(mob2))	// npc to becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// npc to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// npc to npc corpse
			{
				return false;
			}
		}
		else if(_BECOMENPC(mob1))
		{
			if(_BECOMENPC(mob2))			// becomenpc to becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// becomenpc to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// becomenpc to npc corpse
			{
				return false;
			}
		}
		else if(_CLIENTCORPSE(mob1))
		{
			if(_CLIENTCORPSE(mob2))		// client corpse to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client corpse to npc corpse
			{
				return false;
			}
		}
		else if(_NPCCORPSE(mob1))
		{
			if(_NPCCORPSE(mob2))			// npc corpse to npc corpse
			{
				return false;
			}
		}

		// we fell through, now we swap the 2 mobs and run through again once more
		tempmob = mob1;
		mob1 = mob2;
		mob2 = tempmob;
	}
	while( reverse++ == 0 );

	LogFile->write(EQEMuLog::Debug, "Mob::IsBeneficialAllowed: don't have a rule for this - %s to %s\n", this->GetName(), target->GetName());
	return false;
}

bool Mob::CombatRange(Mob* other)
{
	if(!other)
		return(false);

	float size_mod = GetSize();
	float other_size_mod = other->GetSize();

	if(GetRace() == 49 || GetRace() == 158 || GetRace() == 196) //For races with a fixed size
		size_mod = 60.0f;
	else if (size_mod < 6.0)
		size_mod = 8.0f;

	if(other->GetRace() == 49 || other->GetRace() == 158 || other->GetRace() == 196) //For races with a fixed size
		other_size_mod = 60.0f;
	else if (other_size_mod < 6.0)
		other_size_mod = 8.0f;

	if (other_size_mod > size_mod)
		size_mod = other_size_mod;

#ifdef EQBOTS

	if(IsBot()) {
		if(other->GetRace() == 49 || other->GetRace() == 158 || other->GetRace() == 196) { //For races with a fixed size
			// have warrior bots get a little closer to the target
			if(GetClass() == WARRIOR) {
				size_mod = 45.0f;
			}
			else if((GetClass() == PALADIN) || (GetClass() == RANGER) || (GetClass() == SHADOWKNIGHT) || (GetClass() == MONK) || (GetClass() == ROGUE) || (GetClass() == BEASTLORD) || (GetClass() == BERSERKER) || (GetClass() == BARD)) {
				size_mod = 50.0f;
			}
			else {
				size_mod = 55.0f;
			}
		}
		else {
			// have warrior bots get a little closer to the target
			if(GetClass() == WARRIOR) {
				size_mod = 5.5f;
			}
			else if((GetClass() == PALADIN) || (GetClass() == RANGER) || (GetClass() == SHADOWKNIGHT) || (GetClass() == MONK) || (GetClass() == ROGUE) || (GetClass() == BEASTLORD) || (GetClass() == BERSERKER) || (GetClass() == BARD)) {
				size_mod = 6.0f;
			}
			else {
				size_mod = 7.0f;
			}
		}
	}
	if(IsPet() && GetOwner()->IsBot()) {
		if(other->GetRace() == 49 || other->GetRace() == 158 || other->GetRace() == 196) { //For races with a fixed size
			size_mod = 47.0f;
		}
		else {
			size_mod = 5.7f;
		}
	}

#endif //EQBOTS

	size_mod *= size_mod * 4;
	if (DistNoRootNoZ(*other) <= size_mod)
		return true;
	return false;
}

//Old LOS function, prolly not used anymore
//Not removed because I havent looked it over to see if anything
//useful is in here before we delete it.
bool Mob::CheckLos(Mob* other) {
	if (zone->map == 0)
	{
		return true;
	}
	float tmp_x = GetX();
	float tmp_y = GetY();
	float tmp_z = GetZ();
	float trg_x = other->GetX();
	float trg_y = other->GetY();
	float trg_z = other->GetZ();
	float perwalk_x = 0.5;
	float perwalk_y = 0.5;
	float perwalk_z = 0.5;
	float dist_x = tmp_x - trg_x;
	if (dist_x < 0)
		dist_x *= -1;
	float dist_y = tmp_y - trg_y;
	if (dist_y < 0)
		dist_y *= -1;
	float dist_z = tmp_z - trg_z;
	if (dist_z < 0)
		dist_z *= -1;
	if (dist_x  < dist_y && dist_z < dist_y)
	{
		perwalk_x /= (dist_y/dist_x);
		perwalk_z /= (dist_y/dist_z);
	}
	else if (dist_y  < dist_x && dist_z < dist_x)
	{
		perwalk_y /= (dist_x/dist_y);
		perwalk_z /= (dist_x/dist_z);
	}
	else if (dist_x  < dist_z && dist_y < dist_z)
	{
		perwalk_x /= (dist_z/dist_x);
		perwalk_y /= (dist_z/dist_y);
	}
	float steps = (dist_x/perwalk_x + dist_y/perwalk_y + dist_z/perwalk_z)*10; //Just a safety check to prevent endless loops.
	while (steps > 0) {
		steps--;
		if (tmp_x < trg_x)
		{
			if (tmp_x + perwalk_x < trg_x)
				tmp_x += perwalk_x;
			else
				tmp_x = trg_x;
		}
		if (tmp_y < trg_y)
		{
			if (tmp_y + perwalk_y < trg_y)
				tmp_y += perwalk_y;
			else
				tmp_y = trg_y;
		}
		if (tmp_z < trg_z)
		{
			if (tmp_z + perwalk_z < trg_z)
				tmp_z += perwalk_z;
			else
				tmp_z = trg_z;
		}
		if (tmp_x > trg_x)
		{
			if (tmp_x - perwalk_x > trg_x)
				tmp_x -= perwalk_x;
			else
				tmp_x = trg_x;
		}
		if (tmp_y > trg_y)
		{
			if (tmp_y - perwalk_y > trg_y)
				tmp_y -= perwalk_y;
			else
				tmp_y = trg_y;
		}
		if (tmp_z > trg_z)
		{
			if (tmp_z - perwalk_z > trg_z)
				tmp_z -= perwalk_z;
			else
				tmp_z = trg_z;
		}
		if (tmp_y == trg_y && tmp_x == trg_x && tmp_z == trg_z)
		{
			return true;
		}

//I believe this is contributing to breaking mob spawns when a map is loaded
//		NodeRef pnode = zone->map->SeekNode( zone->map->GetRoot(), tmp_x, tmp_y );
		NodeRef pnode = NODE_NONE;
		if (pnode != NODE_NONE)
		{
			const int *iface = zone->map->SeekFace( pnode, tmp_x, tmp_y );
			if (*iface == -1) {
				return false;
			}
			float temp_z = 0;
			float best_z = 999999;
			while(*iface != -1)
			{
				temp_z = zone->map->GetFaceHeight( *iface, x_pos, y_pos );
//UMM.. OMG... sqrtf(pow(x, 2)) == x.... retards
				float best_dist = sqrtf((float)(pow(best_z-tmp_z, 2)));
				float tmp_dist = sqrtf((float)(pow(tmp_z-tmp_z, 2)));
				if (tmp_dist < best_dist)
				{
					best_z = temp_z;
				}
				iface++;
			}
/*	solar: our aggro code isn't using this right now, just spells, so i'm
    taking out the +-10 check for now to make it work right on hills
			if (best_z - 10 > trg_z || best_z + 10 < trg_z)
			{
				return false;
			}
*/
		}
	}
	return true;
}


//Father Nitwit's LOS code
bool Mob::CheckLosFN(Mob* other) {
	if(other == NULL || zone->map == NULL) {
		//not sure what the best return is on error
		//should make this a database variable, but im lazy today
#ifdef LOS_DEFAULT_CAN_SEE
		return(true);
#else
		return(false);
#endif
	}
	_ZP(Mob_CheckLosFN);
	
	VERTEX myloc;
	VERTEX oloc;
	
#define LOS_DEFAULT_HEIGHT 6.0f
	
	myloc.x = GetX();
	myloc.y = GetY();
	myloc.z = GetZ() + (GetSize()==0.0?LOS_DEFAULT_HEIGHT:GetSize())/2 * HEAD_POSITION;
	
	oloc.x = other->GetX();
	oloc.y = other->GetY();
	oloc.z = other->GetZ() + (other->GetSize()==0.0?LOS_DEFAULT_HEIGHT:other->GetSize())/2 * SEE_POSITION;

#if LOSDEBUG>=5
	LogFile->write(EQEMuLog::Debug, "LOS from (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f) sizes: (%.2f, %.2f)", myloc.x, myloc.y, myloc.z, oloc.x, oloc.y, oloc.z, GetSize(), other->GetSize());
#endif
	
	FACE *onhit;
	NodeRef mynode;
	NodeRef onode;
	
	VERTEX hit;
	//see if anything in our node is in the way
	mynode = zone->map->SeekNode( zone->map->GetRoot(), myloc.x, myloc.y);
	if(mynode != NODE_NONE) {
		if(zone->map->LineIntersectsNode(mynode, myloc, oloc, &hit, &onhit)) {
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see.", GetName(), other->GetName() );
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
			return(false);
		}
	}
#if LOSDEBUG>=5
	 else {
		LogFile->write(EQEMuLog::Debug, "WTF, I have no node, what am I standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
	}
#endif
	
	//see if they are in a different node.
	//if so, see if anything in their node is blocking me.
	if(! zone->map->LocWithinNode(mynode, oloc.x, oloc.y)) {
		onode = zone->map->SeekNode( zone->map->GetRoot(), oloc.x, oloc.y);
		if(onode != NODE_NONE && onode != mynode) {
			if(zone->map->LineIntersectsNode(onode, myloc, oloc, &hit, &onhit)) {
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see (2).", GetName(), other->GetName());
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
				return(false);
			}
		}
#if LOSDEBUG>=5
		 else if(onode == NODE_NONE) {
			LogFile->write(EQEMuLog::Debug, "WTF, They have no node, what are they standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
		}
#endif
	}
	
	/*
	if(zone->map->LineIntersectsZone(myloc, oloc, CHECK_LOS_STEP, &onhit)) {
#if LOSDEBUG>=5
		LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see.", GetName(), other->GetName() );
		LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
			onhit->a.x, onhit->a.y, onhit->a.z,
			onhit->b.x, onhit->b.y, onhit->b.z, 
			onhit->c.x, onhit->c.y, onhit->c.z);
#endif
		return(false);
	}*/
	
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, CAN SEE.", GetName(), other->GetName());
#endif
	
	return(true);
}

//offensive spell aggro
sint32 Mob::CheckAggroAmount(int16 spellid) {
	int16 spell_id = spellid;
	sint32 AggroAmount = 0;
	sint32 nonModifiedAggro = 0;
	int16 slevel = GetLevel();

	for (int o = 0; o < EFFECT_COUNT; o++) {
		switch(spells[spell_id].effectid[o]) {
			case SE_CurrentHPOnce:
			case SE_CurrentHP:{
					int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
					if(val < 0)
						AggroAmount -= val;
					break;
				}
			case SE_MovementSpeed: {
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 0)
				{
					AggroAmount+=((1 + slevel*2)*RuleI(Aggro, MovementImpairAggroMod)/100);
					break;
				}
				break;
			}
			case SE_AttackSpeed:
			case SE_AttackSpeed2:
			case SE_AttackSpeed3:{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 100)
				{
					AggroAmount+=((1 + slevel*2)*RuleI(Aggro, SlowAggroMod)/100);
				}
				break;
			}
			case SE_Stun: {
				if (spells[spell_id].base[o] > 1)
					AggroAmount+=((1 + slevel*4)*RuleI(Aggro, IncapacitateAggroMod)/100);
				else
					AggroAmount+=((1 + slevel*2)*RuleI(Aggro, IncapacitateAggroMod)/100);
				break;
			}
			case SE_Blind: {
				AggroAmount+=((1 + slevel*2)*RuleI(Aggro, IncapacitateAggroMod)/100);
				break;
			}
			case SE_Mez: {
				AggroAmount+=((1 + slevel*2)*RuleI(Aggro, IncapacitateAggroMod)/100);
				break;
			}
			case SE_Charm: {
				AggroAmount+=((1 + slevel*2)*RuleI(Aggro, IncapacitateAggroMod)/100);
				break;
			}
			case SE_Root: {
				AggroAmount+=((1 + slevel*2)*RuleI(Aggro, MovementImpairAggroMod)/100);
				break;
			}
			case SE_Fear: {
				AggroAmount+=((1 + slevel*2)*RuleI(Aggro, IncapacitateAggroMod)/100);
				break;
			}
			case SE_ATK:
			case SE_ArmorClass:	{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 0)
				{
					AggroAmount+=(slevel*8) - (val/2);				
				}				
				break;
			}
			case SE_ResistMagic:
			case SE_ResistFire:
			case SE_ResistCold:
			case SE_ResistPoison:
			case SE_ResistDisease:{
					int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
					if (val < 0)
					{
						AggroAmount -= val*2;
					}
					break;
			}
			case SE_ResistAll:{
					int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
					if (val < 0)
					{
						AggroAmount -= val*6;
					}
					break;
			}
			case SE_STR:
			case SE_STA:
			case SE_DEX:
			case SE_AGI:
			case SE_INT:
			case SE_WIS:
			case SE_CHA:{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 0)
				{
					AggroAmount -= val*2;
				}
				break;
			}
			case SE_AllStats:{
					int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
					if (val < 0)
					{
						AggroAmount -= val*6;
					}
					break;
			}
			case SE_BardAEDot:{
					AggroAmount += slevel*2;
					break;
			}
			case SE_SpinTarget:{
					AggroAmount+=((1 + slevel*2)*RuleI(Aggro, IncapacitateAggroMod)/100);
					break;
			}
			case SE_Amnesia:
			case SE_Silence:{
				AggroAmount += slevel*2;			
				break;
			}
			case SE_Destroy:{
				AggroAmount += slevel*2;
				break;
			}
			case SE_Harmony:
			case SE_CastingLevel:
			case SE_MeleeMitigation:
			case SE_CriticalHitChance:
			case SE_AvoidMeleeChance:
			case SE_RiposteChance:
			case SE_DodgeChance:
			case SE_ParryChance:
			case SE_DualWeildChance:
			case SE_DoubleAttackChance:
			case SE_MeleeSkillCheck:
			case SE_HitChance:
			case SE_DamageModifier:
			case SE_MinDamageModifier:
			case SE_IncreaseBlockChance:
			case SE_Accuracy:{
				AggroAmount += slevel*2;
				break;
			}
			case SE_CurrentMana:	
			case SE_ManaPool:
			case SE_CurrentEndurance:{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 0)
				{
					AggroAmount -= val*2;
				}
				break;
			}
			case SE_CancelMagic:
			case SE_DispelDetrimental:{
				AggroAmount += slevel;			
				break;
			}
			case SE_Calm:{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				nonModifiedAggro = val;
				break;
			}
		}
	}
	if (IsBardSong(spell_id))
		AggroAmount = AggroAmount * RuleI(Aggro, SongAggroMod) / 100;
	if (GetOwner())
		AggroAmount = AggroAmount * RuleI(Aggro, PetSpellAggroMod) / 100;
	
	int aaSubtlety = ( GetAA(aaSpellCastingSubtlety) > GetAA(aaSpellCastingSubtlety2) ) ? GetAA(aaSpellCastingSubtlety) : GetAA(aaSpellCastingSubtlety2);

	switch (aaSubtlety)
	{
	case 1:
		AggroAmount = AggroAmount * 95 / 100;
		break;
	case 2:
		AggroAmount = AggroAmount * 90 / 100;
		break;
	case 3:
		AggroAmount = AggroAmount * 80 / 100;
		break;
	}

#ifdef EQBOTS

	// Spell Casting Subtlety for Bots
	if(IsBot()) {
		if(GetLevel() >= 57) {
			AggroAmount = AggroAmount * 95 / 100;
		}
		else if(GetLevel() >= 56) {
			AggroAmount = AggroAmount * 90 / 100;
		}
		else if(GetLevel() >= 55) {
			AggroAmount = AggroAmount * 80 / 100;
		}
	}

#endif //EQBOTS

	AggroAmount = (AggroAmount * RuleI(Aggro, SpellAggroMod))/100;
	AggroAmount += spells[spell_id].HateAdded + spells[spell_id].bonushate + nonModifiedAggro;
	return AggroAmount;
}

//healing and buffing aggro
//I dont think this accounts for direct healing spells.
sint32 Mob::CheckHealAggroAmount(int16 spellid) {
	int16 spell_id = spellid;
	sint32 AggroAmount = 0;
	int16 slevel = GetLevel();

	for (int o = 0; o < EFFECT_COUNT; o++) {
		switch(spells[spell_id].effectid[o]) {
			case SE_Rune:
			case SE_CurrentHP:{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if(val > 0){
					if(val > 3000){
						AggroAmount += 2000 + (val - 3000) * 1 / 10;
					}
					else{
						AggroAmount += val*2/3;
					}
				}
				break;
			}
			case SE_HealOverTime: {
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if(val > 0)
					AggroAmount += val*2/3;
				break;
			}
			default:{
				AggroAmount += 0;
 				break;
 			}
		}
	}
	if (IsBardSong(spell_id))
		AggroAmount = AggroAmount * RuleI(Aggro, SongAggroMod) / 100;
	if (GetOwner())
		AggroAmount = AggroAmount * RuleI(Aggro, PetSpellAggroMod) / 100;

	AggroAmount = (AggroAmount * RuleI(Aggro, SpellAggroMod))/100;

	if(AggroAmount < 0)
		return 0;
	else
		return AggroAmount;
}

void Mob::AddFeignMemory(Client* attacker) {
	if(feign_memory_list.empty() && AIfeignremember_timer != NULL)
		AIfeignremember_timer->Start(AIfeignremember_delay);
	feign_memory_list.insert(attacker->CharacterID());
}

void Mob::RemoveFromFeignMemory(Client* attacker) {
	feign_memory_list.erase(attacker->CharacterID());
	if(feign_memory_list.empty() && AIfeignremember_timer != NULL)
	   AIfeignremember_timer->Disable();
}

void Mob::ClearFeignMemory() {
	feign_memory_list.clear();
	if(AIfeignremember_timer != NULL)
		AIfeignremember_timer->Disable();
}

bool Mob::PassCharismaCheck(Mob* caster, Mob* spellTarget, int16 spell_id) {
	bool Result = false;

	if(!caster) return false;

	if(spells[spell_id].ResistDiff <= -600)
		return true;

	float r1 = ((((float)spellTarget->GetMR() + spellTarget->GetLevel()) / 3) / spellTarget->GetMaxMR()) + ((float)MakeRandomFloat(-10, 10) / 100.0f);
	float r2 = 0.0f;

	if(IsCharmSpell(spell_id)) {
		// Assume this is a charm spell
		int32 TotalDominationRank = 0.00f;
		float TotalDominationBonus = 0.00f;

		if(caster->IsClient())
			TotalDominationRank = caster->CastToClient()->GetAA(aaTotalDomination);

		// WildcardX: If someone ever finds for certain what value the TotalDomination ranks provide, please change the values
		// I implemented below.

		switch(TotalDominationRank) {
			case 1 :
				TotalDominationBonus = 0.05f;
				break;
			case 2 :
				TotalDominationBonus = 0.10f;
				break;
			case 3 :
				TotalDominationBonus = 0.15f;
				break;
			default :
				TotalDominationBonus = 0.00f;
		}

		r2 = ((((float)caster->GetCHA()  + caster->GetLevel()) / 3) / caster->GetMaxCHA()) + ((float)MakeRandomFloat(-10, 10) / 100.0f) + TotalDominationBonus;
	}
	else
		// Assume this is a harmony/pacify spell
		r2 = ((((float)caster->GetCHA()  + caster->GetLevel()) / 3) / caster->GetMaxCHA()) + ((float)MakeRandomFloat(-10, 10) / 100.0f);

	if(r1 < r2)
			Result = true;

	return Result;
}



