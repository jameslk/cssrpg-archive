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
#include <string.h>
#include <ctype.h>

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "engine/IEngineSound.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#define GAME_DLL 1
#include "cbase.h"
#define GAME_DLL 1

#include "MRecipientFilter.h"

#include "cssrpg.h"
#include "cssrpg_menu.h"
#include "cssrpg_misc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IVEngineServer *engine;
extern IPlayerInfoManager *playerinfomanager;
extern CGlobalVars *gpGlobals;
extern IEngineSound *esounds;
extern IFileSystem *filesystem;
extern IServerGameDLL *gamedll;

/*	//////////////////////////////////////
	CRPG_Utils Class
	////////////////////////////////////// */
int CRPG_Utils::saytext = -1;
int CRPG_Utils::hinttext = -1;
int CRPG_Utils::vguimenu = -1;

int CRPG_Utils::maxClients(void) {
	return gpGlobals->maxClients;
}

int CRPG_Utils::currentClients(void) {
	return engine->GetEntityCount();
}

IVEngineServer* CRPG_Utils::s_engine(void) {
	return engine;
}

IPlayerInfoManager* CRPG_Utils::s_playerinfo(void) {
	return playerinfomanager;
}

CGlobalVars* CRPG_Utils::s_globals(void) {
	return gpGlobals;
}

IFileSystem* CRPG_Utils::s_filesys(void) {
	return filesystem;
}

unsigned int CRPG_Utils::IsValidEdict(edict_t *e) {
	if(e == NULL)
		return 0;

	if(engine->GetPlayerUserId(e) < 0)
		return 0;
	else
		return 1;
}

unsigned int CRPG_Utils::IsValidIndex(int index) {
	edict_t *e;

	if((index < 0) || (index > maxClients()))
		return 0;

	e = engine->PEntityOfEntIndex(index);
	if(engine->GetPlayerUserId(e) < 0)
		return 0;
	else
		return 1;
}

int CRPG_Utils::UserIDtoIndex(int userid) {
	edict_t *player;
	IPlayerInfo *info;

	for(int i = 1;i <= maxClients();i++) {  //int maxplayers; has to be added after the includes and clientMax=maxplayers; in the ServerActivate function
		player = engine->PEntityOfEntIndex(i);
		if(!player || player->IsFree())
			continue;

		info = playerinfomanager->GetPlayerInfo(player);
		if(info == NULL)
			continue;
 
		if(info->GetUserID() == userid)
			return i;
	}
 
	ConsoleMsg("UserIDtoIndex failed", MTYPE_WARNING);
	return -1;
}

edict_t* CRPG_Utils::UserIDtoEdict(int userid) {
	edict_t *player;
	IPlayerInfo *info;
	int i;

	for(i = 1;i <= maxClients();i++) {  //int maxplayers; has to be added after the includes and clientMax=maxplayers; in the ServerActivate function
		player = engine->PEntityOfEntIndex(i);
		if(!player || player->IsFree())
			continue;

		info = playerinfomanager->GetPlayerInfo(player);
		if(info == NULL)
			continue;
 
		if(info->GetUserID() == userid)
			return player;
	}
 
	ConsoleMsg("UserIDtoEdict failed", MTYPE_WARNING);
	return NULL;
}

edict_t* CRPG_Utils::IndextoEdict(int index) {
	return engine->PEntityOfEntIndex(index);
}

int CRPG_Utils::IndextoUserID(int index) {
	return engine->GetPlayerUserId(engine->PEntityOfEntIndex(index));
}

int CRPG_Utils::EdicttoIndex(edict_t *e) {
	return engine->IndexOfEdict(e);
}

int CRPG_Utils::EdicttoUserid(edict_t *e) {
	return engine->GetPlayerUserId(e);
}

IPlayerInfo* CRPG_Utils::EdicttoPlayerInfo(edict_t *e) {
	return playerinfomanager->GetPlayerInfo(e);
}

const char* CRPG_Utils::EdicttoSteamID(edict_t *e) {
	IPlayerInfo *info = playerinfomanager->GetPlayerInfo(e);

	return info->GetNetworkIDString();
}

int CRPG_Utils::UserMessageIndex(char *name) {
	char cmpname[256];
	int i, sizereturn = 0;

	for(i = 1;i < 30;i++) {
		gamedll->GetUserMessageInfo(i, cmpname, 256, sizereturn);
		if(name && !strcmp(name, cmpname))
			return i;
	}

	return -1;
}

int CRPG_Utils::FindPlayer(char *str) {
	int userid = atoi(str), max = maxClients(), index = -1;
	edict_t *player;
	IPlayerInfo *info;
	int i;

	if(!userid) {
		/* Search for player by name */
		for(i = 1;i <= max;i++) {
			player = engine->PEntityOfEntIndex(i);
			if(!player || player->IsFree())
				continue;

			info = playerinfomanager->GetPlayerInfo(player);
			if(info == NULL)
				continue;

			if(CRPG_Utils::istrcmp(str, (char*)info->GetNetworkIDString()))
				return engine->IndexOfEdict(player);
			else if(CRPG_Utils::istrstr((char*)info->GetName(), str) != NULL)
				return engine->IndexOfEdict(player);
		}
	}
	else {
		/* Search for player by userid */
		for(i = 1;i <= max;i++) {
			player = engine->PEntityOfEntIndex(i);
			if(!player || player->IsFree())
				continue;
	 
			info = playerinfomanager->GetPlayerInfo(player);
			if(info == NULL)
				continue;

			if(info->GetUserID() == userid)
				return engine->IndexOfEdict(player);
		}
	}

	return index;
}

void CRPG_Utils::ChatAreaMsg(int index, char *msgf, ...) {
	MRecipientFilter filter;
	char msg[1024];
	bf_write *buffer;
	va_list ap;

	if (index > maxClients() || index < 0)
		return ;

	if (!index)
		filter.AddAllPlayers(maxClients());
	else
		filter.AddRecipient(index);

	va_start(ap, msgf);
	Q_vsnprintf(msg, sizeof(msg)-1, msgf, ap);
	va_end(ap);

	sprintf(msg, "%s\n", msg);

	buffer = engine->UserMessageBegin(static_cast<IRecipientFilter*>(&filter), 3);
	buffer->WriteByte(0);
	buffer->WriteString(msg);
	buffer->WriteByte(0);
	engine->MessageEnd();

	return ;
}

void CRPG_Utils::HintTextMsg(int index, char *msgf, ...) {
	MRecipientFilter filter;
	char msg[1024];
	bf_write *buffer;
	va_list ap;

	if (index > maxClients() || index < 0)
		return ;

	if (!index)
		filter.AddAllPlayers(maxClients());
	else
		filter.AddRecipient(index);

	va_start(ap, msgf);
	Q_vsnprintf(msg, sizeof(msg)-1, msgf, ap);
	va_end(ap);

	sprintf(msg, "%s", msg);

	buffer = engine->UserMessageBegin(static_cast<IRecipientFilter*>(&filter), hinttext);
	buffer->WriteByte(-1);
	buffer->WriteString(msg);
	engine->MessageEnd();

	return ;
}

void CRPG_Utils::EmitSound(int index, char *sound_path) {
	MRecipientFilter filter;

	if (index > maxClients() || index < 0)
		return ;

	if (!index)
		filter.AddAllPlayers(maxClients());
	else
		filter.AddRecipient(index);

	if(!esounds->IsSoundPrecached(sound_path))
		esounds->PrecacheSound(sound_path, true);

	esounds->EmitSound((IRecipientFilter&)filter, index, CHAN_AUTO, sound_path, 0.7, 0);
	return ;
}

void CRPG_Utils::ShowMOTD(int index, char *title, char *msg, motd_type type, char *cmd) {
	bf_write *buffer;
    MRecipientFilter filter;

	if(index > maxClients() || index < 0)
		filter.AddAllPlayers(maxClients());
	else
		filter.AddRecipient(index);

	buffer = engine->UserMessageBegin(&filter, vguimenu);
	buffer->WriteString("info");
	buffer->WriteByte(1);

	if(cmd != NULL)
		buffer->WriteByte(4);
	else
		buffer->WriteByte(3);

	buffer->WriteString("title");
	buffer->WriteString(title);

	buffer->WriteString("type");
	switch(type) {
		case motd_text:
			buffer->WriteString("0"); //TYPE_TEXT = 0,   just display this plain text
			break;

		case motd_index:
			buffer->WriteString("1"); //TYPE_INDEX,      lookup text & title in stringtable
			break;

		case motd_url:
			buffer->WriteString("2"); //TYPE_URL,        show this URL
			break;

		case motd_file:
			buffer->WriteString("3"); //TYPE_FILE,       show this local file
			break;
	}

	buffer->WriteString("msg");
	buffer->WriteString(msg);

	if(cmd != NULL) {
		buffer->WriteString("cmd");
		buffer->WriteString(cmd); // exec this command if panel closed
	}

	engine->MessageEnd();
	return ;
}

void CRPG_Utils::ConsoleMsg(char *msgf, char *msg_type, ...) {
	char msg[1024];
	va_list ap;

	va_start(ap, msg_type);
	Q_vsnprintf(msg, sizeof(msg)-1, msgf, ap);
	va_end(ap);

	if(msg_type == NULL)
		Msg("CSS:RPG Plugin: %s\n", msg);
	else
		Msg("CSS:RPG %s: %s\n", msg_type, msg);

	return ;
}

void CRPG_Utils::DebugMsg(char *msgf, ...) {
	char msg[1024];
	va_list ap;

	if(CRPG_GlobalSettings::debug_mode) {
		va_start(ap, msgf);
		Q_vsnprintf(msg, sizeof(msg)-1, msgf, ap);
		va_end(ap);

		Msg("CSS:RPG Plugin: %s\n", msg);
	}

	return ;
}

unsigned int CRPG_Utils::steamid_check(char *steamid) {
	unsigned int repeats;
    
    if(steamid == NULL)
        return 0;
    
    if(strlen(steamid) < 10)
        return 0;
    
    if(memcmp(steamid, "STEAM_", 6))
        return 0;
    
    steamid += 6;
    
    for(repeats = 0;repeats < 3;repeats++) {
        while(1) {
            if(!*steamid) {
                if(repeats != 2)
                    return 0;
                else
                    break;
            }
            else if(isdigit(*steamid)) {
                steamid++;
                continue;
            }
            else if(*steamid == ':') {
                steamid++;
                break;
            }
            else {
                return 0;
            }
        }
    }
    
    return 1;
}

unsigned char* CRPG_Utils::ustrncpy(unsigned char *dest, const unsigned char *src, int len) {
	while(len--)
		dest[len] = src[len];

	return dest;
}

unsigned int CRPG_Utils::istrcmp(char *str1, char *str2) {
	unsigned int i, len1 = strlen(str1), len2 = strlen(str2);

	if(len1 != len2)
		return 0;

	for(i = 0;i < len1;i++) {
		if(tolower(str1[i]) != tolower(str2[i]))
			return 0;
	}

	return 1;
}

char* CRPG_Utils::istrstr(char *str, char *substr) {
	while(*str) {
		if(tolower(*str++) == tolower(*substr)) {
			if(!*++substr)
				return str;
		}
	}

	return NULL;
}

void CRPG_Utils::Init() {
	saytext = UserMessageIndex("SayText");
	hinttext = UserMessageIndex("HintText");
	vguimenu = UserMessageIndex("VGUIMenu");

	return ;
}

/*	//////////////////////////////////////
	CRPG_Timer Class 
	////////////////////////////////////// */
template class CRPG_LinkedList<CRPG_Timer>;
template<> CRPG_Timer* CRPG_LinkedList<CRPG_Timer>::ll_first;
template<> CRPG_Timer* CRPG_LinkedList<CRPG_Timer>::ll_last;
template<> unsigned int CRPG_LinkedList<CRPG_Timer>::ll_count;
float CRPG_Timer::nextrun_tm;

bool comp_times(CRPG_Timer *timer1, CRPG_Timer *timer2) {
	if(timer1->next_tm < timer2->next_tm)
		return true;

	return false;
}

void CRPG_Timer::Init() {
	nextrun_tm = 0;
	ll_init();

	return ;
}

CRPG_Timer* CRPG_Timer::AddTimer(float secs, unsigned int repeats, timer_func *func, int argc, ...) {
	float now;
	CRPG_Timer *timer = new CRPG_Timer;
	void **argv = NULL;
	int i;
	va_list ap;

	now = gpGlobals->curtime;

	timer->inc_tm = secs;

	timer->next_tm = now+secs;
	if((!nextrun_tm) || (nextrun_tm > timer->next_tm))
		nextrun_tm = timer->next_tm;

	timer->repeats = repeats;
	timer->call_count = 0;

	timer->func = func;

	if(argc > 0) {
		argv = new void *[argc];
		va_start(ap, argc);
		for(i = 0;i < argc;i++)
			argv[i] = va_arg(ap, void*);
		va_end(ap);
	}

	timer->argc = argc;
	timer->argv = argv;

	timer->ll_add();
	ll_sort(comp_times);
	
	return timer;
}

void CRPG_Timer::DelTimer(void) {
	this->ll_del();
	delete[] this->argv;
	delete this;

	return ;
}

void CRPG_Timer::RunEvents(void) {
	register float now;
	register CRPG_Timer *timer, *next;

	if(!nextrun_tm)
		return ;

	now = gpGlobals->curtime;
	if(now < nextrun_tm)
		return ;

	for(timer = ll_first;timer != NULL;timer = next) {
		next = timer->ll_next; // in case we delete timer
		if(now >= timer->next_tm) {
			timer->func(timer->argv, timer->argc);
			if(timer->repeats) {
				if(++timer->call_count >= timer->repeats) {
					timer->DelTimer();
					continue;
				}
			}
			timer->next_tm += timer->inc_tm;
		}
		else {
			break;
		}
	}

	if(!ll_count) {
		nextrun_tm = 0;
	}
	else {
		ll_sort(comp_times);
		nextrun_tm = ll_first->next_tm;
	}

	return ;
}

void CRPG_Timer::FreeMemory(void) {
	CRPG_Timer *timer, *next;

	for(timer = ll_first;timer != NULL;timer = next) {
		next = timer->ll_next;
		timer->DelTimer();
	}

	return ;
}
