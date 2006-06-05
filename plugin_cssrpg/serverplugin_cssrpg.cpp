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
#include "cssrpg_textdb.h"

#include "items/rpgi.h"
#include "items/rpgi_hbonus.h"
#include "items/rpgi_vamp.h"
#include "items/rpgi_stealth.h"
#include "items/rpgi_ljump.h"
#include "items/rpgi_fnade.h"
#include "items/rpgi_icestab.h"
#include "items/rpgi_denial.h"
#include "items/rpgi_fpistol.h"

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
	CRPG_Utils::ShutDown();

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
	gameeventmanager->AddListener(this, "item_pickup", true);
	gameeventmanager->AddListener(this, "hegrenade_detonate", true);
	gameeventmanager->AddListener(this, "flashbang_detonate", true);
	gameeventmanager->AddListener(this, "smokegrenade_detonate", true);
	gameeventmanager->AddListener(this, "player_spawn", true);
	gameeventmanager->AddListener(this, "player_death", true);
	gameeventmanager->AddListener(this, "player_say", true);
	gameeventmanager->AddListener(this, "round_start", true);
	gameeventmanager->AddListener(this, "round_end", true);
	gameeventmanager->AddListener(this, "bomb_planted", true);
	gameeventmanager->AddListener(this, "bomb_defused", true);
	gameeventmanager->AddListener(this, "bomb_exploded", true);
	gameeventmanager->AddListener(this, "hostage_rescued", true);
	gameeventmanager->AddListener(this, "vip_escaped", true);
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

	if(!esounds->IsSoundPrecached("physics/glass/glass_impact_bullet1.wav")) {
		esounds->PrecacheSound("physics/glass/glass_impact_bullet1.wav", true);
	}
	if(!esounds->IsSoundPrecached("physics/glass/glass_impact_bullet2.wav")) {
		esounds->PrecacheSound("physics/glass/glass_impact_bullet2.wav", true);
	}
	if(!esounds->IsSoundPrecached("physics/glass/glass_impact_bullet3.wav")) {
		esounds->PrecacheSound("physics/glass/glass_impact_bullet3.wav", true);
	}

	if(!esounds->IsSoundPrecached("physics/glass/glass_sheet_impact_hard1.wav")) {
		esounds->PrecacheSound("physics/glass/glass_impact_bullet1.wav", true);
	}
	if(!esounds->IsSoundPrecached("physics/glass/glass_sheet_impact_hard2.wav")) {
		esounds->PrecacheSound("physics/glass/glass_impact_bullet2.wav", true);
	}
	if(!esounds->IsSoundPrecached("physics/glass/glass_sheet_impact_hard3.wav")) {
		esounds->PrecacheSound("physics/glass/glass_impact_bullet3.wav", true);
	}

	if(!esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet1.wav")) {
		esounds->PrecacheSound("physics/surfaces/tile_impact_bullet1.wav", true);
	}
	if(!esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet2.wav")) {
		esounds->PrecacheSound("physics/surfaces/tile_impact_bullet2.wav", true);
	}
	if(!esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet3.wav")) {
		esounds->PrecacheSound("physics/surfaces/tile_impact_bullet3.wav", true);
	}
	if(!esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet4.wav")) {
		esounds->PrecacheSound("physics/surfaces/tile_impact_bullet4.wav", true);
	}

	esounds->PrecacheSound("npc/overwatch/cityvoice/fprison_missionfailurereminder.wav", true);

	CRPG_Utils::Init();
	CRPG_TextDB::Init();
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
	CRPG_Player::AutoSave();
	CRPGI_LJump::CheckAll();
	CRPGI_IceStab::GameFrame();
	CRPGI_FPistol::GameFrame();
	CRPGI_Denial::NextFrame();

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
		CRPG_TextDB::ShutDown();
		is_shutdown = 1;
	}

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CPluginCSSRPG::ClientActive(edict_t *pEntity) {
	int index = CRPG::EdicttoIndex(pEntity);
	CRPG_Player *player;

	player = CRPG_Player::AddPlayer(pEntity);
	CRPGI_HBonus::AddPlayer(pEntity);

	if(CRPG::IsValidIndex(index)) {
		if(!CRPG_GlobalSettings::enable)
			return ;

		CRPG::ChatAreaMsg(index, "\x01This server is running CSS:RPG v%s (\x04http://cssrpg.sf.net\x01).", CSSRPG_VERSION);
		CRPG::ChatAreaMsg(index, TXTDB(player, greeting.msg1));

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

	if(!CRPG_GlobalSettings::enable)
		return PLUGIN_CONTINUE;

	if(!pEntity || pEntity->IsFree())
		return PLUGIN_CONTINUE;

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
		else if(FStrEq(pcmd, "rank"))
			type = none;
		else if(FStrEq(pcmd, "top10"))
			type = top10;
		else
			return PLUGIN_CONTINUE;

		if(type == none) {
			CRPG_Player *player;

			if(engine->Cmd_Argc() < 2) {
				player = EdicttoRPGPlayer(pEntity);
				WARN_IF(player == NULL, return PLUGIN_STOP)

				CRPG_RankManager::ChatAreaRank(player, CRPG::EdicttoIndex(pEntity));
			}
			else {
				player = IndextoRPGPlayer(CRPG::FindPlayer(engine->Cmd_Argv(1)));
				if(player == NULL) /* don't warn */
					return PLUGIN_STOP;

				CRPG_RankManager::ChatAreaRank(player, CRPG::EdicttoIndex(pEntity));
			}

			return PLUGIN_STOP;
		}

		menu = CRPG_Menu::AddMenu(pEntity);
		if(menu == NULL) {
			CRPG::ConsoleMsg("menu = NULL (3)", MTYPE_ERROR);
			return PLUGIN_STOP;
		}
		menu->submenu = type;
		if(type == top10)
			menu->header = 0; /* turn off Credits header */

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
	const unsigned int name_len = strlen(name);

	if(FStrEq(name, "player_hurt")) {
		CRPG_Player *attacker = UserIDtoRPGPlayer(event->GetInt("attacker"));
		CRPG_Player *victim = UserIDtoRPGPlayer(event->GetInt("userid"));
		int dmg_health = event->GetInt("dmg_health"), dmg_armor = event->GetInt("dmg_armor");
		const char *weapon = event->GetString("weapon");

		if(attacker == NULL) /* probably "world" */
			return ;

		WARN_IF(victim == NULL, return)

		CRPGI_IceStab::LimitDamage(victim, &dmg_health, (char*)weapon); /* limit damage to prevent lame headshots */

		CRPG_StatsManager::PlayerDamage(attacker, victim, weapon, dmg_health, dmg_armor);
		CRPGI_Vamp::PlayerDamage(attacker, victim, dmg_health, dmg_armor);
		if(CRPG::istrcmp((char*)weapon, "hegrenade"))
			CRPGI_FNade::PlayerDamage(attacker, victim, dmg_health, dmg_armor);
		else if(CRPG::istrcmp((char*)weapon, "knife"))
			CRPGI_IceStab::PlayerDamage(attacker, victim, dmg_health, dmg_armor);
		else
			CRPGI_FPistol::PlayerDamage(attacker, victim, (char*)weapon);
	}
	else if(FStrEq(name, "player_jump")) {
		CRPGI_LJump::PlayerJump(event->GetInt("userid"));
	}
	else if(FStrEq(name, "item_pickup")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		WARN_IF(player == NULL, return)

		CRPGI_Denial::ItemPickup(player, (char*)event->GetString("item"));
	}
	else if(CRPG::memrcmp((void*)(name+name_len), (void*)("detonate"+8), 9)) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		WARN_IF(player == NULL, return)

		CRPGI_Denial::NadeDetonate(player, (char*)name);
	}
	else if(FStrEq(name, "player_spawn")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		CRPGI_Denial *dn;

		WARN_IF(player == NULL, return)

		if(CRPGI_Denial::round_end) {
			CRPGI_Denial::players_spawned = 1;

			if(player->css.isdead) {
				dn = IndextoDenial(player->index);
				WARN_IF(dn == NULL, return)
				dn->was_dead = 1;
			}
		}

		player->css.isdead = 0;

		CRPGI_HBonus::SetSpawnHealth(player);
		CRPGI_Stealth::SetVisibilities();
	}
	else if(FStrEq(name, "player_death")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		WARN_IF(player == NULL, return)

		player->css.isdead = 1;
		CRPG_StatsManager::PlayerKill(event->GetInt("attacker"), player->userid, event->GetBool("headshot"));
	}
	else if(FStrEq(name, "player_say")) {
		if(!CRPG_GlobalSettings::enable)
			return ;

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
		else if(!memcmp(text, "rpgrank", 7)) {
			CRPG_Player *player;

			text += 7;
			if(!*text) {
				player = UserIDtoRPGPlayer(userid);
				WARN_IF(player == NULL, return)

				CRPG_RankManager::ChatAreaRank(player);
			}
			else {
				if(*text == ' ') {
					text += 1;
					if(!*text) {
						return ;
					}
					else {
						player = IndextoRPGPlayer(CRPG::FindPlayer((char*)text));
						if(player == NULL) /* don't warn */
							return ;

						CRPG_RankManager::ChatAreaRank(player);
					}
				}
			}
		}
		else if(FStrEq(text, "rpgtop10")) {
			CRPG_Menu *menu;

			menu = CRPG_Menu::AddMenu(CRPG::UserIDtoEdict(userid));
			if(menu == NULL) {
				CRPG::ConsoleMsg("menu = NULL (5)", MTYPE_ERROR);
				return ;
			}
			menu->submenu = top10;
			menu->header = 0; /* turn off Credits header */
			menu->CreateMenu();
		}

		/* So I was bored... */
		else if(FStrEq(text, "rpggaben")) {
			CRPG_Player *player = UserIDtoRPGPlayer(userid);
			if(player == NULL)
				return ;

			CRPG::EmitSound(player->index, "npc/overwatch/cityvoice/fprison_missionfailurereminder.wav");
		}
	}
	else if(FStrEq(name, "round_start")) {
		CRPGI_Denial::round_end = 0;
	}
	else if(FStrEq(name, "round_end")) {
		CRPG_StatsManager::WinningTeam(event->GetInt("winner"), event->GetInt("reason"));
		CRPG_TeamBalance::RoundEnd();
		CRPGI_Denial::round_end = 1;
	}
	else if(FStrEq(name, "bomb_planted")) {
		CRPG_StatsManager::BombPlanted(event->GetInt("userid"));
	}
	else if(FStrEq(name, "bomb_defused")) {
		CRPG_StatsManager::BombDefused(event->GetInt("userid"));
	}
	else if(FStrEq(name, "bomb_exploded")) {
		CRPG_StatsManager::BombExploded(event->GetInt("userid"));
	}
	else if(FStrEq(name, "hostage_rescued")) {
		CRPG_StatsManager::HostageRescued(event->GetInt("userid"));
	}
	else if(FStrEq(name, "vip_escaped")) {
		CRPG_StatsManager::VipEscaped(event->GetInt("userid"));
	}
	else if(FStrEq(name, "player_team")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		CRPGI_Denial *dn;
		int team;

		if(player == NULL) /* don't warn */
			return ;

		if(CRPG_GlobalSettings::enable && (player->level <= 1) && (player->css.team == team_none))
			CRPG::ShowMOTD(player->index, "CSS:RPG Disclaimer", "http://cssrpg.sourceforge.net/help/disclaimer.html", motd_url);

		team = event->GetInt("team");

		if(team == 2) {
			CRPG_TeamBalance::teamt_count++;
			if(player->css.team == team_ct) /* previous team */
				CRPG_TeamBalance::teamct_count--;

			player->css.team = team_t;
		}
		else if(team == 3) {
			CRPG_TeamBalance::teamct_count++;
			if(player->css.team == team_t) /* previous team */
				CRPG_TeamBalance::teamt_count--;

			player->css.team = team_ct;
		}
		else {
			if(player->css.team == team_t)
				CRPG_TeamBalance::teamt_count--;
			else if(player->css.team == team_t)
				CRPG_TeamBalance::teamct_count--;

			player->css.team = team_none;
		}

		dn = IndextoDenial(player->index);
		if(dn != NULL) { /* don't warn */
			dn->ResetItems();
		}

		CRPG_TeamBalance::roundend_check = 1;
	}
	else {
		CRPG::DebugMsg("Warning: Event \"%s\" was not used", name);
	}

	return ;
}
