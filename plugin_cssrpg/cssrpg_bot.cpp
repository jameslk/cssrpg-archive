/*
# Copyright (c) 2005-2006 CSS:RPG Mod for Counter-Strike Source(TM) Developement Team
#
# zlib/libpng License
# This software is provided 'as-is', without any express or implied warranty. In no event
# will the authors be held liable for any damages arising from the use of this software.
# Permission is granted to anyone to use this software for any purpose, including
# commercial applications, and to alter it and redistribute it freely, subject to the
# following restrictions:
#   1. The origin of this software must not be misrepresented; you must not claim that
#      you wrote the original software. If you use this software in a product, an
#      acknowledgment in the product documentation would be appreciated but is not required.
#   2. Altered source versions must be plainly marked as such, and must not be
#      misrepresented as being the original software.
#   3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#include "cssrpg.h"
#include "cssrpg_bot.h"

#include "items/rpgi.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CRPG_Bot::PickUpgrade(CRPG_Player *bot) {
	unsigned int items_bought, i, cost, credits = bot->credits;

	while(credits) {
		items_bought = 0;
		for(i = 0;i < ITEM_COUNT;i++) {
			if(!CRPG::item_types[i].enable)
				continue;

			cost = CRPGI::GetItemCost(i, bot->items[i].level+1);
			if(bot->credits >= cost) {
				bot->BuyItem(i);
			}
		}
		if(!items_bought)
			break; /* Couldn't afford anything */
	}

	return ;
}