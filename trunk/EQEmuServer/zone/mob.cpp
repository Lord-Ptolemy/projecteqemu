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
#include "features.h"
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include "masterentity.h"
#include "pathing.h"
#include "zone.h"
#include "spdat.h"
#include "../common/skills.h"
#include "map.h"
#include "StringIDs.h"
#include "../common/rulesys.h"

#include "../common/emu_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "zonedb.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "../common/bodytypes.h"
#include "../common/guilds.h"
#include "../common/MiscFunctions.h"
#include <stdio.h>
extern EntityList entity_list;
#if !defined(NEW_LoadSPDat) && !defined(DB_LoadSPDat)
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;

extern Zone* zone;

Mob::Mob(const char*   in_name,
         const char*   in_lastname,
         sint32  in_cur_hp,
         sint32  in_max_hp,
         int8    in_gender,
         uint16    in_race,
         int8    in_class,
         bodyType in_bodytype,
         int8    in_deity,
         int8    in_level,
         int32	 in_npctype_id,
		 float	in_size,
		 float	in_runspeed,
         float	in_heading,
         float	in_x_pos,
         float	in_y_pos,
         float	in_z_pos,

         int8    in_light,
		 int8	 in_texture,
		 int8	 in_helmtexture,
		 int16	 in_ac,
		 int16	 in_atk,
		 int16	 in_str,
		 int16	 in_sta,
		 int16	 in_dex,
		 int16	 in_agi,
		 int16	 in_int,
		 int16	 in_wis,
		 int16	 in_cha,
		 int8	in_haircolor,
		 int8	in_beardcolor,
		 int8	in_eyecolor1, // the eyecolors always seem to be the same, maybe left and right eye?
		 int8	in_eyecolor2,
		 int8	in_hairstyle,
		 int8	in_luclinface,
		 int8	in_beard,
		 int32	in_drakkin_heritage,
		 int32	in_drakkin_tattoo,
		 int32	in_drakkin_details,
		 int32	in_armor_tint[MAX_MATERIALS],

		 int8	in_aa_title,
		 int8	in_see_invis,			// see through invis/ivu
		 int8   in_see_invis_undead,
		 int8   in_see_hide,
		 int8   in_see_improved_hide,
		 sint16 in_hp_regen,
		 sint16 in_mana_regen,
		 int8	in_qglobal,
		 float	in_slow_mitigation,	// Allows for mobs to mitigate how much they are slowed.
		 int8	in_maxlevel,
		 int32	in_scalerate
		 ) : 
		attack_timer(2000),
		attack_dw_timer(2000),
		ranged_timer(2000),
		tic_timer(6000),
		mana_timer(2000),
		spellend_timer(0),
		rewind_timer(30000), //Timer used for determining amount of time between actual player position updates for /rewind.
		stunned_timer(0),
		bardsong_timer(6000),
		flee_timer(FLEE_CHECK_TIMER),
		bindwound_timer(10000)
	//	mezzed_timer(0)
{
	targeted = 0;
	logpos = false;
	tar_ndx=0;
	tar_vector=0;
	tar_vx=0;
	tar_vy=0;
	tar_vz=0;
	tarx=0;
	tary=0;
	tarz=0;
	fear_walkto_x = -999999;
	fear_walkto_y = -999999;
	fear_walkto_z = -999999;
	curfp = false;

	AI_Init();
	SetMoving(false);
	moved=false;
 	rewind_x = 0;		//Stored x_pos for /rewind
 	rewind_y = 0;		//Stored y_pos for /rewind
 	rewind_z = 0;		//Stored z_pos for /rewind
	move_tic_count = 0;
	
	_egnode = NULL;
	adverrorinfo = 0;
	name[0]=0;
	orig_name[0]=0;
	clean_name[0]=0;
	lastname[0]=0;
	if(in_name) {
		strncpy(name,in_name,64);
		strncpy(orig_name,in_name,64);
	}
	if(in_lastname)
		strncpy(lastname,in_lastname,64);
	cur_hp		= in_cur_hp;
	max_hp		= in_max_hp;
	base_hp		= in_max_hp;
	gender		= in_gender;
	race		= in_race;
	base_gender	= in_gender;
	base_race	= in_race;
	class_		= in_class;
    bodytype    = in_bodytype;
	deity		= in_deity;
	level		= in_level;
	npctype_id	= in_npctype_id; // rembrant, Dec. 20, 2001
	size		= in_size;
	base_size	= size;
	runspeed   = in_runspeed;

	
    // neotokyo: sanity check
    if (runspeed < 0 || runspeed > 20)
        runspeed = 1.25f;
	
	heading		= in_heading;
	x_pos		= in_x_pos;
	y_pos		= in_y_pos;
	z_pos		= in_z_pos;
	light		= in_light;
	texture		= in_texture;
	helmtexture	= in_helmtexture;
	haircolor	= in_haircolor;
	beardcolor	= in_beardcolor;
	eyecolor1	= in_eyecolor1;
	eyecolor2	= in_eyecolor2;
	hairstyle	= in_hairstyle;
	luclinface	= in_luclinface;
	beard		= in_beard;
	drakkin_heritage	= in_drakkin_heritage;
	drakkin_tattoo		= in_drakkin_tattoo;
	drakkin_details		= in_drakkin_details;
	attack_speed= 0;
	findable	= false;
	trackable	= true;

	if(in_aa_title>0)
		aa_title	= in_aa_title;
	else
		aa_title	=0xFF;
	AC		= in_ac;
	ATK		= in_atk;
	STR		= in_str;
	STA		= in_sta;
	DEX		= in_dex;
	AGI		= in_agi;
	INT		= in_int;
	WIS		= in_wis;
	CHA		= in_cha;
	MR = CR = FR = DR = PR = 0;
	
	ExtraHaste = 0;
	bEnraged = false;

	shield_target = NULL;
	cur_mana = 0;
	max_mana = 0;
	hp_regen = in_hp_regen;
	mana_regen = in_mana_regen;
	oocregen = RuleI(NPC, OOCRegen); //default Out of Combat Regen
	slow_mitigation = in_slow_mitigation;
	maxlevel = in_maxlevel;
	scalerate = in_scalerate;
	invisible = false;
	invisible_undead = false;
	invisible_animals = false;
	sneaking = false;
	hidden = false;
	improved_hidden = false;
	invulnerable = false;
	IsFullHP	= (cur_hp == max_hp);
	qglobal=0;

	InitializeBuffSlots();

    // clear the proc arrays
	int i;
	int j;
	for (j = 0; j < MAX_PROCS; j++)
    {
        PermaProcs[j].spellID = SPELL_UNKNOWN;
        PermaProcs[j].chance = 0;
        PermaProcs[j].pTimer = NULL;
        SpellProcs[j].spellID = SPELL_UNKNOWN;

		DefensiveProcs[j].spellID = SPELL_UNKNOWN;
		DefensiveProcs[j].chance = 0;
		DefensiveProcs[j].pTimer = NULL;
		RangedProcs[j].spellID = SPELL_UNKNOWN;
		RangedProcs[j].chance = 0;
		RangedProcs[j].pTimer = NULL;
    }

	for (i = 0; i < MAX_MATERIALS; i++)
	{
		if (in_armor_tint)
		{
			armor_tint[i] = in_armor_tint[i];
		}
		else
		{
			armor_tint[i] = 0;
		}
	}

	delta_heading = 0;
	delta_x = 0;
	delta_y = 0;
	delta_z = 0;
	animation = 0;

	logging_enabled = false;
	isgrouped = false;
	israidgrouped = false;
	_appearance = eaStanding;
	standstate = GetAppearanceValue(_appearance);
	pRunAnimSpeed = 0;
//	guildeqid = GUILD_NONE;
	
    spellend_timer.Disable();
	bardsong_timer.Disable();
	bardsong = 0;
	bardsong_target_id = 0;
	casting_spell_id = 0;
	casting_spell_timer = 0;
	casting_spell_timer_duration = 0;
	casting_spell_type = 0;
	target = 0;
	
	memset(&itembonuses, 0, sizeof(StatBonuses));
	memset(&spellbonuses, 0, sizeof(StatBonuses));
	//memset(&aabonuses, 0, sizeof(StatBonuses)); //don't need this until we start using Client::CalcAABonuses()
	spellbonuses.AggroRange = -1;
	spellbonuses.AssistRange = -1;
	pLastChange = 0;
	SetPetID(0);
	SetOwnerID(0);
	typeofpet = petCharmed;		//default to charmed...
	held = false;
	
	attacked_count = 0;
	mezzed = false;
	stunned = false;
	silenced = false;
	inWater = false;
    int m;
	for (m = 0; m < MAX_SHIELDERS; m++)
	{
		shielder[m].shielder_id = 0;
		shielder[m].shielder_bonus = 0;
	}
	for (i=0; i<SPECATK_MAXNUM ; i++) {
		SpecAttacks[i] = false;
		SpecAttackTimers[i] = 0;
	}
	wandertype=0;
	pausetype=0;
	cur_wp = 0;
	cur_wp_x = 0;
	cur_wp_y = 0;
	cur_wp_z = 0;
	cur_wp_pause = 0;
	patrol=0;
	follow=0;
	follow_dist = 100;	// Default Distance for Follow
	flee_mode = false;
	fear_walkto_x = -999999;
	fear_walkto_y = -999999;
	fear_walkto_z = -999999;
	curfp = false;	
	flee_timer.Start();

	permarooted = (runspeed > 0) ? false : true;

	movetimercompleted = false;
	roamer = false;
	rooted = false;
	charmed = false;
	pStandingPetOrder = SPO_Follow;

	see_invis = in_see_invis;
	see_invis_undead = in_see_invis_undead;
	see_hide = in_see_hide;
	see_improved_hide = in_see_improved_hide;
	qglobal=in_qglobal;
	
	// Bind wound
	bindwound_timer.Disable();
	bindwound_target = 0;
	
	trade = new Trade(this);
	// hp event
	nexthpevent = -1;
	nextinchpevent = -1;
	
	fix_pathing = false;
	TempPets(false);
	SetHasRune(false);
	SetHasSpellRune(false);
	SetHasPartialMeleeRune(false);
	SetHasPartialSpellRune(false);

	m_hasDeathSaveChance = false;

	m_is_running = false;

	nimbus_effect1 = 0;
	nimbus_effect2 = 0;
	nimbus_effect3 = 0;

	flymode = FlyMode3;
	// Pathing
	PathingLOSState = UnknownLOS;
	PathingLoopCount = 0;
	PathingLastNodeVisited = -1;
	PathingLOSCheckTimer = new Timer(RuleI(Pathing, LOSCheckFrequency));
	PathingRouteUpdateTimerShort = new Timer(RuleI(Pathing, RouteUpdateFrequencyShort));
	PathingRouteUpdateTimerLong = new Timer(RuleI(Pathing, RouteUpdateFrequencyLong));
	AggroedAwayFromGrid = 0;
	PathingTraversedNodes = 0;
	hate_list.SetOwner(this);
}

Mob::~Mob()
{
	// Our Entity ID is set to 0 in NPC::Death. This leads to mobs hanging around for a while in 
	// the entity list, even after they have been destroyed. Use our memory pointer to remove the mob 
	// if our EntityID is 0.
	//
	if(GetID() > 0)
		entity_list.RemoveMob(GetID());
	else
		entity_list.RemoveMob(this);

	AI_Stop();
	if (GetPet()) {
		if (GetPet()->Charmed())
			GetPet()->BuffFadeByEffect(SE_Charm);
		else
			SetPet(0);
	}
	for (int i=0; i<SPECATK_MAXNUM ; i++) {
		safe_delete(SpecAttackTimers[i]);
	}
	EQApplicationPacket app;
	CreateDespawnPacket(&app, !IsCorpse());
	Corpse* corpse = entity_list.GetCorpseByID(GetID());
	if(!corpse || (corpse && !corpse->IsPlayerCorpse()))
		entity_list.QueueClients(this, &app, true);
	
	entity_list.RemoveFromTargets(this);
	
	safe_delete(trade);
	if(HadTempPets()){
		entity_list.DestroyTempPets(this);
	}
	entity_list.UnMarkNPC(GetID());
	safe_delete(PathingLOSCheckTimer);
	safe_delete(PathingRouteUpdateTimerShort);
	safe_delete(PathingRouteUpdateTimerLong);
	UninitializeBuffSlots();
}

int32 Mob::GetAppearanceValue(EmuAppearance iAppearance) {
	switch (iAppearance) {
		// 0 standing, 1 sitting, 2 ducking, 3 lieing down, 4 looting
		case eaStanding: {
			return ANIM_STAND;
		}
		case eaSitting: {
			return ANIM_SIT;
		}
		case eaCrouching: {
			return ANIM_CROUCH;
		}
		case eaDead: {
			return ANIM_DEATH;
		}
		case eaLooting: {
			return ANIM_LOOT;
		}
		//to shup up compiler:
		case _eaMaxAppearance:
			break;
	}
	return(ANIM_STAND);
}
int32 Mob::GetPRange(float x, float y, float z){
	return 0;
}
void Mob::SetInvisible(bool state)
{
	invisible = state;
	SendAppearancePacket(AT_Invis, invisible);
    // Invis and hide breaks charms

    if ((this->GetPetType() == petCharmed) && (invisible || hidden || improved_hidden))
    {
        Mob* formerpet = this->GetPet();

        if(formerpet)
             formerpet->BuffFadeByEffect(SE_Charm);
    }
   
}

//check to see if `this` is invisible to `other`
bool Mob::IsInvisible(Mob* other) const
{
	if(!other)
		return(false);
	
	//check regular invisibility
	if (invisible && !other->SeeInvisible())
		return true;
	
	//check invis vs. undead
	if (other->GetBodyType() == BT_Undead || other->GetBodyType() == BT_SummonedUndead) {
		if(invisible_undead && !other->SeeInvisibleUndead())
			return true;
	}
	
	//check invis vs. animals...
	if (other->GetBodyType() == BT_Animal){
		if(invisible_animals && !other->SeeInvisible())
			return true;
	}

	if(hidden){
		if(!other->see_hide && !other->see_improved_hide){
			return true;
		}
	}

	if(improved_hidden){
		if(!other->see_improved_hide){
			return true;
		}
	}
	
	//handle sneaking
	if(sneaking) {
		if(BehindMob(other, GetX(), GetY()) )
			return true;
	}
	
	return(false);
}

float Mob::_GetMovementSpeed(int mod) const {
      // List of movement speed modifiers, including AAs & spells:
	// http://everquest.allakhazam.com/db/item.html?item=1721;page=1;howmany=50#m10822246245352
	if (IsRooted())
		return 0.0f;
	
	float aa_mod = 0.0f;
	float speed_mod = runspeed;
	bool has_horse = false;
	if (IsClient())
	{
		if(CastToClient()->GetGMSpeed())
		{
			speed_mod = 3.125f;
		}
		else
		{
			Mob* horse = entity_list.GetMob(CastToClient()->GetHorseId());
			if(horse)
			{
				speed_mod = horse->GetBaseRunspeed();
				has_horse = true;
			}
		}

		aa_mod += ((CastToClient()->GetAA(aaInnateRunSpeed) * 0.10)
			+ (CastToClient()->GetAA(aaFleetofFoot) * 0.10)
			+ (CastToClient()->GetAA(aaSwiftJourney) * 0.10)
			);
		//Selo's Enduring Cadence should be +7% per level
	}

	int spell_mod = spellbonuses.movementspeed + itembonuses.movementspeed;
	int movemod = 0;

	if(spell_mod < 0)
	{
		movemod += spell_mod;
	}
	else if(spell_mod > (aa_mod*100))
	{
		movemod = spell_mod;
	}
	else
	{
		movemod = (aa_mod * 100);
	}
	
	if(movemod < -85) //cap it at moving very very slow
		movemod = -85;
	
	if (!has_horse && movemod != 0)
		speed_mod += (speed_mod * float(movemod) / 100.0f);

	if(mod != 0)
		speed_mod += (speed_mod * (float)mod / 100.0f);

	if(speed_mod <= 0.0f)
		return(0.0001f);

	//runspeed cap.
	if(IsClient())
	{
		if(GetClass() == BARD) {
			//this extra-high bard cap should really only apply if they have AAs
			if(speed_mod > 1.74)
				speed_mod = 1.74;
		} else {
			if(speed_mod > 1.58)
				speed_mod = 1.58;
		}
	}

	return speed_mod;
}

sint32 Mob::CalcMaxMana() {
	switch (GetCasterClass()) {
		case 'I':
			max_mana = (((GetINT()/2)+1) * GetLevel()) + spellbonuses.Mana + itembonuses.Mana;
			break;
		case 'W':
			max_mana = (((GetWIS()/2)+1) * GetLevel()) + spellbonuses.Mana + itembonuses.Mana;
			break;
		case 'N':
		default:
			max_mana = 0;
			break;
	}

	return max_mana;
}

sint32 Mob::CalcMaxHP() 
{

	max_hp = (base_hp  + itembonuses.HP + spellbonuses.HP);
	
	int slot = GetBuffSlotFromType(SE_MaxHPChange);
	if(slot >= 0)
	{
		for(int i = 0; i < EFFECT_COUNT; i++)
		{
			if (spells[buffs[slot].spellid].effectid[i] == SE_MaxHPChange)
			{
				max_hp += max_hp * spells[buffs[slot].spellid].base[i] / 10000;
			}
		}
	}
	return max_hp;
}

char Mob::GetCasterClass() const {
	switch(class_)
	{
	case CLERIC:
	case PALADIN:
	case RANGER:
	case DRUID:
	case SHAMAN:
	case BEASTLORD:
	case CLERICGM:
	case PALADINGM:
	case RANGERGM:
	case DRUIDGM:
	case SHAMANGM:
	case BEASTLORDGM:
		return 'W';
		break;

	case SHADOWKNIGHT:
	case BARD:
	case NECROMANCER:
	case WIZARD:
	case MAGICIAN:
	case ENCHANTER:
	case SHADOWKNIGHTGM:
	case BARDGM:
	case NECROMANCERGM:
	case WIZARDGM:
	case MAGICIANGM:
	case ENCHANTERGM:
		return 'I';
		break;

	default:
		return 'N';
		break;
	}
}

void Mob::CreateSpawnPacket(EQApplicationPacket* app, Mob* ForWho) {
	app->SetOpcode(OP_NewSpawn);
	app->size = sizeof(NewSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);	
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);
}

void Mob::CreateSpawnPacket(EQApplicationPacket* app, NewSpawn_Struct* ns) {
	app->SetOpcode(OP_NewSpawn);
	app->size = sizeof(NewSpawn_Struct);
	
	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)];
	
	// Copy ns directly into packet
	memcpy(app->pBuffer, ns, sizeof(NewSpawn_Struct));
	
	// Custom packet data
	NewSpawn_Struct* ns2 = (NewSpawn_Struct*)app->pBuffer;
	strcpy(ns2->spawn.name, ns->spawn.name);
	/*if (ns->spawn.class_==MERCHANT)
		strcpy(ns2->spawn.lastName, "EQEmu Shopkeeper");
	else*/ if (ns->spawn.class_==TRIBUTE_MASTER)
		strcpy(ns2->spawn.lastName, "Tribute Master");
	else if (ns->spawn.class_==ADVENTURERECRUITER)
		strcpy(ns2->spawn.lastName, "Adventure Recruiter");
	else if (ns->spawn.class_==BANKER)
		strcpy(ns2->spawn.lastName, "Banker");
	else if (ns->spawn.class_==ADVENTUREMERCHANT)
		strcpy(ns->spawn.lastName,"Adventure Merchant");
	else if (ns->spawn.class_==WARRIORGM)
		strcpy(ns2->spawn.lastName, "GM Warrior");
	else if (ns->spawn.class_==PALADINGM)
		strcpy(ns2->spawn.lastName, "GM Paladin");
	else if (ns->spawn.class_==RANGERGM)
		strcpy(ns2->spawn.lastName, "GM Ranger");
	else if (ns->spawn.class_==SHADOWKNIGHTGM)
		strcpy(ns2->spawn.lastName, "GM ShadowKnight");
	else if (ns->spawn.class_==DRUIDGM)
		strcpy(ns2->spawn.lastName, "GM Druid");
	else if (ns->spawn.class_==BARDGM)
		strcpy(ns2->spawn.lastName, "GM Bard");
	else if (ns->spawn.class_==ROGUEGM)
		strcpy(ns2->spawn.lastName, "GM Rogue");
	else if (ns->spawn.class_==SHAMANGM)
		strcpy(ns2->spawn.lastName, "GM Shaman");
	else if (ns->spawn.class_==NECROMANCERGM)
		strcpy(ns2->spawn.lastName, "GM Necromancer");
	else if (ns->spawn.class_==WIZARDGM)
		strcpy(ns2->spawn.lastName, "GM Wizard");
	else if (ns->spawn.class_==MAGICIANGM)
		strcpy(ns2->spawn.lastName, "GM Magician");
	else if (ns->spawn.class_==ENCHANTERGM)
		strcpy(ns2->spawn.lastName, "GM Enchanter");
	else if (ns->spawn.class_==BEASTLORDGM)
		strcpy(ns2->spawn.lastName, "GM Beastlord");
	else if (ns->spawn.class_==BERSERKERGM)
		strcpy(ns2->spawn.lastName, "GM Berserker");
	else
		strcpy(ns2->spawn.lastName, ns->spawn.lastName);
	memset(&app->pBuffer[sizeof(Spawn_Struct)-7],0xFF,7);
}

void Mob::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho)
{
	int i;

	strcpy(ns->spawn.name, name);
	if(IsClient())
		strncpy(ns->spawn.lastName,lastname,sizeof(lastname));
	ns->spawn.heading	= FloatToEQ19(heading);
	ns->spawn.x			= FloatToEQ19(x_pos);//((sint32)x_pos)<<3;
	ns->spawn.y			= FloatToEQ19(y_pos);//((sint32)y_pos)<<3;
	ns->spawn.z			= FloatToEQ19(z_pos);//((sint32)z_pos)<<3;
	ns->spawn.spawnId	= GetID();
	ns->spawn.curHp	= (sint16)GetHPRatio();
	ns->spawn.max_hp	= 100;		//this field needs a better name
	ns->spawn.race		= race;
	ns->spawn.runspeed	= runspeed;
	ns->spawn.walkspeed	= runspeed * 0.5f;
	ns->spawn.class_	= class_;
	ns->spawn.gender	= gender;
	ns->spawn.level		= level;
	ns->spawn.deity		= deity;
	ns->spawn.animation	= 0;
	ns->spawn.findable	= findable?1:0;
// vesuvias - appearence fix
	ns->spawn.light		= light;
	ns->spawn.showhelm = 1;	

	ns->spawn.invis		= (invisible || hidden) ? 1 : 0;	// TODO: load this before spawning players
	ns->spawn.NPC		= IsClient() ? 0 : 1;
	ns->spawn.petOwnerId	= ownerid;

	ns->spawn.haircolor = haircolor;
	ns->spawn.beardcolor = beardcolor;
	ns->spawn.eyecolor1 = eyecolor1;
	ns->spawn.eyecolor2 = eyecolor2;
	ns->spawn.hairstyle = hairstyle;
	ns->spawn.face = luclinface;
	ns->spawn.beard = beard;
	ns->spawn.StandState = standstate;
	ns->spawn.drakkin_heritage = drakkin_heritage;
	ns->spawn.drakkin_tattoo = drakkin_tattoo;
	ns->spawn.drakkin_details = drakkin_details;
	ns->spawn.equip_chest2  = texture;

//	ns->spawn.invis2 = 0xff;//this used to be labeled beard.. if its not FF it will turn
								   //mob invis

	if(helmtexture && helmtexture != 0xFF)
	{
		//ns->spawn.equipment[MATERIAL_HEAD] = helmtexture;
		ns->spawn.helm=helmtexture;
	} else {
		//ns->spawn.equipment[MATERIAL_HEAD] = 0;
		ns->spawn.helm = 0;
	}
	
	ns->spawn.guildrank	= 0xFF;
	ns->spawn.size			= size;
	ns->spawn.bodytype = bodytype;
	// The 'flymode' settings have the following effect:
	// 0 - Mobs in water sink like a stone to the bottom
	// 1 - Same as #flymode 1
	// 2 - Same as #flymode 2
	// 3 - Mobs in water do not sink. A value of 3 in this field appears to be the default setting for all mobs
	//     (in water or not) according to 6.2 era packet collects.
	if(IsClient())
	{
		ns->spawn.flymode = FindType(SE_Levitate) ? 2 : 0;
	}
	else
		ns->spawn.flymode = flymode;

	ns->spawn.lastName[0] = '\0';
	
	strncpy(ns->spawn.lastName, lastname, sizeof(lastname));

	for(i = 0; i < MAX_MATERIALS; i++)
	{
		ns->spawn.equipment[i] = GetEquipmentMaterial(i);
		if (armor_tint[i])
		{
			ns->spawn.colors[i].color = armor_tint[i];
		}
		else
		{
			ns->spawn.colors[i].color = GetEquipmentColor(i);
		}
	}
	
	memset(ns->spawn.set_to_0xFF, 0xFF, sizeof(ns->spawn.set_to_0xFF));
	
}

void Mob::CreateDespawnPacket(EQApplicationPacket* app, bool Decay)
{
	app->SetOpcode(OP_DeleteSpawn);
	app->size = sizeof(DeleteSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);
	DeleteSpawn_Struct* ds = (DeleteSpawn_Struct*)app->pBuffer;
	ds->spawn_id = GetID();
	// The next field only applies to corpses. If 0, they vanish instantly, otherwise they 'decay'
	ds->Decay = Decay ? 1 : 0;
}

void Mob::CreateHPPacket(EQApplicationPacket* app)
{ 
	this->IsFullHP=(cur_hp>=max_hp); 
	app->SetOpcode(OP_MobHealth); 
	app->size = sizeof(SpawnHPUpdate_Struct2); 
	app->pBuffer = new uchar[app->size]; 
	memset(app->pBuffer, 0, sizeof(SpawnHPUpdate_Struct2)); 
	SpawnHPUpdate_Struct2* ds = (SpawnHPUpdate_Struct2*)app->pBuffer; 

	ds->spawn_id = GetID(); 
	// they don't need to know the real hp
	ds->hp = (int)GetHPRatio();
 
	// hp event 
	if ( IsNPC() && ( GetNextHPEvent() > 0 ) ) { 
		if ( ds->hp < GetNextHPEvent() ) { 
			int lasthpevent = nexthpevent;
			parse->Event(EVENT_HP, GetNPCTypeID(), 0, CastToNPC(), NULL);
			if ( lasthpevent == nexthpevent ) {
				SetNextHPEvent(-1);
			}
		} 
	} 

	if ( IsNPC() && ( GetNextIncHPEvent() > 0 ) ) { 
		if ( ds->hp > GetNextIncHPEvent() ) { 
			int lastinchpevent = nextinchpevent;
			parse->Event(EVENT_HP, GetNPCTypeID(), 0, CastToNPC(), NULL);
			if ( lastinchpevent == nextinchpevent ) {
				SetNextIncHPEvent(-1);
			}
		} 
 	}   

#if 0	// solar: old stuff, leaving while testing changes
	// we dont give the actual hp of npcs
	if(IsNPC() || GetMaxHP() > 30000)
	{
		ds->cur_hp = (int)GetHPRatio();
		ds->max_hp = 100;
	}
	else
	{
		if(IsClient())
		{
			ds->cur_hp = CastToClient()->GetHP() - itembonuses.HP;
			ds->max_hp = CastToClient()->GetMaxHP() - itembonuses.HP;
#ifdef SOLAR
			Message(0, "HP: %d/%d", ds->cur_hp, ds->max_hp);
#endif
		}
		else
		{
			ds->cur_hp = GetHP();
			ds->max_hp = GetMaxHP();
		}
	}
#endif
} 

// sends hp update of this mob to people who might care
void Mob::SendHPUpdate()
{
	EQApplicationPacket hp_app;
	Group *group;
	
	// destructor will free the pBuffer
 	CreateHPPacket(&hp_app);

#ifdef MANAGE_HP_UPDATES
	entity_list.QueueManaged(this, &hp_app, true);
#else
	// send to people who have us targeted
 	entity_list.QueueClientsByTarget(this, &hp_app, false, 0, false, true, BIT_AllClients);
	entity_list.QueueToGroupsForNPCHealthAA(this, &hp_app);

	// send to group
	if(IsGrouped())
	{
		group = entity_list.GetGroupByMob(this);
		if(group)	//not sure why this might be null, but it happens
			group->SendHPPacketsFrom(this);
	}	

	if(IsClient()){
		Raid *r = entity_list.GetRaidByClient(CastToClient());
		if(r){
			r->SendHPPacketsFrom(CastToClient());
		}
	}

	// send to master
	if(GetOwner() && GetOwner()->IsClient())
	{
		GetOwner()->CastToClient()->QueuePacket(&hp_app, false);
	}

	// send to pet
	if(GetPet() && GetPet()->IsClient())
	{
		GetPet()->CastToClient()->QueuePacket(&hp_app, false);
	}
#endif	//MANAGE_HP_PACKETS

	// send to self - we need the actual hps here
	if(IsClient())
	{
		EQApplicationPacket* hp_app2 = new EQApplicationPacket(OP_HPUpdate,sizeof(SpawnHPUpdate_Struct));
		SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)hp_app2->pBuffer; 
		ds->cur_hp = CastToClient()->GetHP() - itembonuses.HP;
		ds->spawn_id = GetID();
		ds->max_hp = CastToClient()->GetMaxHP() - itembonuses.HP;
		CastToClient()->QueuePacket(hp_app2);
		safe_delete(hp_app2);
	}
}

// this one just warps the mob to the current location
void Mob::SendPosition() 
{
	EQApplicationPacket* app = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;	
	MakeSpawnUpdateNoDelta(spu);
	move_tic_count = 0;
	entity_list.QueueClients(this, app, true);
	safe_delete(app);
}

// this one is for mobs on the move, with deltas - this makes them walk
void Mob::SendPosUpdate(int8 iSendToSelf) {
	EQApplicationPacket* app = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;
	MakeSpawnUpdate(spu);
	
	if (iSendToSelf == 2) {
		if (this->IsClient())
			this->CastToClient()->FastQueuePacket(&app,false);
	}
	else
	{
#ifdef PACKET_UPDATE_MANAGER
		entity_list.QueueManaged(this, app, (iSendToSelf==0),false);
#else
		if(move_tic_count == RuleI(Zone,  NPCPositonUpdateTicCount))
		{
			entity_list.QueueClients(this, app, (iSendToSelf==0), false);
			move_tic_count = 0;
		}
		else
		{
			entity_list.QueueCloseClients(this, app, (iSendToSelf==0), 800, NULL, false);
			move_tic_count++;
		}
#endif
	}
	safe_delete(app);
}

// this is for SendPosition()
void Mob::MakeSpawnUpdateNoDelta(PlayerPositionUpdateServer_Struct *spu){
	memset(spu,0xff,sizeof(PlayerPositionUpdateServer_Struct));
	spu->spawn_id	= GetID();
	spu->x_pos		= FloatToEQ19(x_pos);
	spu->y_pos		= FloatToEQ19(y_pos);
	spu->z_pos		= FloatToEQ19(z_pos);
	spu->delta_x	= NewFloatToEQ13(0);
	spu->delta_y	= NewFloatToEQ13(0);
	spu->delta_z	= NewFloatToEQ13(0);
	spu->heading	= FloatToEQ19(heading);
	spu->animation	= 0;
	spu->delta_heading = NewFloatToEQ13(0);
	spu->padding0002	=0;
	spu->padding0006	=7;
	spu->padding0014	=0x7f;
	spu->padding0018	=0x5df27;

}

// this is for SendPosUpdate()
void Mob::MakeSpawnUpdate(PlayerPositionUpdateServer_Struct* spu) {
	spu->spawn_id	= GetID();
	spu->x_pos		= FloatToEQ19(x_pos);
	spu->y_pos		= FloatToEQ19(y_pos);
	spu->z_pos		= FloatToEQ19(z_pos);
	spu->delta_x	= NewFloatToEQ13(delta_x);
	spu->delta_y	= NewFloatToEQ13(delta_y);
	spu->delta_z	= NewFloatToEQ13(delta_z);
	spu->heading	= FloatToEQ19(heading);
	spu->padding0002	=0;
	spu->padding0006	=7;
	spu->padding0014	=0x7f;
	spu->padding0018	=0x5df27;
	if(this->IsClient())
		spu->animation = animation;
	else
		spu->animation	= pRunAnimSpeed;//animation;
	spu->delta_heading = NewFloatToEQ13(delta_heading);
}

void Mob::ShowStats(Client* client) {

	int16 attackRating = 0;
	int16 WornCap = GetATKBonus();

	attackRating = GetATK();

	Client *c = NULL;
	if (this->IsClient())
		c = this->CastToClient();
	
	client->Message(0, "Name: %s %s", GetName(), lastname);
	if (c)
	{
		client->Message(0, "  Level: %i  AC: %i  Class: %i  Size: %1.1f  Weight: %.1f/%d  Haste: %i", GetLevel(), GetAC(), GetClass(), GetSize(), (float)c->CalcCurrentWeight() / 10.0f, c->GetSTR(), GetHaste());
		client->Message(0, "  HP: %i  Max HP: %i  HP Regen: %i",GetHP(), GetMaxHP(), c->CalcHPRegen());
		client->Message(0, "  Mana: %i  Max Mana: %i  Mana Regen: %i", GetMana(), GetMaxMana(), c->CalcManaRegen());
		client->Message(0, "  Endurance: %i, Max Endurance: %i  Endurance Regen: %i",c->GetEndurance(), c->GetMaxEndurance(), c->CalcEnduranceRegen());
	}
	else
	{
		client->Message(0, "  Level: %i  AC: %i  Class: %i  Size: %1.1f  Haste: %i", GetLevel(), GetAC(), GetClass(), GetSize(), GetHaste());
		client->Message(0, "  HP: %i  Max HP: %i",GetHP(), GetMaxHP());
		client->Message(0, "  Mana: %i  Max Mana: %i", GetMana(), GetMaxMana());
	}
	client->Message(0, "  Total ATK: %i  Worn/Spell ATK (Cap 250): %i  Server Used ATK: %i", this->CastToClient()->GetTotalATK(), GetATKBonus(), attackRating);
	client->Message(0, "  STR: %i  STA: %i  DEX: %i  AGI: %i  INT: %i  WIS: %i  CHA: %i", GetSTR(), GetSTA(), GetDEX(), GetAGI(), GetINT(), GetWIS(), GetCHA());
	client->Message(0, "  MR: %i  PR: %i  FR: %i  CR: %i  DR: %i", GetMR(), GetPR(), GetFR(), GetCR(), GetDR());
	client->Message(0, "  Race: %i  BaseRace: %i  Texture: %i  HelmTexture: %i  Gender: %i  BaseGender: %i", GetRace(), GetBaseRace(), GetTexture(), GetHelmTexture(), GetGender(), GetBaseGender());
	
	if (client->Admin() >= 100) {
		client->Message(0, "  EntityID: %i  PetID: %i  OwnerID: %i  AIControlled: %i  Targetted: %i", 
						this->GetID(), this->GetPetID(), this->GetOwnerID(), this->IsAIControlled(), this->targeted);
		if (c) {
			client->Message(0, "  CharID: %i  PetID: %i", c->CharacterID(), this->GetPetID());
		}
		else if (this->IsCorpse()) {
			if (this->IsPlayerCorpse()) {
				Corpse *corpse = this->CastToCorpse();
				client->Message(0, "  CharID: %i  PlayerCorpse: %i", corpse->GetCharID(), corpse->GetDBID());
			}
			else {
				client->Message(0, "  NPCCorpse", this->GetID());
			}
		}
		else if (this->IsNPC()) {
			NPC *n = this->CastToNPC();
			int32 spawngroupid = 0;
			if(n->respawn2 != 0)
				spawngroupid = n->respawn2->SpawnGroupID();
			client->Message(0, "  NPCID: %u  SpawnGroupID: %u LootTable: %u  FactionID: %i  SpellsID: %u MerchantID: %i", this->GetNPCTypeID(),spawngroupid, n->GetLoottableID(), n->GetNPCFactionID(), n->GetNPCSpellsID(), n->MerchantType);
			client->Message(0, "  Accuracy: %i", n->GetAccuracyRating());
		}
		if (this->IsAIControlled()) {
			client->Message(0, "  AIControlled: AggroRange: %1.0f  AssistRange: %1.0f", this->GetAggroRange(), this->GetAssistRange());
		}
	}
}

void Mob::DoAnim(const int animnum, int type, bool ackreq, eqFilterType filter) {
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Animation, sizeof(Animation_Struct));
	Animation_Struct* anim = (Animation_Struct*)outapp->pBuffer;
	anim->spawnid = GetID();
	if(type == 0){
		anim->action = 10;
		anim->value=animnum;
	}
	else{
		anim->action = animnum;
		anim->value=type;
	}
	entity_list.QueueCloseClients(this, outapp, false, 200, 0, ackreq, filter);
	safe_delete(outapp);
}

void Mob::ShowBuffs(Client* client) {
	if (!spells_loaded)
		return;
	client->Message(0, "Buffs on: %s", this->GetName());
	uint32 i;
	uint32 buff_count = GetMaxTotalSlots();
	for (i=0; i < buff_count; i++) {
		if (buffs[i].spellid != SPELL_UNKNOWN) {
			if (buffs[i].durationformula == DF_Permanent)
				client->Message(0, "  %i: %s: Permanent", i, spells[buffs[i].spellid].name);
			else
				client->Message(0, "  %i: %s: %i tics left", i, spells[buffs[i].spellid].name, buffs[i].ticsremaining);

		}
	}
	if (IsClient()){
		client->Message(0, "itembonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", itembonuses.ATK, itembonuses.AC, itembonuses.HPRegen, itembonuses.HP, itembonuses.Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			itembonuses.STR,itembonuses.STA,itembonuses.DEX,itembonuses.AGI,itembonuses.INT,itembonuses.WIS,itembonuses.CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				itembonuses.MR,itembonuses.FR,itembonuses.CR,itembonuses.PR,itembonuses.DR);
		client->Message(0, "DmgShield:%i Haste:%i", itembonuses.DamageShield, itembonuses.haste );
		client->Message(0, "spellbonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", spellbonuses.ATK, spellbonuses.AC, spellbonuses.HPRegen, spellbonuses.HP, spellbonuses.Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			spellbonuses.STR,spellbonuses.STA,spellbonuses.DEX,spellbonuses.AGI,spellbonuses.INT,spellbonuses.WIS,spellbonuses.CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				spellbonuses.MR,spellbonuses.FR,spellbonuses.CR,spellbonuses.PR,spellbonuses.DR);
		client->Message(0, "DmgShield:%i Haste:%i", spellbonuses.DamageShield, spellbonuses.haste );
	}
}

void Mob::ShowBuffList(Client* client) {
	if (!spells_loaded)
		return;

	client->Message(0, "Buffs on: %s", this->GetCleanName());
	uint32 i;
	uint32 buff_count = GetMaxTotalSlots();
	for (i=0; i < buff_count; i++) {
		if (buffs[i].spellid != SPELL_UNKNOWN) {
			if (buffs[i].durationformula == DF_Permanent)
				client->Message(0, "  %i: %s: Permanent", i, spells[buffs[i].spellid].name);
			else
				client->Message(0, "  %i: %s: %i tics left", i, spells[buffs[i].spellid].name, buffs[i].ticsremaining);
		}
	}
}

void Mob::GMMove(float x, float y, float z, float heading, bool SendUpdate) {

	Route.clear();

	x_pos = x;
	y_pos = y;
	z_pos = z;
	if (heading != 0.01)
		this->heading = heading;
	if(IsNPC())
		CastToNPC()->SaveGuardSpot(true);
	if(SendUpdate)
		SendPosition();
	//SendPosUpdate(1);
#ifdef PACKET_UPDATE_MANAGER
	if(IsClient()) {
		CastToClient()->GetUpdateManager()->FlushQueues();
	}
#endif
}

void Mob::SendIllusionPacket(int16 in_race, int8 in_gender, int8 in_texture, int8 in_helmtexture, int8 in_haircolor, int8 in_beardcolor, int8 in_eyecolor1, int8 in_eyecolor2, int8 in_hairstyle, int8 in_luclinface, int8 in_beard, int8 in_aa_title, int32 in_drakkin_heritage, int32 in_drakkin_tattoo, int32 in_drakkin_details, float in_size) {

	int16 BaseRace = GetBaseRace();

	if (in_race == 0) {
		this->race = BaseRace;
		if (in_gender == 0xFF)
			this->gender = GetBaseGender();
		else
			this->gender = in_gender;
	}
	else {
		this->race = in_race;
		if (in_gender == 0xFF) {
			int8 tmp = Mob::GetDefaultGender(this->race, gender);
			if (tmp == 2)
				gender = 2;
			else if (gender == 2 && GetBaseGender() == 2)
				gender = tmp;
			else if (gender == 2)
				gender = GetBaseGender();
		}
		else
			gender = in_gender;
	}
	if (in_texture == 0xFF) {
		if (in_race <= 12 || in_race == 128 || in_race == 130 || in_race == 330 || in_race == 522)
			this->texture = 0xFF;
		else
			this->texture = GetTexture();
	}
	else
		this->texture = in_texture;

	if (in_helmtexture == 0xFF) {
		if (in_race <= 12 || in_race == 128 || in_race == 130 || in_race == 330 || in_race == 522)
			this->helmtexture = 0xFF;
		else if (in_texture != 0xFF)
			this->helmtexture = in_texture;
		else
			this->helmtexture = GetHelmTexture();
	}
	else
		this->helmtexture = in_helmtexture;

	if (in_haircolor == 0xFF)
		this->haircolor = GetHairColor();
	else
		this->haircolor = in_haircolor;

	if (in_beardcolor == 0xFF)
		this->beardcolor = GetBeardColor();
	else
		this->beardcolor = in_beardcolor;

	if (in_eyecolor1 == 0xFF)
		this->eyecolor1 = GetEyeColor1();
	else
		this->eyecolor1 = in_eyecolor1;

	if (in_eyecolor2 == 0xFF)
		this->eyecolor2 = GetEyeColor2();
	else
		this->eyecolor2 = in_eyecolor2;

	if (in_hairstyle == 0xFF)
		this->hairstyle = GetHairStyle();
	else
		this->hairstyle = in_hairstyle;

	if (in_luclinface == 0xFF)
		this->luclinface = GetLuclinFace();
	else
		this->luclinface = in_luclinface;

	if (in_beard == 0xFF)
		this->beard	= GetBeard();
	else
		this->beard = in_beard;

	this->aa_title = 0xFF;

	if (in_drakkin_heritage == 0xFFFFFFFF)
		this->drakkin_heritage = GetDrakkinHeritage();
	else
		this->drakkin_heritage = in_drakkin_heritage;

	if (in_drakkin_tattoo == 0xFFFFFFFF)
		this->drakkin_tattoo = GetDrakkinTattoo();
	else
		this->drakkin_tattoo = in_drakkin_tattoo;

	if (in_drakkin_details == 0xFFFFFFFF)
		this->drakkin_details = GetDrakkinDetails();
	else
		this->drakkin_details = in_drakkin_details;

	if (in_size == 0xFFFFFFFF)
		this->size = GetSize();
	else
		this->size = in_size;

	// Forces the feature information to be pulled from the Player Profile
	if (this->IsClient() && in_race == 0) {
		this->race = CastToClient()->GetBaseRace();
		this->gender = CastToClient()->GetBaseGender();
		this->texture = 0xFF;
		this->helmtexture = 0xFF;
		this->haircolor = CastToClient()->GetBaseHairColor();
		this->beardcolor = CastToClient()->GetBaseBeardColor();
		this->eyecolor1 = CastToClient()->GetBaseEyeColor();
		this->eyecolor2 = CastToClient()->GetBaseEyeColor();
		this->hairstyle = CastToClient()->GetBaseHairStyle();
		this->luclinface = CastToClient()->GetBaseFace();
		this->beard	= CastToClient()->GetBaseBeard();
		this->aa_title = 0xFF;
		this->drakkin_heritage = CastToClient()->GetBaseHeritage();
		this->drakkin_tattoo = CastToClient()->GetBaseTattoo();
		this->drakkin_details = CastToClient()->GetBaseDetails();
		switch(race){
			case OGRE:
				this->size = 9;
				break;
			case TROLL:
				this->size = 8;
				break;
			case VAHSHIR:
			case BARBARIAN:
				this->size = 7;
				break;
			case HALF_ELF:
			case WOOD_ELF:
			case DARK_ELF:
			case FROGLOK:
				this->size = 5;
				break;
			case DWARF:
				this->size = 4;
				break;
			case HALFLING:
			case GNOME:
				this->size = 3;
				break;
			default:
				this->size = 6;
				break;
		}
	}

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Illusion, sizeof(Illusion_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
	is->spawnid = this->GetID();
	strcpy(is->charname, GetCleanName());
	is->race = this->race;
	is->gender = this->gender;
	is->texture = this->texture;
	is->helmtexture = this->helmtexture;
	is->haircolor = this->haircolor;
	is->beardcolor = this->beardcolor;
	is->beard = this->beard;
	is->eyecolor1 = this->eyecolor1;
	is->eyecolor2 = this->eyecolor2;
	is->hairstyle = this->hairstyle;
	is->face = this->luclinface;
	//is->aa_title = this->aa_title;
	is->drakkin_heritage = this->drakkin_heritage;
	is->drakkin_tattoo = this->drakkin_tattoo;
	is->drakkin_details = this->drakkin_details;
	is->size = this->size;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
	mlog(CLIENT__SPELLS, "Illusion: Race = %i, Gender = %i, Texture = %i, HelmTexture = %i, HairColor = %i, BeardColor = %i, EyeColor1 = %i, EyeColor2 = %i, HairStyle = %i, Face = %i, DrakkinHeritage = %i, DrakkinTattoo = %i, DrakkinDetails = %i, Size = %f",
		this->race, this->gender, this->texture, this->helmtexture, this->haircolor, this->beardcolor, this->eyecolor1, this->eyecolor2, this->hairstyle, this->luclinface, this->drakkin_heritage, this->drakkin_tattoo, this->drakkin_details, this->size);
}

int8 Mob::GetDefaultGender(int16 in_race, int8 in_gender) {
//cout << "Gender in:  " << (int)in_gender << endl;
	if ((in_race > 0 && in_race <= GNOME )
		|| in_race == IKSAR || in_race == VAHSHIR || in_race == FROGLOK || in_race == DRAKKIN
		|| in_race == 15 || in_race == 50 || in_race == 57 || in_race == 70 || in_race == 98 || in_race == 118) {
		if (in_gender >= 2) {
			// Female default for PC Races
			return 1;
		}
		else
			return in_gender;
	}
	else if (in_race == 44 || in_race == 52 || in_race == 55 || in_race == 65 || in_race == 67 || in_race == 88 || in_race == 117 || in_race == 127 ||
		in_race == 77 || in_race == 78 || in_race == 81 || in_race == 90 || in_race == 92 || in_race == 93 || in_race == 94 || in_race == 106 || in_race == 112 || in_race == 471) {
		// Male only races
		return 0;

	}
	else if (in_race == 25 || in_race == 56) {
		// Female only races
		return 1;
	}
	else {
		// Neutral default for NPC Races
		return 2;
	}
}

void Mob::SendAppearancePacket(int32 type, int32 value, bool WholeZone, bool iIgnoreSelf, Client *specific_target) {
	if (!GetID())
		return;
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
	SpawnAppearance_Struct* appearance = (SpawnAppearance_Struct*)outapp->pBuffer;
	appearance->spawn_id = this->GetID();
	appearance->type = type;
	appearance->parameter = value;
	if (WholeZone)
		entity_list.QueueClients(this, outapp, iIgnoreSelf);
	else if(specific_target != NULL)
		specific_target->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	else if (this->IsClient())
		this->CastToClient()->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	safe_delete(outapp);
}

void Mob::SendLevelAppearance(){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->parm1 = 0x4D;
	la->parm2 = la->parm1 + 1;
	la->parm3 = la->parm2 + 1;
	la->parm4 = la->parm3 + 1;
	la->parm5 = la->parm4 + 1;
	la->spawn_id = GetID();
	la->value1a = 1;
	la->value2a = 2;
	la->value3a = 1;
	la->value3b = 1;
	la->value4a = 1;
	la->value4b = 1;
	la->value5a = 2;
	entity_list.QueueCloseClients(this,outapp);
	safe_delete(outapp);
}

void Mob::SendAppearanceEffect(int32 parm1, int32 parm2, int32 parm3, int32 parm4, int32 parm5, Client *specific_target){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->spawn_id = GetID();
	la->parm1 = parm1;
	la->parm2 = parm2;
	la->parm3 = parm3;
	la->parm4 = parm4;
	la->parm5 = parm5;
	// Note that setting the b values to 0 will disable the related effect from the corresponding parameter.
	// Setting the a value appears to have no affect at all.
	la->value1a = 1;
	la->value1b = 1;
	la->value2a = 1;
	la->value2b = 1;
	la->value3a = 1;
	la->value3b = 1;
	la->value4a = 1;
	la->value4b = 1;
	la->value5a = 1;
	la->value5b = 1;
	if(specific_target == NULL) {
		entity_list.QueueClients(this,outapp);
	}
	else if (specific_target->IsClient()) {
		specific_target->CastToClient()->QueuePacket(outapp, false);
	}
	safe_delete(outapp);
}

void Mob::QuestReward(Client *c, int32 silver, int32 gold, int32 platinum) {

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Sound, sizeof(QuestReward_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	QuestReward_Struct* qr = (QuestReward_Struct*) outapp->pBuffer;

	qr->from_mob = GetID();		// Entity ID for the from mob name	
	qr->silver = silver;
	qr->gold = gold;
	qr->platinum = platinum;
	
	if(c)
		c->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	
	safe_delete(outapp);
}

void Mob::CameraEffect(uint32 duration, uint32 intensity, Client *c) {

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_CameraEffect, sizeof(Camera_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	Camera_Struct* cs = (Camera_Struct*) outapp->pBuffer;
	cs->duration = duration;	// Duration in milliseconds
	cs->intensity = ((intensity * 6710886) + 1023410176);	// Intensity ranges from 1023410176 to 1090519040, so simplify it from 0 to 10.
	
	if(c)
		c->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	else
		entity_list.QueueClients(this, outapp);
	
	safe_delete(outapp);
}

void Mob::SendSpellEffect(uint32 effectid, int32 duration, int32 finish_delay, bool zone_wide, int32 unk020, bool perm_effect, Client *c) {

	EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpellEffect, sizeof(SpellEffect_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	SpellEffect_Struct* se = (SpellEffect_Struct*) outapp->pBuffer;
	se->EffectID = effectid;	// ID of the Particle Effect
	se->EntityID = GetID();
	se->EntityID2 = GetID();	// EntityID again
	se->Duration = duration;	// In Milliseconds
	se->FinishDelay = finish_delay;	// Seen 0
	se->Unknown020 = unk020;	// Seen 3000
	se->Unknown024 = 1;		// Seen 1 for SoD
	se->Unknown025 = 1;		// Seen 1 for Live
	se->Unknown026 = 0;		// Seen 1157
	
	if(c)
		c->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
	else if(zone_wide)
		entity_list.QueueClients(this, outapp);
	else
		entity_list.QueueCloseClients(this, outapp);
	
	safe_delete(outapp);

	if (perm_effect) {
		if(!IsNimbusEffectActive(effectid)) {
			SetNimbusEffect(effectid);
		}
	}

}

void Mob::TempName(const char *newname)
{
	char temp_name[64];
	char old_name[64];
	strncpy(old_name, GetName(), 64);

	if(newname)
		strncpy(temp_name, newname, 64);	

	// Reset the name to the original if left null.
	if(!newname) {
		strncpy(temp_name, GetOrigName(), 64);
		SetName(temp_name);
		//CleanMobName(GetName(), temp_name);
		strncpy(temp_name, GetCleanName(), 64);
	}

	// Make the new name unique and set it
	strncpy(temp_name, entity_list.MakeNameUnique(temp_name), 64);
	

	// Send the new name to all clients
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_MobRename, sizeof(MobRename_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	MobRename_Struct* mr = (MobRename_Struct*) outapp->pBuffer;
	strncpy(mr->old_name, old_name, 64);
	strncpy(mr->old_name_again, old_name, 64);
	strncpy(mr->new_name, temp_name, 64);
	mr->unknown192 = 0;
	mr->unknown196 = 1;
	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);

	SetName(temp_name);
}

const sint32& Mob::SetMana(sint32 amount)
{
	CalcMaxMana();
	sint32 mmana = GetMaxMana();
	cur_mana = amount < 0 ? 0 : (amount > mmana ? mmana : amount);
/*
	if(IsClient())
		LogFile->write(EQEMuLog::Debug, "Setting mana for %s to %d (%4.1f%%)", GetName(), amount, GetManaRatio());
*/

	return cur_mana;
}


void Mob::SetAppearance(EmuAppearance app, bool iIgnoreSelf) {
	if (_appearance != app) {
		_appearance = app;
		SendAppearancePacket(AT_Anim, GetAppearanceValue(app), true, iIgnoreSelf);
		if (this->IsClient() && this->IsAIControlled())
			SendAppearancePacket(AT_Anim, ANIM_FREEZE, false, false);
	}
}

void Mob::ChangeSize(float in_size = 0, bool bNoRestriction) {
	//Neotokyo's Size Code
	if (!bNoRestriction)
	{
		if (this->IsClient() || this->petid != 0)
			if (in_size < 3.0)
				in_size = 3.0;


			if (this->IsClient() || this->petid != 0)
				if (in_size > 15.0)
					in_size = 15.0;
	}


	if (in_size < 1.0)
		in_size = 1.0;

	if (in_size > 255.0)
		in_size = 255.0;
	//End of Neotokyo's Size Code
	this->size = in_size;
	SendAppearancePacket(AT_Size, (int32) in_size);
}

Mob* Mob::GetOwnerOrSelf() {
	if (!GetOwnerID())
		return this;
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (!owner) {
		SetOwnerID(0);
		return(this);
	}
	if (owner->GetPetID() == this->GetID()) {
		return owner;
	}
	if(IsNPC() && CastToNPC()->GetSwarmInfo()){
		return (CastToNPC()->GetSwarmInfo()->GetOwner());
	}
	SetOwnerID(0);
	return this;
}

Mob* Mob::GetOwner() {
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (owner && owner->GetPetID() == this->GetID()) {

		return owner;
	}
	if(IsNPC() && CastToNPC()->GetSwarmInfo()){
		return (CastToNPC()->GetSwarmInfo()->GetOwner());
	}
	SetOwnerID(0);
	return 0;
}

Mob* Mob::GetUltimateOwner()
{
	Mob* Owner = GetOwner();

	if(!Owner)
		return this;

	while(Owner && Owner->HasOwner())
		Owner = Owner->GetOwner();

	return Owner ? Owner : this;
}

void Mob::SetOwnerID(int16 NewOwnerID) {
	if (NewOwnerID == GetID() && NewOwnerID != 0) // ok, no charming yourself now =p
		return;
	ownerid = NewOwnerID;
	if (ownerid == 0 && this->IsNPC() && this->GetPetType() != petCharmed)
		this->Depop();
}

//heko: for backstab
bool Mob::BehindMob(Mob* other, float playerx, float playery) const {
    if (!other)
        return true; // sure your behind your invisible friend?? (fall thru for sneak)
	//see if player is behind mob
	float angle, lengthb, vectorx, vectory;
	float mobx = -(other->GetX());	// mob xlocation (inverse because eq is confused)
	float moby = other->GetY();		// mobylocation
	float heading = other->GetHeading();	// mob heading
	heading = (heading * 360.0)/256.0;	// convert to degrees
	if (heading < 270)
		heading += 90;
	else
		heading -= 270;
	heading = heading*3.1415/180.0;	// convert to radians
	vectorx = mobx + (10.0 * cosf(heading));	// create a vector based on heading
	vectory = moby + (10.0 * sinf(heading));	// of mob length 10

	//length of mob to player vector
	//lengthb = (float)sqrtf(pow((-playerx-mobx),2) + pow((playery-moby),2));
	lengthb = (float) sqrtf( ( (-playerx-mobx) * (-playerx-mobx) ) + ( (playery-moby) * (playery-moby) ) );

	// calculate dot product to get angle
	angle = acosf(((vectorx-mobx)*(-playerx-mobx)+(vectory-moby)*(playery-moby)) / (10 * lengthb));
	angle = angle * 180 / 3.1415;
	if (angle > 90.0) //not sure what value to use (90*2=180 degrees is front)
		return true;
	else
		return false;
}

void Mob::SetZone(int32 zone_id, int32 instance_id)
{
	if(IsClient())
	{
		CastToClient()->GetPP().zone_id = zone_id;
		CastToClient()->GetPP().zoneInstance = instance_id;
	}
	Save();
}

void Mob::Kill() {
	Death(this, 0, SPELL_UNKNOWN, HAND_TO_HAND);
}

void Mob::SetAttackTimer() {
	float PermaHaste;
	if(GetHaste() > 0)
		PermaHaste = 1 / (1 + (float)GetHaste()/100);
	else if(GetHaste() < 0)
		PermaHaste = 1 * (1 - (float)GetHaste()/100);
	else
		PermaHaste = 1.0f;
	
	//default value for attack timer in case they have
	//an invalid weapon equipped:
	attack_timer.SetAtTrigger(4000, true);
	
	Timer* TimerToUse = NULL;
	const Item_Struct* PrimaryWeapon = NULL;
	
	for (int i=SLOT_RANGE; i<=SLOT_SECONDARY; i++) {
		
		//pick a timer
		if (i == SLOT_PRIMARY)
			TimerToUse = &attack_timer;
		else if (i == SLOT_RANGE)
			TimerToUse = &ranged_timer;
		else if(i == SLOT_SECONDARY)
			TimerToUse = &attack_dw_timer;
		else	//invalid slot (hands will always hit this)
			continue;
		
		const Item_Struct* ItemToUse = NULL;
		
		//find our item
		if (IsClient()) {
			ItemInst* ci = CastToClient()->GetInv().GetItem(i);
			if (ci)
				ItemToUse = ci->GetItem();
		} else if(IsNPC())
		{
			//The code before here was fundementally flawed because equipment[] 
			//isn't the same as PC inventory and also:
			//NPCs don't use weapon speed to dictate how fast they hit anyway.
			ItemToUse = NULL;
		}
		
		//special offhand stuff
		if(i == SLOT_SECONDARY) {
			//if we have a 2H weapon in our main hand, no dual
			if(PrimaryWeapon != NULL) {
				if(	PrimaryWeapon->ItemClass == ItemClassCommon
					&& (PrimaryWeapon->ItemType == ItemType2HS
					||	PrimaryWeapon->ItemType == ItemType2HB
					||	PrimaryWeapon->ItemType == ItemType2HPierce)) {
					attack_dw_timer.Disable();
					continue;
				}
			}

			//clients must have the skill to use it...
			if(IsClient()) {
				//if we cant dual wield, skip it
				if (!CanThisClassDualWield()) {
					attack_dw_timer.Disable();
					continue;
				}
			} else {
				//NPCs get it for free at 13
				if(GetLevel() < 13) {
					attack_dw_timer.Disable();
					continue;
				}
			}
		}
		
		//see if we have a valid weapon
		if(ItemToUse != NULL) {
			//check type and damage/delay
			if(ItemToUse->ItemClass != ItemClassCommon 
				|| ItemToUse->Damage == 0 
				|| ItemToUse->Delay == 0) {
				//no weapon
				ItemToUse = NULL;
			}
			// Check to see if skill is valid
			else if((ItemToUse->ItemType > ItemTypeThrowing) && (ItemToUse->ItemType != ItemTypeHand2Hand) && (ItemToUse->ItemType != ItemType2HPierce)) {
				//no weapon
				ItemToUse = NULL;
			}
		}

		//if we have no weapon..
		if (ItemToUse == NULL) {
			//above checks ensure ranged weapons do not fall into here
			// Work out if we're a monk
			if ((GetClass() == MONK) || (GetClass() == BEASTLORD)) {
				//we are a monk, use special delay
				int speed = (int)(GetMonkHandToHandDelay()*(100.0f+attack_speed)*PermaHaste);
				// neotokyo: 1200 seemed too much, with delay 10 weapons available
				if(speed < 800)	//lower bound
					speed = 800;
				TimerToUse->SetAtTrigger(speed, true);	// Hand to hand, delay based on level or epic
			} else {
				//not a monk... using fist, regular delay
				int speed = (int)(36*(100.0f+attack_speed)*PermaHaste);
				if(speed < 800 && IsClient())	//lower bound
					speed = 800;
				TimerToUse->SetAtTrigger(speed, true); 	// Hand to hand, non-monk 2/36
			}
		} else {
			//we have a weapon, use its delay
			// Convert weapon delay to timer resolution (milliseconds)
			//delay * 100
			int speed = (int)(ItemToUse->Delay*(100.0f+attack_speed)*PermaHaste);
			if(speed < 800)
				speed = 800;

			if(ItemToUse && (ItemToUse->ItemType == ItemTypeBow || ItemToUse->ItemType == ItemTypeThrowing))
			{
				if(IsClient())
				{
					float max_quiver = 0;
					for(int r = SLOT_PERSONAL_BEGIN; r <= SLOT_PERSONAL_END; r++) 
					{
						const ItemInst *pi = CastToClient()->GetInv().GetItem(r);
						if(!pi)
							continue;
						if(pi->IsType(ItemClassContainer) && pi->GetItem()->BagType == bagTypeQuiver)
						{
							float temp_wr = (pi->GetItem()->BagWR / 3);
							if(temp_wr > max_quiver)
							{
								max_quiver = temp_wr;
							}
						}
					}
					if(max_quiver > 0)
					{
						float quiver_haste = 1 / (1 + max_quiver / 100);
						speed *= quiver_haste;
					}
				}
			}
			TimerToUse->SetAtTrigger(speed, true);
		}
		
		if(i == SLOT_PRIMARY)
			PrimaryWeapon = ItemToUse;
	}
	
}

bool Mob::CanThisClassDualWield(void) const
{
	// All npcs over level 13 can dual wield
	if (this->IsNPC() && (this->GetLevel() >= 13))
		return true;
	
	// Kaiyodo - Check the classes that can DW, and make sure we're not using a 2 hander
	bool dh2h = false;
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
	case BERSERKER:
	case ROGUE:
	case WARRIORGM:
	case BERSERKERGM:
	case ROGUEGM:
		{
			if(GetLevel() < 13)
				return false;
			break;
		}
	case BARD:
	case RANGER:
	case BARDGM:
	case RANGERGM:
		{
			if(GetLevel() < 17)
				return false;
			break;
		}
	case BEASTLORD:
	case BEASTLORDGM:
		{
			if(GetLevel() < 17)
				return false;
			dh2h = true;
			break;
		}
	case MONK:
	case MONKGM:
		{
			dh2h = true;
			break;
		}
	default:
		{
			return false;
		}
	}
	
	if (IsClient()) {
		const ItemInst* inst = CastToClient()->GetInv().GetItem(SLOT_PRIMARY);
		// 2HS, 2HB, or 2HP
		if (inst && inst->IsType(ItemClassCommon)) {
			const Item_Struct* item = inst->GetItem();
			if ((item->ItemType == ItemType2HB) || (item->ItemType == ItemType2HS) || (item->ItemType == ItemType2HPierce))
				return false;
		} else {
			//No weapon in hand... using hand-to-hand...
			//only monks and beastlords? can dual wield their fists.
			return(dh2h);
		}
		
		return (this->CastToClient()->HasSkill(DUAL_WIELD));	// No skill = no chance
	}
	else
		return true;	//if we get here, we are the right class
						//and are at the right level, and are NPC
}

bool Mob::CanThisClassDoubleAttack(void) const
{
    // All npcs over level 26 can double attack
    if (IsNPC() && GetLevel() >= 26)
        return true;
	
	if(GetAA(aaBestialFrenzy) || GetAA(aaHarmoniousAttack) || 
		GetAA(aaKnightsAdvantage) || GetAA(aaFerocity)){
			return true;
		}

	// Kaiyodo - Check the classes that can DA
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case BERSERKER:
	case BERSERKERGM:	
	case WARRIOR:
	case WARRIORGM:
	case MONK:
	case MONKGM:
		{
			if(GetLevel() < 15)
				return false;
			break;
		}
	case ROGUE:
	case ROGUEGM:
		{
			if(GetLevel() < 16)
				return false;
			break;
		}
	case RANGER:
	case RANGERGM:
	case PALADIN:
	case PALADINGM:
	case SHADOWKNIGHT:
	case SHADOWKNIGHTGM:
		{
			if(GetLevel() < 20)
				return false;
			break;
		}
	default:
		{
			return false;
		}
	}

	if (IsClient())
		return(CastToClient()->HasSkill(DOUBLE_ATTACK));	// No skill = no chance
	else
		return true;	//if we get here, we are the right class
						//and are at the right level, and are NPC
}

bool Mob::IsWarriorClass(void) const
{
	switch(GetClass())
	{
	case WARRIOR:
	case WARRIORGM:
	case ROGUE:
	case ROGUEGM:
	case MONK:
	case MONKGM:
	case PALADIN:
	case PALADINGM:
	case SHADOWKNIGHT:
	case SHADOWKNIGHTGM:
	case RANGER:
	case RANGERGM:
	case BEASTLORD:
	case BEASTLORDGM:
	case BERSERKER:
	case BERSERKERGM:
	case BARD:
	case BARDGM:
		{
			return true;
		}
	default:
		{
			return false;
		}
	}

}

bool Mob::CanThisClassParry(void) const
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
	case WARRIORGM:
		{
		if(GetLevel() < 10)
			return false;
		break;
		}
	case ROGUE:
	case BERSERKER:
	case ROGUEGM:
	case BERSERKERGM:
		{
		if(GetLevel() < 12)
			return false;
		break;
		}
	case BARD:
	case BARDGM:
		{
		if(GetLevel() < 53)
			return false;
		break;
		}
	case RANGER:
	case RANGERGM:
		{
		if(GetLevel() < 18)
			return false;
		break;
		}
	case SHADOWKNIGHT:
	case PALADIN:
	case SHADOWKNIGHTGM:
	case PALADINGM:
		{
		if(GetLevel() < 17)
			return false;
		break;
		}
	default:
		{
			return false;
		}
	}

	if (this->IsClient())
		return(this->CastToClient()->HasSkill(PARRY));	// No skill = no chance
	else
		return true;
}

bool Mob::CanThisClassDodge(void) const
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
	case WARRIORGM:
		{
			if(GetLevel() < 6)
				return false;
			break;
		}
	case MONK:
	case MONKGM:
		{
			break;
		}
	case ROGUE:
	case ROGUEGM:
		{
			if(GetLevel() < 4)
				return false;
			break;
		}
	case RANGER:
	case RANGERGM:
		{
			if(GetLevel() < 8)
				return false;
			break;
		}
	case BARD:
	case BEASTLORD:
	case SHADOWKNIGHT:
	case BERSERKER:
	case PALADIN:
	case BARDGM:
	case BEASTLORDGM:
	case SHADOWKNIGHTGM:
	case BERSERKERGM:
	case PALADINGM:
		{
			if(GetLevel() < 10)
				return false;
			break;
		}
	case CLERIC:
	case SHAMAN:
	case DRUID:
	case CLERICGM:
	case SHAMANGM:
	case DRUIDGM:
		{
			if( GetLevel() < 15 )
				return false;
			break;
		}
	case NECROMANCER:
	case ENCHANTER:
	case WIZARD:
	case MAGICIAN:
	case NECROMANCERGM:
	case ENCHANTERGM:
	case WIZARDGM:
	case MAGICIANGM:
		{
			if( GetLevel() < 22 )
				return false;
			break;
		}
	default:
		{
			return false;
		}
	}
	
	if (this->IsClient())
		return(this->CastToClient()->HasSkill(DODGE));	// No skill = no chance
	else
		return true;
}

bool Mob::CanThisClassRiposte(void) const //Could just check if they have the skill?
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
	case WARRIORGM:
		{
			if(GetLevel() < 25)
				return false;
			break;
		}
	case ROGUE:
	case RANGER:
	case SHADOWKNIGHT:
	case PALADIN:
	case BERSERKER:
	case ROGUEGM:
	case RANGERGM:
	case SHADOWKNIGHTGM:
	case PALADINGM:
	case BERSERKERGM:
		{
			if(GetLevel() < 30)
				return false;
			break;
		}
	case MONK:
	case MONKGM:
		{
			if(GetLevel() < 35)
				return false;
			break;
		}
	case BEASTLORD:
	case BEASTLORDGM:
		{
			if(GetLevel() < 40)
				return false;
			break;
		}
	case BARD:
	case BARDGM:
		{
			if(GetLevel() < 58)
				return false;
			break;
		}
	default:
		{
			return false;
		}
	}
	
	if (this->IsClient())
		return(this->CastToClient()->HasSkill(RIPOSTE));	// No skill = no chance
	else
		return true;
}

bool Mob::CanThisClassBlock(void) const
{
	switch(GetClass())
	{
	case BEASTLORDGM:
	case BEASTLORD:
		{
		if(GetLevel() < 25)
			return false;
		break;
		}
	case MONKGM:
	case MONK:
		{
		if(GetLevel() < 12)
			return false;
		break;
		}
	default:
		{
			return false;
		}
	}

	if (this->IsClient())
		return(this->CastToClient()->HasSkill(BLOCKSKILL));	// No skill = no chance
	else
		return true;
}

float Mob::Dist(const Mob &other) const {
	_ZP(Mob_Dist);
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return sqrtf( (xDiff * xDiff) 
	           + (yDiff * yDiff) 
		       + (zDiff * zDiff) );
}

float Mob::DistNoZ(const Mob &other) const {
	_ZP(Mob_DistNoZ);
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	
	return sqrtf( (xDiff * xDiff) 
		       + (yDiff * yDiff) );
}

float Mob::DistNoRoot(const Mob &other) const {
	_ZP(Mob_DistNoRoot);
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return ( (xDiff * xDiff)  
	       + (yDiff * yDiff)  
	       + (zDiff * zDiff) );
}

float Mob::DistNoRoot(float x, float y, float z) const {
	_ZP(Mob_DistNoRoot);
	float xDiff = x - x_pos;
	float yDiff = y - y_pos;
	float zDiff = z - z_pos;

	return ( (xDiff * xDiff)  
	       + (yDiff * yDiff)  
	       + (zDiff * zDiff) );
}

float Mob::DistNoRootNoZ(float x, float y) const {
	_ZP(Mob_DistNoRoot);
	float xDiff = x - x_pos;
	float yDiff = y - y_pos;

	return ( (xDiff * xDiff)  
	       + (yDiff * yDiff) );
}

float Mob::DistNoRootNoZ(const Mob &other) const {
	_ZP(Mob_DistNoRootNoZ);
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;

	return ( (xDiff * xDiff) + (yDiff * yDiff) );
}

float Mob::GetReciprocalHeading(Mob* target) {
	float Result = 0;

	if(target) {
		// Convert to radians
		float h = (target->GetHeading() / 256) * 6.283184;

		// Calculate the reciprocal heading in radians
		Result =  h + 3.141592;

		// Convert back to eq heading from radians
		Result = (Result / 6.283184) * 256;
	}

	return Result;
}

bool Mob::PlotPositionAroundTarget(Mob* target, float &x_dest, float &y_dest, float &z_dest, bool lookForAftArc) {
	bool Result = false;

	if(target) {
		float look_heading = 0;

		if(lookForAftArc)
			look_heading = GetReciprocalHeading(target);
		else
			look_heading = target->GetHeading();

		// Convert to sony heading to radians
		look_heading = (look_heading / 256) * 6.283184;

		float tempX = 0;
		float tempY = 0;
		float tempZ = 0;
		float tempSize = 0;
		const float rangeCreepMod = 0.25;
		const uint8 maxIterationsAllowed = 4;
		uint8 counter = 0;
		float rangeReduction= 0;

		tempSize = target->GetSize();
		rangeReduction = (tempSize * rangeCreepMod);

		while(tempSize > 0 && counter != maxIterationsAllowed) {
			tempX = GetX() + (tempSize * sin(double(look_heading)));
			tempY = GetY() + (tempSize * cos(double(look_heading)));
			tempZ = target->GetZ();

			if(!CheckLosFN(tempX, tempY, tempZ, tempSize)) {
				tempSize -= rangeReduction;
			}
			else {
				Result = true;
				break;
			}

			counter++;
		}

		if(!Result) {
			// Try to find an attack arc to position at from the opposite direction.
			look_heading += (3.141592 / 2);

			tempSize = target->GetSize();
			counter = 0;

			while(tempSize > 0 && counter != maxIterationsAllowed) {
				tempX = GetX() + (tempSize * sin(double(look_heading)));
				tempY = GetY() + (tempSize * cos(double(look_heading)));
				tempZ = target->GetZ();

				if(!CheckLosFN(tempX, tempY, tempZ, tempSize)) {
					tempSize -= rangeReduction;
				}
				else {
					Result = true;
					break;
				}

				counter++;
			}
		}

		if(Result) {
			x_dest = tempX;
			y_dest = tempY;
			z_dest = tempZ;
		}
	}

	return Result;
}

bool Mob::HateSummon() {
    // check if mob has ability to summon
    // we need to be hurt and level 51+ or ability checked to continue
// Sandy - fix so not automatic summon
	if (GetHPRatio() >= 95 || SpecAttacks[SPECATK_SUMMON] == false)
        return false;

    // now validate the timer
    if (!SpecAttackTimers[SPECATK_SUMMON])
    {
        SpecAttackTimers[SPECATK_SUMMON] = new Timer(6000);
        SpecAttackTimers[SPECATK_SUMMON]->Start();
    }

    // now check the timer
    if (!SpecAttackTimers[SPECATK_SUMMON]->Check())
        return false;

    // get summon target
    SetTarget(GetHateTop());
    if(target)
    {
		if (target->IsClient())
			target->CastToClient()->Message(15,"You have been summoned!");
		entity_list.MessageClose(this, true, 500, 10, "%s says,'You will not evade me, %s!' ", GetCleanName(), GetHateTop()->GetCleanName() );

		// RangerDown - GMMove doesn't seem to be working well with players, so use MovePC for them, GMMove for NPC's
		if (target->IsClient()) {
			target->CastToClient()->MovePC(zone->GetZoneID(), zone->GetInstanceID(), x_pos, y_pos, z_pos, target->GetHeading(), 0, SummonPC);
		}
		else
			GetHateTop()->GMMove(x_pos, y_pos, z_pos, target->GetHeading());

        return true;
	}
	return false;
}

void Mob::FaceTarget(Mob* MobToFace) {
	Mob* facemob = MobToFace;
	if(!facemob) {
		if(!GetTarget()) {
			return;
		}
		else {
			facemob = GetTarget();
		}
	}

	float oldheading = GetHeading();
	float newheading = CalculateHeadingToTarget(facemob->GetX(), facemob->GetY());
	if(oldheading != newheading) {
		SetHeading(newheading);
		if(moving)
			SendPosUpdate();
		else
		{
			SendPosition();
		}
	}
}

bool Mob::RemoveFromHateList(Mob* mob) 
{
	SetRunAnimSpeed(0);
	bool bFound = false;
	if(IsEngaged())
	{
		bFound = hate_list.RemoveEnt(mob);	
		if(hate_list.IsEmpty())
		{
			AI_Event_NoLongerEngaged();
			zone->DelAggroMob();
		}
	}
	if(GetTarget() == mob)
	{
		SetTarget(hate_list.GetTop(this));
	}

	return bFound;
}
void Mob::WipeHateList() 
{
	if(IsEngaged()) 
	{
		AI_Event_NoLongerEngaged();
	}
	hate_list.Wipe();
}

int32 Mob::RandomTimer(int min,int max) {
    int r = 14000;
	if(min != 0 && max != 0 && min < max)
	{
	    r = MakeRandomInt(min, max);
	}
	return r;
}

int32 NPC::GetEquipment(int8 material_slot) const
{
	if(material_slot > 8)
		return 0;

	return equipment[material_slot];
}

void Mob::SendWearChange(int8 material_slot)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = GetID();
	wc->material = GetEquipmentMaterial(material_slot);
	wc->elite_material = IsEliteMaterialItem(material_slot);
	wc->color.color = GetEquipmentColor(material_slot);
	wc->wear_slot_id = material_slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

void Mob::SendTextureWC(int8 slot, int16 texture)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = this->GetID();
	wc->material = texture;
	if (this->IsClient())
		wc->color.color = GetEquipmentColor(slot);
	else
		wc->color.color = this->GetArmorTint(slot);
	wc->wear_slot_id = slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

sint32 Mob::GetEquipmentMaterial(int8 material_slot) const
{
	const Item_Struct *item;
	
	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		if	// for primary and secondary we need the model, not the material
		(
			material_slot == MATERIAL_PRIMARY ||
			material_slot == MATERIAL_SECONDARY
		)
		{
			if(strlen(item->IDFile) > 2)
				return atoi(&item->IDFile[2]);
			else	//may as well try this, since were going to 0 anyways
				return item->Material;
		}
		else
		{
			return item->Material;
		}
	}

	return 0;
}

uint32 Mob::GetEquipmentColor(int8 material_slot) const
{
	const Item_Struct *item;
	
	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return item->Color;
	}

	return 0;
}

uint32 Mob::IsEliteMaterialItem(int8 material_slot) const
{
	const Item_Struct *item;
	
	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return item->EliteMaterial;
	}

	return 0;
}

// works just like a printf
void Mob::Say(const char *format, ...)
{
	char buf[1000];
	va_list ap;
	
	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);
	
	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_SAY, GetCleanName(), buf);
}

//
// solar: this is like the above, but the first parameter is a string id
//
void Mob::Say_StringID(int32 string_id, const char *message3, const char *message4, const char *message5, const char *message6, const char *message7, const char *message8, const char *message9)
{
	char string_id_str[10];
	
	snprintf(string_id_str, 10, "%d", string_id);

	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_STRINGID_SAY, GetCleanName(), string_id_str, message3, message4, message5,
		message6, message7, message8, message9
	);
}

void Mob::Shout(const char *format, ...)
{
	char buf[1000];
	va_list ap;
	
	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);
	
	entity_list.Message_StringID(this, false, MT_Shout,
		GENERIC_SHOUT, GetCleanName(), buf);
}

void Mob::Emote(const char *format, ...)
{
	char buf[1000];
	va_list ap;
	
	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);
	
	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_EMOTE, GetCleanName(), buf);
}

void Mob::QuestJournalledSay(Client *QuestInitiator, const char *str)
{
        entity_list.QuestJournalledSayClose(this, QuestInitiator, 200, GetCleanName(), str);
}

const char *Mob::GetCleanName()
{
	if(!strlen(clean_name))
	{
		CleanMobName(GetName(), clean_name);
	}

	return clean_name;
}

// hp event 
void Mob::SetNextHPEvent( int hpevent ) 
{ 
	nexthpevent = hpevent;
}

void Mob::SetNextIncHPEvent( int inchpevent ) 
{ 
	nextinchpevent = inchpevent;
}
//warp for quest function,from sandy
void Mob::Warp( float x, float y, float z ) 
{ 
   x_pos = x; 
   y_pos = y; 
   z_pos = z; 

   Mob* target = GetTarget(); 
   if ( target ) { 
      FaceTarget( target ); 
   } 

   SendPosition(); 

}

bool Mob::DivineAura() const
{
	uint32 l;
	uint32 buff_count = GetMaxTotalSlots();
	for (l = 0; l < buff_count; l++)
	{
		if (buffs[l].spellid != SPELL_UNKNOWN)
		{
			for (int k = 0; k < EFFECT_COUNT; k++)
			{
				if (spells[buffs[l].spellid].effectid[k] == SE_DivineAura)
				{
					return true;
				}
			}
		}
	}
	return false;
}



/*bool Mob::SeeInvisibleUndead()
{
	if (IsNPC() && CastToNPC()->Undead() && CastToNPC()->immunities[0] == 0)
		return false;
	return true;
}

bool Mob::SeeInvisible()
{
	if (IsNPC() && CastToNPC()->immunities[0] > 0)
		return true;
	int l;
	for (l = 0; l < BUFF_COUNT; l++)
	{
		if (buffs[l].spellid != SPELL_UNKNOWN)
		{
			for (int k = 0; k < EFFECT_COUNT; k++)
			{
				if (spells[buffs[l].spellid].effectid[k] == SE_SeeInvis)
				{
					return true;
				}
			}
		}
	}
	return false;
}*/

sint16 Mob::GetResist(int8 type) const
{
	if (IsNPC())
	{
		if (type == 1)
			return MR + spellbonuses.MR + itembonuses.MR;
		else if (type == 2)
			return FR + spellbonuses.FR + itembonuses.FR;
		else if (type == 3)
			return CR + spellbonuses.CR + itembonuses.CR;
		else if (type == 4)
			return PR + spellbonuses.PR + itembonuses.PR;
		else if (type == 5)
			return DR + spellbonuses.DR + itembonuses.DR;
	}
	else if (IsClient())
	{
		if (type == 1)
			return CastToClient()->GetMR();
		else if (type == 2)
			return CastToClient()->GetFR();
		else if (type == 3)
			return CastToClient()->GetCR();
		else if (type == 4)
			return CastToClient()->GetPR();
		else if (type == 5)
			return CastToClient()->GetDR();
	}
	return 25;
}

int32 Mob::GetLevelHP(int8 tlevel)
{
	//cout<<"Tlevel: "<<(int)tlevel<<endl;
	int multiplier = 0;
	if (tlevel < 10)
	{
		multiplier = tlevel*20;
	}
	else if (tlevel < 20)
	{
		multiplier = tlevel*25;
	}
	else if (tlevel < 40)
	{
		multiplier = tlevel*tlevel*12*((tlevel*2+60)/100)/10;
	}
	else if (tlevel < 45)
	{
		multiplier = tlevel*tlevel*15*((tlevel*2+60)/100)/10;
	}
	else if (tlevel < 50)
	{
		multiplier = tlevel*tlevel*175*((tlevel*2+60)/100)/100;
	}
	else
	{
		multiplier = tlevel*tlevel*2*((tlevel*2+60)/100)*(1+((tlevel-50)*20/10));
	}
	return multiplier;
}

sint32 Mob::GetActSpellCasttime(int16 spell_id, sint32 casttime) {
	if (level >= 60 && casttime > 1000)
	{
		casttime = casttime / 2;
		if (casttime < 1000)
			casttime = 1000;
	} else if (level >= 50 && casttime > 1000) {
		sint32 cast_deduction = (casttime*(level - 49))/5;
		if (cast_deduction > casttime/2)
			casttime /= 2;
		else
			casttime -= cast_deduction;
	}
	return(casttime);
}
void Mob::ExecWeaponProc(uint16 spell_id, Mob *on) {
	// Trumpcard: Changed proc targets to look up based on the spells goodEffect flag.
	// This should work for the majority of weapons.
	if(spell_id == SPELL_UNKNOWN)
		return;
	if ( IsBeneficialSpell(spell_id) )
		SpellFinished(spell_id, this, 10, 0);
	else if(!(on->IsClient() && on->CastToClient()->dead))	//dont proc on dead clients
		SpellFinished(spell_id, on, 10, 0);
}

int32 Mob::GetZoneID() const {
	return(zone->GetZoneID());
}

int Mob::GetHaste() {
	int h = spellbonuses.haste + spellbonuses.hastetype2 + itembonuses.haste;
	int cap = 0;
	int level = GetLevel();

	if(level < 30) { // Rogean: Are these caps correct? Will use for now.
		cap = 50;
	} else if(level < 50) {
		cap = 74;
	} else if(level < 55) {
		cap = 84;
	} else if(level < 60) {
		cap = 94;
	} else {
		cap = 100;
	}
	

	if(h > cap) h = cap;

	h += spellbonuses.hastetype3;
	h += ExtraHaste;	//GM granted haste.

	return(h); 
}

void Mob::SetTarget(Mob* mob) {
	if (target == mob) return;
	target = mob;
	entity_list.UpdateHoTT(this);
	if(IsNPC())
		parse->Event(EVENT_TARGET_CHANGE, this->GetNPCTypeID(), 0, this->CastToNPC(), mob);
	else if (IsClient())
		parse->Event(EVENT_TARGET_CHANGE, 0, "", (NPC*)NULL, this->CastToClient());

}

float Mob::FindGroundZ(float new_x, float new_y, float z_offset)
{
	float ret = -999999;
	if (zone->zonemap != 0)
	{
		NodeRef pnode = zone->zonemap->SeekNode( zone->zonemap->GetRoot(), new_x, new_y );
		if (pnode != NODE_NONE)
		{
			VERTEX me;
			me.x = new_x;
			me.y = new_y;
			me.z = z_pos+z_offset;
			VERTEX hit;
			FACE *onhit;
			float best_z = zone->zonemap->FindBestZ(pnode, me, &hit, &onhit);
			if (best_z != -999999)
			{
				ret = best_z;
			}
		}
	}
	return ret;
}

// Copy of above function that isn't protected to be exported to Perl::Mob
float Mob::GetGroundZ(float new_x, float new_y, float z_offset)
{
	float ret = -999999;
	if (zone->zonemap != 0)
	{
		NodeRef pnode = zone->zonemap->SeekNode( zone->zonemap->GetRoot(), new_x, new_y );
		if (pnode != NODE_NONE)
		{
			VERTEX me;
			me.x = new_x;
			me.y = new_y;
			me.z = z_pos+z_offset;
			VERTEX hit;
			FACE *onhit;
			float best_z = zone->zonemap->FindBestZ(pnode, me, &hit, &onhit);
			if (best_z != -999999)
			{
				ret = best_z;
			}
		}
	}
	return ret;
}

//helper function for npc AI; needs to be mob:: cause we need to be able to count buffs on other clients and npcs
int Mob::CountDispellableBuffs()
{
	int val = 0;
	uint32 buff_count = GetMaxTotalSlots();
	for(int x = 0; x < buff_count; x++)
	{
		if(!IsValidSpell(buffs[x].spellid))
			continue;

		if(buffs[x].diseasecounters || buffs[x].poisoncounters || buffs[x].cursecounters)
			continue;
		
		if(spells[buffs[x].spellid].goodEffect == 0)
			continue;

		if(buffs[x].spellid != SPELL_UNKNOWN &&	buffs[x].durationformula != DF_Permanent)
			val++;
	}
	return val;
}

// Returns the % that a mob is snared (as a positive value). -1 means not snared
int Mob::GetSnaredAmount()
{
	int worst_snare = -1;

	uint32 buff_count = GetMaxTotalSlots();
	for (int i = 0; i < buff_count; i++)
	{
		if (!IsValidSpell(buffs[i].spellid))
			continue;

		for(int j = 0; j < EFFECT_COUNT; j++)
		{
			if (spells[buffs[i].spellid].effectid[j] == SE_MovementSpeed)
			{
				int val = CalcSpellEffectValue_formula(spells[buffs[i].spellid].formula[j], spells[buffs[i].spellid].base[j], spells[buffs[i].spellid].max[j], buffs[i].casterlevel, buffs[i].spellid);
				//int effect = CalcSpellEffectValue(buffs[i].spellid, spells[buffs[i].spellid].effectid[j], buffs[i].casterlevel);
				if (val < 0 && abs(val) > worst_snare)
					worst_snare = abs(val);
			}
		}
	}

	return worst_snare;
}

void Mob::TriggerDefensiveProcs(Mob *on)
{
	if (this->HasDefensiveProcs()) {
		this->TryDefensiveProc(on);
	}

	return;
}

void Mob::SetDeltas(float dx, float dy, float dz, float dh) {
	delta_x = dx;
	delta_y = dy;
	delta_z = dz;
	delta_heading = dh;
}


bool Mob::HasBuffIcon(Mob *caster, Mob *target, int16 spell_id)
{
	if((caster->CalcBuffDuration(caster, target, spell_id)-1) > 0)
		return true;
	else
		return false;
}

void Mob::SetEntityVariable(int32 id, const char *m_var)
{
	if(!id)
		return;

	std::string n_m_var = m_var;
	m_EntityVariables[id] = n_m_var;
}

const char* Mob::GetEntityVariable(int32 id)
{
	if(!id)
		return NULL;

	std::map<int32, std::string>::iterator iter = m_EntityVariables.find(id);
	if(iter != m_EntityVariables.end())
	{
		return iter->second.c_str();
	}
	return NULL;
}

bool Mob::EntityVariableExists(int32 id)
{
	if(!id)
		return false;

	std::map<int32, std::string>::iterator iter = m_EntityVariables.find(id);
	if(iter != m_EntityVariables.end())
	{
		return true;
	}
	return false;
}

void Mob::SetFlyMode(int8 flymode)
{
	if(IsClient() && flymode >= 0 && flymode < 3)
	{
		this->SendAppearancePacket(AT_Levitate, flymode);
	}
	else if(IsNPC() && flymode >= 0 && flymode <= 3)
	{
		this->SendAppearancePacket(AT_Levitate, flymode);
		this->CastToNPC()->SetFlyMode(flymode);
	}
}

bool Mob::IsNimbusEffectActive(uint32 nimbus_effect)
{
	if(nimbus_effect1 == nimbus_effect || nimbus_effect2 == nimbus_effect || nimbus_effect3 == nimbus_effect)
	{
		return true;
	}
	return false;
}

void Mob::SetNimbusEffect(uint32 nimbus_effect)
{
	if(nimbus_effect1 == 0)
	{
		nimbus_effect1 = nimbus_effect;
	}
	else if(nimbus_effect2 == 0)
	{
		nimbus_effect2 = nimbus_effect;
	}
	else
	{
		nimbus_effect3 = nimbus_effect;
	}
}


void Mob::TryTriggerOnCast(Mob *target, uint32 spell_id)
{
	if(target == NULL || !IsValidSpell(spell_id))
	{
		return;
	}

	uint32 buff_count = GetMaxTotalSlots();
	for(int i = 0; i < buff_count; i++) 
	{
		if(IsEffectInSpell(buffs[i].spellid, SE_TriggerOnCast))
		{
			sint32 focus = CalcFocusEffect(focusTriggerOnCast, buffs[i].spellid, spell_id);
			if(focus == 1)
			{
				if(MakeRandomInt(0, 1000) < spells[buffs[i].spellid].base[0])
				{
					SpellOnTarget(spells[buffs[i].spellid].base2[0], target);
				}
			}
		}
	}
}

void Mob::TrySpellTrigger(Mob *target, uint32 spell_id)
{
	if(target == NULL || !IsValidSpell(spell_id))
	{
		return;
	}
	int spell_trig = 0;
	// Count all the percentage chances to trigger for all effects
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_SpellTrigger)
			spell_trig += spells[spell_id].base[i];
	}
	// If all the % add to 100, then only one of the effects can fire but one has to fire.
	if (spell_trig == 100)
	{
		int trig_chance = 100;
		for(int i = 0; i < EFFECT_COUNT; i++)
		{
			if (spells[spell_id].effectid[i] == SE_SpellTrigger)
			{
				if(MakeRandomInt(0, trig_chance) <= spells[spell_id].base[i]) 
				{
					// If we trigger an effect then its over.
					SpellOnTarget(spells[spell_id].base2[i], target);
					break;
				}
				else
				{
					// Increase the chance to fire for the next effect, if all effects fail, the final effect will fire.
					trig_chance -= spells[spell_id].base[i];
				}
			}
		
		}
	}
	// if the chances don't add to 100, then each effect gets a chance to fire, chance for no trigger as well.
	else
	{
		for(int i = 0; i < EFFECT_COUNT; i++)
		{
			if (spells[spell_id].effectid[i] == SE_SpellTrigger)
			{
				if(MakeRandomInt(0, 100) <= spells[spell_id].base[i]) 
				{
					SpellOnTarget(spells[spell_id].base2[i], target);
				}
			}
		}
	}
}

void Mob::TryApplyEffect(Mob *target, uint32 spell_id)
{
	if(target == NULL || !IsValidSpell(spell_id))
	{
		return;
	}

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if (spells[spell_id].effectid[i] == SE_ApplyEffect)
		{
			if(MakeRandomInt(0, 100) <= spells[spell_id].base[i]) 
			{
				SpellOnTarget(spells[spell_id].base2[i], target);
			}
		}
	}
}

void Mob::TryTwincast(Mob *caster, Mob *target, uint32 spell_id)
{
	if(!IsValidSpell(spell_id))
	{
		return;
	}

	uint32 buff_count = GetMaxTotalSlots();
	for(int i = 0; i < buff_count; i++) 
	{
		if(IsEffectInSpell(buffs[i].spellid, SE_Twincast))
		{
			sint32 focus = CalcFocusEffect(focusTwincast, buffs[i].spellid, spell_id);
			if(focus == 1)
			{
				if(MakeRandomInt(0, 100) <= spells[buffs[i].spellid].base[0])
				{
					uint32 mana_cost = (spells[spell_id].mana);
					if(this->IsClient())
					{
						mana_cost = GetActSpellCost(spell_id, mana_cost);
						this->Message(MT_Spells,"You twincast %s!",spells[spell_id].name);
					}
					this->SetMana(GetMana() - mana_cost);
					SpellOnTarget(spell_id, target);
				}
			}
		}
	}
}

sint32 Mob::GetVulnerability(sint32 damage, Mob *caster, uint32 spell_id, int32 ticsremaining)
{
	// If we increased the datatype on GetBuffSlotFromType, this wouldnt be needed
	uint32 buff_count = GetMaxTotalSlots();
	for(int i = 0; i < buff_count; i++) 
	{
		if(IsEffectInSpell(buffs[i].spellid, SE_SpellVulnerability))
		{
			// For Clients, Pets and Bots that are casting the spell, see if the vulnerability affects their spell.
			if(!caster->IsNPC())
			{
				sint32 focus = caster->CalcFocusEffect(focusSpellVulnerability, buffs[i].spellid, spell_id);
				if(focus == 1)
				{
					damage += damage * spells[buffs[i].spellid].base[0] / 100;
					break;
				}
			}
			// If an NPC is casting the spell on a player that has a vulnerability, relaxed restrictions on focus 
			// so that either the Client is vulnerable to DoTs or DDs of various resists or all.
			else if (caster->IsNPC())
			{
				int npc_resist = 0;
				int npc_instant = 0;
				int npc_duration = 0;
				for(int j = 0; j < EFFECT_COUNT; j++)
				{
					switch (spells[buffs[i].spellid].effectid[j]) 
					{
					
					case SE_Blank:
						break;

					case SE_LimitResist:
						if(spells[buffs[i].spellid].base[j])
						{
							if(spells[spell_id].resisttype == spells[buffs[i].spellid].base[j])
								npc_resist = 1;
						}
						break;

					case SE_LimitInstant:
						if(!ticsremaining) 
						{
							npc_instant = 1;
							break;
						}

					case SE_LimitMinDur:
						if(ticsremaining) 
						{
							npc_duration = 1;
							break;
						}

						default:{
						// look pretty
							break;
						}	
					}
				}
				// DDs and Dots of all resists 
				if ((npc_instant) || (npc_duration)) 
					damage += damage * spells[buffs[i].spellid].base[0] / 100;
				
				else if (npc_resist) 
				{
					// DDs and Dots restricted by resists
					if ((npc_instant) || (npc_duration)) 
					{
						damage += damage * spells[buffs[i].spellid].base[0] / 100;
					}
					// DD and Dots of 1 resist ... these are to maintain compatibility with current spells, not ideal.
					else if (!npc_instant && !npc_duration)
					{
						damage += damage * spells[buffs[i].spellid].base[0] / 100;
					}
				}
			}
		}
	}
	return damage;
}

sint32 Mob::GetSkillDmgTaken(const SkillType skill_used, sint32 damage)
{
	if (this->GetTarget())
	{
		int slot = this->GetTarget()->GetBuffSlotFromType(SE_SkillDamageTaken);
		if(slot >= 0)
		{
			int spell_id = this->GetTarget()->GetSpellIDFromSlot(slot);
			if (spell_id)
			{
				for(int i = 0; i < EFFECT_COUNT; i++)
				{
					if (spells[spell_id].effectid[i] == SE_SkillDamageTaken)
					{
						// Check the skill against the spell, or allow all melee skills.
						if(skill_used == spells[spell_id].base2[i] || spells[spell_id].base2[i] == -1)
						{
						damage += damage * spells[spell_id].base[i] / 100;
						return damage;
						}
					}
				}
			}
		}
	}
	return damage;
}

int32 Mob::GetHealRate(uint32 amount, Mob *target)
{

	if(target) {
		int slot = target->GetBuffSlotFromType(SE_HealRate);
		if(slot >= 0)
		{
			sint32 modify_amount = amount;
			for(int i = 0; i < EFFECT_COUNT; i++)
			{
				if (spells[buffs[slot].spellid].effectid[i] == SE_HealRate)
				{
					// if the effect reduces the heal amount below 0, return 0.
					if(spells[buffs[slot].spellid].base[i] < -100)
					{
						amount = 0;
						break;
					}
					else
					{
						amount += (modify_amount * spells[buffs[slot].spellid].base[i] / 100);
						break;
					}
				}
			}
		}
	}
	return amount;
}

bool Mob::TryFadeEffect(int slot)
{
	if(slot)
	{
		for(int i = 0; i < EFFECT_COUNT; i++)
		{
			if (spells[buffs[slot].spellid].effectid[i] == SE_CastOnWearoff || spells[buffs[slot].spellid].effectid[i] == SE_EffectOnFade)
			{
				int16 spell_id = spells[buffs[slot].spellid].base[i];
				BuffFadeBySlot(slot);
				if(spell_id)
				{
					ExecWeaponProc(spell_id, this);
					return true;
				}
			}
		}
	}
	return false;
}

void Mob::TrySympatheticProc(Mob *target, uint32 spell_id)
{
	if(target == NULL || !IsValidSpell(spell_id))
	{
		return;
	}
	
	int focus_spell = this->CastToClient()->GetFocusEffect(focusSympatheticProc,spell_id);
	if(focus_spell > 0)
	{
		int focus_trigger = spells[focus_spell].base2[0];
		// For beneficial spells, if the triggered spell is also beneficial then proc it on the target
		// if the triggered spell is detrimental, then it will trigger on the caster(ie cursed items)
		if(IsBeneficialSpell(spell_id))
		{
			if(IsBeneficialSpell(focus_trigger))
			{
				SpellOnTarget(focus_trigger, target);
			}
			else
			{
				SpellOnTarget(focus_trigger, this);
			}
		}
		// For detrimental spells, if the triggered spell is beneficial, then it will land on the caster
		// if the triggered spell is also detrimental, then it will land on the target
		else
		{
			if(IsBeneficialSpell(focus_trigger))
			{
				SpellOnTarget(focus_trigger, this);
			}
			else
			{
				SpellOnTarget(focus_trigger, target);
			}
		}
	}
}

int32 Mob::GetItemStat(int32 itemid, const char *identifier)
{
	const ItemInst* inst = database.CreateItem(itemid);
	if (!inst)
		return 0;

	const Item_Struct* item = inst->GetItem();
	if (!item)
		return 0;

	if (!identifier)
		return 0;

	int32 stat = 0;

	std::string id = identifier;
	for(int i = 0; i < id.length(); ++i)
	{
		id[i] = tolower(id[i]);
	}

	if (id == "itemclass")
		stat = int32(item->ItemClass);
	if (id == "id")
		stat = int32(item->ID);
	if (id == "weight")
		stat = int32(item->Weight);
	if (id == "norent")
		stat = int32(item->NoRent);
	if (id == "nodrop")
		stat = int32(item->NoDrop);
	if (id == "size")
		stat = int32(item->Size);
	if (id == "slots")
		stat = int32(item->Slots);
	if (id == "price")
		stat = int32(item->Price);
	if (id == "icon")
		stat = int32(item->Icon);
	if (id == "loregroup")
		stat = int32(item->LoreGroup);
	if (id == "loreflag")
		stat = int32(item->LoreFlag);
	if (id == "pendingloreflag")
		stat = int32(item->PendingLoreFlag);
	if (id == "artifactflag")
		stat = int32(item->ArtifactFlag);
	if (id == "summonedflag")
		stat = int32(item->SummonedFlag);
	if (id == "fvnodrop")
		stat = int32(item->FVNoDrop);
	if (id == "favor")
		stat = int32(item->Favor);
	if (id == "guildfavor")
		stat = int32(item->GuildFavor);
	if (id == "pointtype")
		stat = int32(item->PointType);
	if (id == "bagtype")
		stat = int32(item->BagType);
	if (id == "bagslots")
		stat = int32(item->BagSlots);
	if (id == "bagsize")
		stat = int32(item->BagSize);
	if (id == "bagwr")
		stat = int32(item->BagWR);
	if (id == "benefitflag")
		stat = int32(item->BenefitFlag);
	if (id == "tradeskills")
		stat = int32(item->Tradeskills);
	if (id == "cr")
		stat = int32(item->CR);
	if (id == "dr")
		stat = int32(item->DR);
	if (id == "pr")
		stat = int32(item->PR);
	if (id == "mr")
		stat = int32(item->MR);
	if (id == "fr")
		stat = int32(item->FR);
	if (id == "astr")
		stat = int32(item->AStr);
	if (id == "asta")
		stat = int32(item->ASta);
	if (id == "aagi")
		stat = int32(item->AAgi);
	if (id == "adex")
		stat = int32(item->ADex);
	if (id == "acha")
		stat = int32(item->ACha);
	if (id == "aint")
		stat = int32(item->AInt);
	if (id == "awis")
		stat = int32(item->AWis);
	if (id == "hp")
		stat = int32(item->HP);
	if (id == "mana")
		stat = int32(item->Mana);
	if (id == "ac")
		stat = int32(item->AC);
	if (id == "deity")
		stat = int32(item->Deity);
	if (id == "skillmodvalue")
		stat = int32(item->SkillModValue);
	if (id == "skillmodtype")
		stat = int32(item->SkillModType);
	if (id == "banedmgrace")
		stat = int32(item->BaneDmgRace);
	if (id == "banedmgamt")
		stat = int32(item->BaneDmgAmt);
	if (id == "banedmgbody")
		stat = int32(item->BaneDmgBody);
	if (id == "magic")
		stat = int32(item->Magic);
	if (id == "casttime_")
		stat = int32(item->CastTime_);
	if (id == "reqlevel")
		stat = int32(item->ReqLevel);
	if (id == "bardtype")
		stat = int32(item->BardType);
	if (id == "bardvalue")
		stat = int32(item->BardValue);
	if (id == "light")
		stat = int32(item->Light);
	if (id == "delay")
		stat = int32(item->Delay);
	if (id == "reclevel")
		stat = int32(item->RecLevel);
	if (id == "recskill")
		stat = int32(item->RecSkill);
	if (id == "elemdmgtype")
		stat = int32(item->ElemDmgType);
	if (id == "elemdmgamt")
		stat = int32(item->ElemDmgAmt);
	if (id == "range")
		stat = int32(item->Range);
	if (id == "damage")
		stat = int32(item->Damage);
	if (id == "color")
		stat = int32(item->Color);
	if (id == "classes")
		stat = int32(item->Classes);
	if (id == "races")
		stat = int32(item->Races);
	if (id == "maxcharges")
		stat = int32(item->MaxCharges);
	if (id == "itemtype")
		stat = int32(item->ItemType);
	if (id == "material")
		stat = int32(item->Material);
	if (id == "casttime")
		stat = int32(item->CastTime);
	if (id == "elitematerial")
		stat = int32(item->EliteMaterial);
	if (id == "procrate")
		stat = int32(item->ProcRate);
	if (id == "combateffects")
		stat = int32(item->CombatEffects);
	if (id == "shielding")
		stat = int32(item->Shielding);
	if (id == "stunresist")
		stat = int32(item->StunResist);
	if (id == "strikethrough")
		stat = int32(item->StrikeThrough);
	if (id == "extradmgskill")
		stat = int32(item->ExtraDmgSkill);
	if (id == "extradmgamt")
		stat = int32(item->ExtraDmgAmt);
	if (id == "spellshield")
		stat = int32(item->SpellShield);
	if (id == "avoidance")
		stat = int32(item->Avoidance);
	if (id == "accuracy")
		stat = int32(item->Accuracy);
	if (id == "charmfileid")
		stat = int32(item->CharmFileID);
	if (id == "factionmod1")
		stat = int32(item->FactionMod1);
	if (id == "factionmod2")
		stat = int32(item->FactionMod2);
	if (id == "factionmod3")
		stat = int32(item->FactionMod3);
	if (id == "factionmod4")
		stat = int32(item->FactionMod4);
	if (id == "factionamt1")
		stat = int32(item->FactionAmt1);
	if (id == "factionamt2")
		stat = int32(item->FactionAmt2);
	if (id == "factionamt3")
		stat = int32(item->FactionAmt3);
	if (id == "factionamt4")
		stat = int32(item->FactionAmt4);
	if (id == "augtype")
		stat = int32(item->AugType);
	if (id == "ldontheme")
		stat = int32(item->LDoNTheme);
	if (id == "ldonprice")
		stat = int32(item->LDoNPrice);
	if (id == "ldonsold")
		stat = int32(item->LDoNSold);
	if (id == "banedmgraceamt")
		stat = int32(item->BaneDmgRaceAmt);
	if (id == "augrestrict")
		stat = int32(item->AugRestrict);
	if (id == "endur")
		stat = int32(item->Endur);
	if (id == "dotshielding")
		stat = int32(item->DotShielding);
	if (id == "attack")
		stat = int32(item->Attack);
	if (id == "regen")
		stat = int32(item->Regen);
	if (id == "manaregen")
		stat = int32(item->ManaRegen);
	if (id == "enduranceregen")
		stat = int32(item->EnduranceRegen);
	if (id == "haste")
		stat = int32(item->Haste);
	if (id == "damageshield")
		stat = int32(item->DamageShield);
	if (id == "recastdelay")
		stat = int32(item->RecastDelay);
	if (id == "recasttype")
		stat = int32(item->RecastType);
	if (id == "augdistiller")
		stat = int32(item->AugDistiller);
	if (id == "attuneable")
		stat = int32(item->Attuneable);
	if (id == "nopet")
		stat = int32(item->NoPet);
	if (id == "potionbelt")
		stat = int32(item->PotionBelt);
	if (id == "stackable")
		stat = int32(item->Stackable);
	if (id == "notransfer")
		stat = int32(item->NoTransfer);
	if (id == "questitemflag")
		stat = int32(item->QuestItemFlag);
	if (id == "stacksize")
		stat = int32(item->StackSize);
	if (id == "potionbeltslots")
		stat = int32(item->PotionBeltSlots);
	if (id == "book")
		stat = int32(item->Book);
	if (id == "booktype")
		stat = int32(item->BookType);
	if (id == "svcorruption")
		stat = int32(item->SVCorruption);
	if (id == "purity")
		stat = int32(item->Purity);
	if (id == "backstabdmg")
		stat = int32(item->BackstabDmg);
	if (id == "dsmitigation")
		stat = int32(item->DSMitigation);
	if (id == "heroicstr")
		stat = int32(item->HeroicStr);
	if (id == "heroicint")
		stat = int32(item->HeroicInt);
	if (id == "heroicwis")
		stat = int32(item->HeroicWis);
	if (id == "heroicagi")
		stat = int32(item->HeroicAgi);
	if (id == "heroicdex")
		stat = int32(item->HeroicDex);
	if (id == "heroicsta")
		stat = int32(item->HeroicSta);
	if (id == "heroiccha")
		stat = int32(item->HeroicCha);
	if (id == "heroicmr")
		stat = int32(item->HeroicMR);
	if (id == "heroicfr")
		stat = int32(item->HeroicFR);
	if (id == "heroiccr")
		stat = int32(item->HeroicCR);
	if (id == "heroicdr")
		stat = int32(item->HeroicDR);
	if (id == "heroicpr")
		stat = int32(item->HeroicPR);
	if (id == "heroicsvcorrup")
		stat = int32(item->HeroicSVCorrup);
	if (id == "healamt")
		stat = int32(item->HealAmt);
	if (id == "spelldmg")
		stat = int32(item->SpellDmg);
	if (id == "ldonsellbackrate")
		stat = int32(item->LDoNSellBackRate);
	if (id == "scriptfileid")
		stat = int32(item->ScriptFileID);
	if (id == "expendablearrow")
		stat = int32(item->ExpendableArrow);
	if (id == "clairvoyance")
		stat = int32(item->Clairvoyance);
	// Begin Effects
	if (id == "clickeffect")
		stat = int32(item->Click.Effect);
	if (id == "clicktype")
		stat = int32(item->Click.Type);
	if (id == "clicklevel")
		stat = int32(item->Click.Level);
	if (id == "clicklevel2")
		stat = int32(item->Click.Level2);
	if (id == "proceffect")
		stat = int32(item->Proc.Effect);
	if (id == "proctype")
		stat = int32(item->Proc.Type);
	if (id == "proclevel")
		stat = int32(item->Proc.Level);
	if (id == "proclevel2")
		stat = int32(item->Proc.Level2);
	if (id == "worneffect")
		stat = int32(item->Worn.Effect);
	if (id == "worntype")
		stat = int32(item->Worn.Type);
	if (id == "wornlevel")
		stat = int32(item->Worn.Level);
	if (id == "wornlevel2")
		stat = int32(item->Worn.Level2);
	if (id == "focuseffect")
		stat = int32(item->Focus.Effect);
	if (id == "focustype")
		stat = int32(item->Focus.Type);
	if (id == "focuslevel")
		stat = int32(item->Focus.Level);
	if (id == "focuslevel2")
		stat = int32(item->Focus.Level2);
	if (id == "scrolleffect")
		stat = int32(item->Scroll.Effect);
	if (id == "scrolltype")
		stat = int32(item->Scroll.Type);
	if (id == "scrolllevel")
		stat = int32(item->Scroll.Level);
	if (id == "scrolllevel2")
		stat = int32(item->Scroll.Level2);

	safe_delete(inst);
	return stat;
}
