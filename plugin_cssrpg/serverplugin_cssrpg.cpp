//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

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
#include <time.h>

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"

#include "SoundEmitterSystem/isoundemittersystembase.h"
#define GAME_DLL 1
#include "engine/IStaticPropMgr.h"
#include "cbase.h"
#include "globalstate.h"
#include "isaverestore.h"
#include "client.h"
#include "decals.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "eventqueue.h"
#include "hierarchy.h"
#include "basecombatweapon.h"
#include "const.h"
#include "player.h"
#include "ndebugoverlay.h"
#include "physics.h"
#include "model_types.h"
#include "team.h"
#include "sendproxy.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#define GAME_DLL 1
#include "networkvar.h"
#include "baseentity.h"
#include "collisionutils.h"
#include "coordsize.h"
#include "vstdlib/strtools.h"
#include "engine/IEngineSound.h"
#include "physics_saverestore.h"
#include "saverestore_utlvector.h"
#include "bone_setup.h"
#include "vcollide_parse.h"
#include "filters.h"
#include "te_effect_dispatch.h"
#include "AI_Criteria.h"
#include "AI_ResponseSystem.h"
#include "world.h"
#include "globals.h"
#include "saverestoretypes.h"
#include "SkyCamera.h"
#include "sceneentity.h"
#include "game.h"
#include "tier0/vprof.h"
#ifndef __linux__
#include "ai_basenpc.h"
#endif

#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"

#include "IEffects.h" 
#include "props.h"
#include "imapoverview.h"
#include "bitbuf.h"
#include "MRecipientFilter.h"
#include "shake.h"
#include "ivoiceserver.h"
#include "engine/IVEngineCache.h"

#include "icvar.h"

#include "cssrpg.h"
#include "cssrpg_menu.h"
#include "cssrpg_stats.h"
#include "cssrpg_console.h"
#include "cssrpg_hacks.h"
#include "cssrpg_database.h"

#include "items/rpgi.h"
#include "items/rpgi_hbonus.h"
#include "items/rpgi_vamp.h"
#include "items/rpgi_stealth.h"
#include "items/rpgi_ljump.h"
#include "items/rpgi_fnade.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace std;

// Interfaces from the engine
IVEngineServer	*engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)
IFileSystem		*filesystem = NULL; // file I/O 
IGameEventManager2 *gameeventmanager = NULL; // game events interface /* 2 */
IPlayerInfoManager *playerinfomanager = NULL; // game dll interface to interact with players
IBotManager *botmanager = NULL; // game dll interface to interact with bots
IServerPluginHelpers *helpers = NULL; // special 3rd party plugin helpers from the engine
IUniformRandomStream *randomStr = NULL;
IEngineTrace *enginetrace = NULL;
IServerGameDLL *gamedll = NULL;

IVoiceServer	*g_pVoiceServer = NULL;
ICvar			*cvar = NULL;
INetworkStringTableContainer *networkstringtable = NULL;
IStaticPropMgrServer *staticpropmgr = NULL;
IEngineSound *enginesound = NULL;
ISpatialPartition *partition = NULL;
IVModelInfo *modelinfo = NULL;
IVEngineCache *engineCache = NULL;
IEffects *effects = NULL; // fx	
IEngineSound *esounds = NULL; // sound 
ISoundEmitterSystemBase *soundemitterbase = NULL;

CGlobalVars *gpGlobals = NULL;

// function to initialize any cvars/command in this plugin
void InitCVars(CreateInterfaceFn cvarFactory);
void Bot_RunAll(void);

// useful helper func
/*inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}*/

//---------------------------------------------------------------------------------
// Purpose: CSS RPG plugin class
//---------------------------------------------------------------------------------
class CPluginCSSRPG: public IServerPluginCallbacks, public IGameEventListener2 { /* 2 */
public:
	CPluginCSSRPG();
	~CPluginCSSRPG();

	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void			Unload(void);
	virtual void			Pause(void);
	virtual void			UnPause(void);
	virtual const char     *GetPluginDescription(void);      
	virtual void			LevelInit(char const *pMapName);
	virtual void			ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	virtual void			GameFrame(bool simulating);
	virtual void			LevelShutdown(void);
	virtual void			ClientActive(edict_t *pEntity);
	virtual void			ClientDisconnect(edict_t *pEntity);
	virtual void			ClientPutInServer(edict_t *pEntity, char const *playername);
	virtual void			SetCommandClient(int index);
	virtual void			ClientSettingsChanged(edict_t *pEdict);
	virtual PLUGIN_RESULT	ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	virtual PLUGIN_RESULT	ClientCommand(edict_t *pEntity);
	virtual PLUGIN_RESULT	NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);

	// IGameEventListener Interface
	virtual void FireGameEvent(IGameEvent *event); //KeyValues *event

	virtual int GetCommandIndex() { return m_iClientCommandIndex; }

private:
	/* Private Variables */
	int m_iClientCommandIndex;
	char is_shutdown;

	/* Private Functions */
	void RunEvents(void);
};


// 
// The plugin is a static singleton that is exported as an interface
//
CPluginCSSRPG g_CSSRPGPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPluginCSSRPG, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_CSSRPGPlugin);

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CPluginCSSRPG::CPluginCSSRPG() {
	m_iClientCommandIndex = 0;
	is_shutdown = 1;
}

CPluginCSSRPG::~CPluginCSSRPG() {
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CPluginCSSRPG::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	playerinfomanager = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, NULL);
	if(!playerinfomanager)
		CRPG::ConsoleMsg("Unable to load playerinfomanager, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	modelinfo = (IVModelInfo*)interfaceFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL);
	if(!modelinfo)
		CRPG::ConsoleMsg("Unable to load modelinfo, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	effects = (IEffects*)gameServerFactory(IEFFECTS_INTERFACE_VERSION,NULL);
	if(!effects)
		CRPG::ConsoleMsg("Unable to load effects engine, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	esounds = (IEngineSound*)interfaceFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL); 
    if(!esounds)
		CRPG::ConsoleMsg("Unable to load sounds engine, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	partition = (ISpatialPartition*)interfaceFactory(INTERFACEVERSION_SPATIALPARTITION, NULL);
    if(!partition)
		CRPG::ConsoleMsg("Unable to load partition engine, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	g_pVoiceServer = (IVoiceServer*)interfaceFactory(INTERFACEVERSION_VOICESERVER, NULL);
    if(!g_pVoiceServer)
		CRPG::ConsoleMsg("Unable to load VoiceServer engine, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	engineCache = (IVEngineCache*)interfaceFactory(VENGINE_CACHE_INTERFACE_VERSION, NULL);
    if(!engineCache)
		CRPG::ConsoleMsg("Unable to load Engine Cache, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	staticpropmgr = (IStaticPropMgrServer*)interfaceFactory(INTERFACEVERSION_STATICPROPMGR_SERVER,NULL);
    if(!staticpropmgr)
		CRPG::ConsoleMsg("Unable to load Static Prob Manager, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific player data

	randomStr = (IUniformRandomStream*)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL);
	if(!randomStr)
		CRPG::ConsoleMsg("Unable to load random number engine, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific bot functions

	soundemitterbase = (ISoundEmitterSystemBase*)interfaceFactory(SOUNDEMITTERSYSTEM_INTERFACE_VERSION, NULL);
	if(!soundemitterbase)
		CRPG::ConsoleMsg("Unable to load sound emitter engine, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific bot functions

	cvar = (ICvar*)interfaceFactory(VENGINE_CVAR_INTERFACE_VERSION, NULL);
	if(!cvar)
		CRPG::ConsoleMsg("Unable to load cvar engine, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific bot functions

	botmanager = (IBotManager*)gameServerFactory(INTERFACEVERSION_PLAYERBOTMANAGER, NULL);
	if(!botmanager)
		CRPG::ConsoleMsg("Unable to load botcontroller, ignoring", MTYPE_WARNING); // this isn't fatal, we just won't be able to access specific bot functions

	networkstringtable = (INetworkStringTableContainer*)interfaceFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER, NULL);
	if(!networkstringtable)
		CRPG::ConsoleMsg("Unable to load Network String Table, ignoring", MTYPE_WARNING);

	// get the interfaces we want to use
	if(!(engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL)) ||
		!(gameeventmanager = (IGameEventManager2*)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2, NULL)) || /* 2 */
		!(filesystem = (IFileSystem*)interfaceFactory(FILESYSTEM_INTERFACE_VERSION, NULL)) ||
		!(helpers = (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL)) || 
		!(enginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL)) ||
		!(randomStr = (IUniformRandomStream*)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL)) ||
		!(gamedll = (IServerGameDLL*)gameServerFactory("ServerGameDLL004", NULL))) {
			CRPG::ConsoleMsg("Failed to load a game interface", MTYPE_ERROR);
			return false; // we require all these interface to function
	}

	if(playerinfomanager) {
		gpGlobals = playerinfomanager->GetGlobalVars();
	}

	/* Initiate our data */
	CRPG::DebugMsg("Initializing plugin...");
	CRPG::Init();
	CRPG_Timer::Init();
	CRPG_Setting::Init();

	InitCVars(interfaceFactory); // register any cvars we have defined
	MathLib_Init(2.2f, 2.2f, 0.0f, 2.0f);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::Unload(void) {
	gameeventmanager->RemoveListener(this); // make sure we are unloaded from the event system

	/* Destructor */
	CRPG::DebugMsg("Shutting down plugin...");
	CRPG_Setting::FreeMemory();
	CRPG_Timer::FreeMemory();
	CRPG::ShutDown();
	CRPG::DebugMsg("Shut down complete.");
	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::Pause(void) {
	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::UnPause(void) {
	return ;
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
char desc[256];
const char *CPluginCSSRPG::GetPluginDescription(void) {
	sprintf(desc, "CSSRPG v%s", CSSRPG_VERSION);
	return desc;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CPluginCSSRPG::LevelInit(char const *pMapName) {
	gameeventmanager->AddListener(this, "player_hurt", true);
	gameeventmanager->AddListener(this, "player_jump", true);
	gameeventmanager->AddListener(this, "player_spawn", true);
	gameeventmanager->AddListener(this, "player_death", true);
	gameeventmanager->AddListener(this, "player_say", true);
	gameeventmanager->AddListener(this, "player_team", true);

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CPluginCSSRPG::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
	if(!esounds->IsSoundPrecached("buttons/button14.wav")) {
		esounds->PrecacheSound("buttons/button14.wav", true);
	}
	if(!esounds->IsSoundPrecached("buttons/blip2.wav")) {
		esounds->PrecacheSound("buttons/blip2.wav", true);
	}

	CRPG_Utils::Init();
	CRPG_Player::Init();
	CRPG_Menu::Init();

	#ifdef WIN32
	init_sigs();
	#else
	init_lsym_funcs();
	#endif

	CRPGI::Init();
	CRPG::DatabaseMaid();
	is_shutdown = 0;
	CRPG::DebugMsg("Initialization done.");

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::GameFrame(bool simulating) {
	CRPG_Timer::RunEvents();
	CRPGI_LJump::CheckAll();
	CRPG_Player::AutoSave();

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::LevelShutdown(void) { // !!!!this can get called multiple times per map change
	gameeventmanager->RemoveListener(this);

	if(!is_shutdown) {
		CRPG_Player::SaveAll();
		CRPG_Player::ShutDown();
		CRPG_Menu::ShutDown();
		CRPGI::ShutDown();
		is_shutdown = 1;
	}

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::ClientActive(edict_t *pEntity) {
	int index = CRPG::EdicttoIndex(pEntity);

	CRPG_Player::AddPlayer(pEntity);
	CRPGI_HBonus::AddPlayer(pEntity);

	if(CRPG::IsValidIndex(index)) {
		CRPG::ChatAreaMsg(index, "\x01This server is running CSS:RPG v%s (\x04http://cssrpg.sf.net\x01).", CSSRPG_VERSION);
		CRPG::ChatAreaMsg(index, "Type \"rpgmenu\" to bring up your options.");

		CRPGI_Stealth::SetVisibilities();
	}

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::ClientDisconnect(edict_t *pEntity) {
	CRPG_Player *player;
	CRPG_Menu *menu;
	CRPGI_HBonus *hb;

	if(!is_shutdown) {
		player = EdicttoRPGPlayer(pEntity);
		if(player != NULL)
			player->DelPlayer();

		menu = EdicttoRPGMenu(pEntity);
		if(menu != NULL)
			menu->DelMenu();

		hb = EdicttoHBonus(pEntity);
		if(hb != NULL)
			hb->DelPlayer();
	}

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: Client is connected and should be put in the game
//---------------------------------------------------------------------------------
void CPluginCSSRPG::ClientPutInServer(edict_t *pEntity, char const *playername) {
	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CPluginCSSRPG::SetCommandClient(int index) {
	m_iClientCommandIndex = index;
	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CPluginCSSRPG::ClientSettingsChanged(edict_t *pEdict) {
	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CPluginCSSRPG::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CPluginCSSRPG::ClientCommand(edict_t *pEntity) {
	static char rpgstr[] = "rpg";
	char *pcmd;
	CRPG_Menu *menu;
	submenu_t type;
	int i;

	if(!pEntity || pEntity->IsFree()) {
		return PLUGIN_CONTINUE;
	}

	pcmd = engine->Cmd_Argv(0);
	if(FStrEq(pcmd, "menuselect")) {
		menu = EdicttoRPGMenu(pEntity);
		if(menu == NULL) {
			/* menuselect probably for a different plugin */
			return PLUGIN_CONTINUE;
		}
		menu->SelectOption(atoi(engine->Cmd_Argv(1)));
		return PLUGIN_STOP;
	}
	else if(FStrEq(pcmd, "rpgmenu")) {
		menu = CRPG_Menu::AddMenu(pEntity);
		if(menu == NULL) {
			CRPG::ConsoleMsg("menu = NULL (1)", MTYPE_ERROR);
			return PLUGIN_STOP;
		}
		menu->CreateMenu();
		return PLUGIN_STOP;
	}
	else {
		i = 0;
		while(pcmd) {
			if(*pcmd++ != rpgstr[i++])
				return PLUGIN_CONTINUE;
			if(i >= 3)
				break;
		}

		if(FStrEq(pcmd, "stats"))
			type = stats;
		else if(FStrEq(pcmd, "buy"))
			type = upgrades;
		else if(FStrEq(pcmd, "upgrades"))
			type = upgrades;
		else if(FStrEq(pcmd, "help"))
			type = help;
		else if(FStrEq(pcmd, "sell"))
			type = sell;
		else
			return PLUGIN_CONTINUE;

		menu = CRPG_Menu::AddMenu(pEntity);
		if(menu == NULL) {
			CRPG::ConsoleMsg("menu = NULL (3)", MTYPE_ERROR);
			return PLUGIN_STOP;
		}
		menu->submenu = type;
		menu->CreateMenu();
		return PLUGIN_STOP;
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CPluginCSSRPG::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CPluginCSSRPG::FireGameEvent(IGameEvent *event) {
	const char *name = event->GetName();

	if(FStrEq(name, "player_hurt")) {
		int attacker = event->GetInt("attacker"), userid = event->GetInt("userid");
		int dmg_health = event->GetInt("dmg_health"), dmg_armor = event->GetInt("dmg_armor");
		const char *weapon = event->GetString("weapon");

		CRPG_StatsManager::PlayerDamage(attacker, dmg_health, dmg_armor);
		CRPGI_Vamp::PlayerDamage(attacker, dmg_health, dmg_armor);
		CRPGI_FNade::PlayerDamage(attacker, userid, weapon, dmg_health, dmg_armor);
	}
	else if(FStrEq(name, "player_jump")) {
		CRPGI_LJump::PlayerJump(event->GetInt("userid"));
	}
	else if(FStrEq(name, "player_spawn")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		if(player == NULL)
			return ;

		player->css.isdead = 0;

		CRPGI_HBonus::SetSpawnHealth(player);
		CRPGI_Stealth::SetVisibilities();
	}
	else if(FStrEq(name, "player_death")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		if(player == NULL)
			return ;

		player->css.isdead = 1;
		CRPG_StatsManager::PlayerKill(event->GetInt("attacker"), player->userid, event->GetBool("headshot"));
	}
	else if(FStrEq(name, "player_say")) {
		const int userid = event->GetInt("userid");
		const char *text = event->GetString("text");

		if(FStrEq(text, "rpgmenu")) {
			CRPG_Menu *menu;

			menu = CRPG_Menu::AddMenu(CRPG::UserIDtoEdict(userid));
			if(menu == NULL) {
				CRPG::ConsoleMsg("menu = NULL (2)", MTYPE_ERROR);
				return ;
			}
			menu->CreateMenu();
		}
		else if(FStrEq(text, "rpghelp")) {
			CRPG_Menu *menu;

			menu = CRPG_Menu::AddMenu(CRPG::UserIDtoEdict(userid));
			if(menu == NULL) {
				CRPG::ConsoleMsg("menu = NULL (4)", MTYPE_ERROR);
				return ;
			}
			menu->submenu = help;
			menu->CreateMenu();
		}
	}
	else if(FStrEq(name, "player_team")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		int team;

		if(player == NULL)
			return ;

		if((player->level <= 1) && (player->css.team == team_none))
			CRPG::ShowMOTD(player->index, "CSS:RPG Disclaimer", "http://cssrpg.sourceforge.net/help/disclaimer.html", motd_url);

		team = event->GetInt("team");

		if(team == 2)
			player->css.team = team_t;
		else if(team == 3)
			player->css.team = team_ct;
		else
			player->css.team = team_none;
	}
	else {
		CRPG::DebugMsg("Warning: Event \"%s\" was not used", name);
	}

	return ;
}
