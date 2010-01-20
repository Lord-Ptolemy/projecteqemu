/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2004  EQEMu Development Team (http://eqemu.org)

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
#include "spdat.h"
#include "spells.h"
#include "buff.h"
#include "masterentity.h"
#include "../common/packet_dump.h"
#include "../common/moremath.h"
#include "../common/Item.h"
#include "worldserver.h"
#include "../common/skills.h"
#include "../common/bodytypes.h"
#include "../common/classes.h"
#include "../common/rulesys.h"
#include <math.h>
#include <assert.h>
#ifndef WIN32
#include <stdlib.h>
#include "../common/unix.h"
#endif

#include "StringIDs.h"


extern Zone* zone;
extern volatile bool ZoneLoaded;
#if !defined(NEW_LoadSPDat) && !defined(DB_LoadSPDat)
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern WorldServer worldserver;

typedef void (Mob::*SpellEffectProc)(const Spell *spell_to_cast, const uint32 effect_id_index, const float partial, ItemInst **summoned_item, Buff *buff_in_use);
SpellEffectProc SpellEffectDispatch[SE_LAST_EFFECT + 1];

void MapSpellEffects()
{
	memset(SpellEffectDispatch, 0, sizeof(SpellEffectDispatch));
	SpellEffectDispatch[SE_CurrentHP] = &Mob::Handle_SE_CurrentHP;

}

// the spell can still fail here, if the buff can't stack
// in this case false will be returned, true otherwise
//TODO: FIXME
bool Mob::SpellEffect(Mob* caster, Spell *spell_to_cast, float partial)
{
	_ZP(Mob_SpellEffect);
	ItemInst *summoned_item = NULL;
	Buff *new_buff = NULL;
	uint32 caster_level = 0;

	//calculate caster level
	if(caster && caster->IsClient() && GetCastedSpellInvSlot() > 0)
	{
		const ItemInst* inst = caster->CastToClient()->GetInv().GetItem(GetCastedSpellInvSlot());
		if(inst)
		{
			if(inst->GetItem()->Click.Level > 0)
			{
				caster_level = inst->GetItem()->Click.Level;
			}
			else
			{
				caster_level = caster ? caster->GetCasterLevel() : GetCasterLevel();
			}
		}
		else
			caster_level = caster ? caster->GetCasterLevel() : GetCasterLevel();
	}
	else
		caster_level = caster ? caster->GetCasterLevel() : GetCasterLevel();

	//Add the buff and junk
	sint32 buff_duration = CalcBuffDuration(caster, this, spell_to_cast, caster_level) - 1;
	if(buff_duration > 0)
	{
		//new_buff = AddBuff(caster, spell_to_cast, buff_duration, caster_level);
		if(!new_buff)
		{
			mlog(SPELLS__EFFECT_VALUES, "Unable to apply buff for spell %s(%d) via Mob::AddBuff()", spell_to_cast->GetSpell().name, spell_to_cast->GetSpellID());
			return false;
		}
	}

	//loop through all effects...
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		SpellEffectProc proc = SpellEffectDispatch[spell_to_cast->GetSpell().effectid[i]];
		if(proc == NULL) 
		{
			mlog(SPELLS__EFFECT_VALUES, "Unhandled spell effect: %s(%d) effect id %d at index %d", spell_to_cast->GetSpell().name, 
				spell_to_cast->GetSpellID(), spell_to_cast->GetSpell().effectid[i], i);
			Message(13, "Unhandled spell effect: %s(%d) effect id %d at index %d", spell_to_cast->GetSpell().name, 
				spell_to_cast->GetSpellID(), spell_to_cast->GetSpell().effectid[i], i);
			continue;
		}

		(this->*proc)(spell_to_cast, i, partial, &summoned_item, new_buff);
	}

	if (summoned_item) 
	{
		if(IsClient())
		{
			Client *c = CastToClient();
			c->PushItemOnCursor(*summoned_item);
			c->SendItemPacket(SLOT_CURSOR, summoned_item, ItemPacketSummonItem);
		}
		safe_delete(summoned_item);
	}

	return true;
}

void Mob::Handle_SE_CurrentHP(const Spell *spell_to_cast, const uint32 effect_id_index, const float partial, ItemInst **summoned_item, Buff *buff_in_use)
{
}

int Mob::CalcSpellEffectValue(Spell *spell_to_cast, int effect_id, int caster_level, Mob *caster, int ticsremaining)
{
	int formula, base, max, effect_value;

	if(effect_id < 0 || effect_id >= EFFECT_COUNT)
		return 0;

	formula = spell_to_cast->GetSpell().formula[effect_id];
	base = spell_to_cast->GetSpell().base[effect_id];
	max = spell_to_cast->GetSpell().max[effect_id];

	if(spell_to_cast->IsBlankSpellEffect(effect_id))
		return 0;

	effect_value = CalcSpellEffectValue_formula(formula, base, max, caster_level, spell_to_cast, ticsremaining);

	if(caster && spell_to_cast->IsBardSong() &&
	(spell_to_cast->GetSpell().effectid[effect_id] != SE_AttackSpeed) &&
	(spell_to_cast->GetSpell().effectid[effect_id] != SE_AttackSpeed2) &&
	(spell_to_cast->GetSpell().effectid[effect_id] != SE_AttackSpeed3)) {
		int oval = effect_value;
		int mod = caster->GetInstrumentMod(spell_to_cast);
		effect_value = effect_value * mod / 10;
		mlog(SPELLS__BARDS, "Effect value %d altered with bard modifier of %d to yeild %d", oval, mod, effect_value);
	}

	return(effect_value);
}

// solar: generic formula calculations
int Mob::CalcSpellEffectValue_formula(int formula, int base, int max, int caster_level, Spell* spell_to_cast, int ticsremaining)
{
/*
neotokyo: i need those formulas checked!!!!

0 = base
1 - 99 = base + level * formulaID
100 = base
101 = base + level / 2
102 = base + level
103 = base + level * 2
104 = base + level * 3
105 = base + level * 4
106 ? base + level * 5
107 ? min + level / 2
108 = min + level / 3
109 = min + level / 4
110 = min + level / 5
119 ? min + level / 8
121 ? min + level / 4
122 = splurt
123 ?
203 = stacking issues ? max
205 = stacking issues ? 105


  0x77 = min + level / 8
*/

	int result = 0, updownsign = 1, ubase = base;
	if(ubase < 0)
		ubase = 0 - ubase;

	// solar: this updown thing might look messed up but if you look at the
	// spells it actually looks like some have a positive base and max where
	// the max is actually less than the base, hence they grow downward
/*
This seems to mainly catch spells where both base and max are negative.
Strangely, damage spells  have a negative base and positive max, but
snare has both of them negative, yet their range should work the same:
(meaning they both start at a negative value and the value gets lower)
*/
	if (max < base && max != 0)
	{
		// values are calculated down
		updownsign = -1;
	}
	else
	{
		// values are calculated up
		updownsign = 1;
	}

	mlog(SPELLS__EFFECT_VALUES, "CSEV: spell %d, formula %d, base %d, max %d, lvl %d. Up/Down %d",
		spell_to_cast->GetSpellID(), formula, base, max, caster_level, updownsign);

	switch(formula)
	{
		case 60:	//used in stun spells..?
		case 70:
			result = ubase/100; break;
		case   0:
		case 100:	// solar: confirmed 2/6/04
			result = ubase; break;
		case 101:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level / 2)); break;
		case 102:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + caster_level); break;
		case 103:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level * 2)); break;
		case 104:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level * 3)); break;
		case 105:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level * 4)); break;

		case 107:
			//Used on Reckless Strength, I think it should decay over time
			result = updownsign * (ubase + (caster_level / 2)); break;
		case 108:
			result = updownsign * (ubase + (caster_level / 3)); break;
		case 109:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level / 4)); break;

		case 110:	// solar: confirmed 2/6/04
			//is there a reason we dont use updownsign here???
			result = ubase + (caster_level / 5); break;

		case 111:
            result = updownsign * (ubase + 6 * (caster_level - spell_to_cast->GetMinLevel())); break;
		case 112:
            result = updownsign * (ubase + 8 * (caster_level - spell_to_cast->GetMinLevel())); break;
		case 113:
            result = updownsign * (ubase + 10 * (caster_level - spell_to_cast->GetMinLevel())); break;
		case 114:
            result = updownsign * (ubase + 15 * (caster_level - spell_to_cast->GetMinLevel())); break;

        //these formula were updated according to lucy 10/16/04
		case 115:	// solar: this is only in symbol of transal
			result = ubase + 6 * (caster_level - spell_to_cast->GetMinLevel()); break;
		case 116:	// solar: this is only in symbol of ryltan
            result = ubase + 8 * (caster_level - spell_to_cast->GetMinLevel()); break;
		case 117:	// solar: this is only in symbol of pinzarn
            result = ubase + 12 * (caster_level - spell_to_cast->GetMinLevel()); break;
		case 118:	// solar: used in naltron and a few others
            result = ubase + 20 * (caster_level - spell_to_cast->GetMinLevel()); break;

		case 119:	// solar: confirmed 2/6/04
			result = ubase + (caster_level / 8); break;
		case 121:	// solar: corrected 2/6/04
			result = ubase + (caster_level / 3); break;
		case 122: {
			int ticdif = spell_to_cast->GetSpell().buffduration - (ticsremaining-1);
			if(ticdif < 0)
				ticdif = 0;
			result = -(11 + 11*ticdif);
			break;
		}
		case 123:	// solar: added 2/6/04
			result = MakeRandomInt(ubase, abs(max));
			break;

		//these are used in stacking effects... formula unknown
		case 201:
		case 203:
			result = max;
			break;
		default:
			if (formula < 100)
				result = ubase + (caster_level * formula);
			else
				LogFile->write(EQEMuLog::Debug, "Unknown spell effect value forumula %d", formula);
	}

	int oresult = result;

	// now check result against the allowed maximum
	if (max != 0)
	{
		if (updownsign == 1)
		{
			if (result > max)
				result = max;
		}
		else
		{
			if (result < max)
				result = max;
		}
	}

	// if base is less than zero, then the result need to be negative too
	if (base < 0 && result > 0)
		result *= -1;

	mlog(SPELLS__EFFECT_VALUES, "Result: %d (orig %d), cap %d %s", result, oresult, max, (base < 0 && result > 0)?"Inverted due to negative base":"");

	return result;
}


void Mob::BuffProcess() 
{
}

void Mob::DoBuffTic(int16 spell_id, int32 ticsremaining, int8 caster_level, Mob* caster) {
	_ZP(Mob_DoBuffTic);

	int effect, effect_value;

	if(!IsValidSpell(spell_id))
		return;

	const SPDat_Spell_Struct &spell = spells[spell_id];

	if (spell_id == SPELL_UNKNOWN)
		return;

	for (int i=0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(spell_id, i))
			continue;

		effect = spell.effectid[i];
		//I copied the calculation into each case which needed it instead of
		//doing it every time up here, since most buff effects dont need it

		switch(effect)
		{
		case SE_CurrentHP:
		{
			effect_value = 0;//CalcSpellEffectValue(spell_id, i, caster_level, caster, ticsremaining);

			//TODO: account for AAs and stuff
			//TODO: FIXME

			//dont know what the signon this should be... - makes sense
			if (caster && caster->IsClient() &&
				IsDetrimentalSpell(spell_id) &&
				effect_value < 0) {
				sint32 modifier = 100;
				//modifier += caster->CastToClient()->GetFocusEffect(focusImprovedDamage, spell_id);

				if(caster){
					if(caster->IsClient() && !caster->CastToClient()->GetFeigned()){
						AddToHateList(caster, -effect_value);
					}
					else if(!caster->IsClient())
					{
						if(!IsClient())
							AddToHateList(caster, -effect_value);
					}

					TryDotCritical(spell_id, caster, effect_value);
				}
				effect_value = effect_value * modifier / 100;
			}

			if(effect_value < 0) {
				effect_value = -effect_value;
				Damage(caster, effect_value, spell_id, spell.skill, false, i, true);
			} else if(effect_value > 0) {
				//healing spell...
				//healing aggro would go here; removed for now
				//if(caster)
					//effect_value = caster->GetActSpellHealing(spell_id, effect_value);
				HealDamage(effect_value, caster);
			}

			break;
		}
		case SE_HealOverTime:
		{
			effect_value = 0;//CalcSpellEffectValue(spell_id, i, caster_level);

			//is this affected by stuff like GetActSpellHealing??
			HealDamage(effect_value, caster);
			//healing aggro would go here; removed for now
			break;
		}

		case SE_CurrentMana:
		{
			effect_value = 0;//CalcSpellEffectValue(spell_id, i, caster_level);

			SetMana(GetMana() + effect_value);
			break;
		}

		case SE_CurrentEndurance: {
			if(IsClient()) {
				effect_value = 0;//CalcSpellEffectValue(spell_id, i, caster_level);

				CastToClient()->SetEndurance(CastToClient()->GetEndurance() + effect_value);
			}
			break;
		}

		case SE_BardAEDot:
		{
			effect_value = 0;//CalcSpellEffectValue(spell_id, i, caster_level);

			if (invulnerable || /*effect_value > 0 ||*/ DivineAura())
				break;

			if(effect_value < 0) {
				effect_value = -effect_value;
				if(caster){
					if(caster->IsClient() && !caster->CastToClient()->GetFeigned()){
						AddToHateList(caster, effect_value);
					}
					else if(!caster->IsClient())
						AddToHateList(caster, effect_value);
				}
				Damage(caster, effect_value, spell_id, spell.skill, false, i, true);
			} else if(effect_value > 0) {
				//healing spell...
				HealDamage(effect_value, caster);
				//healing aggro would go here; removed for now
			}
			break;
		}

		case SE_Hate2:{
			effect_value = 0;//CalcSpellEffectValue(spell_id, i, caster_level);
			if(caster){
				if(effect_value > 0){
					if(caster){
						if(caster->IsClient() && !caster->CastToClient()->GetFeigned()){
							AddToHateList(caster, effect_value);
						}
						else if(!caster->IsClient())
							AddToHateList(caster, effect_value);
					}
				}else{
					sint32 newhate = GetHateAmount(caster) + effect_value;
					if (newhate < 1) {
						SetHate(caster,1);
					} else {
						SetHate(caster,newhate);
					}
				}
			}
			break;
		}

		case SE_Charm: {
			/*if (!caster || !PassCharismaCheck(caster, this, spell_id)) {
				BuffFadeByEffect(SE_Charm);
			}*/

			break;
		}

		case SE_Root: {
			/*float SpellEffectiveness = ResistSpell(spells[spell_id].resisttype, spell_id, caster);
			if(SpellEffectiveness < 25) {
				BuffFadeByEffect(SE_Root);
			}*/

			break;
		}

		case SE_Hunger: {
			// this procedure gets called 7 times for every once that the stamina update occurs so we add 1/7 of the subtraction.  
			// It's far from perfect, but works without any unnecessary buff checks to bog down the server.
			if(IsClient()) {
				CastToClient()->m_pp.hunger_level += 5;
				CastToClient()->m_pp.thirst_level += 5;
			}
			break;
		}
		case SE_Invisibility:
		case SE_InvisVsAnimals:
		case SE_InvisVsUndead:
			if(ticsremaining > 3)
			{
				if(!IsBardSong(spell_id))
				{
					double break_chance = 2.0;
					if(caster)
					{
						break_chance -= (2 * (((double)caster->GetSkill(DIVINATION) + ((double)caster->GetLevel() * 3.0)) / 650.0));
					}
					else
					{
						break_chance -= (2 * (((double)GetSkill(DIVINATION) + ((double)GetLevel() * 3.0)) / 650.0));
					}

					if(MakeRandomFloat(0.0, 100.0) < break_chance)
					{
						BuffModifyDurationBySpellID(spell_id, 3);
					}
				}
			}
		
		case SE_Invisibility2:
		case SE_InvisVsUndead2:
			if(ticsremaining <= 3 && ticsremaining > 1)
			{
				Message_StringID(MT_Spells, INVIS_BEGIN_BREAK);
			}
			break;
		default: {
			// do we need to do anyting here?
		}
		}
	}
}

// solar: removes the buff in the buff slot 'slot'
void Mob::BuffFadeBySlot(int slot, bool iRecalcBonuses)
{
	//TODO:
	/*
	if(slot < 0 || slot > BUFF_COUNT)
		return;

	if(!IsValidSpell(buffs[slot].spellid))
		return;

	if (IsClient() && !CastToClient()->IsDead())
		CastToClient()->MakeBuffFadePacket(buffs[slot].spellid, slot);

	mlog(SPELLS__BUFFS, "Fading buff %d from slot %d", buffs[slot].spellid, slot);

	for (int i=0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(buffs[slot].spellid, i))
			continue;

		switch (spells[buffs[slot].spellid].effectid[i])
		{
			case SE_WeaponProc:
			{
				uint16 procid = GetProcID(buffs[slot].spellid, i);
				RemoveProcFromWeapon(procid, false);
				break;
			}

			case SE_DefensiveProc:
			{
				uint16 procid = GetProcID(buffs[slot].spellid, i);
				RemoveDefensiveProc(procid);
				break;
			}

			case SE_RangedProc:
			{
				uint16 procid = GetProcID(buffs[slot].spellid, i);
				RemoveRangedProc(procid);
				break;
			}

			case SE_SummonHorse:
			{
				if(IsClient())
				{
					CastToClient()->SetHorseId(0);
				}
				break;
			}

			case SE_IllusionCopy:
			case SE_Illusion:
			{
				SendIllusionPacket(0, GetBaseGender());
				if(GetRace() == OGRE){
					SendAppearancePacket(AT_Size, 9);
				}
				else if(GetRace() == TROLL){
					SendAppearancePacket(AT_Size, 8);
				}
				else if(GetRace() == VAHSHIR || GetRace() == FROGLOK || GetRace() == BARBARIAN){
					SendAppearancePacket(AT_Size, 7);
				}
				else if(GetRace() == HALF_ELF || GetRace() == WOOD_ELF || GetRace() == DARK_ELF){
					SendAppearancePacket(AT_Size, 5);
				}
				else if(GetRace() == DWARF){
					SendAppearancePacket(AT_Size, 4);
				}
				else if(GetRace() == HALFLING || GetRace() == GNOME){
					SendAppearancePacket(AT_Size, 3);
				}
				else{
					SendAppearancePacket(AT_Size, 6);
				}
				for(int x = 0; x < 7; x++){
					SendWearChange(x);
				}
				break;
			}

			case SE_Levitate:
			{
				if (!AffectedExcludingSlot(slot, SE_Levitate))
					SendAppearancePacket(AT_Levitate, 0);
				break;
			}

			case SE_Invisibility2:
			case SE_Invisibility:
			{
				SetInvisible(false);
				break;
			}

			case SE_InvisVsUndead2:
			case SE_InvisVsUndead:
			{
				invisible_undead = false;	// Mongrel: No longer IVU
				break;
			}

			case SE_InvisVsAnimals:
			{
				invisible_animals = false;
				break;
			}

			case SE_Silence:
			{
				Silence(false);
			}


			case SE_DivineAura:
			{
				SetInvul(false);
				break;
			}

			case SE_Rune:
			{
				buffs[slot].melee_rune = 0;
				break;
			}

			case SE_AbsorbMagicAtt:
			{
				buffs[slot].magic_rune = 0;
				break;
			}

			case SE_Familiar:
			{
				Mob *mypet = GetPet();
				if (mypet){
					if(mypet->IsNPC())
						mypet->CastToNPC()->Depop();
					SetPetID(0);
				}
				break;
			}

			case SE_Mez:
			{
				SendAppearancePacket(AT_Anim, ANIM_STAND);	// unfreeze
				this->mezzed = false;
				break;
			}

			case SE_Charm:
			{
				Mob* tempmob = GetOwner();
				SetOwnerID(0);
				if(tempmob)
				{
					tempmob->SetPet(0);
				}
				if (IsAIControlled())
				{
					// clear the hate list of the mobs
					entity_list.ReplaceWithTarget(this, tempmob);
					WipeHateList();
					if(tempmob)
						AddToHateList(tempmob, 1, 0);
					SendAppearancePacket(AT_Anim, ANIM_STAND);
				}
				if(tempmob && tempmob->IsClient())
				{
					EQApplicationPacket *app = new EQApplicationPacket(OP_Charm, sizeof(Charm_Struct));
					Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
					ps->owner_id = tempmob->GetID();
					ps->pet_id = this->GetID();
					ps->command = 0;
					entity_list.QueueClients(this, app);
					safe_delete(app);
				}
				if(IsClient())
				{
					InterruptSpell();
					if (this->CastToClient()->IsLD())
						AI_Start(CLIENT_LD_TIMEOUT);
					else
					{
						bool feared = FindType(SE_Fear);
						if(!feared)
							AI_Stop();
					}
				}
				break;
			}

			case SE_Root:
			{
				rooted = false;
				break;
			}

			case SE_Fear:
			{
				if(RuleB(Combat, EnableFearPathing)){
					if(IsClient())
					{
						bool charmed = FindType(SE_Charm);
						if(!charmed)
							AI_Stop();
					}

					if(curfp) {
						curfp = false;
						break;
					}
				}
				else
				{
					UnStun();
				}
				break;
			}

			case SE_BindSight:
			{
				if(IsClient())
				{
					CastToClient()->SetBindSightTarget(NULL);
				}
				break;			
			}

			case SE_MovementSpeed:
			{
				if(IsClient())
				{
					Client *my_c = CastToClient();
					int32 cur_time = Timer::GetCurrentTime();
					if((cur_time - my_c->m_TimeSinceLastPositionCheck) > 1000)
					{
						float speed = (my_c->m_DistanceSinceLastPositionCheck * 100) / (float)(cur_time - my_c->m_TimeSinceLastPositionCheck);
						float runs = my_c->GetRunspeed();
						if(speed > (runs * RuleR(Zone, MQWarpDetectionDistanceFactor)))
						{
							if(!my_c->GetGMSpeed() && (runs >= my_c->GetBaseRunspeed() || (speed > (my_c->GetBaseRunspeed() * RuleR(Zone, MQWarpDetectionDistanceFactor)))))
							{
								printf("%s %i moving too fast! moved: %.2f in %ims, speed %.2f\n", __FILE__, __LINE__,
									my_c->m_DistanceSinceLastPositionCheck, (cur_time - my_c->m_TimeSinceLastPositionCheck), speed);
								if(my_c->IsShadowStepExempted())
								{
									if(my_c->m_DistanceSinceLastPositionCheck > 800)
									{
										my_c->CheatDetected(MQWarpShadowStep, my_c->GetX(), my_c->GetY(), my_c->GetZ());
									}
								}
								else if(my_c->IsKnockBackExempted())
								{
									//still potential to trigger this if you're knocked back off a 
									//HUGE fall that takes > 2.5 seconds
									if(speed > 30.0f)
									{
										my_c->CheatDetected(MQWarpKnockBack, my_c->GetX(), my_c->GetY(), my_c->GetZ());
									}
								}
								else if(!my_c->IsPortExempted())
								{
									if(!my_c->IsMQExemptedArea(zone->GetZoneID(), my_c->GetX(), my_c->GetY(), my_c->GetZ()))
									{
										if(speed > (runs * 2 * RuleR(Zone, MQWarpDetectionDistanceFactor)))
										{
											my_c->m_TimeSinceLastPositionCheck = cur_time;
											my_c->m_DistanceSinceLastPositionCheck = 0.0f;
											my_c->CheatDetected(MQWarp, my_c->GetX(), my_c->GetY(), my_c->GetZ());
											//my_c->Death(my_c, 10000000, SPELL_UNKNOWN, _1H_BLUNT);
										}
										else
										{
											my_c->CheatDetected(MQWarpLight, my_c->GetX(), my_c->GetY(), my_c->GetZ());
										}
									}
								}
							}
						}
					}
					my_c->m_TimeSinceLastPositionCheck = cur_time;
					my_c->m_DistanceSinceLastPositionCheck = 0.0f;
				}
			}
		}
	}

	// notify caster (or their master) of buff that it's worn off
	Mob *p = entity_list.GetMob(buffs[slot].casterid);
	if (p && p != this && !IsBardSong(buffs[slot].spellid))
	{
		Mob *notify = p;
		if(p->IsPet())
			notify = p->GetOwner();
		if(p) {
			notify->Message_StringID(MT_Broadcasts, SPELL_WORN_OFF_OF,
				spells[buffs[slot].spellid].name, GetCleanName());
		}
	}

	buffs[slot].spellid = SPELL_UNKNOWN;
	if(IsPet() && GetOwner() && GetOwner()->IsClient()) {
		SendPetBuffsToClient();
	}

	if (iRecalcBonuses)
		CalcBonuses();
		*/
}

//given an item/spell's focus ID and the spell being cast, determine the focus ammount, if any
//assumes that spell_id is not a bard spell and that both ids are valid spell ids
sint16 Client::CalcFocusEffect(focusType type, int16 focus_id, Spell *spell_to_cast) {

	const SPDat_Spell_Struct &focus_spell = spells[focus_id];
	const SPDat_Spell_Struct &spell = spell_to_cast->GetSpell();

	sint16 value = 0;
	int lvlModifier = 100;

	for (int i = 0; i < EFFECT_COUNT; i++) {
		switch (focus_spell.effectid[i]) {
		case SE_Blank:
			break;

		//check limits

		//missing limits:
		//SE_LimitTarget

		case SE_LimitResist:{
			if(focus_spell.base[i]){
				if(spell.resisttype != focus_spell.base[i])
					return(0);
			}
			break;
		}
		case SE_LimitInstant:{
			if(spell.buffduration)
				return(0);
			break;
		}

		case SE_LimitMaxLevel:{
			int lvldiff = (spell.classes[(GetClass()%16) - 1]) - focus_spell.base[i];

			if(lvldiff > 0){ //every level over cap reduces the effect by spell.base2[i] percent
				lvlModifier -= spell.base2[i]*lvldiff;
				if(lvlModifier < 1)
					return 0;
			}
			break;
		}

		case SE_LimitMinLevel:
			if (spell.classes[(GetClass()%16) - 1] < focus_spell.base[i])
				return(0);
			break;

		case SE_LimitCastTime:
			if (spell.cast_time < (uint16)focus_spell.base[i])
				return(0);
			break;

		case SE_LimitSpell:
			if(focus_spell.base[i] < 0) {	//exclude spell
				if (spell_to_cast->GetSpellID() == (focus_spell.base[i]*-1))
					return(0);
			} else {
				//this makes the assumption that only one spell can be explicitly included...
				if (spell_to_cast->GetSpellID()!= focus_spell.base[i])
					return(0);
			}
			break;

		case SE_LimitMinDur:
				if (focus_spell.base[i] > CalcBuffDuration_formula(GetLevel(), spell.buffdurationformula, spell.buffduration))
					return(0);
			break;

		case SE_LimitEffect:
			if(focus_spell.base[i] < 0){
				if(spell_to_cast->IsEffectInSpell(focus_spell.base[i]))
				{ 
					//we limit this effect, can't have
					return 0;
				}
			}
			else{
				if(focus_spell.base[i] == SE_SummonPet) //summoning haste special case
				{	//must have one of the three pet effects to qualify
					if(!spell_to_cast->IsEffectInSpell(SE_SummonPet) &&
						!spell_to_cast->IsEffectInSpell(SE_NecPet) &&
						!spell_to_cast->IsEffectInSpell(SE_SummonBSTPet))
					{
						return 0;
					}
				}
				else if(!spell_to_cast->IsEffectInSpell(focus_spell.base[i]))
				{ 
					//we limit this effect, must have
					return 0;
				}
			}
			break;


		case SE_LimitSpellType:
			switch( focus_spell.base[i] )
			{
				case 0:
					if (!spell_to_cast->IsDetrimentalSpell())
						return 0;
					break;
				case 1:
					if (!spell_to_cast->IsBeneficialSpell())
						return 0;
					break;
				default:
					LogFile->write(EQEMuLog::Normal, "CalcFocusEffect:  unknown limit spelltype %d", focus_spell.base[i]);
			}
			break;



		//handle effects

		case SE_ImprovedDamage:
			switch (focus_spell.max[i])
			{
				case 0:
					if (type == focusImprovedDamage && focus_spell.base[i] > value)
					{
						value = focus_spell.base[i];
					}
					break;
				case 1:
					if (type == focusImprovedCritical && focus_spell.base[i] > value)
					{
						value = focus_spell.base[i];
					}
					break;
				case 2:
					if (type == focusImprovedUndeadDamage && focus_spell.base[i] > value)
					{
						value = focus_spell.base[i];
					}
					break;
				case 3:
					if (type == 10 && focus_spell.base[i] > value)
					{
						value = focus_spell.base[i];
					}
					break;
				default: //Resist stuff
					if (type == (focusType)focus_spell.max[i] && focus_spell.base[i] > value)
					{
						value = focus_spell.base[i];
					}
					break;
			}
			break;
		case SE_ImprovedHeal:
			if (type == focusImprovedHeal && focus_spell.base[i] > value)
			{
				value = focus_spell.base[i];
			}
			break;
		case SE_IncreaseSpellHaste:
			if (type == focusSpellHaste && focus_spell.base[i] > value)
			{
				value = focus_spell.base[i];
			}
			break;
		case SE_IncreaseSpellDuration:
			if (type == focusSpellDuration && focus_spell.base[i] > value)
			{
				value = focus_spell.base[i];
			}
			break;
		case SE_IncreaseRange:
			if (type == focusRange && focus_spell.base[i] > value)
			{
				value = focus_spell.base[i];
			}
			break;
		case SE_ReduceReagentCost:
			if (type == focusReagentCost && focus_spell.base[i] > value)
			{
				value = focus_spell.base[i];
			}
			break;
		case SE_ReduceManaCost:
			if (type == focusManaCost)
			{
				if(focus_spell.base[i] > value)
				{
					value = focus_spell.base[i];
				}
			}
			break;
		case SE_PetPowerIncrease:
			if (type == focusPetPower && focus_spell.base[i] > value)
			{
				value = focus_spell.base[i];
			}
			break;
		case SE_SpellResistReduction:
			if (type == focusResistRate && focus_spell.base[i] > value)
			{
				value = focus_spell.base[i];
			}
			break;
		case SE_SpellHateMod:
			if (type == focusSpellHateMod)
			{
				if(value != 0)
				{
					if(value > 0)
					{
						if(focus_spell.base[i] > value)
						{
							value = focus_spell.base[i];
						}
					}
					else
					{
						if(focus_spell.base[i] < value)
						{
							value = focus_spell.base[i];
						}
					}
				}
				else
					value = focus_spell.base[i];
			}
			break;
#if EQDEBUG >= 6
		//this spits up a lot of garbage when calculating spell focuses
		//since they have all kinds of extra effects on them.
		default:
			LogFile->write(EQEMuLog::Normal, "CalcFocusEffect:  unknown effectid %d", focus_spell.effectid[i]);
#endif
		}
	}
	return(value*lvlModifier/100);
}

sint16 Client::GetFocusEffect(focusType type, Spell *spell_to_cast) 
{
	if(spell_to_cast->IsBardSong())
		return 0;
	const Item_Struct* TempItem = 0;
	const Item_Struct* UsedItem = 0;
	sint16 Total = 0;
	sint16 realTotal = 0;

	//item focus
	for(int x=0; x<=21; x++)
	{
		TempItem = NULL;
		ItemInst* ins = GetInv().GetItem(x);
		if (!ins)
			continue;
		TempItem = ins->GetItem();
		if (TempItem && TempItem->Focus.Effect > 0 && TempItem->Focus.Effect != SPELL_UNKNOWN) {
			Total = CalcFocusEffect(type, TempItem->Focus.Effect, spell_to_cast);

			if (Total > 0 && realTotal >= 0 && Total > realTotal) {
				realTotal = Total;
				UsedItem = TempItem;
			} else if (Total < 0 && Total < realTotal) {
				realTotal = Total;
				UsedItem = TempItem;
			}
		}
		for(int y = 0; y < MAX_AUGMENT_SLOTS; ++y)
		{
			ItemInst *aug = NULL;
			aug = ins->GetAugment(y);
			if(aug)
			{
				const Item_Struct* TempItemAug = aug->GetItem();
				if (TempItemAug && TempItemAug->Focus.Effect > 0 && TempItemAug->Focus.Effect != SPELL_UNKNOWN) {
					Total = CalcFocusEffect(type, TempItemAug->Focus.Effect, spell_to_cast);
					if (Total > 0 && realTotal >= 0 && Total > realTotal) {
						realTotal = Total;
						UsedItem = TempItem;
					} else if (Total < 0 && Total < realTotal) {
						realTotal = Total;
						UsedItem = TempItem;
					}
				}
			}
		}
	}

	if (realTotal != 0 && UsedItem && spell_to_cast->GetSpell().buffduration == 0) {
		Message_StringID(MT_Spells, BEGINS_TO_GLOW, UsedItem->Name);
	}

	//Spell Focus
	sint16 Total2 = 0;
	sint16 realTotal2 = 0;

	//TODO:
	/*
	for (int y = 0; y < BUFF_COUNT; y++) {
		int16 focusspellid = buffs[y].spellid;
		if (focusspellid == 0 || focusspellid >= SPDAT_RECORDS)
			continue;

		Total2 = CalcFocusEffect(type, focusspellid, spell_id);
		if (Total2 > 0 && realTotal2 >= 0 && Total2 > realTotal2) {
				realTotal2 = Total2;
			} else if (Total2 < 0 && Total2 < realTotal2) {
				realTotal2 = Total2;
			}
	}*/

	if(type == focusReagentCost && spell_to_cast->IsSummonPetSpell() && GetAA(aaElementalPact))
		return 100;

	if(type == focusReagentCost && (spell_to_cast->IsEffectInSpell(SE_SummonItem) || spell_to_cast->IsSacrificeSpell())){
		return 0;
	//Summon Spells that require reagents are typically imbue type spells, enchant metal, sacrifice and shouldn't be affected
	//by reagent conservation for obvious reasons.
	}

	return realTotal + realTotal2;
}

//for some stupid reason SK procs return theirs one base off...
uint16 Mob::GetProcID(uint16 spell_id, uint8 effect_index) {
	bool sk = false;
	bool other = false;
	for(int x = 0; x < 16; x++)
	{
		if(x == 4){
			if(spells[spell_id].classes[4] < 255)
				sk = true;
		}
		else{
			if(spells[spell_id].classes[x] < 255)
				other = true;
		}
	}
	
	if(sk && !other)
	{
		return(spells[spell_id].base[effect_index] + 1);
	}
	else{
		return(spells[spell_id].base[effect_index]);
	}
}

bool Mob::TryDeathSave() {
	bool Result = false;

	int aaClientTOTD = IsClient() ? CastToClient()->GetAA(aaTouchoftheDivine) : -1;

	if (aaClientTOTD > 0) {
		int aaChance = (1.2 * CastToClient()->GetAA(aaTouchoftheDivine));
		
		if (MakeRandomInt(0,100) < aaChance) {
			Result = true;
			/*
			int touchHealSpellID = 4544;
			switch (aaClientTOTD) {
				case 1:
					touchHealSpellID = 4544;
					break;
				case 2:
					touchHealSpellID = 4545;
					break;
				case 3:
					touchHealSpellID = 4546;
					break;
				case 4:
					touchHealSpellID = 4547;
					break;
				case 5:
					touchHealSpellID = 4548;
					break;
			} */

			// The above spell effect is not currently working. So, do a manual heal instead:

			float touchHealAmount = 0;
			switch (aaClientTOTD) {
				case 1:
					touchHealAmount = 0.15;
					break;
				case 2:
					touchHealAmount = 0.3;
					break;
				case 3:
					touchHealAmount = 0.6;
					break;
				case 4:
					touchHealAmount = 0.8;
					break;
				case 5:
					touchHealAmount = 1.0;
					break;
			}

			this->Message(0, "Divine power heals your wounds.");
			SetHP(this->max_hp * touchHealAmount);

			// and "Touch of the Divine", an Invulnerability/HoT/Purify effect, only one for all 5 levels
			Spell *tod_spell = new Spell(4789, this, this);
			SpellOnTarget(tod_spell, this);
			safe_delete(tod_spell);

			// skip checking for DI fire if this goes off...
			if (Result == true) {
				return Result;
			}
		}
	}

	int buffSlot = GetBuffSlotFromType(SE_DeathSave);

	if(buffSlot >= 0) {

		//TODO:
		int8 SuccessChance = 0;//buffs[buffSlot].deathSaveSuccessChance;
		int8 CasterUnfailingDivinityAARank = 0;//buffs[buffSlot].casterAARank;
		int16 BuffSpellID = 0;//buffs[buffSlot].spellid;
		int SaveRoll = MakeRandomInt(0, 100);

		LogFile->write(EQEMuLog::Debug, "%s chance for a death save was %i and the roll was %i", GetCleanName(), SuccessChance, SaveRoll);

		if(SuccessChance >= SaveRoll) {
			// Success
			Result = true;

			if(IsFullDeathSaveSpell(BuffSpellID)) {
				// Lazy man's full heal.
				SetHP(50000);
			}
			else {
				SetHP(300);
			}

			entity_list.MessageClose_StringID(this, false, 200, MT_CritMelee, DIVINE_INTERVENTION, GetCleanName());

			// Fade the buff
			BuffFadeBySlot(buffSlot);

			SetDeathSaveChance(false);
		}
		else if (CasterUnfailingDivinityAARank >= 1) {
			// Roll the virtual dice to see if the target atleast gets a heal out of this
			SuccessChance = 30;
			SaveRoll = MakeRandomInt(0, 100);

			LogFile->write(EQEMuLog::Debug, "%s chance for a Unfailing Divinity AA proc was %i and the roll was %i", GetCleanName(), SuccessChance, SaveRoll);

			if(SuccessChance >= SaveRoll) {
				// Yep, target gets a modest heal
				SetHP(1500);
			}
		}
	}

	return Result;
}

void Mob::TryDotCritical(int16 spell_id, Mob *caster, int &damage)
{
	if(!caster)
		return;

	float critChance = 0.00f;

	switch(caster->GetAA(aaCriticalAffliction))
	{
		case 1:
			critChance += 0.03f;
			break;
		case 2:
			critChance += 0.06f;
			break;
		case 3:
			critChance += 0.10f;
			break;
		default:
			break;
	}

	switch (caster->GetAA(aaImprovedCriticalAffliction))
	{
		case 1:
			critChance += 0.03f;
			break;
		case 2:
			critChance += 0.06f;
			break;
		case 3:
			critChance += 0.10f;
			break;
		default:
			break;
	}

	// since DOTs are the Necromancer forte, give an innate bonus
	// however, no chance to crit unless they've trained atleast one level in the AA first
	if (caster->GetClass() == NECROMANCER && critChance > 0.0f){
		critChance += 0.05f;
	}

	if (critChance > 0.0f){
		if (MakeRandomFloat(0, 1) <= critChance)
		{
			damage *= 2;
		}
	}
}

bool Mob::AffectedExcludingSlot(int slot, int effect)
{
	for (int i = 0; i <= EFFECT_COUNT; i++)
	{
		if (i == slot)
			continue;

		//TODO:if (IsEffectInSpell(buffs[i].spellid, effect))
			//return true;
	}
	return false;
}

