/*
 * This file was generated automatically by xsubpp version 1.9508 from the
 * contents of tmp. Do not edit this file, edit tmp instead.
 *
 *		ANY CHANGES MADE HERE WILL BE LOST!
 *
 */


/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2004  EQEMu Development Team (http://eqemulator.net)

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

#include "features.h"
#ifdef EMBPERL_XS_CLASSES
#include "../common/debug.h"
#include "embperl.h"

#include "PlayerCorpse.h"

#ifdef THIS	 /* this macro seems to leak out on some systems */
#undef THIS		
#endif


XS(XS_Corpse_GetCharID); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetCharID)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetCharID(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetCharID();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_GetDecayTime); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetDecayTime)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetDecayTime(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetDecayTime();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_Lock); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_Lock)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::Lock(THIS)");
	{
		Corpse *		THIS;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->Lock();
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_UnLock); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_UnLock)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::UnLock(THIS)");
	{
		Corpse *		THIS;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->UnLock();
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_IsLocked); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_IsLocked)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::IsLocked(THIS)");
	{
		Corpse *		THIS;
		bool		RETVAL;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->IsLocked();
		ST(0) = boolSV(RETVAL);
		sv_2mortal(ST(0));
	}
	XSRETURN(1);
}

XS(XS_Corpse_ResetLooter); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_ResetLooter)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::ResetLooter(THIS)");
	{
		Corpse *		THIS;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->ResetLooter();
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_GetDBID); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetDBID)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetDBID(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetDBID();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_GetOwnerName); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetOwnerName)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetOwnerName(THIS)");
	{
		Corpse *		THIS;
		char *		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetOwnerName();
		sv_setpv(TARG, RETVAL); XSprePUSH; PUSHTARG;
	}
	XSRETURN(1);
}

XS(XS_Corpse_SetDecayTimer); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_SetDecayTimer)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: Corpse::SetDecayTimer(THIS, decaytime)");
	{
		Corpse *		THIS;
		uint32		decaytime = (uint32)SvUV(ST(1));

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->SetDecayTimer(decaytime);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_IsEmpty); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_IsEmpty)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::IsEmpty(THIS)");
	{
		Corpse *		THIS;
		bool		RETVAL;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->IsEmpty();
		ST(0) = boolSV(RETVAL);
		sv_2mortal(ST(0));
	}
	XSRETURN(1);
}

XS(XS_Corpse_AddItem); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_AddItem)
{
	dXSARGS;
	if (items < 3 || items > 4)
		Perl_croak(aTHX_ "Usage: Corpse::AddItem(THIS, itemnum, charges, slot= 0)");
	{
		Corpse *		THIS;
		uint32		itemnum = (uint32)SvUV(ST(1));
		uint16		charges = (uint16)SvUV(ST(2));
		int16		slot;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		if (items < 4)
			slot = 0;
		else {
			slot = (int16)SvIV(ST(3));
		}

		THIS->AddItem(itemnum, charges, slot);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_GetWornItem); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetWornItem)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: Corpse::GetWornItem(THIS, equipSlot)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;
		int16		equipSlot = (int16)SvIV(ST(1));

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetWornItem(equipSlot);
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_RemoveItem); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_RemoveItem)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: Corpse::RemoveItem(THIS, lootslot)");
	{
		Corpse *		THIS;
		uint16		lootslot = (uint16)SvUV(ST(1));

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->RemoveItem(lootslot);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_SetCash); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_SetCash)
{
	dXSARGS;
	if (items != 5)
		Perl_croak(aTHX_ "Usage: Corpse::SetCash(THIS, in_copper, in_silver, in_gold, in_platinum)");
	{
		Corpse *		THIS;
		uint16		in_copper = (uint16)SvUV(ST(1));
		uint16		in_silver = (uint16)SvUV(ST(2));
		uint16		in_gold = (uint16)SvUV(ST(3));
		uint16		in_platinum = (uint16)SvUV(ST(4));

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->SetCash(in_copper, in_silver, in_gold, in_platinum);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_RemoveCash); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_RemoveCash)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::RemoveCash(THIS)");
	{
		Corpse *		THIS;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->RemoveCash();
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_CountItems); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_CountItems)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::CountItems(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->CountItems();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_Delete); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_Delete)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::Delete(THIS)");
	{
		Corpse *		THIS;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->Delete();
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_GetCopper); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetCopper)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetCopper(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetCopper();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_GetSilver); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetSilver)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetSilver(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetSilver();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_GetGold); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetGold)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetGold(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetGold();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_GetPlatinum); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_GetPlatinum)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::GetPlatinum(THIS)");
	{
		Corpse *		THIS;
		uint32		RETVAL;
		dXSTARG;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->GetPlatinum();
		XSprePUSH; PUSHu((UV)RETVAL);
	}
	XSRETURN(1);
}

XS(XS_Corpse_Summon); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_Summon)
{
	dXSARGS;
	if (items != 3)
		Perl_croak(aTHX_ "Usage: Corpse::Summon(THIS, client, spell)");
	{
		Corpse *		THIS;
		Client*		client;
		bool		spell = (bool)SvTRUE(ST(2));

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		if (sv_derived_from(ST(1), "Client")) {
			IV tmp = SvIV((SV*)SvRV(ST(1)));
			client = INT2PTR(Client *,tmp);
		}
		else
			Perl_croak(aTHX_ "client is not of type Client");
		if(client == NULL)
			Perl_croak(aTHX_ "client is NULL, avoiding crash.");

		THIS->Summon(client, spell, true);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_CastRezz); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_CastRezz)
{
	dXSARGS;
	if (items != 3)
		Perl_croak(aTHX_ "Usage: Corpse::CastRezz(THIS, spellid, Caster)");
	{
		Corpse *		THIS;
		uint16		spellid = (uint16)SvUV(ST(1));
		Mob*		Caster;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		if (sv_derived_from(ST(2), "Mob")) {
			IV tmp = SvIV((SV*)SvRV(ST(2)));
			Caster = INT2PTR(Mob *,tmp);
		}
		else
			Perl_croak(aTHX_ "Caster is not of type Mob");
		if(Caster == NULL)
			Perl_croak(aTHX_ "Caster is NULL, avoiding crash.");

		THIS->CastRezz(spellid, Caster);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_CompleteRezz); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_CompleteRezz)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::CompleteRezz(THIS)");
	{
		Corpse *		THIS;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		THIS->CompleteRezz();
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_CanMobLoot); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_CanMobLoot)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: Corpse::CanMobLoot(THIS, charid)");
	{
		Corpse *		THIS;
		bool		RETVAL;
		int		charid = (int)SvIV(ST(1));

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->CanMobLoot(charid);
		ST(0) = boolSV(RETVAL);
		sv_2mortal(ST(0));
	}
	XSRETURN(1);
}

XS(XS_Corpse_AllowMobLoot); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_AllowMobLoot)
{
	dXSARGS;
	if (items != 3)
		Perl_croak(aTHX_ "Usage: Corpse::AllowMobLoot(THIS, them, slot)");
	{
		Corpse *		THIS;
		Mob *		them;
		uint8		slot = (uint8)SvUV(ST(2));

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		if (sv_derived_from(ST(1), "Mob")) {
			IV tmp = SvIV((SV*)SvRV(ST(1)));
			them = INT2PTR(Mob *,tmp);
		}
		else
			Perl_croak(aTHX_ "them is not of type Mob");
		if(them == NULL)
			Perl_croak(aTHX_ "them is NULL, avoiding crash.");

		THIS->AllowMobLoot(them, slot);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_AddLooter); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_AddLooter)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: Corpse::AddLooter(THIS, who)");
	{
		Corpse *		THIS;
		Mob *		who;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		if (sv_derived_from(ST(1), "Mob")) {
			IV tmp = SvIV((SV*)SvRV(ST(1)));
			who = INT2PTR(Mob *,tmp);
		}
		else
			Perl_croak(aTHX_ "who is not of type Mob");
		if(who == NULL)
			Perl_croak(aTHX_ "who is NULL, avoiding crash.");

		THIS->AddLooter(who);
	}
	XSRETURN_EMPTY;
}

XS(XS_Corpse_IsRezzed); /* prototype to pass -Wmissing-prototypes */
XS(XS_Corpse_IsRezzed)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Corpse::IsRezzed(THIS)");
	{
		Corpse *		THIS;
		bool		RETVAL;

		if (sv_derived_from(ST(0), "Corpse")) {
			IV tmp = SvIV((SV*)SvRV(ST(0)));
			THIS = INT2PTR(Corpse *,tmp);
		}
		else
			Perl_croak(aTHX_ "THIS is not of type Corpse");
		if(THIS == NULL)
			Perl_croak(aTHX_ "THIS is NULL, avoiding crash.");

		RETVAL = THIS->Rezzed();
		ST(0) = boolSV(RETVAL);
		sv_2mortal(ST(0));
	}
	XSRETURN(1);
}

#ifdef __cplusplus
extern "C"
#endif
XS(boot_Corpse); /* prototype to pass -Wmissing-prototypes */
XS(boot_Corpse)
{
	dXSARGS;
	char file[256];
	strncpy(file, __FILE__, 256);
	file[255] = 0;
	
	if(items != 1)
		fprintf(stderr, "boot_quest does not take any arguments.");
	char buf[128];

	//add the strcpy stuff to get rid of const warnings....



	XS_VERSION_BOOTCHECK ;

		newXSproto(strcpy(buf, "GetCharID"), XS_Corpse_GetCharID, file, "$");
		newXSproto(strcpy(buf, "GetDecayTime"), XS_Corpse_GetDecayTime, file, "$");
		newXSproto(strcpy(buf, "Lock"), XS_Corpse_Lock, file, "$");
		newXSproto(strcpy(buf, "UnLock"), XS_Corpse_UnLock, file, "$");
		newXSproto(strcpy(buf, "IsLocked"), XS_Corpse_IsLocked, file, "$");
		newXSproto(strcpy(buf, "ResetLooter"), XS_Corpse_ResetLooter, file, "$");
		newXSproto(strcpy(buf, "GetDBID"), XS_Corpse_GetDBID, file, "$");
		newXSproto(strcpy(buf, "GetOwnerName"), XS_Corpse_GetOwnerName, file, "$");
		newXSproto(strcpy(buf, "SetDecayTimer"), XS_Corpse_SetDecayTimer, file, "$$");
		newXSproto(strcpy(buf, "IsEmpty"), XS_Corpse_IsEmpty, file, "$");
		newXSproto(strcpy(buf, "AddItem"), XS_Corpse_AddItem, file, "$$$;$");
		newXSproto(strcpy(buf, "GetWornItem"), XS_Corpse_GetWornItem, file, "$$");
		newXSproto(strcpy(buf, "RemoveItem"), XS_Corpse_RemoveItem, file, "$$");
		newXSproto(strcpy(buf, "SetCash"), XS_Corpse_SetCash, file, "$$$$$");
		newXSproto(strcpy(buf, "RemoveCash"), XS_Corpse_RemoveCash, file, "$");
		newXSproto(strcpy(buf, "CountItems"), XS_Corpse_CountItems, file, "$");
		newXSproto(strcpy(buf, "Delete"), XS_Corpse_Delete, file, "$");
		newXSproto(strcpy(buf, "GetCopper"), XS_Corpse_GetCopper, file, "$");
		newXSproto(strcpy(buf, "GetSilver"), XS_Corpse_GetSilver, file, "$");
		newXSproto(strcpy(buf, "GetGold"), XS_Corpse_GetGold, file, "$");
		newXSproto(strcpy(buf, "GetPlatinum"), XS_Corpse_GetPlatinum, file, "$");
		newXSproto(strcpy(buf, "Summon"), XS_Corpse_Summon, file, "$$$");
		newXSproto(strcpy(buf, "CastRezz"), XS_Corpse_CastRezz, file, "$$$");
		newXSproto(strcpy(buf, "CompleteRezz"), XS_Corpse_CompleteRezz, file, "$");
		newXSproto(strcpy(buf, "CanMobLoot"), XS_Corpse_CanMobLoot, file, "$$");
		newXSproto(strcpy(buf, "AllowMobLoot"), XS_Corpse_AllowMobLoot, file, "$$$");
		newXSproto(strcpy(buf, "AddLooter"), XS_Corpse_AddLooter, file, "$$");
		newXSproto(strcpy(buf, "IsRezzed"), XS_Corpse_IsRezzed, file, "$");
	XSRETURN_YES;
}

#endif //EMBPERL_XS_CLASSES

