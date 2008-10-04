/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemulator.net)

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

#if EQDEBUG >= 5
//#define ATTACK_DEBUG 20
#endif

#include "../common/debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
using namespace std;
#include <assert.h>

#include "masterentity.h"
#include "NpcAI.h"
#include "../common/packet_dump.h"
#include "../common/eq_packet_structs.h"
#include "../common/eq_constants.h"
#include "../common/skills.h"
#include "spdat.h"
#include "zone.h"
#include "StringIDs.h"
#include "../common/MiscFunctions.h"
#include "../common/rulesys.h"

#ifdef WIN32
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif


extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif

#ifdef RAIDADDICTS
#include "RaidAddicts.h"
extern RaidAddicts raidaddicts;
#endif


extern Zone* zone;

bool Mob::AttackAnimation(SkillType &skillinuse, int Hand, const ItemInst* weapon)
{
	// Determine animation
	int type = 0;
	if (weapon && weapon->IsType(ItemClassCommon)) {
		const Item_Struct* item = weapon->GetItem();
#if EQDEBUG >= 11
			LogFile->write(EQEMuLog::Debug, "Weapon skill:%i", item->ItemType);
#endif		
		switch (item->ItemType)
		{
		case ItemType1HS: // 1H Slashing
		{
			skillinuse = _1H_SLASHING;
			type = anim1HWeapon;
			break;
		}
		case ItemType2HS: // 2H Slashing
		{
			skillinuse = _2H_SLASHING;
			type = anim2HSlashing;
			break;
		}
		case ItemTypePierce: // Piercing
		{
			skillinuse = PIERCING;
			type = animPiercing;
			break;
		}
		case ItemType1HB: // 1H Blunt
		{
			skillinuse = _1H_BLUNT;
			type = anim1HWeapon;
			break;
		}
		case ItemType2HB: // 2H Blunt
		{
			skillinuse = _2H_BLUNT;
			type = anim2HWeapon;
			break;
		}
		case ItemType2HPierce: // 2H Piercing
		{
			skillinuse = PIERCING;
			type = anim2HWeapon;
			break;
		}
		case ItemTypeHand2Hand:
		{
			skillinuse = HAND_TO_HAND;
			type = animHand2Hand;
			break;
		}
		default:
		{
			skillinuse = HAND_TO_HAND;
			type = animHand2Hand;
			break;
		}
		}// switch
	}
	else
	{
		skillinuse = HAND_TO_HAND;
		type = animHand2Hand;
	}
	
	// Kaiyodo - If we're attacking with the seconary hand, play the duel wield anim
	if (Hand == 14)	// DW anim
		type = animDualWeild;
	
	DoAnim(type);
    return true;
}

// solar: called when a mob is attacked, does the checks to see if it's a hit
// and does other mitigation checks.  'this' is the mob being attacked.
bool Mob::CheckHitChance(Mob* other, SkillType skillinuse, int Hand)
{
/*
	Father Nitwit:
		Reworked a lot of this code to achieve better balance at higher levels.
		The old code basically meant that any in high level (50+) combat,
		both parties always had 95% chance to hit the other one.
*/
	Mob *attacker=other;
	Mob *defender=this;
	float chancetohit = 54.0f;

#if ATTACK_DEBUG>=11
		LogFile->write(EQEMuLog::Debug, "CheckHitChance(%s) attacked by %s", defender->GetName(), attacker->GetName());
#endif
	
	bool pvpmode = false;
	if(IsClient() && other->IsClient())
		pvpmode = true;
	
	float bonus;
	
	////////////////////////////////////////////////////////
	// To hit calcs go here
	////////////////////////////////////////////////////////

	int8 attacker_level = attacker->GetLevel() ? attacker->GetLevel() : 1;
	int8 defender_level = defender->GetLevel() ? defender->GetLevel() : 1;

	//Calculate the level difference
	sint32 level_difference = attacker_level - defender_level;
	if(level_difference < -20) level_difference = -20;
	if(level_difference > 20) level_difference = 20;
	chancetohit += (145 * level_difference / 100);
	chancetohit -= ((float)defender->GetAGI() * 0.015);

#ifdef EQBOTS

	if(attacker->IsBot()){
		int skilldiff = defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse);
		bonus = 0;
		if(pvpmode){
			if(skilldiff > 10){
				bonus = -(2 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse) - 10) / 10));
			}
			else if(skilldiff <= 10 && skilldiff > 0){
				bonus = -(1 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse)) / 25));
			}
			else{
				bonus = (attacker->GetSkill(skillinuse) - defender->GetSkill(DEFENSE)) / 25;
			}
		}
		else{
			if(skilldiff > 10){
				bonus = -(4 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse) - 10) * 1 / 5));
			}
			else if(skilldiff <= 10 && skilldiff > 0){
				bonus = -(2 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse)) / 10));
			}
			else{
				bonus = 1 + (((attacker->GetSkill(skillinuse) - defender->GetSkill(DEFENSE)) / 25));
			}
		}
		chancetohit += bonus;
	}
	else

#endif //EQBOTS

	if(attacker->IsClient()){
		int skilldiff = defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse);
		bonus = 0;
		if(pvpmode){
			if(skilldiff > 10){
				bonus = -(2 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse) - 10) / 10));
			}
			else if(skilldiff <= 10 && skilldiff > 0){
				bonus = -(1 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse)) / 25));
			}
			else{
				bonus = (attacker->GetSkill(skillinuse) - defender->GetSkill(DEFENSE)) / 25;
			}
		}
		else{
			if(skilldiff > 10){
				bonus = -(4 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse) - 10) * 1 / 5));
			}
			else if(skilldiff <= 10 && skilldiff > 0){
				bonus = -(2 + ((defender->GetSkill(DEFENSE) - attacker->GetSkill(skillinuse)) / 10));
			}
			else{
				bonus = 1 + (((attacker->GetSkill(skillinuse) - defender->GetSkill(DEFENSE)) / 25));
			}
		}
		chancetohit += bonus;
	}
	else{
		//some class combos have odd caps, base our attack skill based off of a warriors 1hslash since 
		//it scales rather evenly across all levels for warriors, based on the defenders level so things like
		//light blues can still hit their target.
		uint16 atkSkill = (database.GetSkillCap(WARRIOR, (SkillType)_1H_SLASHING, defender->GetLevel()) + 5);
		int skilldiff = defender->GetSkill(DEFENSE) - atkSkill;
		bonus = 0;
		if(skilldiff > 10){
			bonus = -(4 + ((defender->GetSkill(DEFENSE) - atkSkill - 10) * 1 / 5));
		}
		else if(skilldiff <= 10 && skilldiff > 0){
			bonus = -(2 + ((defender->GetSkill(DEFENSE) - atkSkill) / 10));
		}
		else{
			bonus = 1 + ((atkSkill - defender->GetSkill(DEFENSE)) / 25);
		}
		chancetohit += bonus;
	}
		
	//I dont think this is 100% correct, but at least it does something...
	if(attacker->spellbonuses.MeleeSkillCheckSkill == skillinuse || attacker->spellbonuses.MeleeSkillCheckSkill == 255) {
		chancetohit += attacker->spellbonuses.MeleeSkillCheck;
		mlog(COMBAT__TOHIT, "Applied spell melee skill bonus %d, yeilding %.2f", attacker->spellbonuses.MeleeSkillCheck, chancetohit);
	}
	if(attacker->itembonuses.MeleeSkillCheckSkill == skillinuse || attacker->itembonuses.MeleeSkillCheckSkill == 255) {
		chancetohit += attacker->itembonuses.MeleeSkillCheck;
		mlog(COMBAT__TOHIT, "Applied item melee skill bonus %d, yeilding %.2f", attacker->spellbonuses.MeleeSkillCheck, chancetohit);
	}
	
	//subtract off avoidance by the defender
	bonus = defender->spellbonuses.AvoidMeleeChance + defender->itembonuses.AvoidMeleeChance;
	if(bonus > 0) {
		chancetohit -= (bonus) / 10;
		mlog(COMBAT__TOHIT, "Applied avoidance chance %.2f/10, yeilding %.2f", bonus, chancetohit);
	}

	if(attacker->IsNPC())
		chancetohit += (chancetohit * attacker->CastToNPC()->GetAccuracyRating() / 1000);

	uint16 AA_mod = 0;
	switch(GetAA(aaCombatAgility))
	{
	case 1:
		AA_mod = 2;
		break;
	case 2:
		AA_mod = 5;
		break;
	case 3:
		AA_mod = 10;
		break;
	}
	AA_mod += 3*GetAA(aaPhysicalEnhancement);
	AA_mod += 2*GetAA(aaLightningReflexes);
	AA_mod += GetAA(aaReflexiveMastery);
	chancetohit -= chancetohit * AA_mod / 100;

#ifdef EQBOTS

	// Bot AA's for the above 3

	if(IsBot()) {
		if(GetLevel() >= 65) {
			AA_mod = 23;
		}
		else if(GetLevel() >= 64) {
			AA_mod = 21;
		}
		else if(GetLevel() >= 63) {
			AA_mod = 19;
		}
		else if(GetLevel() >= 62) {
			AA_mod = 17;
		}
		else if(GetLevel() >= 61) {
			AA_mod = 15;
		}
		else if(GetLevel() >= 59) {
			AA_mod = 13;
		}
		else if(GetLevel() >= 57) {
			AA_mod = 10;
		}
		else if(GetLevel() >= 56) {
			AA_mod = 5;
		}
		else if(GetLevel() >= 55) {
			AA_mod = 2;
		}
		chancetohit -= chancetohit * AA_mod / 100;
	}

#endif //EQBOTS

	//add in our hit chance bonuses if we are using the right skill
	//does the hit chance cap apply to spell bonuses from disciplines?
	if(attacker->spellbonuses.HitChanceSkill == 255 || attacker->spellbonuses.HitChanceSkill == skillinuse) {
		chancetohit += (chancetohit * (attacker->spellbonuses.HitChance / 15.0f) / 100);
		mlog(COMBAT__TOHIT, "Applied spell melee hit chance %d/15, yeilding %.2f", attacker->spellbonuses.HitChance, chancetohit);
	}
	if(attacker->itembonuses.HitChanceSkill == 255 || attacker->itembonuses.HitChanceSkill == skillinuse) {
		chancetohit += (chancetohit * (attacker->itembonuses.HitChance / 15.0f) / 100);
		mlog(COMBAT__TOHIT, "Applied item melee hit chance %d/15, yeilding %.2f", attacker->itembonuses.HitChance, chancetohit);
	}

	// Chance to hit;   Max 95%, Min 30%
	if(chancetohit > 1000) {
		//if chance to hit is crazy high, that means a discipline is in use, and let it stay there
	} 
	else if(chancetohit > 99) {
		chancetohit = 99;
	} 
	else if(chancetohit < 5) {
		chancetohit = 5;
	}
	
	//I dont know the best way to handle a garunteed hit discipline being used
	//agains a garunteed riposte (for example) discipline... for now, garunteed hit wins
	
		
	#if EQDEBUG>=11
		LogFile->write(EQEMuLog::Debug, "3 FINAL calculated chance to hit is: %5.2f", chancetohit);
	#endif

	//
	// Did we hit?
	//

	float tohit_roll = MakeRandomFloat(0, 100);
	

#ifdef EQBOTS

    // EQoffline - Raid mantank has a special defensive disc reducing by 50% the chance to be hitted.
    if(defender->IsBot() && defender->IsBotRaiding()) {
        BotRaids *br = entity_list.GetBotRaidByMob(defender);
        if(br && br->GetBotMainTank() && ((br->GetBotMainTank() == defender) || (br->GetBotSecondTank() == defender))) {
            chancetohit = chancetohit/2;
        }
    }
	
#endif //EQBOTS

	mlog(COMBAT__TOHIT, "Final hit chance: %.2f%%. Hit roll %.2f", chancetohit, tohit_roll);
	
	return(tohit_roll <= chancetohit);
}

/* solar: called when a mob is attacked, does the checks to see if it's a hit
 *  and does other mitigation checks.  'this' is the mob being attacked.
 * 
 * special return values:
 *    -1 - block
 *    -2 - parry
 *    -3 - riposte
 *    -4 - dodge
 * 
 */
bool Mob::AvoidDamage(Mob* other, sint32 &damage)
{
	float skill;
	float bonus;
	float RollTable[4] = {0,0,0,0};
	float roll;
	Mob *attacker=other;
	Mob *defender=this;

	//garunteed hit
	bool ghit = false;
	if((attacker->spellbonuses.MeleeSkillCheck + attacker->itembonuses.MeleeSkillCheck) > 500)
		ghit = true;
	
	//////////////////////////////////////////////////////////
	// make enrage same as riposte
	/////////////////////////////////////////////////////////
	if (IsEnraged() && !other->BehindMob(this, other->GetX(), other->GetY())) {
		damage = -3;
		mlog(COMBAT__DAMAGE, "I am enraged, riposting frontal attack.");
	}
	
	/////////////////////////////////////////////////////////
	// riposte
	/////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassRiposte() && !other->BehindMob(this, other->GetX(), other->GetY()))
	{
        skill = GetSkill(RIPOSTE);
		if (IsClient()) {
        	if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(RIPOSTE);
		}
		
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus = (defender->spellbonuses.RiposteChance + defender->itembonuses.RiposteChance);
			bonus += 2.0 + skill/35.0 + (GetDEX()/200);
			RollTable[0] = bonus;
		}
	}
	
	///////////////////////////////////////////////////////	
	// block
	///////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassBlock() && !other->BehindMob(this, other->GetX(), other->GetY()))
	{
		skill = CastToClient()->GetSkill(BLOCKSKILL);
		if (IsClient()) {
			if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(BLOCKSKILL);
		}
		
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus = 2.0 + skill/35.0 + (GetDEX()/200);
			RollTable[1] = RollTable[0] + bonus;
		}
	}
	else{
		RollTable[1] = RollTable[0];
	}
	
	//////////////////////////////////////////////////////		
	// parry
	//////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassParry() && !other->BehindMob(this, other->GetX(), other->GetY()))
	{
        skill = CastToClient()->GetSkill(PARRY);
		if (IsClient()) {
			if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(PARRY); 
		}
		
		bonus = (defender->spellbonuses.ParryChance + defender->itembonuses.ParryChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus += 2.0 + skill/35.0 + (GetDEX()/200);
			RollTable[2] = RollTable[1] + bonus;
		}
	}
	else{
		RollTable[2] = RollTable[1];
	}
	
	////////////////////////////////////////////////////////
	// dodge
	////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassDodge() && !other->BehindMob(this, other->GetX(), other->GetY()))
	{
	
        skill = CastToClient()->GetSkill(DODGE);
		if (IsClient()) {
			if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(DODGE);
		}
		
		bonus = (defender->spellbonuses.DodgeChance + defender->itembonuses.DodgeChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus += 2.0 + skill/35.0 + (GetAGI()/200);
			RollTable[3] = RollTable[2] + bonus;
		}
	}
	else{
		RollTable[3] = RollTable[2];
	}

	if(damage > 0){
		roll = MakeRandomFloat(0,100);
		if(roll <= RollTable[0]){
			damage = -3;
		}
		else if(roll <= RollTable[1]){
			damage = -1;
		}
		else if(roll <= RollTable[2]){
			damage = -2;
		}
		else if(roll <= RollTable[3]){
			damage = -4;
		}
	}

	mlog(COMBAT__DAMAGE, "Final damage after all avoidances: %d", damage);
	
	if (damage < 0)
		return true;
	return false;
}

void Mob::MeleeMitigation(Mob *attacker, sint32 &damage, sint32 minhit)
{
	if(damage <= 0)
		return;

	Mob* defender = this;
	int totalMit = 0;

	switch(GetAA(aaCombatStability)){
		case 1:
			totalMit += 2;
			break;
		case 2:
			totalMit += 5;
			break;
		case 3:
			totalMit += 10;
			break;
	}

	totalMit += GetAA(aaPhysicalEnhancement)*2;
	totalMit += GetAA(aaInnateDefense);
	totalMit += GetAA(aaDefensiveInstincts)*0.5;

#ifdef EQBOTS

	// Bot AA's for the above 3
	if(IsBot()) {
		if(GetLevel() >= 65) {
			totalMit = 19;
		}
		else if(GetLevel() >= 64) {
			totalMit = 17;
		}
		else if(GetLevel() >= 63) {
			totalMit = 17;
		}
		else if(GetLevel() >= 62) {
			totalMit = 16;
		}
		else if(GetLevel() >= 61) {
			totalMit = 15;
		}
		else if(GetLevel() >= 59) {
			totalMit = 12;
		}
		else if(GetLevel() >= 57) {
			totalMit = 10;
		}
		else if(GetLevel() >= 56) {
			totalMit = 5;
		}
		else if(GetLevel() >= 55) {
			totalMit = 2;
		}
	}

#endif //EQBOTS

	if(RuleB(Combat, UseIntervalAC)){
		//AC Mitigation
		sint32 attackRating = 0;

#ifdef EQBOTS

		if(attacker->IsBot()) {
			attackRating = attacker->GetATK() + ((attacker->GetSTR() + attacker->GetSkill(OFFENSE)) * 9 / 10);
		}
		else

#endif //EQBOTS

		if(attacker->IsClient())
			attackRating = attacker->GetATK() + ((attacker->GetSTR() + attacker->GetSkill(OFFENSE)) * 9 / 10);
		else
			attackRating = attacker->GetATK() + (attacker->GetSTR() * 9 / 10);

		sint32 defenseRating = defender->GetAC();
		defenseRating += 125;
		defenseRating = (defenseRating < attackRating)?attackRating:defenseRating;
		defenseRating += (totalMit * defenseRating / 100);

		//Add these to rules eventually
		//double the clients intervals to make their damage output look right, move to rule eventually too
		int intervalsAllowed = 20; 
		if(defender->IsClient())
			intervalsAllowed *= 2;

		uint32 intervalUsed = 0;
		uint32 intervalRoll = MakeRandomInt(0, (defenseRating - attackRating));
		if(MakeRandomInt(0, 9) < 1){ //still have a chance to hit for any value
			intervalUsed = MakeRandomInt(0, intervalsAllowed);
		}
		else{
			intervalUsed = MakeRandomInt(0, 2);
			//move the hardcoded value to a rule eventually, it impacts how lenient or strict the AC is
			if(defender->GetLevel() != 0)
				intervalUsed += ((intervalRoll * intervalsAllowed) / (19 * defender->GetLevel()));
			else
				intervalUsed += ((intervalRoll * intervalsAllowed) / (19 * 1));
		}

		mlog(COMBAT__DAMAGE, "attackRating: %d defenseRating: %d intervalRoll: %d intervalUsed: %d", attackRating, defenseRating, intervalRoll, intervalUsed);
		if(intervalUsed > intervalsAllowed){
			if((intervalUsed-intervalsAllowed) > 5)
				damage = 0;
			else
				damage = 1;
		}
		else{
			if(intervalsAllowed != 0)
				damage -= (((damage - minhit) * intervalUsed) / intervalsAllowed);
		}
	}
	else{
		////////////////////////////////////////////////////////
		// Scorpious2k: Include AC in the calculation
		// use serverop variables to set values
		int myac = GetAC();
		if (damage > 0 && myac > 0) {
			int acfail=1000;
			char tmp[10];

			if (database.GetVariable("ACfail", tmp, 9)) {
				acfail = (int) (atof(tmp) * 100);
				if (acfail>100) acfail=100;
			}

			if (acfail<=0 || rand()%101>acfail) {
				float acreduction=1;
				int acrandom=300;
				if (database.GetVariable("ACreduction", tmp, 9))
				{
					acreduction=atof(tmp);
					if (acreduction>100) acreduction=100;
				}
		
				if (database.GetVariable("ACrandom", tmp, 9))
				{
					acrandom = (int) ((atof(tmp)+1) * 100);
					if (acrandom>10100) acrandom=10100;
				}
				
				if (acreduction>0) {
					damage -= (int) (GetAC() * acreduction/100.0f);
				}		
				if (acrandom>0) {
					damage -= (myac * MakeRandomInt(0, acrandom) / 10000);
				}
				if (damage<1) damage=1;
				mlog(COMBAT__DAMAGE, "AC Damage Reduction: fail chance %d%%. Failed. Reduction %.3f%%, random %d. Resulting damage %d.", acfail, acreduction, acrandom, damage);
			} else {
				mlog(COMBAT__DAMAGE, "AC Damage Reduction: fail chance %d%%. Did not fail.", acfail);
			}
		}

		damage -= (totalMit * damage / 100);
	}

	if(damage != 0 && damage < minhit)
		damage = minhit;

	//reduce the damage from shielding item and aa based on the min dmg
	//spells offer pure mitigation
	damage -= (minhit * defender->itembonuses.MeleeMitigation / 100);
	damage -= (damage * defender->spellbonuses.MeleeMitigation / 100);

	if(damage < 0)
		damage = 0;

	mlog(COMBAT__DAMAGE, "Applied %d percent mitigation, remaining damage %d", totalMit, damage);
}

//Returns the weapon damage against the input mob
//if we cannot hit the mob with the current weapon we will get a value less than or equal to zero
//Else we know we can hit.
//GetWeaponDamage(mob*, const Item_Struct*) is intended to be used for mobs or any other situation where we do not have a client inventory item
//GetWeaponDamage(mob*, const ItemInst*) is intended to be used for situations where we have a client inventory item
int Mob::GetWeaponDamage(Mob *against, const Item_Struct *weapon_item) {
	int dmg = 0;
	int banedmg = 0;

	//can't hit invulnerable stuff with weapons.
	if(against->GetInvul() || against->SpecAttacks[IMMUNE_MELEE]){
		return 0;
	}
	
	//check to see if our weapons or fists are magical.
	if(against->SpecAttacks[IMMUNE_MELEE_NONMAGICAL]){
		if(weapon_item){
			if(weapon_item->Magic){
				dmg = weapon_item->Damage;

				//this is more for non weapon items, ex: boots for kick
				//they don't have a dmg but we should be able to hit magical
				dmg = dmg <= 0 ? 1 : dmg;
			}
			else
				return 0;
		}
		else{
			if((GetClass() == MONK || GetClass() == BEASTLORD) && GetLevel() >= 30){
				dmg = GetMonkHandToHandDamage();
			}
			else if(GetOwner() && GetLevel() >= RuleI(Combat, PetAttackMagicLevel)){
				//pets wouldn't actually use this but...
				//it gives us an idea if we can hit due to the dual nature of this function
				dmg = 1;						   
			}
			else
				return 0;
		}
	}
	else{
		if(weapon_item){
			dmg = weapon_item->Damage;

			dmg = dmg <= 0 ? 1 : dmg;
		}
		else{
			if(GetClass() == MONK || GetClass() == BEASTLORD){
				dmg = GetMonkHandToHandDamage();
			}
			else{
				dmg = 1;
			}
		}
	}
	
	int eledmg = 0;
	if(!against->SpecAttacks[IMMUNE_MAGIC]){
		if(weapon_item && weapon_item->ElemDmgAmt){
			//we don't check resist for npcs here
			eledmg = weapon_item->ElemDmgAmt;
			dmg += eledmg;
		}
	}

	if(against->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE]){
		if(weapon_item){
			if(weapon_item->BaneDmgBody == against->GetBodyType()){
				banedmg += weapon_item->BaneDmgAmt;
			}

			if(weapon_item->BaneDmgRace == against->GetRace()){
				banedmg += weapon_item->BaneDmgRaceAmt;
			}
		}

		if(!eledmg && !banedmg){
			return 0;
		}
		else
			dmg += banedmg;
	}
	else{
		if(weapon_item){
			if(weapon_item->BaneDmgBody == against->GetBodyType()){
				banedmg += weapon_item->BaneDmgAmt;
			}

			if(weapon_item->BaneDmgRace == against->GetRace()){
				banedmg += weapon_item->BaneDmgRaceAmt;
			}
		}

		dmg += (banedmg + eledmg);
	}

	if(dmg <= 0){
		return 0;
	}
	else
		return dmg;
}

int Mob::GetWeaponDamage(Mob *against, const ItemInst *weapon_item)
{
	int dmg = 0;
	int banedmg = 0;

	if(against->GetInvul() || against->SpecAttacks[IMMUNE_MELEE]){
		return 0;
	}

	//check for items being illegally attained
	if(weapon_item){
		const Item_Struct *mWeaponItem = weapon_item->GetItem();
		if(mWeaponItem){
			if(mWeaponItem->ReqLevel > GetLevel()){
				return 0;
			}

			if(!weapon_item->IsEquipable(GetBaseRace(), GetClass())){
				return 0;
			}
		}
		else{
			return 0;
		}
	}

	if(against->SpecAttacks[IMMUNE_MELEE_NONMAGICAL]){
		if(weapon_item){
			if(weapon_item->GetItem() && weapon_item->GetItem()->Magic){

				if(IsClient() && GetLevel() < weapon_item->GetItem()->RecLevel){
					dmg = CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->GetItem()->RecLevel, weapon_item->GetItem()->Damage);
				}
				else{
					dmg = weapon_item->GetItem()->Damage;
				}

				for(int x = 0; x < 5; x++){
					if(weapon_item->GetAugment(x) && weapon_item->GetAugment(x)->GetItem()){
						dmg += weapon_item->GetAugment(x)->GetItem()->Damage;
					}
				}
				dmg = dmg <= 0 ? 1 : dmg;
			}
			else
				return 0;
		}
		else{
			if((GetClass() == MONK || GetClass() == BEASTLORD) && GetLevel() >= 30){
				dmg = GetMonkHandToHandDamage();
			}
			else if(GetOwner() && GetLevel() >= RuleI(Combat, PetAttackMagicLevel)){ //pets wouldn't actually use this but...
				dmg = 1;						   //it gives us an idea if we can hit
			}
			else
				return 0;
		}
	}
	else{
		if(weapon_item){
			if(weapon_item->GetItem()){

				if(IsClient() && GetLevel() < weapon_item->GetItem()->RecLevel){
					dmg = CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->GetItem()->RecLevel, weapon_item->GetItem()->Damage);
				}
				else{
					dmg = weapon_item->GetItem()->Damage;
				}

				for(int x = 0; x < 5; x++){
					if(weapon_item->GetAugment(x) && weapon_item->GetAugment(x)->GetItem()){
						dmg += weapon_item->GetAugment(x)->GetItem()->Damage;
					}
				}
				dmg = dmg <= 0 ? 1 : dmg;
			}
		}
		else{
			if(GetClass() == MONK || GetClass() == BEASTLORD){
				dmg = GetMonkHandToHandDamage();
			}
			else{
				dmg = 1;
			}
		}
	}

	int eledmg = 0;
	if(!against->SpecAttacks[IMMUNE_MAGIC]){
		if(weapon_item && weapon_item->GetItem() && weapon_item->GetItem()->ElemDmgAmt){
			if(IsClient() && GetLevel() < weapon_item->GetItem()->RecLevel){
				eledmg = CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->GetItem()->RecLevel, weapon_item->GetItem()->ElemDmgAmt);
			}
			else{
				eledmg = weapon_item->GetItem()->ElemDmgAmt;
			}

			if(eledmg)
				dmg += (eledmg * against->ResistSpell(weapon_item->GetItem()->ElemDmgType, 0, this) / 100);		
		}

		if(weapon_item){
			for(int x = 0; x < 5; x++){
				if(weapon_item->GetAugment(x) && weapon_item->GetAugment(x)->GetItem()){
					eledmg += weapon_item->GetAugment(x)->GetItem()->ElemDmgAmt;
					if(weapon_item->GetAugment(x)->GetItem()->ElemDmgAmt)
						dmg += (weapon_item->GetAugment(x)->GetItem()->ElemDmgAmt * against->ResistSpell(weapon_item->GetAugment(x)->GetItem()->ElemDmgType, 0, this) / 100);
				}
			}
		}
	}

	if(against->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE]){
		if(weapon_item && weapon_item->GetItem()){
			if(weapon_item->GetItem()->BaneDmgBody == against->GetBodyType()){
				if(IsClient() && GetLevel() < weapon_item->GetItem()->RecLevel){
					banedmg += CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->GetItem()->RecLevel, weapon_item->GetItem()->BaneDmgAmt);
				}
				else{
					banedmg += weapon_item->GetItem()->BaneDmgAmt;
				}
			}

			if(weapon_item->GetItem()->BaneDmgRace == against->GetRace()){
				if(IsClient() && GetLevel() < weapon_item->GetItem()->RecLevel){
					banedmg += CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->GetItem()->RecLevel, weapon_item->GetItem()->BaneDmgRaceAmt);
				}
				else{
					banedmg += weapon_item->GetItem()->BaneDmgRaceAmt;
				}
			}

			for(int x = 0; x < 5; x++){
				if(weapon_item->GetAugment(x) && weapon_item->GetAugment(x)->GetItem()){
					if(weapon_item->GetAugment(x)->GetItem()->BaneDmgBody == against->GetBodyType()){
						banedmg += weapon_item->GetAugment(x)->GetItem()->BaneDmgAmt;
					}

					if(weapon_item->GetAugment(x)->GetItem()->BaneDmgRace == against->GetRace()){
						banedmg += weapon_item->GetAugment(x)->GetItem()->BaneDmgRaceAmt;
					}
				}
			}
		}

		if(!eledmg && !banedmg){
			return 0;
		}
		else
			dmg += banedmg;
	}
	else{
		if(weapon_item && weapon_item->GetItem()){
			if(weapon_item->GetItem()->BaneDmgBody == against->GetBodyType()){
				if(IsClient() && GetLevel() < weapon_item->GetItem()->RecLevel){
					banedmg += CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->GetItem()->RecLevel, weapon_item->GetItem()->BaneDmgAmt);
				}
				else{
					banedmg += weapon_item->GetItem()->BaneDmgAmt;
				}
			}

			if(weapon_item->GetItem()->BaneDmgRace == against->GetRace()){
				if(IsClient() && GetLevel() < weapon_item->GetItem()->RecLevel){
					banedmg += CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->GetItem()->RecLevel, weapon_item->GetItem()->BaneDmgRaceAmt);
				}
				else{
					banedmg += weapon_item->GetItem()->BaneDmgRaceAmt;
				}
			}

			for(int x = 0; x < 5; x++){
				if(weapon_item->GetAugment(x) && weapon_item->GetAugment(x)->GetItem()){
					if(weapon_item->GetAugment(x)->GetItem()->BaneDmgBody == against->GetBodyType()){
						banedmg += weapon_item->GetAugment(x)->GetItem()->BaneDmgAmt;
					}

					if(weapon_item->GetAugment(x)->GetItem()->BaneDmgRace == against->GetRace()){
						banedmg += weapon_item->GetAugment(x)->GetItem()->BaneDmgRaceAmt;
					}
				}
			}
		}
		dmg += (banedmg + eledmg);
	}

	if(dmg <= 0){
		return 0;
	}
	else
		return dmg;
}

//note: throughout this method, setting `damage` to a negative is a way to
//stop the attack calculations
bool Client::Attack(Mob* other, int Hand, bool bRiposte)
{
	_ZP(Client_Attack);

	if (!other) {
		SetTarget(NULL);
		LogFile->write(EQEMuLog::Error, "A null Mob object was passed to Client::Attack() for evaluation!");
		return false;
	}
	
	if(!GetTarget())
		SetTarget(other);
	
	mlog(COMBAT__ATTACKS, "Attacking %s with hand %d %s", other?other->GetName():"(NULL)", Hand, bRiposte?"(this is a riposte)":"");
	
	//SetAttackTimer();
	if (
		   (IsCasting() && GetClass() != BARD)
		|| other == NULL
		|| ((IsClient() && CastToClient()->dead) || (other->IsClient() && other->CastToClient()->dead))
		|| (GetHP() < 0)
		|| (!IsAttackAllowed(other))
		) {
		mlog(COMBAT__ATTACKS, "Attack canceled, invalid circumstances.");
		return false; // Only bards can attack while casting
	}
	if(DivineAura() && !GetGM()) {//cant attack while invulnerable unless your a gm
		mlog(COMBAT__ATTACKS, "Attack canceled, Divine Aura is in effect.");
		Message(10,"You can't attack while invulnerable!");
		return false;
	}

	
	ItemInst* weapon;
	if (Hand == 14)	// Kaiyodo - Pick weapon from the attacking hand
		weapon = GetInv().GetItem(SLOT_SECONDARY);
	else
		weapon = GetInv().GetItem(SLOT_PRIMARY);

	if(weapon != NULL) {
		if (!weapon->IsWeapon()) {
			mlog(COMBAT__ATTACKS, "Attack canceled, Item %s (%d) is not a weapon.", weapon->GetItem()->Name, weapon->GetID());
			return(false);
		}
		mlog(COMBAT__ATTACKS, "Attacking with weapon: %s (%d)", weapon->GetItem()->Name, weapon->GetID());
	} else {
		mlog(COMBAT__ATTACKS, "Attacking without a weapon.");
	}
	
	// calculate attack_skill and skillinuse depending on hand and weapon
	// also send Packet to near clients
	SkillType skillinuse;
	AttackAnimation(skillinuse, Hand, weapon);
	mlog(COMBAT__ATTACKS, "Attacking with %s in slot %d using skill %d", weapon?weapon->GetItem()->Name:"Fist", Hand, skillinuse);
	
	/// Now figure out damage
	int damage = 0;
	int8 mylevel = GetLevel() ? GetLevel() : 1;
	int8 otherlevel = other->GetLevel() ? other->GetLevel() : 1;
	int weapon_damage = GetWeaponDamage(other, weapon);
	
	//if weapon damage > 0 then we know we can hit the target with this weapon
	//otherwise we cannot and we set the damage to -5 later on
	if(weapon_damage > 0){
		
		//Berserker Berserk damage bonus
		if(berserk && GetClass() == BERSERKER){
			int bonus = 3 + GetLevel()/10;		//unverified
			weapon_damage = weapon_damage * (100+bonus) / 100;
			mlog(COMBAT__DAMAGE, "Berserker damage bonus increases DMG to %d", weapon_damage);
		}

		//try a finishing blow.. if successful end the attack
		if(TryFinishingBlow(other, skillinuse))
			return (true);
		
		//damage formula needs some work
		int min_hit = 1;

		//old formula:  max_hit = (weapon_damage * ((GetSTR()*20) + (GetSkill(OFFENSE)*15) + (mylevel*10)) / 1000);
		int max_hit = (2*weapon_damage) + (weapon_damage*(GetSTR()+GetSkill(OFFENSE))/225);

		if(GetLevel() < 10 && max_hit > 20)
			max_hit = 20;
		else if(GetLevel() < 20 && max_hit > 40)
			max_hit = 40;

		CheckIncreaseSkill(skillinuse, -10);
		CheckIncreaseSkill(OFFENSE, -10);

		//monks are supposed to be a step ahead of other classes in
		//toe to toe combat, small bonuses for hitting key levels too
		int bonus = 0;
		if(GetClass() == MONK)
			bonus += 20;
		if(GetLevel() > 50)
			bonus += 15;
		if(GetLevel() >= 55)
			bonus += 15;
		if(GetLevel() >= 60)
			bonus += 15;
		if(GetLevel() >= 65)
			bonus += 15;
		
		min_hit += (min_hit * bonus / 100);
		max_hit += (max_hit * bonus / 100);

		// ***************************************************************
		// *** Calculate the damage bonus, if applicable, for this hit ***
		// ***************************************************************

#ifndef EQEMU_NO_WEAPON_DAMAGE_BONUS

		// If you include the preprocessor directive "#define EQEMU_NO_WEAPON_DAMAGE_BONUS", that indicates that you do not
		// want damage bonuses added to weapon damage at all. This feature was requested by ChaosSlayer on the EQEmu Forums.
		//
		// This is not recommended for normal usage, as the damage bonus represents a non-trivial component of the DPS output
		// of weapons wielded by higher-level melee characters (especially for two-handed weapons).

		if( Hand == 13 && GetLevel() >= 28 && IsWarriorClass() )
		{
			// Damage bonuses apply only to hits from the main hand (Hand == 13) by characters level 28 and above
			// who belong to a melee class. If we're here, then all of these conditions apply.

			int8 ucDamageBonus = GetWeaponDamageBonus( weapon ? weapon->GetItem() : (const Item_Struct*) NULL );

			min_hit += (int) ucDamageBonus;
			max_hit += (int) ucDamageBonus;
		}
#endif

		min_hit = min_hit * (100 + itembonuses.MinDamageModifier + spellbonuses.MinDamageModifier) / 100;

		if(max_hit < min_hit)
			max_hit = min_hit;

		if(RuleB(Combat, UseIntervalAC))
			damage = max_hit;
		else
			damage = MakeRandomInt(min_hit, max_hit);

		mlog(COMBAT__DAMAGE, "Damage calculated to %d (min %d, max %d, str %d, skill %d, DMG %d, lv %d)", damage, min_hit, max_hit
		, GetSTR(), GetSkill(skillinuse), weapon_damage, mylevel);

		//check to see if we hit..
		if(!other->CheckHitChance(this, skillinuse, Hand)) {
			mlog(COMBAT__ATTACKS, "Attack missed. Damage set to 0.");
			damage = 0;
			other->AddToHateList(this, 0);
		} else {	//we hit, try to avoid it
			other->AvoidDamage(this, damage);
			other->MeleeMitigation(this, damage, min_hit);
			ApplyMeleeDamageBonus(skillinuse, damage);
			TryCriticalHit(other, skillinuse, damage);
			mlog(COMBAT__DAMAGE, "Final damage after all reductions: %d", damage);

			if(damage != 0){
				sint32 hate = max_hit;
				if(GetFeigned()) {
					mlog(COMBAT__HITS, "Attacker %s avoids %d hate due to feign death", GetName(), hate);
				} else {
					mlog(COMBAT__HITS, "Generating hate %d towards %s", hate, GetName());
					// now add done damage to the hate list
					other->AddToHateList(this, hate);
				}
			}
			else
				other->AddToHateList(this, 0);
		}

		//riposte
		bool slippery_attack = false; // Part of hack to allow riposte to become a miss, but still allow a Strikethrough chance (like on Live)
		if (damage == -3)  {
			if (bRiposte) return false;
			else {
				if (Hand == 14 && GetAA(aaSlipperyAttacks) > 0) {// Do we even have it & was attack with mainhand? If not, don't bother with other calculations
					if (MakeRandomInt(0, 100) < (GetAA(aaSlipperyAttacks) * 20)) {
						damage = 0; // Counts as a miss
						slippery_attack = true;
					} else DoRiposte(other);
				}
				else DoRiposte(other);
			}
		}

		//strikethrough..
		if (((damage < 0) || slippery_attack) && !bRiposte) { // Hack to still allow Strikethrough chance w/ Slippery Attacks AA
			if(MakeRandomInt(0, 100) < (itembonuses.StrikeThrough + spellbonuses.StrikeThrough)) {
				Message_StringID(MT_StrikeThrough, 9078); // You strike through your opponents defenses!
				Attack(other, Hand, true); // Strikethrough only gives another attempted hit
				return false;
			}
		}
	}
	else{
		damage = -5;
	}
	
	///////////////////////////////////////////////////////////
	//////    Send Attack Damage
	///////////////////////////////////////////////////////////
	other->Damage(this, damage, SPELL_UNKNOWN, skillinuse);
	if(damage > 0 && (spellbonuses.MeleeLifetap || itembonuses.MeleeLifetap)) {
		mlog(COMBAT__DAMAGE, "Melee lifetap healing for %d damage.", damage);
		//heal self for damage done..
		HealDamage(damage);
	}
	
	//break invis when you attack
	if(invisible) {
		mlog(COMBAT__ATTACKS, "Removing invisibility due to melee attack.");
		BuffFadeByEffect(SE_Invisibility);
		BuffFadeByEffect(SE_Invisibility2);
		invisible = false;
	}
	if(invisible_undead) {
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. undead due to melee attack.");
		BuffFadeByEffect(SE_InvisVsUndead);
		BuffFadeByEffect(SE_InvisVsUndead2);
		invisible_undead = false;
	}
	if(invisible_animals){
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. animals due to melee attack.");
		BuffFadeByEffect(SE_InvisVsAnimals);
		invisible_animals = false;
	}

	if(hidden || improved_hidden){
		hidden = false;
		improved_hidden = false;
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
		SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
		sa_out->spawn_id = GetID();
		sa_out->type = 0x03;
		sa_out->parameter = 0;
		entity_list.QueueClients(this, outapp, true);
		safe_delete(outapp);
	}
	
	////////////////////////////////////////////////////////////
	////////  PROC CODE
	////////  Kaiyodo - Check for proc on weapon based on DEX
	///////////////////////////////////////////////////////////
	if(other->GetHP() > -10 && !bRiposte && !IsDead()) {
		TryWeaponProc(weapon, other);
	}
	
	if (damage > 0)
        return true;
	else
		return false;
}

//used by complete heal and #heal
void Mob::Heal()
{
	SetMaxHP();
	SendHPUpdate();
}

void Client::Damage(Mob* other, sint32 damage, int16 spell_id, SkillType attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic)
{
	if(dead || IsCorpse())
		return;
	
	if(spell_id==0)
		spell_id = SPELL_UNKNOWN;
	
	// cut all PVP spell damage to 2/3 -solar
	// EverHood - Blasting ourselfs is considered PvP to
	if(other && other->IsClient() && damage > 0) {
		int PvPMitigation = 100;
		if(attack_skill == ARCHERY)
			PvPMitigation = 80;
		else
			PvPMitigation = 67;
		damage = (damage * PvPMitigation) / 100;
	}
			
	//do a majority of the work...
	CommonDamage(other, damage, spell_id, attack_skill, avoidable, buffslot, iBuffTic);
	
	if (damage > 0) {
		//if the other is not green, and this is not a spell
		if (other && other->IsNPC() && (spell_id == SPELL_UNKNOWN) && GetLevelCon(other->GetLevel()) != CON_GREEN )
			CheckIncreaseSkill(DEFENSE, -10);
	}
}

#ifdef EQBOTS

bool NPC::BotAttackMelee(Mob* other, int Hand, bool bRiposte)
{
	_ZP(NPC_BotAttackMelee);

	if (!other) {
		SetTarget(NULL);
		LogFile->write(EQEMuLog::Error, "A null Mob object was passed to NPC::BotAttackMelee for evaluation!");
		return false;
	}
	
	if(!GetTarget())
		SetTarget(other);
	
	mlog(COMBAT__ATTACKS, "Attacking %s with hand %d %s", other?other->GetName():"(NULL)", Hand, bRiposte?"(this is a riposte)":"");
	
	if (
		   (IsCasting() && GetClass() != BARD)
		|| other == NULL
		|| (GetHP() < 0)
		|| (!IsAttackAllowed(other))
		) {
			if (this->GetOwnerID())
				entity_list.MessageClose(this, 1, 200, 10, "%s says, 'That is not a legal target master.'", this->GetCleanName());
			if(other)
				RemoveFromHateList(other);
			mlog(COMBAT__ATTACKS, "I am not allowed to attack %s", other->GetName());
			return false;
	}
	if(DivineAura()) {//cant attack while invulnerable
		mlog(COMBAT__ATTACKS, "Attack canceled, Divine Aura is in effect.");
		Message(10,"You can't attack while invulnerable!");
		return false;
	}

	float calcheading=CalculateHeadingToTarget(target->GetX(), target->GetY());
	if((calcheading)!=GetHeading()){
		SetHeading(calcheading);
		FaceTarget(target, true);
	}
	
	ItemInst* weapon = NULL;
	const Item_Struct* botweapon = NULL;
	if((Hand == SLOT_PRIMARY) && equipment[MATERIAL_PRIMARY])
	    botweapon = database.GetItem(equipment[MATERIAL_PRIMARY]);
	if((Hand == SLOT_SECONDARY) && equipment[MATERIAL_SECONDARY])
	    botweapon = database.GetItem(equipment[MATERIAL_SECONDARY]);
	if(botweapon != NULL)
		weapon = new ItemInst(botweapon);

	if(weapon != NULL) {
		if (!weapon->IsWeapon()) {
			mlog(COMBAT__ATTACKS, "Attack canceled, Item %s (%d) is not a weapon.", weapon->GetItem()->Name, weapon->GetID());
			return(false);
		}
		mlog(COMBAT__ATTACKS, "Attacking with weapon: %s (%d)", weapon->GetItem()->Name, weapon->GetID());
	} else {
		mlog(COMBAT__ATTACKS, "Attacking without a weapon.");
	}
	
	// calculate attack_skill and skillinuse depending on hand and weapon
	// also send Packet to near clients
	SkillType skillinuse;
	AttackAnimation(skillinuse, Hand, weapon);
	mlog(COMBAT__ATTACKS, "Attacking with %s in slot %d using skill %d", weapon?weapon->GetItem()->Name:"Fist", Hand, skillinuse);
	
	/// Now figure out damage
	int damage = 0;
	int weapon_damage = GetWeaponDamage(other, weapon);
	
	//if weapon damage > 0 then we know we can hit the target with this weapon
	//otherwise we cannot and we set the damage to -5 later on
	if(weapon_damage > 0){
		
		//Berserker Berserk damage bonus
		if((GetHPRatio() < 30) && (GetClass() == BERSERKER)){
			int bonus = 3 + GetLevel()/10;		//unverified
			weapon_damage = weapon_damage * (100+bonus) / 100;
			mlog(COMBAT__DAMAGE, "Berserker damage bonus increases DMG to %d", weapon_damage);
		}

		//try a finishing blow.. if successful end the attack
		if(TryFinishingBlow(other, skillinuse))
			return (true);
		
		//damage formula needs some work
		int min_hit = 1;

		int max_hit = (2*weapon_damage) + (weapon_damage*(GetSTR()+GetSkill(OFFENSE))/225);

		if(GetLevel() < 10 && max_hit > 20)
			max_hit = 20;
		else if(GetLevel() < 20 && max_hit > 40)
			max_hit = 40;

		//monks are supposed to be a step ahead of other classes in
		//toe to toe combat, small bonuses for hitting key levels too
		int bonus = 0;
		if(GetClass() == MONK)
			bonus += 20;
		if(GetLevel() > 50)
			bonus += 15;
		if(GetLevel() >= 55)
			bonus += 15;
		if(GetLevel() >= 60)
			bonus += 15;
		if(GetLevel() >= 65)
			bonus += 15;
		
		min_hit += (min_hit * bonus / 100);
		max_hit += (max_hit * bonus / 100);

		//if mainhand only, get the bonus damage from level
		if(Hand == SLOT_PRIMARY) {
			int damage_bonus = GetWeaponDamageBonus(weapon ? weapon->GetItem() : (const Item_Struct*)NULL);
			min_hit += damage_bonus;
			max_hit += damage_bonus;
		}

		min_hit = min_hit * (100 + itembonuses.MinDamageModifier + spellbonuses.MinDamageModifier) / 100;

		if(max_hit < min_hit)
			max_hit = min_hit;

		if(RuleB(Combat, UseIntervalAC))
			damage = max_hit;
		else
			damage = MakeRandomInt(min_hit, max_hit);

		mlog(COMBAT__DAMAGE, "Damage calculated to %d (min %d, max %d, str %d, skill %d, DMG %d, lv %d)", damage, min_hit, max_hit
		, GetSTR(), GetSkill(skillinuse), weapon_damage, GetLevel());

		//check to see if we hit..
		if(!other->CheckHitChance(this, skillinuse, Hand)) {
			mlog(COMBAT__ATTACKS, "Attack missed. Damage set to 0.");
			damage = 0;
			other->AddToHateList(this, 0);
		} else {	//we hit, try to avoid it
			other->AvoidDamage(this, damage);
			other->MeleeMitigation(this, damage, min_hit);
			ApplyMeleeDamageBonus(skillinuse, damage);
			TryCriticalHit(other, skillinuse, damage);
			mlog(COMBAT__DAMAGE, "Final damage after all reductions: %d", damage);

			if(damage > 0){
				sint32 hate = max_hit;
				mlog(COMBAT__HITS, "Generating hate %d towards %s", hate, GetName());
				// now add done damage to the hate list
				other->AddToHateList(this, hate);
			}
			else
				other->AddToHateList(this, 0);
		}

		//riposte
		if (damage == -3)  {
			if (bRiposte) return false;
			else DoRiposte(other);
		}

		//strikethrough..
		if (damage < 0 && !bRiposte) {
			if(MakeRandomInt(0, 100) < (itembonuses.StrikeThrough + spellbonuses.StrikeThrough)) {
				Message_StringID(MT_StrikeThrough, 9078); // You strike through your opponents defenses!
				BotAttackMelee(other, Hand, true); // Strikethrough only gives another attempted hit
				return false;
			}
		}
	}
	else{
		damage = -5;
	}
	
	///////////////////////////////////////////////////////////
	//////    Send Attack Damage
	///////////////////////////////////////////////////////////
	other->Damage(this, damage, SPELL_UNKNOWN, skillinuse);
	if(damage > 0 && (spellbonuses.MeleeLifetap || itembonuses.MeleeLifetap)) {
		mlog(COMBAT__DAMAGE, "Melee lifetap healing for %d damage.", damage);
		//heal self for damage done..
		HealDamage(damage);
	}
	
	//break invis when you attack
	if(invisible) {
		mlog(COMBAT__ATTACKS, "Removing invisibility due to melee attack.");
		BuffFadeByEffect(SE_Invisibility);
		BuffFadeByEffect(SE_Invisibility2);
		invisible = false;
	}
	if(invisible_undead) {
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. undead due to melee attack.");
		BuffFadeByEffect(SE_InvisVsUndead);
		BuffFadeByEffect(SE_InvisVsUndead2);
		invisible_undead = false;
	}
	if(invisible_animals){
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. animals due to melee attack.");
		BuffFadeByEffect(SE_InvisVsAnimals);
		invisible_animals = false;
	}

	if(hidden || improved_hidden){
		hidden = false;
		improved_hidden = false;
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
		SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
		sa_out->spawn_id = GetID();
		sa_out->type = 0x03;
		sa_out->parameter = 0;
		entity_list.QueueClients(this, outapp, true);
		safe_delete(outapp);
	}
	
	////////////////////////////////////////////////////////////
	////////  PROC CODE
	////////  Kaiyodo - Check for proc on weapon based on DEX
	///////////////////////////////////////////////////////////
	if(other->GetHP() > -10 && !bRiposte && this) {
		TryWeaponProc(weapon, other);
	}
	
	if (damage > 0)
            return true;
	else
            return false;
}

#endif //EQBOTS

void Client::Death(Mob* other, sint32 damage, int16 spell, SkillType attack_skill)
{
	if(dead)
		return;	//cant die more than once...

#ifdef EQBOTS

	Mob *clientmob = CastToMob();
	if(clientmob) {
		int16 cmid = GetID();
		if(clientmob->IsBotRaiding()) {
			BotRaids* br = entity_list.GetBotRaidByMob(clientmob);
			if(br) {
				br->RemoveRaidBots();
				br = NULL;
			}
		}
		if(clientmob->IsGrouped()) {
			Group *g = entity_list.GetGroupByMob(clientmob);
			if(g) {
				bool hasBots = false;
				for(int i=5; i>=0; i--) {
					if(g->members[i] && g->members[i]->IsBot()) {
						hasBots = true;
						g->members[i]->BotOwner = NULL;
						g->members[i]->Kill();
					}
				}
				if(hasBots) {
					hasBots = false;
					if(g->BotGroupCount() <= 1) {
						g->DisbandGroup();
					}
				}
			}
		}
		database.CleanBotLeader(cmid);
	}

#endif //EQBOTS

	int exploss;

	mlog(COMBAT__HITS, "Fatal blow dealt by %s with %d damage, spell %d, skill %d", other->GetName(), damage, spell, attack_skill);
	
	//
	// #1: Send death packet to everyone
	//

	if(!spell) spell = SPELL_UNKNOWN;
	
	SendLogoutPackets();
	
	//make our become corpse packet, and queue to ourself before OP_Death.
	EQApplicationPacket app2(OP_BecomeCorpse, sizeof(BecomeCorpse_Struct));
	BecomeCorpse_Struct* bc = (BecomeCorpse_Struct*)app2.pBuffer;
	bc->spawn_id = GetID();
	bc->x = GetX();
	bc->y = GetY();
	bc->z = GetZ();
	QueuePacket(&app2);
	
	// make death packet
	EQApplicationPacket app(OP_Death, sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)app.pBuffer;
	d->spawn_id = GetID();
	d->killer_id = other ? other->GetID() : 0;
	//d->unknown12 = 1;
	d->bindzoneid = m_pp.binds[0].zoneId;
	d->spell_id = spell == SPELL_UNKNOWN ? 0xffffffff : spell;
	d->attack_skill = spell != SPELL_UNKNOWN ? 0xe7 : attack_skill;
	d->damage = damage;
	app.priority = 6;
	entity_list.QueueClients(this, &app);

	//
	// #2: figure out things that affect the player dying and mark them dead
	//

	InterruptSpell();
	SetPet(0);
	SetHorseId(0);
	dead = true;
	dead_timer.Start(5000, true);

	if (other != NULL)
	{
		if (other->IsNPC())
			parse->Event(EVENT_SLAY, other->GetNPCTypeID(), 0, other->CastToNPC(), this);
		
		if(other->IsClient() && (IsDueling() || other->CastToClient()->IsDueling())) {
			SetDueling(false);
			SetDuelTarget(0);
			if (other->IsClient() && other->CastToClient()->IsDueling() && other->CastToClient()->GetDuelTarget() == GetID())
			{
				//if duel opponent killed us...
				other->CastToClient()->SetDueling(false);
				other->CastToClient()->SetDuelTarget(0);
				entity_list.DuelMessage(other,this,false);
			} else {
				//otherwise, we just died, end the duel.
				Mob* who = entity_list.GetMob(GetDuelTarget());
				if(who && who->IsClient()) {
					who->CastToClient()->SetDueling(false);
					who->CastToClient()->SetDuelTarget(0);
				}
			}
		}
	}

	entity_list.RemoveFromTargets(this);
	hate_list.RemoveEnt(this);
	
	if(isgrouped) {
		Group *g = GetGroup();
		if(g)
			g->MemberZoned(this);
	}
	
	Raid* r = entity_list.GetRaidByClient(this);
	if(r){
		r->MemberZoned(this);
	}
	
	//remove ourself from all proximities
	ClearAllProximities();

	//
	// #3: exp loss and corpse generation
	//

	// figure out if they should lose exp
	exploss = (int)(GetLevel() * (GetLevel() / 18.0) * 12000);

	if( (GetLevel() < RuleI(Character, DeathExpLossLevel)) || IsBecomeNPC() )
	{
		exploss = 0;
	}
	else if( other )
	{
		if( other->IsClient() )
		{
			exploss = 0;
		}
		else if( other->GetOwner() && other->GetOwner()->IsClient() )
		{
			exploss = 0;
		}
	}

	if(spell != SPELL_UNKNOWN)
	{
		for(uint16 buffIt = 0; buffIt < BUFF_COUNT; buffIt++)
		{
			if(buffs[buffIt].spellid == spell && buffs[buffIt].client)
			{
				exploss = 0;	// no exp loss for pvp dot
				break;
			}
		}
	}
	
	// now we apply the exp loss, unmem their spells, and make a corpse
	// unless they're a GM (or less than lvl 10
	if(!GetGM())
	{
		if(exploss > 0) {
			sint32 newexp = GetEXP();
			if(exploss > newexp) {
				//lost more than we have... wtf..
				newexp = 1;
			} else {
				newexp -= exploss;
			}
			SetEXP(newexp, GetAAXP());
			//m_epp.perAA = 0;	//reset to no AA exp on death.
		}

		//this generates a lot of 'updates' to the client that the client does not need
		BuffFadeAll();
		UnmemSpellAll(false);
		
		if(RuleB(Character, LeaveCorpses) && GetLevel() >= RuleI(Character, DeathItemLossLevel) || RuleB(Character, LeaveNakedCorpses))
		{
			// creating the corpse takes the cash/items off the player too
			Corpse *new_corpse = new Corpse(this, exploss);

			char tmp[20];
			database.GetVariable("ServerType", tmp, 9);
			if(atoi(tmp)==1 && other != NULL && other->IsClient()){
				char tmp2[10] = {0};
				database.GetVariable("PvPreward", tmp, 9);
				int reward = atoi(tmp);
				if(reward==3){
					database.GetVariable("PvPitem", tmp2, 9);
					int pvpitem = atoi(tmp2);
					if(pvpitem>0 && pvpitem<200000)
						new_corpse->SetPKItem(pvpitem);
				}
				else if(reward==2)
					new_corpse->SetPKItem(-1);
				else if(reward==1)
					new_corpse->SetPKItem(1);
				else
					new_corpse->SetPKItem(0);
				if(other->CastToClient()->isgrouped) {
					Group* group = entity_list.GetGroupByClient(other->CastToClient());
					if(group != 0) 
					{
						for(int i=0;i<6;i++) 
						{
							if(group->members[i] != NULL) 
							{
								new_corpse->AllowMobLoot(group->members[i],i);
							}
						}
					}
				}
			}
			
			entity_list.AddCorpse(new_corpse, GetID());
			SetID(0);
			
			//send the become corpse packet to everybody else in the zone.
			entity_list.QueueClients(this, &app2, true);
		}

//		if(!IsLD())//Todo: make it so an LDed client leaves corpse if its enabled
//			MakeCorpse(exploss);
	} else {
		BuffFadeDetrimental();
	}



#if 0	// solar: commenting this out for now TODO reimplement becomenpc stuff
	if (IsBecomeNPC() == true)
	{
		if (other != NULL && other->IsClient()) {
			if (other->CastToClient()->isgrouped && entity_list.GetGroupByMob(other) != 0)
				entity_list.GetGroupByMob(other->CastToClient())->SplitExp((uint32)(level*level*75*3.5f), this);

			else
				other->CastToClient()->AddEXP((uint32)(level*level*75*3.5f)); // Pyro: Comment this if NPC death crashes zone
			//hate_list.DoFactionHits(GetNPCFactionID());
		}

		Corpse* corpse = new Corpse(this->CastToClient(), 0);
		entity_list.AddCorpse(corpse, this->GetID());
		this->SetID(0);
		if(other->GetOwner() != 0 && other->GetOwner()->IsClient())
			other = other->GetOwner();
		if(other != 0 && other->IsClient()) {
			corpse->AllowMobLoot(other, 0);
			if(other->CastToClient()->isgrouped) {
				Group* group = entity_list.GetGroupByClient(other->CastToClient());
				if(group != 0) {
					for(int i=0; i < MAX_GROUP_MEMBERS; i++) { // Doesnt work right, needs work
						if(group->members[i] != NULL) {
							corpse->AllowMobLoot(group->members[i],i);
						}
					}
				}
			}
		}
	}
#endif


	//
	// Finally, send em home
	//

	// we change the mob variables, not pp directly, because Save() will copy
	// from these and overwrite what we set in pp anyway
	//

	m_pp.zone_id = m_pp.binds[0].zoneId;
	database.MoveCharacterToZone(this->CharacterID(), database.GetZoneName(m_pp.zone_id));
		
	Save();
	
	GoToDeath();
}

bool NPC::Attack(Mob* other, int Hand, bool bRiposte)	 // Kaiyodo - base function has changed prototype, need to update overloaded version
{
	_ZP(NPC_Attack);
	int damage = 0;
	
	if (!other) {
		SetTarget(NULL);
		LogFile->write(EQEMuLog::Error, "A null Mob object was passed to NPC::Attack() for evaluation!");
		return false;
	}
	
	if(DivineAura())
		return(false);
	
	if(!GetTarget())
		SetTarget(other);

	//Check that we can attack before we calc heading and face our target	
	if (!IsAttackAllowed(other)) {
		if (this->GetOwnerID())
			entity_list.MessageClose(this, 1, 200, 10, "%s says, 'That is not a legal target master.'", this->GetCleanName());
		if(other)
			RemoveFromHateList(other);
		mlog(COMBAT__ATTACKS, "I am not allowed to attack %s", other->GetName());
		return false;
	}
	float calcheading=CalculateHeadingToTarget(target->GetX(), target->GetY());
	if((calcheading)!=GetHeading()){
		SetHeading(calcheading);
		FaceTarget(target, true);
	}
	
	if(!combat_event) {
		mlog(COMBAT__HITS, "Triggering EVENT_COMBAT due to attack on %s", other->GetName());
		parse->Event(EVENT_COMBAT, this->GetNPCTypeID(), "1", this, other);
		combat_event = true;
	}
	combat_event_timer.Start(CombatEventTimer_expire);
	
	SkillType skillinuse = HAND_TO_HAND;

	//figure out what weapon they are using, if any
	const Item_Struct* weapon = NULL;
	if (Hand == 13 && equipment[7] > 0)
	    weapon = database.GetItem(equipment[7]);
	else if (equipment[8])
	    weapon = database.GetItem(equipment[8]);
	
	//We dont factor much from the weapon into the attack.
	//Just the skill type so it doesn't look silly using punching animations and stuff while wielding weapons
	if(weapon) {
		mlog(COMBAT__ATTACKS, "Attacking with weapon: %s (%d) (too bad im not using it for much)", weapon->Name, weapon->ID);
		
		if(Hand == 14 && weapon->ItemType == ItemTypeShield){
			mlog(COMBAT__ATTACKS, "Attack with shield canceled.");
			return false;
		}

		switch(weapon->ItemType){
			case ItemType1HS:
				skillinuse = _1H_SLASHING;
				break;
			case ItemType2HS:
				skillinuse = _2H_SLASHING;
				break;
			case ItemTypePierce:
			case ItemType2HPierce:
				skillinuse = PIERCING;
				break;
			case ItemType1HB:
				skillinuse = _1H_BLUNT;
				break;
			case ItemType2HB:
				skillinuse = _2H_BLUNT;
				break;
			case ItemTypeBow:
				skillinuse = ARCHERY;
				break;
			case ItemTypeThrowing:
			case ItemTypeThrowingv2:
				skillinuse = THROWING;
				break;
			default:
				skillinuse = HAND_TO_HAND;
				break;
		}
	}
	
	int weapon_damage = GetWeaponDamage(other, weapon);
	
	//do attack animation regardless of whether or not we can hit below
	sint16 charges = 0;
	ItemInst weapon_inst(weapon, charges);
	AttackAnimation(skillinuse, Hand, &weapon_inst);

	//basically "if not immune" then do the attack
	if((weapon_damage) > 0) {

		//ele and bane dmg too
		//NPCs add this differently than PCs
		//if NPCs can't inheriently hit the target we don't add bane/magic dmg which isn't exactly the same as PCs
		int16 eleBane = 0;
		if(weapon){
			if(weapon->BaneDmgBody == other->GetBodyType()){
				eleBane += weapon->BaneDmgAmt;
			}

			if(weapon->BaneDmgRace == other->GetRace()){
				eleBane += weapon->BaneDmgRaceAmt;
			}

			if(weapon->ElemDmgAmt){
				eleBane += (weapon->ElemDmgAmt * other->ResistSpell(weapon->ElemDmgType, 0, this) / 100);
			}
		}
		
		if(!RuleB(NPC, UseItemBonusesForNonPets)){
			if(!GetOwner()){
				eleBane = 0;
			}
		}
		
		int8 otherlevel = other->GetLevel();
		int8 mylevel = this->GetLevel();
				
		otherlevel = otherlevel ? otherlevel : 1;
		mylevel = mylevel ? mylevel : 1;
		
		//instead of calcing damage in floats lets just go straight to ints
		if(RuleB(Combat, UseIntervalAC))
			damage = (max_dmg+eleBane);
		else
			damage = MakeRandomInt((min_dmg+eleBane),(max_dmg+eleBane));


		//check if we're hitting above our max or below it.
		if((min_dmg+eleBane) != 0 && damage < (min_dmg+eleBane)) {
			mlog(COMBAT__DAMAGE, "Damage (%d) is below min (%d). Setting to min.", damage, (min_dmg+eleBane));
		    damage = (min_dmg+eleBane);
		}
		if((max_dmg+eleBane) != 0 && damage > (max_dmg+eleBane)) {
			mlog(COMBAT__DAMAGE, "Damage (%d) is above max (%d). Setting to max.", damage, (max_dmg+eleBane));
		    damage = (max_dmg+eleBane);
		}
		
		sint32 hate = damage;
		//THIS IS WHERE WE CHECK TO SEE IF WE HIT:
		if(other->IsClient() && other->CastToClient()->IsSitting()) {
			mlog(COMBAT__DAMAGE, "Client %s is sitting. Hitting for max damage (%d).", other->GetName(), (max_dmg+eleBane));
			damage = (max_dmg+eleBane);

			mlog(COMBAT__HITS, "Generating hate %d towards %s", hate, GetName());
			// now add done damage to the hate list
			other->AddToHateList(this, hate);
		} else {
			if(!other->CheckHitChance(this, skillinuse, Hand)) {
				damage = 0;	//miss
			} else {	//hit, check for damage avoidance
				other->AvoidDamage(this, damage);
				other->MeleeMitigation(this, damage, min_dmg+eleBane);
				ApplyMeleeDamageBonus(skillinuse, damage);
				TryCriticalHit(other, skillinuse, damage);

				mlog(COMBAT__HITS, "Generating hate %d towards %s", hate, GetName());
				// now add done damage to the hate list
				if(damage != 0)
					other->AddToHateList(this, hate);
				else
					other->AddToHateList(this, 0);
			}
		}
		
		mlog(COMBAT__DAMAGE, "Final damage against %s: %d", other->GetName(), damage);
		
		if(other->IsClient() && IsPet() && GetOwner()->IsClient()) {
			//pets do half damage to clients in pvp
			damage=damage/2;
		}
	}
	else
		damage = -5;
		
	//cant riposte a riposte
	if (bRiposte && damage == -3) {
		mlog(COMBAT__DAMAGE, "Riposte of riposte canceled.");
		return false;
	}
		
	if(GetHP() > 0 && other->GetHP() >= -11) {
		other->Damage(this, damage, SPELL_UNKNOWN, skillinuse, false); // Not avoidable client already had thier chance to Avoid
    }
	
	
	//break invis when you attack
	if(invisible) {
		mlog(COMBAT__ATTACKS, "Removing invisibility due to melee attack.");
		BuffFadeByEffect(SE_Invisibility);
		BuffFadeByEffect(SE_Invisibility2);
		invisible = false;
	}
	if(invisible_undead) {
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. undead due to melee attack.");
		BuffFadeByEffect(SE_InvisVsUndead);
		BuffFadeByEffect(SE_InvisVsUndead2);
		invisible_undead = false;
	}
	if(invisible_animals){
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. animals due to melee attack.");
		BuffFadeByEffect(SE_InvisVsAnimals);
		invisible_animals = false;
	}

	if(hidden || improved_hidden)
	{
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
		SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
		sa_out->spawn_id = GetID();
		sa_out->type = 0x03;
		sa_out->parameter = 0;
		entity_list.QueueClients(this, outapp, true);
		safe_delete(outapp);
	}


	hidden = false;
	improved_hidden = false;
	
	//I doubt this works...
	if (!target)
		return true; //We killed them
	
	// Kaiyodo - Check for proc on weapon based on DEX
	if( !bRiposte && other->GetHP() > 0 ) {
		TryWeaponProc(weapon, other);	//no weapon
	}
	
	// now check ripostes
	if (damage == -3) { // riposting
		DoRiposte(other);
	}
	
	if (damage > 0)
        return true;
	else
        return false;
}

void NPC::Damage(Mob* other, sint32 damage, int16 spell_id, SkillType attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic) {
	if(spell_id==0)
		spell_id = SPELL_UNKNOWN;
	
	//handle EVENT_ATTACK. Resets after we have not been attacked for 12 seconds
	if(!attack_event) {
		mlog(COMBAT__HITS, "Triggering EVENT_ATTACK due to attack by %s", other->GetName());
		parse->Event(EVENT_ATTACK, this->GetNPCTypeID(), 0, this, other);
		attack_event = true;
	}
	if(!combat_event) {
		mlog(COMBAT__HITS, "Triggering EVENT_COMBAT due to attack by %s", other->GetName());
		parse->Event(EVENT_COMBAT, this->GetNPCTypeID(), "1", this, other);
		combat_event = true;
	}
	attacked_timer.Start(CombatEventTimer_expire - 1);	//-1 to solidify an assumption in NPC::Process
	combat_event_timer.Start(CombatEventTimer_expire);
    
	if (!IsEngaged())
		zone->AddAggroMob();
	
	//do a majority of the work...
	CommonDamage(other, damage, spell_id, attack_skill, avoidable, buffslot, iBuffTic);
	
	if(damage > 0) {
		//see if we are gunna start fleeing
		if(!IsPet()) CheckFlee();
	}

#ifdef EQBOTS

    // franck-add: when a bot takes some dmg, its leader must see it in the group HP bar
	if(IsBot() && IsGrouped()) {
		if(GetHP() > 0) {
			Group *g = entity_list.GetGroupByMob(this);
			if(g) {
				EQApplicationPacket hp_app;
				CreateHPPacket(&hp_app);
				for(int i=0; i<MAX_GROUP_MEMBERS; i++) {
					if(g->members[i] && g->members[i]->IsClient()) {
						g->members[i]->CastToClient()->QueuePacket(&hp_app);
					}
				}
			}
		}
	}

#endif //EQBOTS

}

void NPC::Death(Mob* other, sint32 damage, int16 spell, SkillType attack_skill) {

	mlog(COMBAT__HITS, "Fatal blow dealt by %s with %d damage, spell %d, skill %d", other->GetName(), damage, spell, attack_skill);
	
	if (this->IsEngaged())
	{
		zone->DelAggroMob();
#if EQDEBUG >= 11
		LogFile->write(EQEMuLog::Debug,"NPC::Death() Mobs currently Aggro %i", zone->MobsAggroCount());
#endif
	}
	SetHP(0);
	SetPet(0);
	Mob* killer = GetHateDamageTop(this);
	
#ifdef EQBOTS

	int16 botid = 0;
	if(IsBot()) {
		botid = GetID();
	}

    //franck-add: EQoffline. If a bot kill a mob, the Killer is its leader.	
	if(!IsBot()) {
		if((killer != NULL) && killer->AmIaBot && killer->qglobal) {
			killer = other;
		}
		if((killer != NULL) && killer->BotOwner) {
			killer = killer->BotOwner;
		}
	}

#endif //EQBOTS

	entity_list.RemoveFromTargets(this);

	if(p_depop == true)
		return;

	BuffFadeAll();
	
	EQApplicationPacket* app= new EQApplicationPacket(OP_Death,sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)app->pBuffer;
	d->spawn_id = GetID();
	d->killer_id = other ? other->GetID() : 0;
//	d->unknown12 = 1;
	d->bindzoneid = 0;
	d->spell_id = spell == SPELL_UNKNOWN ? 0xffffffff : spell;
	d->attack_skill = SkillDamageTypes[attack_skill];
	d->damage = damage;
	app->priority = 6;
	entity_list.QueueClients(other, app, false);
	
	if(respawn2) {
		respawn2->Reset();
		if(respawn2->spawn2_id!=0 && respawn2->respawn_!=0)
		    database.UpdateSpawn2Timeleft(respawn2->spawn2_id, respawn2->respawn_);
	}

	if (other) {
//		if (other->IsClient())
//			other->CastToClient()->QueuePacket(app);
		hate_list.Add(other, damage);
	}

	safe_delete(app);
	
	
	Mob *give_exp = hate_list.GetDamageTop(this);

#ifdef EQBOTS

    //franck-add: EQoffline. Same there, if a bot kill a mob, the exp goes to its leader.
	if(!IsBot()) {
		if((give_exp != NULL) && give_exp->AmIaBot && give_exp->qglobal) {
			give_exp = other;
		}
		if((give_exp != NULL) && give_exp->BotOwner) {
			give_exp = give_exp->BotOwner;
		}
	}

#endif //EQBOTS

	if(give_exp == NULL)
		give_exp = killer;
	if(give_exp && give_exp->GetOwner() != 0)
		give_exp = give_exp->GetOwner();
	
	Client *give_exp_client = NULL;
	if(give_exp && give_exp->IsClient())
		give_exp_client = give_exp->CastToClient();
	
	if(!(this->GetClass() == LDON_TREASURE))
	{
		// cb: if we're not a LDON treasure chest, we give XP
		if (give_exp_client && !IsCorpse() && MerchantType == 0)
		{
			Group *kg = entity_list.GetGroupByClient(give_exp_client);
			Raid *kr = entity_list.GetRaidByClient(give_exp_client);
			if (give_exp_client->IsGrouped() && kg != NULL)
			{
				if(give_exp_client->GetAdventureID()>0){
					AdventureInfo AF = database.GetAdventureInfo(give_exp_client->GetAdventureID());
					if(zone->GetZoneID() == AF.zonedungeonid && AF.type==ADVENTURE_MASSKILL)
						give_exp_client->SendAdventureUpdate();
					else if(zone->GetZoneID() == AF.zonedungeonid && AF.type==ADVENTURE_NAMED && (AF.Objetive==GetNPCTypeID() || AF.ObjetiveValue==GetNPCTypeID()))
						give_exp_client->SendAdventureFinish(1, AF.points,true);
				}
				kg->SplitExp((EXP_FORMULA), this);

				/* Send the EVENT_KILLED_MERIT event and update kill tasks
				 * for all group members */
				for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
					if (kg->members[i] != NULL && kg->members[i]->IsClient()) { // If Group Member is Client
						Client *c = kg->members[i]->CastToClient();
						parse->Event(EVENT_KILLED_MERIT, GetNPCTypeID(), "killed", this, c);
						if(RuleB(TaskSystem, EnableTaskSystem))
							c->UpdateTasksOnKill(GetNPCTypeID());
					}
				}
			}
			else if(kr)
			{
				kr->SplitExp((EXP_FORMULA), this);
				/* Send the EVENT_KILLED_MERIT event for all raid members */
				for (int i = 0; i < MAX_RAID_MEMBERS; i++) {
					if (kr->members[i].member != NULL) { // If Group Member is Client
						parse->Event(EVENT_KILLED_MERIT, GetNPCTypeID(), "killed", this, kr->members[i].member);
					}
				}
			}
			else
			{
				int conlevel = give_exp->GetLevelCon(GetLevel());
				if (conlevel != CON_GREEN)
				{
					if(GetOwner() && GetOwner()->IsClient()){
					}
					else{
						give_exp_client->AddEXP((EXP_FORMULA), conlevel); // Pyro: Comment this if NPC death crashes zone
					}
				}
				 /* Send the EVENT_KILLED_MERIT event */
				parse->Event(EVENT_KILLED_MERIT, GetNPCTypeID(), "killed", this, give_exp_client);
				if(RuleB(TaskSystem, EnableTaskSystem))
					give_exp_client->UpdateTasksOnKill(GetNPCTypeID());
			}
		}
	}
	
	//do faction hits even if we are a merchant, so long as a player killed us
	if(give_exp_client)
		hate_list.DoFactionHits(GetNPCFactionID());
	
	if (!HasOwner() && class_ != MERCHANT && class_ != ADVENTUREMERCHANT && !GetSwarmInfo()
		&& MerchantType == 0 && killer && (killer->IsClient() || (killer->HasOwner() && killer->GetOwner()->IsClient()) ||
		(killer->IsNPC() && killer->CastToNPC()->GetSwarmInfo() && killer->CastToNPC()->GetSwarmInfo()->owner && killer->CastToNPC()->GetSwarmInfo()->owner->IsClient()))) {
		Corpse* corpse = new Corpse(this, &itemlist, GetNPCTypeID(), &NPCTypedata,level>54?RuleI(NPC,MajorNPCCorpseDecayTimeMS):RuleI(NPC,MinorNPCCorpseDecayTimeMS));
		entity_list.LimitRemoveNPC(this);
		entity_list.AddCorpse(corpse, this->GetID());

#ifdef EQBOTS

		//franck-add: Bot's corpses always disapear..
		if(IsBot()) {
			corpse->Depop();
		}

#endif //EQBOTS

		this->SetID(0);
		if(killer->GetOwner() != 0 && killer->GetOwner()->IsClient())
			killer = killer->GetOwner();
		if(killer != 0 && killer->IsClient()) {
			corpse->AllowMobLoot(killer, 0);
			if(killer->IsGrouped()) {
				Group* group = entity_list.GetGroupByClient(killer->CastToClient());
				if(group != 0) {
					for(int i=0;i<6;i++) { // Doesnt work right, needs work
						if(group->members[i] != NULL) {
							corpse->AllowMobLoot(group->members[i],i);
						}
					}
				}
			}
			else if(killer->IsRaidGrouped()){
				Raid* r = entity_list.GetRaidByClient(killer->CastToClient());
				if(r){
					int i = 0;
					for(int x = 0; x < MAX_RAID_MEMBERS; x++)
					{
						switch(r->GetLootType())
						{
						case 0:
						case 1:
							if(r->members[x].member && r->members[x].IsRaidLeader){
								corpse->AllowMobLoot(r->members[x].member, i);
								i++;
							}
							break;
						case 2:
							if(r->members[x].member && r->members[x].IsRaidLeader){
								corpse->AllowMobLoot(r->members[x].member, i);
								i++;
							}
							else if(r->members[x].member && r->members[x].IsGroupLeader){
								corpse->AllowMobLoot(r->members[x].member, i);
								i++;
							}
							break;
						case 3:
							if(r->members[x].member && r->members[x].IsLooter){
								corpse->AllowMobLoot(r->members[x].member, i);
								i++;
							}
							break;
						}
					}
				}
			}
		}
	}
	
	// Parse quests even if we're killed by an NPC
	if(other) {
		Mob *oos = other->GetOwnerOrSelf();
		parse->Event(EVENT_DEATH, this->GetNPCTypeID(),0, this, oos);
		if(oos->IsNPC())
			parse->Event(EVENT_NPC_SLAY, this->GetNPCTypeID(), 0, oos->CastToNPC(), this);
	}
	
	this->WhipeHateList();
	p_depop = true;
	if(other && other->GetTarget() == this) //we can kill things without having them targeted
		other->SetTarget(NULL); //via AE effects and such..

#ifdef EQBOTS

	// handle group removal for dead bots
	if(IsBot()) {
		Group *g = entity_list.GetGroupByMob(this);
        if(g) {
			for(int i=0; i<MAX_GROUP_MEMBERS; i++) {
                if(g->members[i]) {
                    if(g->members[i] == this) {
						// If the leader dies, make the next bot the leader
						// and reset all bots followid
                        if(g->IsLeader(g->members[i])) {
                            if(g->members[i+1]) {
                                g->SetLeader(g->members[i+1]);
                                g->members[i+1]->SetFollowID(g->members[i]->GetFollowID());
                                for(int j=0; j<MAX_GROUP_MEMBERS; j++) {
                                    if(g->members[j] && (g->members[j] != g->members[i+1])) {
                                        g->members[j]->SetFollowID(g->members[i+1]->GetID());
                                    }
                                }
                            }
                        }
						// Delete from database
						database.CleanBotLeaderEntries(botid);
						entity_list.RemoveNPC(botid);

						// delete from group data
						g->membername[i][0] = '\0';
						memset(g->membername[i], 0, 64);
						g->members[i]->BotOwner = NULL;
						g->members[i] = NULL;

						// if group members exist below this one, move
						// them all up one slot in the group list
						int j = i+1;
						for(; j<MAX_GROUP_MEMBERS; j++) {
							if(g->members[j]) {
								g->members[j-1] = g->members[j];
								strcpy(g->membername[j-1], g->members[j]->GetName());
								g->membername[j][0] = '\0';
								memset(g->membername[j], 0, 64);
								g->members[j] = NULL;
							}
						}

						// update the client group
						EQApplicationPacket* outapp = new EQApplicationPacket(OP_GroupUpdate, sizeof(GroupJoin_Struct));
						GroupJoin_Struct* gu = (GroupJoin_Struct*)outapp->pBuffer;
						gu->action = groupActLeave;
						strcpy(gu->membername, GetName());
						if(g) {
							for(int k=0; k<MAX_GROUP_MEMBERS; k++) {
								if(g->members[k] && g->members[k]->IsClient())
									g->members[k]->CastToClient()->QueuePacket(outapp);
							}
						}
						safe_delete(outapp);
						Say("oof");

						// now that's done, lets see if all we have left is the client
						// and we can clean up the clients raid group and group
						if(IsBotRaiding()) {
							BotRaids* br = entity_list.GetBotRaidByMob(this);
							if(br) {
								if(this == br->botmaintank) {
									br->botmaintank = NULL;
								}
								if(this == br->botsecondtank) {
									br->botsecondtank = NULL;
								}
							}
							if(g->BotGroupCount() == 0) {
								int32 gid = g->GetID();
								if(br) {
									br->RemoveEmptyBotGroup();
								}
								entity_list.RemoveGroup(gid);
							}
							if(br && (br->RaidBotGroupsCount() == 1)) {
								br->RemoveClientGroup(br->GetRaidBotLeader());
							}
							if(br && (br->RaidBotGroupsCount() == 0)) {
								br->DisbandBotRaid();
							}
						}
					}
				}
			}
		}
	}

#endif //EQBOTS

}

void Mob::AddToHateList(Mob* other, sint32 hate, sint32 damage, bool iYellForHelp, bool bFrenzy, bool iBuffTic) {
    assert(other != NULL);
    if (other == this)
        return;
    if(damage < 0){
        hate = 1;
    }
	bool wasengaged = IsEngaged();
	Mob* owner = other->GetOwner();
	Mob* mypet = this->GetPet();
	Mob* myowner = this->GetOwner();
	
	if(other){
		AddRampage(other);
		int hatemod = 100 + other->spellbonuses.hatemod + other->itembonuses.hatemod;
		if(hatemod < 1)
			hatemod = 1;
		hate = ((hate * (hatemod))/100);
	}
	
	if(IsPet() && GetOwner() && GetOwner()->GetAA(aaPetDiscipline) && IsHeld()){
		return; 
	}	

	if(IsClient() && !IsAIControlled())
		return;

	if(IsFamiliar() || SpecAttacks[IMMUNE_AGGRO])
		return;	

	if (other == myowner)
		return;

	// first add self
	hate_list.Add(other, hate, damage, bFrenzy, !iBuffTic);

#ifdef EQBOTS

	// if other is a bot, add the bots client to the hate list
	if(other->IsBot()) {
		if(other->BotOwner && other->BotOwner->CastToClient()->GetFeigned()) {
			AddFeignMemory(other->BotOwner->CastToClient());
		}
		else {
			hate_list.Add(other->BotOwner, 0, 0, false, true);
		}
	}

#endif //EQBOTS

	// then add pet owner if there's one
	if (owner) { // Other is a pet, add him and it
		// EverHood 6/12/06
		// Can't add a feigned owner to hate list
		if(owner->IsClient() && owner->CastToClient()->GetFeigned()) {
			//they avoid hate due to feign death...
		} else {
			// cb:2007-08-17
			// owner must get on list, but he's not actually gained any hate yet
			if(!owner->SpecAttacks[IMMUNE_AGGRO])
				hate_list.Add(owner, 0, 0, false, !iBuffTic);
		}
	}	
	
	if (mypet && (!(GetAA(aaPetDiscipline) && mypet->IsHeld()))) { // I have a pet, add other to it
		if(!mypet->IsFamiliar() && !mypet->SpecAttacks[IMMUNE_AGGRO])
			mypet->hate_list.Add(other, 0, 0, bFrenzy);
	} else if (myowner) { // I am a pet, add other to owner if it's NPC/LD
		if (myowner->IsAIControlled() && !myowner->SpecAttacks[IMMUNE_AGGRO])
			myowner->hate_list.Add(other, 0, 0, bFrenzy);
	}
	if (!wasengaged) { 
		if(IsNPC() && other->IsClient() && other->CastToClient())
			parse->Event(EVENT_AGGRO, this->GetNPCTypeID(), 0, CastToNPC(), other); 
		AI_Event_Engaged(other, iYellForHelp); 
		adverrorinfo = 8293;
	}
}

// solar: this is called from Damage() when 'this' is attacked by 'other.  
// 'this' is the one being attacked
// 'other' is the attacker
// a damage shield causes damage (or healing) to whoever attacks the wearer
// a reverse ds causes damage to the wearer whenever it attack someone
// given this, a reverse ds must be checked each time the wearer is attacking
// and not when they're attacked
void Mob::DamageShield(Mob* attacker) {
	//a damage shield on a spell is a negative value but on an item it's a positive value so add the spell value and subtract the item value to get the end ds value
	int DS = spellbonuses.DamageShield - itembonuses.DamageShield;
	if(DS == 0)
		return;
		
	if(this == attacker) //I was crashing when I hit myself with melee with a DS on, not sure why but we shouldn't be reflecting damage onto ourselves anyway really.
		return;
	
	mlog(COMBAT__HITS, "Applying Damage Shield of value %d to %s", DS, attacker->GetName());
	
	int16 spellid = SPELL_UNKNOWN;
	if(spellbonuses.DamageShieldSpellID != 0 && spellbonuses.DamageShieldSpellID != SPELL_UNKNOWN)
		spellid = spellbonuses.DamageShieldSpellID;
	//invert DS... spells yeild negative values for a true damage shield
	if(DS < 0) {
		attacker->Damage(this, -DS, spellid, ABJURE/*hackish*/, false);
		// todo: send EnvDamage packet to the attacker
		//entity_list.MessageClose(attacker, 0, 200, 10, "%s takes %d damage from %s's damage shield (%s)", attacker->GetName(), -spells[buffs[i].spellid].base[1], this->GetName(), spells[buffs[i].spellid].name);
	} else {
		//we are healing the attacker...
		attacker->HealDamage(DS);
		//TODO: send a packet???
	}
	
/*	int DS = 0;
//	int DSRev = 0;
	int effect_value, dmg, i, z;

	for (i=0; i < BUFF_COUNT; i++)
	{
		if (buffs[i].spellid != SPELL_UNKNOWN)
		{
			for (z=0; z < EFFECT_COUNT; z++)
			{
				if(IsBlankSpellEffect(buffs[i].spellid, z))
					continue;
				
				effect_value = CalcSpellEffectValue(buffs[i].spellid, z, buffs[i].casterlevel, this);
				
				switch(spells[buffs[i].spellid].effectid[z])
				{
					case SE_DamageShield:
					{
						dmg = effect_value;
						DS += dmg;
						spellid = buffs[i].spellid;
						break;
					}
*/
/*
					case SE_ReverseDS:
					{
						dmg = effect_value;
						DSRev += dmg;
						spellid = buffs[i].spellid;
						break;
					}
*/
/*				}
			}
		}
	}
	// there's somewhat of an issue here that the last (slot wise) damage shield
	// will be the one whose spellid is set here
	if (DS) {
	}*/
/*
	if (DSRev)
	{
		this->ChangeHP(attacker, DSRev, spellid);
	}
*/
}

int8 Mob::GetWeaponDamageBonus( const Item_Struct *Weapon )
{
	// This function calculates and returns the damage bonus for the weapon identified by the parameter "Weapon".
	// Modified 9/21/2008 by Cantus


	// Assert: This function should only be called for hits by the mainhand, as damage bonuses apply only to the
	//         weapon in the primary slot. Be sure to check that Hand == 13 before calling.

	// Assert: The caller should ensure that Weapon is actually a weapon before calling this function.
	//         The ItemInst::IsWeapon() method can be used to quickly determine this.

	// Assert: This function should not be called if the player's level is below 28, as damage bonuses do not begin
	//         to apply until level 28.

	// Assert: This function should not be called unless the player is a melee class, as casters do not receive a damage bonus.


	if( Weapon == NULL || Weapon->ItemType == ItemType1HS || Weapon->ItemType == ItemType1HB || Weapon->ItemType == ItemTypeHand2Hand || Weapon->ItemType == ItemTypePierce )
	{
		// The weapon in the player's main (primary) hand is a one-handed weapon, or there is no item equipped at all.
		//
		// According to player posts on Allakhazam, 1H damage bonuses apply to bare fists (nothing equipped in the mainhand,
		// as indicated by Weapon == NULL).
		//
		// The following formula returns the correct damage bonus for all 1H weapons:

		return (int8) ((GetLevel() - 25) / 3);
	}

	// If we've gotten to this point, the weapon in the mainhand is a two-handed weapon.
	// Calculating damage bonuses for 2H weapons is more complicated, as it's based on PC level AND the delay of the weapon.
	// The formula to calculate 2H bonuses is HIDEOUS. It's a huge conglomeration of ternary operators and multiple operations.
	//
	// The following is a hybrid approach. In cases where the Level and Delay merit a formula that does not use many operators,
	// the formula is used. In other cases, lookup tables are used for speed.
	// Though the following code may look bloated and ridiculous, it's actually a very efficient way of calculating these bonuses.

	// Player Level is used several times in the code below, so save it into a variable.
	// If GetLevel() were an ordinary function, this would DEFINITELY make sense, as it'd cut back on all of the function calling
	// overhead involved with multiple calls to GetLevel(). But in this case, GetLevel() is a simple, inline accessor method.
	// So it probably doesn't matter. If anyone knows for certain that there is no overhead involved with calling GetLevel(),
	// as I suspect, then please feel free to delete the following line, and replace all occurences of "ucPlayerLevel" with "GetLevel()".
	int8 ucPlayerLevel = (int8) GetLevel();


	// The following may look cleaner, and would certainly be easier to understand, if it was
	// a simple 53x150 cell matrix.
	//
	// However, that would occupy 7,950 Bytes of memory (7.76 KB), and would likely result
	// in "thrashing the cache" when performing lookups.
	//
	// Initially, I thought the best approach would be to reverse-engineer the formula used by
	// Sony/Verant to calculate these 2H weapon damage bonuses. But the more than Reno and I
	// worked on figuring out this formula, the more we're concluded that the formula itself ugly
	// (that is, it contains so many operations and conditionals that it's fairly CPU intensive).
	// Because of that, we're decided that, in most cases, a lookup table is the most efficient way
	// to calculate these damage bonuses.
	//
	// The code below is a hybrid between a pure formulaic approach and a pure, brute-force
	// lookup table. In cases where a formula is the best bet, I use a formula. In other places
	// where a formula would be ugly, I use a lookup table in the interests of speed. 


	if( Weapon->Delay <= 27 )
	{
		// Damage Bonuses for all 2H weapons with delays of 27 or less are identical.
		// They are the same as the damage bonus would be for a corresponding 1H weapon, plus one.
		// This formula applies to all levels 28-80, and will probably continue to apply if

		// the level cap on Live ever is increased beyond 80.

		return (ucPlayerLevel - 22) / 3;
	}


	if( ucPlayerLevel == 65 && Weapon->Delay <= 59 )
	{
		// Consider these two facts:
		//   * Level 65 is the maximum level on many EQ Emu servers.
		//   * If you listed the levels of all characters logged on to a server, odds are that the number you'll
		//     see most frequently is level 65. That is, there are more level 65 toons than any other single level.
		//
		// Therefore, if we can optimize this function for level 65 toons, we're speeding up the server!
		//
		// With that goal in mind, I create an array of Damage Bonuses for level 65 characters wielding 2H weapons with
		// delays between 28 and 59 (inclusive). I suspect that this one small lookup array will therefore handle
		// many of the calls to this function.

		static const int8 ucLevel65DamageBonusesForDelays28to59[] = {35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40, 40, 42, 42, 42, 45, 45, 47, 48, 49, 49, 51, 51, 52, 53, 54, 54, 56, 56, 57, 58, 59};

		return ucLevel65DamageBonusesForDelays28to59[Weapon->Delay-28];
	}


	if( ucPlayerLevel > 65 )
	{
		if( ucPlayerLevel > 80 )
		{
			// As level 80 is currently the highest achievable level on Live, we only include
			// damage bonus information up to this level.
			//
			// If there is a custom EQEmu server that allows players to level beyond 80, the
			// damage bonus for their 2H weapons will simply not increase beyond their damage
			// bonus at level 80.

			ucPlayerLevel = 80;
		}

		// Lucy does not list a chart of damage bonuses for players levels 66+,
		// so my original version of this function just applied the level 65 damage
		// bonus for level 66+ toons. That sucked for higher level toons, as their
		// 2H weapons stopped ramping up in DPS as they leveled past 65.
		//
		// Thanks to the efforts of two guys, this is no longer the case:
		//
		// Janusd (Zetrakyl) ran a nifty query against the PEQ item database to list
		// the name of an example 2H weapon that represents each possible unique 2H delay.
		//
		// Romai then wrote an excellent script to automatically look up each of those
		// weapons, open the Lucy item page associated with it, and iterate through all
		// levels in the range 66 - 80. He saved the damage bonus for that weapon for
		// each level, and that forms the basis of the lookup tables below.

		if( Weapon->Delay <= 59 )
		{
			static const int8 ucDelay28to59Levels66to80[32][15]=
			{
			/*							Level:								*/
			/*	 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80	*/

				{36, 37, 38, 39, 41, 42, 43, 44, 45, 47, 49, 49, 49, 50, 53},	/* Delay = 28 */
				{36, 38, 38, 39, 42, 43, 43, 45, 46, 48, 49, 50, 51, 52, 54},	/* Delay = 29 */
				{37, 38, 39, 40, 43, 43, 44, 46, 47, 48, 50, 51, 52, 53, 55},	/* Delay = 30 */
				{37, 39, 40, 40, 43, 44, 45, 46, 47, 49, 51, 52, 52, 52, 54},	/* Delay = 31 */
				{38, 39, 40, 41, 44, 45, 45, 47, 48, 48, 50, 52, 53, 55, 57},	/* Delay = 32 */
				{38, 40, 41, 41, 44, 45, 46, 48, 49, 50, 52, 53, 54, 56, 58},	/* Delay = 33 */
				{39, 40, 41, 42, 45, 46, 47, 48, 49, 51, 53, 54, 55, 57, 58},	/* Delay = 34 */
				{39, 41, 42, 43, 46, 46, 47, 49, 50, 52, 54, 55, 56, 57, 59},	/* Delay = 35 */
				{40, 41, 42, 43, 46, 47, 48, 50, 51, 53, 55, 55, 56, 58, 60},	/* Delay = 36 */
				{40, 42, 43, 44, 47, 48, 49, 50, 51, 53, 55, 56, 57, 59, 61},	/* Delay = 37 */
				{41, 42, 43, 44, 47, 48, 49, 51, 52, 54, 56, 57, 58, 60, 62},	/* Delay = 38 */
				{41, 43, 44, 45, 48, 49, 50, 52, 53, 55, 57, 58, 59, 61, 63},	/* Delay = 39 */
				{43, 45, 46, 47, 50, 51, 52, 54, 55, 57, 59, 60, 61, 63, 65},	/* Delay = 40 */
				{43, 45, 46, 47, 50, 51, 52, 54, 55, 57, 59, 60, 61, 63, 65},	/* Delay = 41 */
				{44, 46, 47, 48, 51, 52, 53, 55, 56, 58, 60, 61, 62, 64, 66},	/* Delay = 42 */
				{46, 48, 49, 50, 53, 54, 55, 58, 59, 61, 63, 64, 65, 67, 69},	/* Delay = 43 */
				{47, 49, 50, 51, 54, 55, 56, 58, 59, 61, 64, 65, 66, 68, 70},	/* Delay = 44 */
				{48, 50, 51, 52, 56, 57, 58, 60, 61, 63, 65, 66, 68, 70, 72},	/* Delay = 45 */
				{50, 52, 53, 54, 57, 58, 59, 62, 63, 65, 67, 68, 69, 71, 74},	/* Delay = 46 */
				{50, 52, 53, 55, 58, 59, 60, 62, 63, 66, 68, 69, 70, 72, 74},	/* Delay = 47 */
				{51, 53, 54, 55, 58, 60, 61, 63, 64, 66, 69, 69, 71, 73, 75},	/* Delay = 48 */
				{52, 54, 55, 57, 60, 61, 62, 65, 66, 68, 70, 71, 73, 75, 77},	/* Delay = 49 */
				{53, 55, 56, 57, 61, 62, 63, 65, 67, 69, 71, 72, 74, 76, 78},	/* Delay = 50 */
				{53, 55, 57, 58, 61, 62, 64, 66, 67, 69, 72, 73, 74, 77, 79},	/* Delay = 51 */
				{55, 57, 58, 59, 63, 64, 65, 68, 69, 71, 74, 75, 76, 78, 81},	/* Delay = 52 */
				{57, 55, 59, 60, 63, 65, 66, 68, 70, 72, 74, 76, 77, 79, 82},	/* Delay = 53 */
				{56, 58, 59, 61, 64, 65, 67, 69, 70, 73, 75, 76, 78, 80, 82},	/* Delay = 54 */
				{57, 59, 61, 62, 66, 67, 68, 71, 72, 74, 77, 78, 80, 82, 84},	/* Delay = 55 */
				{58, 60, 61, 63, 66, 68, 69, 71, 73, 75, 78, 79, 80, 83, 85},	/* Delay = 56 */

				/* Important Note: Janusd's search for 2H weapons did not find	*/
				/* any 2H weapon with a delay of 57. Therefore the values below	*/
				/* are interpolated, not exact!									*/
				{59, 61, 62, 64, 67, 69, 70, 72, 74, 76, 77, 78, 81, 84, 86},	/* Delay = 57 INTERPOLATED */

				{60, 62, 63, 65, 68, 70, 71, 74, 75, 78, 80, 81, 83, 85, 88},	/* Delay = 58 */

				/* Important Note: Janusd's search for 2H weapons did not find	*/
				/* any 2H weapon with a delay of 59. Therefore the values below	*/
				/* are interpolated, not exact!									*/
				{60, 62, 64, 65, 69, 70, 72, 74, 76, 78, 81, 82, 84, 86, 89},	/* Delay = 59 INTERPOLATED */
			};

			return ucDelay28to59Levels66to80[Weapon->Delay-28][ucPlayerLevel-66];
		}
		else
		{
			// Delay is 60+ 

			const static int8 ucDelayOver59Levels66to80[6][15] =
			{
			/*							Level:								*/
			/*	 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80	*/

				{61, 63, 65, 66, 70, 71, 73, 75, 77, 79, 82, 83, 85, 87, 90},				/* Delay = 60 */
				{65, 68, 69, 71, 75, 76, 78, 80, 82, 85, 87, 89, 91, 93, 96},				/* Delay = 65 */

				/* Important Note: Currently, the only 2H weapon with a delay	*/
				/* of 66 is not player equippable (it's None/None). So I'm		*/
				/* leaving it commented out to keep this table smaller.			*/
				//{66, 68, 70, 71, 75, 77, 78, 81, 83, 85, 88, 90, 91, 94, 97},				/* Delay = 66 */

				{70, 72, 74, 76, 80, 81, 83, 86, 88, 88, 90, 95, 97, 99, 102},				/* Delay = 70 */
				{82, 85, 87, 89, 89, 94, 98, 101, 103, 106, 109, 111, 114, 117, 120},		/* Delay = 85 */
				{90, 93, 96, 98, 103, 105, 107, 111, 113, 116, 120, 122, 125, 128, 131},	/* Delay = 95 */

				/* Important Note: Currently, the only 2H weapons with delay	*/
				/* 100 are GM-only items purchased from vendors in Sunset Home	*/
				/* (cshome). Because they are highly unlikely to be used in		*/
				/* combat, I'm commenting it out to keep the table smaller.		*/
				//{95, 98, 101, 103, 108, 110, 113, 116, 119, 122, 126, 128, 131, 134, 138},/* Delay = 100 */

				{136, 140, 144, 148, 154, 157, 161, 166, 170, 174, 179, 183, 187, 191, 196}	/* Delay = 150 */
			};

			if( Weapon->Delay < 65 )
			{
				return ucDelayOver59Levels66to80[0][ucPlayerLevel-66];
			}
			else if( Weapon->Delay < 70 )
			{
				return ucDelayOver59Levels66to80[1][ucPlayerLevel-66];
			}
			else if( Weapon->Delay < 85 )
			{
				return ucDelayOver59Levels66to80[2][ucPlayerLevel-66];
			}
			else if( Weapon->Delay < 95 )
			{
				return ucDelayOver59Levels66to80[3][ucPlayerLevel-66];
			}
			else if( Weapon->Delay < 150 )
			{
				return ucDelayOver59Levels66to80[4][ucPlayerLevel-66];
			}
			else
			{
				return ucDelayOver59Levels66to80[5][ucPlayerLevel-66];
			}
		}
	}


	// If we've gotten to this point in the function without hitting a return statement,
	// we know that the character's level is between 28 and 65, and that the 2H weapon's
	// delay is 28 or higher.

	// The Damage Bonus values returned by this function (in the level 28-65 range) are
	// based on a table of 2H Weapon Damage Bonuses provided by Lucy at the following address:
	// http://lucy.allakhazam.com/dmgbonus.html

	if( Weapon->Delay <= 39 )
	{
		if( ucPlayerLevel <= 53)
		{
			// The Damage Bonus for all 2H weapons with delays between 28 and 39 (inclusive) is the same for players level 53 and below...
			static const int8 ucDelay28to39LevelUnder54[] = {1, 1, 2, 3, 3, 3, 4, 5, 5, 6, 6, 6, 8, 8, 8, 9, 9, 10, 11, 11, 11, 12, 13, 14, 16, 17};

			// As a note: The following formula accurately calculates damage bonuses for 2H weapons with delays in the range 28-39 (inclusive)
			// for characters levels 28-50 (inclusive):
			// return ( (ucPlayerLevel - 22) / 3 ) + ( (ucPlayerLevel - 25) / 5 );
			//
			// However, the small lookup array used above is actually much faster. So we'll just use it instead of the formula
			//
			// (Thanks to Reno for helping figure out the above formula!)

			return ucDelay28to39LevelUnder54[ucPlayerLevel-28];
		}
		else
		{
			// Use a matrix to look up the damage bonus for 2H weapons with delays between 28 and 39 wielded by characters level 54 and above.
			static const int8 ucDelay28to39Level54to64[12][11] =
			{
			/*						Level:					*/
			/*	 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64	*/

				{17, 21, 21, 23, 25, 26, 28, 30, 31, 31, 33},	/* Delay = 28 */
				{17, 21, 22, 23, 25, 26, 29, 30, 31, 32, 34},	/* Delay = 29 */
				{18, 21, 22, 23, 25, 27, 29, 31, 32, 32, 34},	/* Delay = 30 */
				{18, 21, 22, 23, 25, 27, 29, 31, 32, 33, 34},	/* Delay = 31 */
				{18, 21, 22, 24, 26, 27, 30, 32, 32, 33, 35},	/* Delay = 32 */
				{18, 21, 22, 24, 26, 27, 30, 32, 33, 34, 35},	/* Delay = 33 */
				{18, 22, 22, 24, 26, 28, 30, 32, 33, 34, 36},	/* Delay = 34 */
				{18, 22, 23, 24, 26, 28, 31, 33, 34, 34, 36},	/* Delay = 35 */
				{18, 22, 23, 25, 27, 28, 31, 33, 34, 35, 37},	/* Delay = 36 */
				{18, 22, 23, 25, 27, 29, 31, 33, 34, 35, 37},	/* Delay = 37 */
				{18, 22, 23, 25, 27, 29, 32, 34, 35, 36, 38},	/* Delay = 38 */
				{18, 22, 23, 25, 27, 29, 32, 34, 35, 36, 38}	/* Delay = 39 */
			};

			return ucDelay28to39Level54to64[Weapon->Delay-28][ucPlayerLevel-54];
		}
	}
	else if( Weapon->Delay <= 59 )
	{
		if( ucPlayerLevel <= 52 )
		{
			if( Weapon->Delay <= 45 )
			{
				static const int8 ucDelay40to45Levels28to52[6][25] =
				{
				/*												Level:														*/
				/*	 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52		*/

					{2,  2,  3,  4,  4,  4,  5,  6,  6,  7,  7,  7,  9,  9,  9,  10, 10, 11, 12, 12, 12, 13, 14, 16, 18},	/* Delay = 40 */
					{2,  2,  3,  4,  4,  4,  5,  6,  6,  7,  7,  7,  9,  9,  9,  10, 10, 11, 12, 12, 12, 13, 14, 16, 18},	/* Delay = 41 */
					{2,  2,  3,  4,  4,  4,  5,  6,  6,  7,  7,  7,  9,  9,  9,  10, 10, 11, 12, 12, 12, 13, 14, 16, 18},	/* Delay = 42 */
					{4,  4,  5,  6,  6,  6,  7,  8,  8,  9,  9,  9,  11, 11, 11, 12, 12, 13, 14, 14, 14, 15, 16, 18, 20},	/* Delay = 43 */
					{4,  4,  5,  6,  6,  6,  7,  8,  8,  9,  9,  9,  11, 11, 11, 12, 12, 13, 14, 14, 14, 15, 16, 18, 20},	/* Delay = 44 */
					{5,  5,  6,  7,  7,  7,  8,  9,  9,  10, 10, 10, 12, 12, 12, 13, 13, 14, 15, 15, 15, 16, 17, 19, 21} 	/* Delay = 45 */
				};

				return ucDelay40to45Levels28to52[Weapon->Delay-40][ucPlayerLevel-28];
			}
			else
			{
				static const int8 ucDelay46Levels28to52[] = {6,  6,  7,  8,  8,  8,  9,  10, 10, 11, 11, 11, 13, 13, 13, 14, 14, 15, 16, 16, 16, 17, 18, 20, 22};

				return ucDelay46Levels28to52[ucPlayerLevel-28] + ((Weapon->Delay-46) / 3);
			}
		}
		else
		{
			// Player is in the level range 53 - 64

			// Calculating damage bonus for 2H weapons with a delay between 40 and 59 (inclusive) involves, unforunately, a brute-force matrix lookup.
			static const int8 ucDelay40to59Levels53to64[20][37] = 
			{
			/*						Level:							*/
			/*	 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64		*/

				{19, 20, 24, 25, 27, 29, 31, 34, 36, 37, 38, 40},	/* Delay = 40 */
				{19, 20, 24, 25, 27, 29, 31, 34, 36, 37, 38, 40},	/* Delay = 41 */
				{19, 20, 24, 25, 27, 29, 31, 34, 36, 37, 38, 40},	/* Delay = 42 */
				{21, 22, 26, 27, 29, 31, 33, 37, 39, 40, 41, 43},	/* Delay = 43 */
				{21, 22, 26, 27, 29, 32, 34, 37, 39, 40, 41, 43},	/* Delay = 44 */
				{22, 23, 27, 28, 31, 33, 35, 38, 40, 42, 43, 45},	/* Delay = 45 */
				{23, 24, 28, 30, 32, 34, 36, 40, 42, 43, 44, 46},	/* Delay = 46 */
				{23, 24, 29, 30, 32, 34, 37, 40, 42, 43, 44, 47},	/* Delay = 47 */
				{23, 24, 29, 30, 32, 35, 37, 40, 43, 44, 45, 47},	/* Delay = 48 */
				{24, 25, 30, 31, 34, 36, 38, 42, 44, 45, 46, 49},	/* Delay = 49 */
				{24, 26, 30, 31, 34, 36, 39, 42, 44, 46, 47, 49},	/* Delay = 50 */
				{24, 26, 30, 31, 34, 36, 39, 42, 45, 46, 47, 49},	/* Delay = 51 */
				{25, 27, 31, 33, 35, 38, 40, 44, 46, 47, 49, 51},	/* Delay = 52 */
				{25, 27, 31, 33, 35, 38, 40, 44, 46, 48, 49, 51},	/* Delay = 53 */
				{26, 27, 32, 33, 36, 38, 41, 44, 47, 48, 49, 52},	/* Delay = 54 */
				{27, 28, 33, 34, 37, 39, 42, 46, 48, 50, 51, 53},	/* Delay = 55 */
				{27, 28, 33, 34, 37, 40, 42, 46, 49, 50, 51, 54},	/* Delay = 56 */
				{27, 28, 33, 34, 37, 40, 43, 46, 49, 50, 52, 54},	/* Delay = 57 */
				{28, 29, 34, 36, 39, 41, 44, 48, 50, 52, 53, 56},	/* Delay = 58 */
				{28, 29, 34, 36, 39, 41, 44, 48, 51, 52, 54, 56}	/* Delay = 59 */
			};

			return ucDelay40to59Levels53to64[Weapon->Delay-40][ucPlayerLevel-53];
		}
	}
	else
	{
		// The following table allows us to look up Damage Bonuses for weapons with delays greater than or equal to 60.
		//
		// There aren't a lot of 2H weapons with a delay greater than 60. In fact, both a database and Lucy search run by janusd confirm
		// that the only unique 2H delays greater than 60 are: 65, 70, 85, 95, and 150.
		//
		// To be fair, there are also weapons with delays of 66 and 100. But they are either not equippable (None/None), or are
		// only available to GMs from merchants in Sunset Home (cshome). In order to keep this table "lean and mean", I will not
		// include the values for delays 66 and 100. If they ever are wielded, the 66 delay weapon will use the 65 delay bonuses,
		// and the 100 delay weapon will use the 95 delay bonuses. So it's not a big deal.
		//
		// Still, if someone in the future decides that they do want to include them, here are the tables for these two delays:
		//
		// {12, 12, 13, 14, 14, 14, 15, 16, 16, 17, 17, 17, 19, 19, 19, 20, 20, 21, 22, 22, 22, 23, 24, 26, 29, 30, 32, 37, 39, 42, 45, 48, 53, 55, 57, 59, 61, 64}		/* Delay = 66 */
		// {24, 24, 25, 26, 26, 26, 27, 28, 28, 29, 29, 29, 31, 31, 31, 32, 32, 33, 34, 34, 34, 35, 36, 39, 43, 45, 48, 55, 57, 62, 66, 71, 77, 80, 83, 85, 89, 92}		/* Delay = 100 */
		//
		// In case there are 2H weapons added in the future with delays other than those listed above (and until the damage bonuses
		// associated with that new delay are added to this function), this function is designed to do the following:
		//
		//		For weapons with delays in the range 60-64, use the Damage Bonus that would apply to a 2H weapon with delay 60.
		//		For weapons with delays in the range 65-69, use the Damage Bonus that would apply to a 2H weapon with delay 65
		//		For weapons with delays in the range 70-84, use the Damage Bonus that would apply to a 2H weapon with delay 70.
		//		For weapons with delays in the range 85-94, use the Damage Bonus that would apply to a 2H weapon with delay 85.
		//		For weapons with delays in the range 95-149, use the Damage Bonus that would apply to a 2H weapon with delay 95.
		//		For weapons with delays 150 or higher, use the Damage Bonus that would apply to a 2H weapon with delay 150.

		static const int8 ucDelayOver59Levels28to65[6][38] =
		{
		/*																	Level:																					*/
		/*	 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64. 65	*/

			{10, 10, 11, 12, 12, 12, 13, 14, 14, 15, 15, 15, 17, 17, 17, 18, 18, 19, 20, 20, 20, 21, 22, 24, 27, 28, 30, 35, 36, 39, 42, 45, 49, 51, 53, 54, 57, 59},		/* Delay = 60 */
			{12, 12, 13, 14, 14, 14, 15, 16, 16, 17, 17, 17, 19, 19, 19, 20, 20, 21, 22, 22, 22, 23, 24, 26, 29, 30, 32, 37, 39, 42, 45, 48, 52, 55, 57, 58, 61, 63},		/* Delay = 65 */
			{14, 14, 15, 16, 16, 16, 17, 18, 18, 19, 19, 19, 21, 21, 21, 22, 22, 23, 24, 24, 24, 25, 26, 28, 31, 33, 35, 40, 42, 45, 48, 52, 56, 59, 61, 62, 65, 68},		/* Delay = 70 */
			{19, 19, 20, 21, 21, 21, 22, 23, 23, 24, 24, 24, 26, 26, 26, 27, 27, 28, 29, 29, 29, 30, 31, 34, 37, 39, 41, 47, 49, 54, 57, 61, 66, 69, 72, 74, 77, 80},		/* Delay = 85 */
			{22, 22, 23, 24, 24, 24, 25, 26, 26, 27, 27, 27, 29, 29, 29, 30, 30, 31, 32, 32, 32, 33, 34, 37, 40, 43, 45, 52, 54, 59, 62, 67, 73, 76, 79, 81, 84, 88},		/* Delay = 95 */
			{40, 40, 41, 42, 42, 42, 43, 44, 44, 45, 45, 45, 47, 47, 47, 48, 48, 49, 50, 50, 50, 51, 52, 56, 61, 65, 69, 78, 82, 89, 94, 102, 110, 115, 119, 122, 127, 132}	/* Delay = 150 */
		};

		if( Weapon->Delay < 65 )
		{
			return ucDelayOver59Levels28to65[0][ucPlayerLevel-28];
		}
		else if( Weapon->Delay < 70 )
		{
			return ucDelayOver59Levels28to65[1][ucPlayerLevel-28];
		}
		else if( Weapon->Delay < 85 )
		{
			return ucDelayOver59Levels28to65[2][ucPlayerLevel-28];
		}
		else if( Weapon->Delay < 95 )
		{
			return ucDelayOver59Levels28to65[3][ucPlayerLevel-28];
		}
		else if( Weapon->Delay < 150 )
		{
			return ucDelayOver59Levels28to65[4][ucPlayerLevel-28];
		}
		else
		{
			return ucDelayOver59Levels28to65[5][ucPlayerLevel-28];
		}
	}
}

int Mob::GetMonkHandToHandDamage(void)
{
	// Kaiyodo - Determine a monk's fist damage. Table data from www.monkly-business.com
	// saved as static array - this should speed this function up considerably
	static int damage[66] = {
	//   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
        99, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
         8, 8, 8, 8, 8, 9, 9, 9, 9, 9,10,10,10,10,10,11,11,11,11,11,
        12,12,12,12,12,13,13,13,13,13,14,14,14,14,14,14,14,14,14,14,
        14,14,15,15,15,15 };
	
	// Have a look to see if we have epic fists on

#ifdef EQBOTS

	uint32 botWeaponId = INVALID_ID;
	if(IsBot()) {
		botWeaponId = CastToNPC()->GetEquipment(MATERIAL_HANDS);
		if(botWeaponId == 10652) { //Monk Epic ID
			return 9;
		}
		else
		{
			int Level = GetLevel();
			if(Level > 65)
				return 19;
			else
				return damage[Level];
		}
	}

#endif //EQBOTS

	if (IsClient() && CastToClient()->GetItemIDAt(12) == 10652)
		return(9);
	else
	{
		int Level = GetLevel();
        if (Level > 65)
		    return(19);
        else
            return damage[Level];
	}
}

int Mob::GetMonkHandToHandDelay(void)
{
	// Kaiyodo - Determine a monk's fist delay. Table data from www.monkly-business.com
	// saved as static array - this should speed this function up considerably
	static int delayshuman[66] = {
	//  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,33,33,33,33,33,
        32,32,32,32,32,31,31,31,31,31,30,30,30,29,29,29,28,28,28,27,
        26,24,22,20,20,20  };
	static int delaysiksar[66] = {
	//  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,
        33,33,33,33,33,32,32,32,32,32,31,31,31,30,30,30,29,29,29,28,
        27,24,22,20,20,20 };
	
	// Have a look to see if we have epic fists on
	if (IsClient() && CastToClient()->GetItemIDAt(12) == 10652)
		return(16);
	else
	{
		int Level = GetLevel();
		if (GetRace() == HUMAN)
		{
            if (Level > 65)
			    return(24);
            else
                return delayshuman[Level];
		}
		else	//heko: iksar table
		{
            if (Level > 65)
			    return(25);
            else
                return delaysiksar[Level];
		}
	}
}

sint32 Mob::ReduceDamage(sint32 damage){
	if(damage > 0 && HasRune()) {
		int slot = GetBuffSlotFromType(SE_Rune);

		while(slot >= 0) {
			int16 melee_rune_left = this->buffs[slot].melee_rune;
	
			if(melee_rune_left >= damage) {
				melee_rune_left -= damage;
				damage = -6;
				this->buffs[slot].melee_rune = melee_rune_left;
				break;
	}
			else {
				if(melee_rune_left > 0) {
					damage -= melee_rune_left;
					melee_rune_left = 0;
				}
				LogFile->write(EQEMuLog::Debug, "Fading rune from slot %d", slot);
            BuffFadeBySlot(slot);
				slot = GetBuffSlotFromType(SE_Rune);
				if(slot < 0)
					SetHasRune(false);
			}
		}
	}

		return(damage);
}
	
sint32 Mob::ReduceMagicalDamage(sint32 damage) {
	if(damage > 0 && HasSpellRune()) {
		int slot = GetBuffSlotFromType(SE_AbsorbMagicAtt);
	
		while(slot >= 0) {
			int16 magic_rune_left = this->buffs[slot].magic_rune;

			if(magic_rune_left >= damage) {
				magic_rune_left -= damage;
		damage = -6;
				this->buffs[slot].magic_rune = magic_rune_left;
				break;
	}
			else {
				if(magic_rune_left > 0) {
					damage -= magic_rune_left;
					magic_rune_left = 0;
				}
				LogFile->write(EQEMuLog::Debug, "Fading spell rune from slot %d", slot);
            BuffFadeBySlot(slot);
				slot = GetBuffSlotFromType(SE_AbsorbMagicAtt);
				if(slot < 0)
					SetHasSpellRune(false);
			}
	}
	}

	return(damage);
}

bool Mob::HasProcs() const
{
    for (int i = 0; i < MAX_PROCS; i++)
        if (PermaProcs[i].spellID != SPELL_UNKNOWN || SpellProcs[i].spellID != SPELL_UNKNOWN)
            return true;
    return false;
}

bool Client::CheckDoubleAttack(bool AAadd, bool Triple) {
	int skill = 0;
	if (Triple)
	{
		if(!HasSkill(DOUBLE_ATTACK))
			return(false);
		
		if (GetClass() == MONK || GetClass() == WARRIOR || GetClass() == RANGER || GetClass() == BERSERKER)
		{
			skill = GetSkill(DOUBLE_ATTACK)/2;
		} else {
			return(false);
		}
	}
	else
	{
		
		//should these stack with skill, or does that ever even happen?
		int aaskill = GetAA(aaBestialFrenzy)*25 + GetAA(aaHarmoniousAttack)*25
			+ GetAA(aaKnightsAdvantage)*25 + GetAA(aaFerocity)*25;
			
		if (!aaskill && !HasSkill(DOUBLE_ATTACK))
		{
			return false;
		}
		skill = GetSkill(DOUBLE_ATTACK) + aaskill;
		
		//discipline effects
		skill += (spellbonuses.DoubleAttackChance + itembonuses.DoubleAttackChance) * 3;
		
		if(skill < 300)	//only gain if we arnt garunteed
			CheckIncreaseSkill(DOUBLE_ATTACK);
	}
	if(MakeRandomInt(0, 299) < skill)
	{
		return true;
	}
	return false;
}

#ifdef EQBOTS

bool Mob::CheckBotDoubleAttack(bool Triple) {
	int skill = 0;
	if(Triple)
	{
		if(!GetSkill(DOUBLE_ATTACK))
			return(false);
		
		if((GetClass() == MONK) || (GetClass() == WARRIOR) || (GetClass() == RANGER) || (GetClass() == BERSERKER))
		{
			skill = GetSkill(DOUBLE_ATTACK)/2;
		} else {
			return(false);
		}
	}
	else
	{
		int aaskill = 0;
		uint8 aalevel = GetLevel();
		if((GetClass() == BEASTLORD)||(GetClass() == BARD)) { // AA's Beastial Frenzy, Harmonious Attacks
			if(aalevel >= 61) {
				aaskill += 25;
			}
			if(aalevel >= 62) {
				aaskill += 50;
			}
			if(aalevel >= 63) {
				aaskill += 75;
			}
			if(aalevel >= 64) {
				aaskill += 100;
			}
			if(aalevel >= 65) {
				aaskill += 125;
			}
		}
		if((GetClass() == PALADIN)||(GetClass() == SHADOWKNIGHT)) { // AA Knights Advantage
			if(aalevel >= 61) {
				aaskill += 25;
			}
			if(aalevel >= 63) {
				aaskill += 50;
			}
			if(aalevel >= 65) {
				aaskill += 75;
			}
		}
		if((GetClass() == ROGUE)||(GetClass() == WARRIOR)||(GetClass() == RANGER)||(GetClass() == MONK)) { // AA Ferocity and Relentless Assault
			if(aalevel >= 61) {
				aaskill += 25;
			}
			if(aalevel >= 63) {
				aaskill += 50;
			}
			if(aalevel >= 65) {
				aaskill += 75;
			}
			if(aalevel >= 70) {
				aaskill += 150;
			}
		}

		skill = GetSkill(DOUBLE_ATTACK) + aaskill;
		
		//discipline effects
		skill += (spellbonuses.DoubleAttackChance + itembonuses.DoubleAttackChance) * 3;
	}
	if(MakeRandomInt(0, 299) < skill)
	{
		return true;
	}
	return false;
}

#endif //EQBOTS

void Mob::CommonDamage(Mob* attacker, sint32 &damage, const int16 spell_id, const SkillType skill_used, bool &avoidable, const sint8 buffslot, const bool iBuffTic) {
	
	mlog(COMBAT__HITS, "Applying damage %d done by %s with skill %d and spell %d, avoidable? %s, is %sa buff tic in slot %d",
		damage, attacker?attacker->GetName():"NOBODY", skill_used, spell_id, avoidable?"yes":"no", iBuffTic?"":"not ", buffslot);
	
	if (GetInvul() || DivineAura()) {
		mlog(COMBAT__DAMAGE, "Avoiding %d damage due to invulnerability.", damage);
		damage = -5;
	}
	
	if( spell_id != SPELL_UNKNOWN || attacker == NULL )
		avoidable = false;
	
    // only apply DS if physical damage (no spell damage)
    // damage shield calls this function with spell_id set, so its unavoidable
	if (attacker && damage > 0 && spell_id == SPELL_UNKNOWN && skill_used != ARCHERY && skill_used != THROWING) {
		this->DamageShield(attacker);
		for(int bs = 0; bs < BUFF_COUNT; bs++){
			if(buffs[bs].numhits > 0 && !IsDiscipline(buffs[bs].spellid)){
				if(buffs[bs].numhits == 1){
					BuffFadeBySlot(bs, true);
				}
				else{
					buffs[bs].numhits--;
				}
			}
		}		
	}
	
	if(attacker){
		if(attacker->IsClient()){
			if(!attacker->CastToClient()->GetFeigned())
				AddToHateList(attacker, 0, damage, true, false, iBuffTic);
		}
		else
			AddToHateList(attacker, 0, damage, true, false, iBuffTic);
	}
    
	if(damage > 0) {
		//if there is some damage being done and theres an attacker involved
		if(attacker) {
			if(spell_id == SPELL_HARM_TOUCH2 && attacker->IsClient() && attacker->CastToClient()->CheckAAEffect(aaEffectLeechTouch)){
				int healed = damage;
				healed = attacker->GetActSpellHealing(spell_id, healed);
				attacker->HealDamage(healed);
				entity_list.MessageClose(this, true, 300, MT_Emote, "%s beams a smile at %s", attacker->GetCleanName(), this->GetCleanName() );
				attacker->CastToClient()->DisableAAEffect(aaEffectLeechTouch);
			}

			// if spell is lifetap add hp to the caster
			if (spell_id != SPELL_UNKNOWN && IsLifetapSpell( spell_id )) {
				int healed = damage;
				healed = attacker->GetActSpellHealing(spell_id, healed);				
				mlog(COMBAT__DAMAGE, "Applying lifetap heal of %d to %s", healed, attacker->GetName());
				attacker->HealDamage(healed);
				
				//we used to do a message to the client, but its gone now.
				// emote goes with every one ... even npcs
				entity_list.MessageClose(this, true, 300, MT_Emote, "%s beams a smile at %s", attacker->GetCleanName(), this->GetCleanName() );
			}
			
			// if we got a pet, thats not already fighting something send it into battle
			Mob *pet = GetPet();
			if (pet && !pet->IsFamiliar() && !pet->SpecAttacks[IMMUNE_AGGRO] && !pet->IsEngaged() && attacker != this) 
			{
				mlog(PETS__AGGRO, "Sending pet %s into battle due to attack.", pet->GetName());
				pet->AddToHateList(attacker, 1);
				pet->SetTarget(attacker);
				Message_StringID(10, PET_ATTACKING, pet->GetCleanName(), attacker->GetCleanName());
			}			
		}	//end `if there is some damage being done and theres anattacker person involved`
	
		//see if any runes want to reduce this damage
		if(spell_id == SPELL_UNKNOWN) {
			damage = ReduceDamage(damage);
			mlog(COMBAT__HITS, "Melee Damage reduced to %d", damage);
		} else {
			sint32 origdmg = damage;
			damage = ReduceMagicalDamage(damage);
			mlog(COMBAT__HITS, "Melee Damage reduced to %d", damage);
			if (origdmg != damage && attacker && attacker->IsClient()) {
				if(attacker->CastToClient()->GetFilter(FILTER_DAMAGESHIELD) != FilterHide)
					attacker->Message(15, "The Spellshield absorbed %d of %d points of damage", origdmg - damage, origdmg);
			}
		}
		

	if(IsClient() && CastToClient()->sneaking){
		CastToClient()->sneaking = false;
		SendAppearancePacket(AT_Sneak, 0);
	}
	if(attacker && attacker->IsClient() && attacker->CastToClient()->sneaking){
		attacker->CastToClient()->sneaking = false;
		attacker->SendAppearancePacket(AT_Sneak, 0);
	}
		//final damage has been determined.
		
		/*
		//check for death conditions
		if(IsClient()) {
			if((GetHP()) <= -10) {
				Death(attacker, damage, spell_id, skill_used);
				return;
			}
		} else {
			if (damage >= GetHP()) {
				//killed...
				SetHP(-100);
				Death(attacker, damage, spell_id, skill_used);
				return;
			}
		}
		*/
		
		SetHP(GetHP() - damage);
		
		if(HasDied()) {
			bool IsSaved = false;

			if(HasDeathSaveChance()) {
				if(TryDeathSave()) {
					IsSaved = true;
				}
			}
			
			if(!IsSaved) {
				SetHP(-100);
				Death(attacker, damage, spell_id, skill_used);
				return;
			}
		}
		
    	//fade mez if we are mezzed
		if (IsMezzed()) {
			mlog(COMBAT__HITS, "Breaking mez due to attack.");
			BuffFadeByEffect(SE_Mez);
		}
    	
    	//check stun chances if bashing
		if ((skill_used == BASH || skill_used == KICK && (attacker && attacker->GetLevel() >= 55)) && GetLevel() < 56) {
			int stun_resist = itembonuses.StunResist+spellbonuses.StunResist;
			if(this->GetBaseRace() == OGRE && this->IsClient() && !attacker->BehindMob(this, attacker->GetX(), attacker->GetY())) {
				mlog(COMBAT__HITS, "Stun Resisted. Ogres are immune to frontal melee stuns.");
			} else {
				if(stun_resist <= 0 || MakeRandomInt(0,99) >= stun_resist) {
					mlog(COMBAT__HITS, "Stunned. We had %d percent resist chance.");
					Stun(0);
				} else {
					mlog(COMBAT__HITS, "Stun Resisted. We had %dpercent resist chance.");
				}
			}
		}
		
		if(spell_id != SPELL_UNKNOWN) {
			//see if root will break
			if (IsRooted()) { // neotoyko: only spells cancel root
				if(GetAA(aaEnhancedRoot))
				{
					if (MakeRandomInt(0, 99) < 10) {
						mlog(COMBAT__HITS, "Spell broke root! 10percent chance");
						BuffFadeByEffect(SE_Root, buffslot); // buff slot is passed through so a root w/ dam doesnt cancel itself
					} else {
						mlog(COMBAT__HITS, "Spell did not break root. 10 percent chance");
					}
				}
				else
				{
					if (MakeRandomInt(0, 99) < 20) {
						mlog(COMBAT__HITS, "Spell broke root! 20percent chance");
						BuffFadeByEffect(SE_Root, buffslot); // buff slot is passed through so a root w/ dam doesnt cancel itself
					} else {
						mlog(COMBAT__HITS, "Spell did not break root. 20 percent chance");
					}
				}			
			}
		}
		else{
			//increment chances of interrupting
			if(IsCasting()) { //shouldnt interrupt on regular spell damage
				attacked_count++;
				mlog(COMBAT__HITS, "Melee attack while casting. Attack count %d", attacked_count);
			}
		}
		
		//send an HP update if we are hurt
		if(GetHP() < GetMaxHP())
			SendHPUpdate();
	}	//end `if damage was done`
	
    //send damage packet...
   	if(!iBuffTic) { //buff ticks do not send damage, instead they just call SendHPUpdate(), which is done below
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_Damage, sizeof(CombatDamage_Struct));
		CombatDamage_Struct* a = (CombatDamage_Struct*)outapp->pBuffer;
		a->target = GetID();
		if (attacker == NULL)
			a->source = 0;
		else if (attacker->IsClient() && attacker->CastToClient()->GMHideMe())
			a->source = 0;
		else
			a->source = attacker->GetID();
	    a->type = SkillDamageTypes[skill_used]; // was 0x1c
		a->damage = damage;
//		if (attack_skill != 231)
//			a->spellid = SPELL_UNKNOWN;
//		else
			a->spellid = spell_id;
		
		//Note: if players can become pets, they will not receive damage messages of their own
		//this was done to simplify the code here (since we can only effectively skip one mob on queue)
		eqFilterType filter;
		Mob *skip = attacker;
		if(attacker && attacker->GetOwnerID()) {
			//attacker is a pet, let pet owners see their pet's damage
			Mob* owner = attacker->GetOwner();
			if (owner && owner->IsClient()) {
				if ((spell_id != SPELL_UNKNOWN) && damage>0) {
					//special crap for spell damage, looks hackish to me
					char val1[20]={0};
					owner->Message_StringID(MT_NonMelee,OTHER_HIT_NONMELEE,GetCleanName(),ConvertArray(damage,val1));
			    } else {
			    	if(damage > 0) {
						if(spell_id != SPELL_UNKNOWN)
							filter = iBuffTic ? FilterDOT : FilterSpellDamage;
						else
							filter = FILTER_MYPETHITS;
					} else if(damage == -5)
						filter = FilterNone;	//cant filter invulnerable
					else
						filter = FILTER_MYPETMISSES;
					owner->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTED,filter);
				}
			}
			skip = owner;
		} else {
			//attacker is not a pet, send to the attacker
			
			//if the attacker is a client, try them with the correct filter
			if(attacker && attacker->IsClient()) {
				if ((spell_id != SPELL_UNKNOWN) && damage>0) {
					//special crap for spell damage, looks hackish to me
					char val1[20]={0};
					attacker->Message_StringID(MT_NonMelee,OTHER_HIT_NONMELEE,GetCleanName(),ConvertArray(damage,val1));
			    } else {
			    	if(damage > 0) {
						if(spell_id != SPELL_UNKNOWN)
							filter = iBuffTic ? FilterDOT : FilterSpellDamage;
						else
							filter = FilterNone;	//cant filter our own hits
					} else if(damage == -5)
						filter = FilterNone;	//cant filter invulnerable
					else
						filter = FILTER_MYMISSES;
					attacker->CastToClient()->QueuePacket(outapp, true, CLIENT_CONNECTED, filter);
				}
			}
			skip = attacker;
		}
		
		//send damage to all clients around except the specified skip mob (attacker or the attacker's owner) and ourself
		if(damage > 0) {
			if(spell_id != SPELL_UNKNOWN)
				filter = iBuffTic ? FilterDOT : FilterSpellDamage;
			else
				filter = FILTER_OTHERHITS;
		} else if(damage == -5)
			filter = FilterNone;	//cant filter invulnerable
		else
			filter = FILTER_OTHERMISSES;
		//make attacker (the attacker) send the packet so we can skip them and the owner
		//this call will send the packet to `this` as well (using the wrong filter) (will not happen until PC charm works)
//LogFile->write(EQEMuLog::Debug, "Queue damage to all except %s with filter %d (%d), type %d", skip->GetName(), filter, IsClient()?CastToClient()->GetFilter(filter):-1, a->type);
		entity_list.QueueCloseClients(this, outapp, true, 200, skip, true, filter);
		
		//send the damage to ourself if we are a client
		if(IsClient()) {
			//I dont think any filters apply to damage affecting us
			CastToClient()->QueuePacket(outapp);
		}
		
		safe_delete(outapp);
	} else {
        //else, it is a buff tic...
		// Everhood - So we can see our dot dmg like live shows it.
		if(spell_id != SPELL_UNKNOWN && damage > 0 && attacker && attacker != this && attacker->IsClient()) {
			//might filter on (attack_skill>200 && attack_skill<250), but I dont think we need it
			if(attacker->CastToClient()->GetFilter(FilterDOT) != FilterHide) {
				attacker->Message_StringID(MT_DoTDamage, OTHER_HIT_DOT, GetCleanName(),itoa(damage),spells[spell_id].name);
			}
		}
	} //end packet sending
}


void Mob::HealDamage(uint32 amount, Mob* caster) {
	uint32 maxhp = GetMaxHP();
	uint32 curhp = GetHP();
	uint32 acthealed = 0;
	if(amount > (maxhp - curhp))
		acthealed = (maxhp - curhp);
	else
		acthealed = amount;
		
	if(acthealed > 100){
		if(caster){
			Message(MT_NonMelee, "You have been healed by %s for %d points of damage.", caster->GetCleanName(), acthealed);
			if(caster != this){
				caster->Message(MT_NonMelee, "You have healed %s for %d points of damage.", GetCleanName(), acthealed);
			}
		}
		else{
			Message(MT_NonMelee, "You have been healed for %d points of damage.", acthealed);
		}	
	}		
		
	if (curhp < maxhp) {
		if ((curhp+amount)>maxhp)
			curhp=maxhp;
		else
			curhp+=amount;
		SetHP(curhp);

		SendHPUpdate();
	}
}

//proc chance includes proc bonus
float Mob::GetProcChances(float &ProcBonus, float &ProcChance) {
	int mydex = GetDEX();
	int AABonus = 0;
	ProcBonus = 0;
	if(IsClient()) {
		//increases based off 1 guys observed results.
		switch(CastToClient()->GetAA(aaWeaponAffinity)) {
			case 1:
				AABonus = 5;
				break;
			case 2:
				AABonus = 10;
				break;
			case 3:
				AABonus = 15;
				break;
			case 4:
				AABonus = 20;
				break;
			case 5:
				AABonus = 25;
				break;
		}
	}

#ifdef EQBOTS

	// Bot AA WeaponAffinity
	else if(IsBot()) {
		if(GetLevel() >= 55) {
			AABonus += 5;
		}
		if(GetLevel() >= 56) {
			AABonus += 5;
		}
		if(GetLevel() >= 57) {
			AABonus += 5;
		}
		if(GetLevel() >= 58) {
			AABonus += 5;
		}
		if(GetLevel() >= 59) {
			AABonus += 5;
		}
	}

#endif //EQBOTS


	ProcBonus += float(itembonuses.ProcChance + spellbonuses.ProcChance) / 1000.0f;
	
	ProcChance = 0.05f + float(mydex) / 9000.0f;
	ProcBonus += (ProcChance * AABonus) / 100;
	ProcChance += ProcBonus;
	mlog(COMBAT__PROCS, "Proc chance %.2f (%.2f from bonuses)", ProcChance, ProcBonus);
	return ProcChance;
}


void Mob::TryWeaponProc(const ItemInst* weapon_g, Mob *on) {
	if(!on) {
		SetTarget(NULL);
		LogFile->write(EQEMuLog::Error, "A null Mob object was passed  to Mob::TryWeaponProc for evaluation!");
		return;
	}

	if(!weapon_g) {
		TryWeaponProc((const Item_Struct*) NULL, on);
		return;
	}

	if(!weapon_g->IsType(ItemClassCommon)) {
		TryWeaponProc((const Item_Struct*) NULL, on);
		return;
	}
	
	//do main procs
	TryWeaponProc(weapon_g->GetItem(), on);
	
	//we have to calculate these again, oh well
	int ourlevel = GetLevel();
	float ProcChance, ProcBonus;
	GetProcChances(ProcBonus, ProcChance);
	
	//do augment procs
	int r;
	for(r = 0; r < MAX_AUGMENT_SLOTS; r++) {
		const ItemInst* aug_i = weapon_g->GetAugment(r);
		if(!aug_i)
			continue;
		const Item_Struct* aug = aug_i->GetItem();
		if(!aug)
			continue;
		
		if (IsValidSpell(aug->Proc.Effect) 
			&& (aug->Proc.Type == ET_CombatProc)) {
				ProcChance = ProcChance*(100+aug->ProcRate)/100;
			if (MakeRandomFloat(0, 1) < ProcChance) {	// 255 dex = 0.084 chance of proc. No idea what this number should be really.
				if(aug->Proc.Level > ourlevel) {
					Mob * own = GetOwner();
					if(own != NULL) {
						own->Message_StringID(13,PROC_PETTOOLOW);
					} else {
						Message_StringID(13,PROC_TOOLOW);
					}
				} else {
					ExecWeaponProc(aug->Proc.Effect, on);
				}
			}
		}
	}
}

void Mob::TryWeaponProc(const Item_Struct* weapon, Mob *on) {
	
	int ourlevel = GetLevel();
	float ProcChance, ProcBonus;
	GetProcChances(ProcBonus, ProcChance);
	
	//give weapon a chance to proc first.
	if(weapon != NULL) {
		if (IsValidSpell(weapon->Proc.Effect) && (weapon->Proc.Type == ET_CombatProc)) {
			float WPC = ProcChance*(100+weapon->ProcRate)/100;
			if (MakeRandomFloat(0, 1) <= WPC) {	// 255 dex = 0.084 chance of proc. No idea what this number should be really.
				if(weapon->Proc.Level > ourlevel) {
					mlog(COMBAT__PROCS, "Tried to proc (%s), but our level (%d) is lower than required (%d)", weapon->Name, ourlevel, weapon->Proc.Level);
					Mob * own = GetOwner();
					if(own != NULL) {
						own->Message_StringID(13,PROC_PETTOOLOW);
					} else {
						Message_StringID(13,PROC_TOOLOW);
					}
				} else {
					mlog(COMBAT__PROCS, "Attacking weapon (%s) successfully procing spell %d (%.2f percent chance)", weapon->Name, weapon->Proc.Effect, ProcChance*100);
					ExecWeaponProc(weapon->Proc.Effect, on);
				}
			} else {
				mlog(COMBAT__PROCS, "Attacking weapon (%s) did no proc (%.2f percent chance).", weapon->Name, ProcChance*100);
			}
		}
	}
	
	if(ProcBonus == -1) {
		LogFile->write(EQEMuLog::Error, "ProcBonus was -1 value!");
		return;
	}

	//now try our proc arrays
	float procmod =  float(GetDEX()) / 100.0f + ProcBonus*100.0;	//did somebody think about this???

	uint32 i;
	for(i = 0; i < MAX_PROCS; i++) {
		if (PermaProcs[i].spellID != SPELL_UNKNOWN) {
			if(MakeRandomInt(0, 100) < PermaProcs[i].chance) {
				mlog(COMBAT__PROCS, "Permanent proc %d procing spell %d (%d percent chance)", i, PermaProcs[i].spellID, PermaProcs[i].chance);
				ExecWeaponProc(PermaProcs[i].spellID, on);
			} else {
				mlog(COMBAT__PROCS, "Permanent proc %d failed to proc %d (%d percent chance)", i, PermaProcs[i].spellID, PermaProcs[i].chance);
			}
		}
		if (SpellProcs[i].spellID != SPELL_UNKNOWN) {
			int chance = ProcChance + SpellProcs[i].chance;
			if(MakeRandomInt(0, 100) < chance) {
				mlog(COMBAT__PROCS, "Spell proc %d procing spell %d (%d percent chance)", i, SpellProcs[i].spellID, chance);
				ExecWeaponProc(SpellProcs[i].spellID, on);
			} else {
				mlog(COMBAT__PROCS, "Spell proc %d failed to proc %d (%d percent chance)", i, SpellProcs[i].spellID, chance);
			}
		}
	}
}

void Mob::TryCriticalHit(Mob *defender, int16 skill, sint32 &damage)
{
	if(damage < 1) //We can't critical hit if we don't hit.
		return;
 
	float critChance = RuleR(Combat, BaseCritChance);
	if(IsClient())
		critChance += RuleR(Combat, ClientBaseCritChance);	

#ifdef EQBOTS

	else if(IsBot()) {
		critChance += RuleR(Combat, ClientBaseCritChance);	
	}

#endif //EQBOTS

	uint16 critMod = 200; 
	if((GetClass() == WARRIOR || GetClass() == BERSERKER) && GetLevel() >= 12 && IsClient()) 
	{
		if(CastToClient()->berserk)
		{
			critChance += RuleR(Combat, BerserkBaseCritChance);
			critMod = 400;
		}
		else
		{
			critChance += RuleR(Combat, WarBerBaseCritChance);
		}
	}
 
	if(skill == ARCHERY && GetClass() == RANGER && GetSkill(ARCHERY) >= 65){
		critChance += 0.06f;
	}

#ifdef EQBOTS

	if(IsBot() && (GetHPRatio() < 30) && (GetClass() == BERSERKER)) {
		critChance += RuleR(Combat, BerserkBaseCritChance);
		critMod = 400;
	}
	else if(IsBot() && (GetHPRatio() < 30) && (GetClass() == WARRIOR)) {
		critChance += RuleR(Combat, WarBerBaseCritChance);
	}

	// Bot AA's for CombatFury and FuryoftheAges
	if(IsBot()) {
		if(GetLevel() >= 64) {
			critChance += 0.12f;
		}
		else if(GetLevel() >= 63) {
			critChance += 0.10f;
		}
		else if(GetLevel() >= 62) {
			critChance += 0.08f;
		}
		else if(GetLevel() >= 57) {
			critChance += 0.07f;
		}
		else if(GetLevel() >= 56) {
			critChance += 0.04f;
		}
		else if(GetLevel() >= 55) {
			critChance += 0.02f;
		}
	}
  
#endif //EQBOTS

	switch(GetAA(aaCombatFury))
	{
	case 1:
		critChance += 0.02f;
		break;
	case 2:
		critChance += 0.04f;
		break;
	case 3:
		critChance += 0.07f;
		break;
	default:
		break;
	}

	switch(GetAA(aaFuryoftheAges))
	{
	case 1:
		critChance += 0.01f;
		break;
	case 2:
		critChance += 0.03f;
		break;
	case 3:
		critChance += 0.05f;
		break;
	default:
		break;
	}

	float CritBonus = spellbonuses.CriticalHitChance + itembonuses.CriticalHitChance;
	if(CritBonus > 0.0 && critChance < 0.01) //If we have a bonus to crit in items or spells but no actual chance to crit
		critChance = 0.01f; //Give them a small one so skills and items appear to have some effect.
 
	critChance += ((critChance) * (CritBonus) / 100.0f); //crit chance is a % increase to your reg chance
	
	if(defender && defender->GetBodyType() == BT_Undead || defender->GetBodyType() == BT_SummonedUndead || defender->GetBodyType() == BT_Vampire){
		switch(GetAA(aaSlayUndead)){
			case 1:
				critMod += 33;
				break;
			case 2:
				critMod += 66;
				break;
			case 3:
				critMod += 100;
				break;
		}
	}
 
#ifdef EQBOTS

	// Paladin Bot Slay Undead AA
	if(IsBot()) {
		if(GetClass() == PALADIN) {
			if(defender && defender->GetBodyType() == BT_Undead || defender->GetBodyType() == BT_SummonedUndead || defender->GetBodyType() == BT_Vampire) {
				if(GetLevel() >= 61) {
					critMod += 100;
				}
				else if(GetLevel() >= 60) {
					critMod += 66;
				}
				else if(GetLevel() >= 59) {
					critMod += 33;
				}
			}
		}
	}

#endif //EQBOTS

	if(critChance > 0){
		if(MakeRandomFloat(0, 1) <= critChance)
		{
			//Veteran's Wrath AA
			//first, find out of we have it (don't multiply by 0 :-\ )
			int32 AAdmgmod = GetAA(aaVeteransWrath);
			if (AAdmgmod > 0) {
				//now, make sure it's not a special attack
				if (skill == _1H_BLUNT
					|| skill == _2H_BLUNT
					|| skill == _1H_SLASHING
					|| skill == _2H_SLASHING
					|| skill == PIERCING
					|| skill == HAND_TO_HAND
					/*|| skill == ARCHERY*/ //AndMetal: not sure if we should be giving this to rangers
					)
					critMod += AAdmgmod * 3; //AndMetal: guessing
			}

			damage = (damage * critMod) / 100;
			if(IsClient() && CastToClient()->berserk)
			{
				entity_list.MessageClose(this, false, 200, MT_CritMelee, "%s lands a crippling blow!(%d)", GetCleanName(), damage);
			}

#ifdef EQBOTS

			//EQoffline
			else if(IsBot() && ((GetClass() == WARRIOR) || (GetClass() == BERSERKER)) && (GetHPRatio() < 30)) {
				entity_list.MessageClose(this, false, 200, MT_CritMelee, "%s lands a crippling blow!(%d)", GetCleanName(), damage);
			}
			else if(IsBot()) {
				entity_list.MessageClose(this, false, 200, MT_CritMelee, "%s scores a critical hit!(%d)", GetCleanName(), damage);
			}

#endif //EQBOTS

            else
            {
                entity_list.MessageClose(this, false, 200, MT_CritMelee, "%s scores a critical hit!(%d)", GetCleanName(), damage);
            }
		}
	}
}

bool Mob::TryFinishingBlow(Mob *defender, SkillType skillinuse)
{
	int8 aa_item = GetAA(aaFinishingBlow) + GetAA(aaCoupdeGrace) + GetAA(aaDeathblow);

#ifdef EQBOTS

	if(IsBot()) {
		if(GetLevel() >= 55) {
			aa_item += 1;	// Finishing Blow AA 1
		}
		if(GetLevel() >= 56) {
			aa_item += 1;	// Finishing Blow AA 2
		}
		if(GetLevel() >= 57) {
			aa_item += 1;	// Finishing Blow AA 3
		}
		if(GetLevel() >= 62) {
			aa_item += 1;	// Coup de Grace AA 1
		}
		if(GetLevel() >= 63) {
			aa_item += 1;	// Coup de Grace AA 2
		}
		if(GetLevel() >= 64) {
			aa_item += 1;	// Coup de Grace AA 3
		}
	}

#endif //EQBOTS

	if(aa_item && !defender->IsClient() && defender->GetHPRatio() < 10){
		int chance = 0;
		int levelreq = 0;
		switch(aa_item)
		{
		case 1:
			chance = 2;
			levelreq = 50;
			break;
		case 2:
			chance = 5;
			levelreq = 52;
			break;
		case 3:
			chance = 7;
			levelreq = 54;
			break;
		case 4:
			chance = 7;
			levelreq = 55;
			break;
		case 5:
			chance = 7;
			levelreq = 57;
			break;
		case 6:
			chance = 7;
			levelreq = 59;
			break;
            case 7:
			chance = 7;
			levelreq = 61;
			break;
		case 8:
			chance = 7;
			levelreq = 63;
			break;
		case 9:
			chance = 7;
			levelreq = 65;
			break;
		default:
			break;
		}

		if(chance >= MakeRandomInt(0, 100) && defender->GetLevel() <= levelreq){
			mlog(COMBAT__ATTACKS, "Landed a finishing blow: AA at %d, other level %d", aa_item, defender->GetLevel());
			entity_list.MessageClose_StringID(this, false, 200, MT_CritMelee, FINISHING_BLOW, GetName());
			defender->Damage(this, 32000, SPELL_UNKNOWN, skillinuse);
			return true;
		}
		else
		{
			mlog(COMBAT__ATTACKS, "FAILED a finishing blow: AA at %d, other level %d", aa_item, defender->GetLevel());
			return false;
		}
	}
	return false;
}

void Mob::DoRiposte(Mob *defender){
		mlog(COMBAT__ATTACKS, "Preforming a riposte");

#ifdef EQBOTS

		if(defender->IsBot())
			defender->BotAttackMelee(this, 13, true);
		else

#endif //EQBOTS

	    defender->Attack(this, 13, true);
	    
		//double riposte
		int DoubleRipChance = 0;
		switch(defender->GetAA(aaDoubleRiposte)) {
		case 1: 
			DoubleRipChance = 15;
			break;
		case 2:
			DoubleRipChance = 35;
			break;
		case 3:
			DoubleRipChance = 50;
			break;
		}

		DoubleRipChance += 10*GetAA(aaFlashofSteel);

#ifdef EQBOTS

		// Bot AA for DoubleRiposte and FlashofSteel
		if(defender->IsBot()) {
			if(defender->GetLevel() >= 64) {
				DoubleRipChance = 80;
			}
			else if(defender->GetLevel() >= 63) {
				DoubleRipChance = 70;
			}
			else if(defender->GetLevel() >= 62) {
				DoubleRipChance = 60;
			}
			else if(defender->GetLevel() >= 61) {
				DoubleRipChance = 50;
			}
			else if(defender->GetLevel() >= 60) {
				DoubleRipChance = 35;
			}
			else if(defender->GetLevel() >= 59) {
				DoubleRipChance = 15;
			}
		}

#endif //EQBOTS

		if(DoubleRipChance >= MakeRandomInt(0, 100)) {
			mlog(COMBAT__ATTACKS, "Preforming a double riposed (%d percent chance)", DoubleRipChance);

#ifdef EQBOTS

		if(defender->IsBot())
			defender->BotAttackMelee(this, 13, true);
		else

#endif //EQBOTS

			defender->Attack(this, 13, true);
		}

		if(defender->GetAA(aaReturnKick)){
			int ReturnKickChance = 0;
			switch(defender->GetAA(aaReturnKick)){
			case 1:
				ReturnKickChance = 25;
				break;
			case 2:
				ReturnKickChance = 35;
				break;
			case 3:
				ReturnKickChance = 50;
				break;
			}

#ifdef EQBOTS

			// Bot AA ReturnKick
			if(defender->IsBot() && (defender->GetClass() == MONK)) {
				if(defender->GetLevel() >= 61) {
					ReturnKickChance = 50;
				}
				else if(defender->GetLevel() >= 60) {
					ReturnKickChance = 35;
				}
				else if(defender->GetLevel() >= 59) {
					ReturnKickChance = 25;
				}
			}

#endif //EQBOTS

			if(ReturnKickChance >= MakeRandomInt(0, 100)) {
				mlog(COMBAT__ATTACKS, "Preforming a return kick (%d percent chance)", ReturnKickChance);
				defender->MonkSpecialAttack(this, FLYING_KICK);

#ifdef EQBOTS

				if(defender->IsBot()) { // Technique Of Master Wu AA
					int special = 0;
					if(defender->GetLevel() >= 65) {
						special = 100;
					}
					else if(defender->GetLevel() >= 64) {
						special = 80;
					}
					else if(defender->GetLevel() >= 63) {
						special = 60;
					}
					else if(defender->GetLevel() >= 62) {
						special = 40;
					}
					else if(defender->GetLevel() >= 61) {
						special = 20;
					}
					if(special == 100 || special > MakeRandomInt(0,100)) {
						defender->MonkSpecialAttack(this, FLYING_KICK);
						if(20 > MakeRandomInt(0,100)) {
							defender->MonkSpecialAttack(this, FLYING_KICK);
						}
					}
				}

#endif //EQBOTS

			}
		}		
}
 
void Mob::ApplyMeleeDamageBonus(int16 skill, sint32 &damage){
	if(damage < 1)
		return;

	if(!RuleB(Combat, UseIntervalAC)){
		if(IsNPC()){ //across the board NPC damage bonuses.
 			//only account for STR here, assume their base STR was factored into their DB damages
			int dmgbonusmod = 0;
			dmgbonusmod += (100*(itembonuses.STR + spellbonuses.STR))/3;
			dmgbonusmod += (100*(spellbonuses.ATK + itembonuses.ATK))/5;
			mlog(COMBAT__DAMAGE, "Damage bonus: %d percent from ATK and STR bonuses.", (dmgbonusmod/100));
			damage += (damage*dmgbonusmod/10000);
		}
	}
  
	if(spellbonuses.DamageModifierSkill == skill || spellbonuses.DamageModifierSkill == 255){
		damage += ((damage * spellbonuses.DamageModifier)/100);
	}
 
	if(itembonuses.DamageModifierSkill == skill || itembonuses.DamageModifierSkill == 255){
		damage += ((damage * itembonuses.DamageModifier)/100);
	}

	//Rogue sneak attack disciplines make use of this, they are active for one hit
	for(int bs = 0; bs < BUFF_COUNT; bs++){
		if(buffs[bs].numhits > 0 && IsDiscipline(buffs[bs].spellid)){
			if(skill == spells[buffs[bs].spellid].skill){
				if(buffs[bs].numhits == 1){
					BuffFadeBySlot(bs, true);
				}
				else{
					buffs[bs].numhits--;
				}
			}
		}	
	}	
}

bool Mob::HasDied() {
	bool Result = false;

	/*
	if(IsClient()) {
		if((GetHP()) <= -10)
			Result = true;
	}
	else {
		if(GetHP() <= 0)
			Result = true;
	}
	*/

	if(GetHP() <= 0)
		Result = true;

	return Result;
}
