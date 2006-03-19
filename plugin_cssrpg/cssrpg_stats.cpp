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
#include <math.h>

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
#include "cssrpg_stats.h"
#include "cssrpg_database.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*	//////////////////////////////////////
	CRPG_StatsManager Class
	////////////////////////////////////// */
/* Calculate the experience needed for this level */
unsigned int CRPG_StatsManager::LvltoExp(unsigned int lvl) {
	unsigned int exp;

	if(lvl <= 1)
		exp =  exp_start;
	else
		exp = (lvl*exp_inc)+exp_start;

	return exp > exp_max ? exp_max : exp;
}

/* Calculate how many levels to increase by current level and experience */
unsigned int CRPG_StatsManager::calc_lvl_inc(unsigned int lvl, unsigned int exp) {
	unsigned int lvl_inc = 0, exp_req;

	while(1) {
		exp_req = LvltoExp(lvl+lvl_inc);
		if(exp > exp_req) {
			lvl_inc++;
			exp -= exp_req;
			continue;
		}
		else if(exp == exp_req) {
			lvl_inc++;
		}
		break;
	}

	return lvl_inc;
}

float CRPG_StatsManager::team_ratio(enum cssteam_t numerator) {
	unsigned int i = CRPG_Player::player_count;
	float teamratio, ct_count = 0.0, t_count = 0.0;

	if(numerator == team_none)
		return 0.0;

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			if(CRPG_Player::players[i]->css.team == team_ct)
				ct_count++;
			else if(CRPG_Player::players[i]->css.team == team_t)
				t_count++;
		}
	}

	if(numerator == team_t)
		teamratio = t_count/ct_count;
	else
		teamratio = ct_count/t_count;

	return teamratio;
}

void CRPG_StatsManager::player_new_lvl(CRPG_Player *player, unsigned int lvl_inc) {
	if(!lvl_inc) {
		CRPG::ConsoleMsg("player_new_lvl: lvl_inc = 0", MTYPE_WARNING);
		return ;
	}

	player->level += lvl_inc;
	player->exp = 0;
	player->credits += lvl_inc*credits_inc;

	CRPG::DebugMsg("%s is now level %d (%d level increase(s))", player->name(), player->level, lvl_inc);

	if(announce_newlvl)
		CRPG::ChatAreaMsg(0, "%s is now Level %d", player->name(), player->level);

	if(!player->info()->IsFakeClient()) {
		CRPG::EmitSound(player->index, "buttons/blip2.wav");
		if((player->level-lvl_inc) <= 1) {
			/* for newbies */
			CRPG::ChatAreaMsg(player->index, "\x04You have gained a new Level! This means you can buy Upgrades which give you an advantage over your opponents.\x01");
			CRPG::ChatAreaMsg(player->index, "\x04Type \"\x03rpgmenu\x04\" in chat, or type it into the console to bring up a menu from which you can buy Upgrades.\x01");
		}
		else {
			CRPG::ChatAreaMsg(player->index, "You have new credits (%ld total). Type \"rpgmenu\" to buy upgrades.", player->credits);
		}
	}
	else {
		CRPG_Bot::PickUpgrade(player);
	}

	return ;
}

void CRPG_StatsManager::add_exp(CRPG_Player *player, unsigned long exp) {
	unsigned int exp_req;

	if(player == NULL)
		return ;

	IF_BOT_NENABLED(player)
		return ;

	exp_req = LvltoExp(player->level);
	player->exp += exp;

	if(exp_notice)
		CRPG::ChatAreaMsg(player->index, "XP Gained: %ld", exp);

	if(player->exp >= exp_req)
		player_new_lvl(player, calc_lvl_inc(player->level, player->exp));

	return ;
}

void CRPG_StatsManager::PlayerDamage(int attacker, const char *weapon, int dmg_health, int dmg_armor) {
	unsigned int total_dmg = (unsigned int)(dmg_health+dmg_armor);
	unsigned int exp;

	if(!enable)
		return ;

	if(attacker < 1)
		return ;

	if(CRPG::istrcmp((char*)weapon, "knife"))
		exp = (unsigned int)ceil((float)total_dmg*(exp_knifedmg > exp_damage ? exp_knifedmg : exp_damage));
	else
		exp = (unsigned int)ceil((float)total_dmg*exp_damage);

	add_exp(UserIDtoRPGPlayer(attacker), exp);
	return ;
}

void CRPG_StatsManager::PlayerKill(int attacker, int victim, bool headshot) {
	CRPG_Player *a_player, *v_player;
	unsigned int exp;

	if(!enable)
		return ;

	if((attacker < 1) || (victim < 1))
		return ;

	if(victim == attacker)
		return ;

	a_player = UserIDtoRPGPlayer(attacker);
	v_player = UserIDtoRPGPlayer(victim);

	if(a_player == NULL || v_player == NULL)
		return ;

	if(a_player->css.team == v_player->css.team)
		return ;

	exp = (unsigned int)(v_player->level*exp_kill);
	if(headshot)
		exp += exp_headshot;

	add_exp(a_player, exp);
	return ;
}

void CRPG_StatsManager::BombPlanted(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	if(player == NULL)
		return ;

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil(teamratio*exp_bombplanted));

	return ;
}

void CRPG_StatsManager::BombDefused(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	if(player == NULL)
		return ;

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil(teamratio*exp_bombdefused));

	return ;
}

void CRPG_StatsManager::BombExploded(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	if(player == NULL)
		return ;

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil(teamratio*exp_bombexploded));

	return ;
}

void CRPG_StatsManager::HostageRescued(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	if(player == NULL)
		return ;

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil(teamratio*exp_hostage));

	return ;
}

void CRPG_StatsManager::VipEscaped(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	if(player == NULL)
		return ;

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil(teamratio*exp_vipescaped));

	return ;
}

/* Experience given to the team for one of these reasons:
	1   Target Successfully Bombed!
	2   The VIP has escaped!
	3   VIP has been assassinated!
	7   The bomb has been defused!
	11   All Hostages have been rescued!
	12   Target has been saved!
	13   Hostages have not been rescued!
*/
void CRPG_StatsManager::WinningTeam(int team, int reason) {
	CRPG_Player *player;
	unsigned int i = CRPG_Player::player_count, exp = 0;
	cssteam_t winningteam;
	float teamratio;

	if(!enable)
		return ;

	if(team == 2) {
		winningteam = team_t;
		teamratio = team_ratio(team_ct);
	}
	else if(team == 3) {
		winningteam = team_ct;
		teamratio = team_ratio(team_t);
	}
	else {
		return ;
	}

	switch(reason) {
		case 1:
		case 2:
		case 3:
		case 7:
		case 11:
		case 12:
		case 13:
			exp = (unsigned int)ceil(teamratio*exp_teamwin);
			break;

		default:
			return ;
	}

	i = CRPG_Player::player_count;
	while(i--) {
		player = CRPG_Player::players[i];
		if(player != NULL) {
			if(player->css.team == winningteam)
				add_exp(player, exp);
		}
	}

	return ;
}

/*	//////////////////////////////////////
	CRPG_RankManager
	////////////////////////////////////// */
unsigned int CRPG_RankManager::GetPlayerRank(CRPG_Player *player) {
	struct tbl_result *result;
	int retval, rank;

	if(player == NULL)
		return 0;

	retval = CRPG::db->Query(&result, "SELECT COUNT(*) AS c FROM %s WHERE level > '%d' OR (level == '%d' AND exp > '%d')",
		TBL_PLAYERS, player->level, player->level, player->exp);

	if(!retval || (result == NULL))
		return 0;

	rank = atoi(GetCell(result, "c"))+1; // +1 since the query returns the count, not the rank
	FreeResult(result);

	return rank;
}

unsigned int CRPG_RankManager::GetRankCount(void) {
	unsigned int i = CRPG_Player::player_count;
	int count = CRPG::db->RowCount(TBL_PLAYERS);

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			if(CRPG_Player::players[i]->dbinfo.player_id < 0) 
				count++; /* accounts for players not saved in the db */
		}
	}

	if(count > 0)
		return count;

	return 0;
}
