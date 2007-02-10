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
#include "datacache/idatacacheV1.h"

#include "icvar.h"
#include "icliententitylist.h"

#include "cssrpg.h"
#include "cssrpg_interface.h"
#include "cssrpg_fvars.h"
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
#include "items/rpgi_impulse.h"
#include "items/rpgi_medic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// function to initialize any cvars/command in this plugin
void InitCVars(CreateInterfaceFn cvarFactory);
void Bot_RunAll(void);

/**
 * @brief Engine and Game Interface Variables
 *
 * @{
 */
IVEngineServer *s_engine = NULL;
IFileSystem *s_filesystem = NULL;
IGameEventManager2 *s_gameeventmanager = NULL;
IPlayerInfoManager *s_playerinfomanager = NULL;
IBotManager *s_botmanager = NULL;
IServerPluginHelpers *s_helpers = NULL;
IUniformRandomStream *s_randomStr = NULL;
IServerGameClients *s_serverclients;
IEngineTrace *s_enginetrace = NULL;
IServerGameDLL *s_gamedll = NULL;
IServerGameEnts *s_gameents = NULL;

IVoiceServer *s_voiceserver = NULL;
ICvar *s_cvar = NULL;
INetworkStringTableContainer *s_networkstringtable = NULL;
IStaticPropMgrServer *s_staticpropmgr = NULL;
IEngineSound *s_enginesound = NULL;
ISpatialPartition *s_partition = NULL;
IVModelInfo *s_modelinfo = NULL;
IDataCache *s_dataCache = NULL;
IEffects *s_effects = NULL; // fx	
IEngineSound *s_esounds = NULL; // sound 
ISoundEmitterSystemBase *s_soundemitterbase = NULL;
IClientEntityList *s_entitylist; //CBaseEntityList

#ifdef CSSRPG_SOURCEMM
SourceHook::CallClass<IVEngineServer> *s_Engine_CC;
#endif

CGlobalVars *s_globals = NULL;
/** @} */

int beamring_sprite;
int redtrail_sprite;

/*
 * Declare the hooks we will be using in this file.  Hooking will not compile
 * without these. The macro naming scheme is SH_DECL_HOOKn[_void].
 * If you have 5 parameters, it would be HOOK5. If the function is void, add _void.
 * It stands for "SourceHook, Declare Hook".
 *
 * @{
 */
#ifdef CSSRPG_SOURCEMM
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
//SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);
#endif
/** @} */

/*	//////////////////////////////////////
	CPluginCSSRPG Interface Class 
	////////////////////////////////////// */
CPluginCSSRPG g_CSSRPGPlugin;

/* Expose plugin interface */
#ifdef CSSRPG_SOURCEMM
PLUGIN_EXPOSE(CPluginCSSRPG, g_CSSRPGPlugin);
#else
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPluginCSSRPG, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_CSSRPGPlugin);
#endif

void* CPluginCSSRPG::InterfaceSearch(CreateInterfaceFn factory, char *name) {
	char *iface_str, *buf;
	unsigned int len, check, i;
	void *iface;

	WARN_IF(name == NULL, return NULL);

	/* First test if this already a valid interface name */
	iface = factory(name, NULL);
	if(iface != NULL)
		return iface;

	len = strlen(name);
	iface_str = (char*)calloc(len+1, sizeof(char));
	strcpy(iface_str, name);

	if(len > 3) {
		check = isdigit(*(iface_str+(len-3))) ? 1 : 0;
		check &= isdigit(*(iface_str+(len-2))) ? 1 : 0;
		check &= isdigit(*(iface_str+(len-1))) ? 1 : 0;
		if(check)
			iface_str[len-3] = '\0';
	}

	buf = (char*)calloc(strlen(iface_str)+8, sizeof(char));

	for(i = 0;i < 1000;i++) {
		sprintf(buf, "%s%0.3d", iface_str, i);
		iface = factory(buf, NULL);
		if(iface != NULL)
			break;
	}

	free(iface_str);
	free(buf);
	return iface;
}

/**
 * @brief Default constructor.
 */
CPluginCSSRPG::CPluginCSSRPG() {
	m_iClientCommandIndex = 0;
	is_shutdown = 1;
}

/**
 * @brief Destructor.
 */
CPluginCSSRPG::~CPluginCSSRPG() {
}

/**
 * @brief The name of this plugin, returned in "plugin_print" command.
 */
char desc[256];
#ifdef CSSRPG_SOURCEMM
const char *CPluginCSSRPG::GetDescription(void) {
	sprintf(desc, "CSSRPG v%s", CSSRPG_VERSION);
	return desc;
}
#else
const char *CPluginCSSRPG::GetPluginDescription(void) {
	sprintf(desc, "CSSRPG v%s", CSSRPG_VERSION);
	return desc;
}
#endif

/**
 * @brief Output the version number for BAILOPANTS.
 */
#ifdef CSSRPG_SOURCEMM
const char *CPluginCSSRPG::GetVersion(void) {
	return CSSRPG_VERSION;
}
#endif

#define ASSIGN_INTERFACE(VAR, TYPE, FACTORY, NAME) \
	VAR = (TYPE)CPluginCSSRPG::InterfaceSearch(FACTORY, NAME); \
	if(VAR == NULL) \
		CRPG::ConsoleMsg("Unable to find an interface for %s, ignoring", MTYPE_WARNING, #VAR);

#define ASSIGN_INTERFACE_OR_DIE(VAR, TYPE, FACTORY, NAME) \
	VAR = (TYPE)CPluginCSSRPG::InterfaceSearch(FACTORY, NAME); \
	if(VAR == NULL) { \
		CRPG::ConsoleMsg("Failed to load an interface for %s", MTYPE_ERROR, #VAR); \
		return false; \
	}

/**
 * @brief Called when the plugin is loaded, load the interface we need from the engine.
 */
#ifdef CSSRPG_SOURCEMM
bool CPluginCSSRPG::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
	CreateInterfaceFn interfaceFactory = ismm->engineFactory(), gameServerFactory = ismm->serverFactory();

	PLUGIN_SAVEVARS();
#else
bool CPluginCSSRPG::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
#endif

	ASSIGN_INTERFACE_OR_DIE(s_engine, IVEngineServer*, interfaceFactory, INTERFACEVERSION_VENGINESERVER);
	ASSIGN_INTERFACE_OR_DIE(s_filesystem, IFileSystem*, interfaceFactory, FILESYSTEM_INTERFACE_VERSION);
	ASSIGN_INTERFACE_OR_DIE(s_playerinfomanager, IPlayerInfoManager*, gameServerFactory, INTERFACEVERSION_PLAYERINFOMANAGER);
	ASSIGN_INTERFACE_OR_DIE(s_gameeventmanager, IGameEventManager2*, interfaceFactory, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	ASSIGN_INTERFACE_OR_DIE(s_filesystem, IFileSystem*, interfaceFactory, FILESYSTEM_INTERFACE_VERSION);
	ASSIGN_INTERFACE_OR_DIE(s_helpers, IServerPluginHelpers*, interfaceFactory, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	ASSIGN_INTERFACE_OR_DIE(s_enginetrace, IEngineTrace*, interfaceFactory, INTERFACEVERSION_ENGINETRACE_SERVER);
	ASSIGN_INTERFACE_OR_DIE(s_randomStr, IUniformRandomStream*, interfaceFactory, VENGINE_SERVER_RANDOM_INTERFACE_VERSION);
	ASSIGN_INTERFACE_OR_DIE(s_gamedll, IServerGameDLL*, gameServerFactory, "ServerGameDLL006");
	ASSIGN_INTERFACE_OR_DIE(s_serverclients, IServerGameClients*, gameServerFactory, INTERFACEVERSION_SERVERGAMECLIENTS);
	ASSIGN_INTERFACE_OR_DIE(s_gameents, IServerGameEnts*, gameServerFactory, "ServerGameEnts001");
	ASSIGN_INTERFACE_OR_DIE(s_effects, IEffects*, gameServerFactory, IEFFECTS_INTERFACE_VERSION);

	ASSIGN_INTERFACE(s_modelinfo, IVModelInfo*, interfaceFactory, VMODELINFO_SERVER_INTERFACE_VERSION);
	ASSIGN_INTERFACE(s_esounds, IEngineSound*, interfaceFactory, IENGINESOUND_SERVER_INTERFACE_VERSION); 
	ASSIGN_INTERFACE(s_partition, ISpatialPartition*, interfaceFactory, INTERFACEVERSION_SPATIALPARTITION);
	ASSIGN_INTERFACE(s_voiceserver, IVoiceServer*, interfaceFactory, INTERFACEVERSION_VOICESERVER);
	ASSIGN_INTERFACE(s_dataCache, IDataCache*, interfaceFactory, DATACACHE_INTERFACE_VERSION_1);
	ASSIGN_INTERFACE(s_staticpropmgr, IStaticPropMgrServer*, interfaceFactory, INTERFACEVERSION_STATICPROPMGR_SERVER);
	ASSIGN_INTERFACE(s_soundemitterbase, ISoundEmitterSystemBase*, interfaceFactory, SOUNDEMITTERSYSTEM_INTERFACE_VERSION);
	ASSIGN_INTERFACE(s_cvar, ICvar*, interfaceFactory, VENGINE_CVAR_INTERFACE_VERSION);
	ASSIGN_INTERFACE(s_botmanager, IBotManager*, gameServerFactory, INTERFACEVERSION_PLAYERBOTMANAGER);
	ASSIGN_INTERFACE(s_networkstringtable, INetworkStringTableContainer*, interfaceFactory, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	//ASSIGN_INTERFACE(s_entitylist, IClientEntityList*, gameServerFactory, VCLIENTENTITYLIST_INTERFACE_VERSION);

#ifdef CSSRPG_SOURCEMM
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::LevelInit, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::ServerActivate, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::GameFrame, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::LevelShutdown, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientActive, false); //false so this can get called /after/ FireGameEvent
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientDisconnect, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::SetCommandClient, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientSettingsChanged, true);

	/* The following functions are pre handled, because that's how they are in IServerPluginCallbacks */
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientConnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientCommand, false);

	//SH_ADD_HOOK_MEMFUNC(IGameEventManager2, FireEvent, s_gameeventmanager, &g_CSSRPGPlugin, &CPluginCSSRPG::FireGameEvent, false); 

	/* Get the call class for IVServerEngine so we can safely call functions
	   without invoking their hooks (when needed). */
	s_Engine_CC = SH_GET_CALLCLASS(s_engine);
#endif

#ifdef CSSRPG_SOURCEMM
	s_globals = g_SMAPI->pGlobals();
#else
	s_globals = s_playerinfomanager->GetGlobalVars();
#endif

	/* Initiate our data */
	CRPG::DebugMsg("Initializing plugin...");
	CRPG_FileVar::LoadFVars();
	CRPG::Init();
	CRPG_Timer::Init();
	CRPG_Setting::Init();

	InitCVars(interfaceFactory); // register any cvars we have defined
	MathLib_Init(2.2f, 2.2f, 0.0f, 2.0f);
	return true;
}

/**
 * @brief Called when the plugin is unloaded (turned off).
 */
#ifdef CSSRPG_SOURCEMM
bool CPluginCSSRPG::Unload(char *error, size_t maxlen) {
#else
void CPluginCSSRPG::Unload(void) {
	s_gameeventmanager->RemoveListener(this); // make sure we are unloaded from the event system
#endif
	CRPG::DebugMsg("Shutting down plugin...");
	CRPG_Setting::FreeMemory();
	CRPG_Timer::FreeMemory();
	CRPG::ShutDown();
	CRPG::DebugMsg("Shut down complete.");
	CRPG_Utils::ShutDown();

#ifdef CSSRPG_SOURCEMM

	//SH_REMOVE_HOOK_MEMFUNC(IGameEventManager2, FireEvent, s_gameeventmanager, &g_CSSRPGPlugin, &CPluginCSSRPG::FireGameEvent, false); 
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, s_gamedll, &g_CSSRPGPlugin, &CPluginCSSRPG::LevelShutdown, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientActive, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientDisconnect, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::SetCommandClient, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientSettingsChanged, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, s_serverclients, &g_CSSRPGPlugin, &CPluginCSSRPG::ClientCommand, false);

	SH_RELEASE_CALLCLASS(s_Engine_CC);

	return true;
#else
	return ;
#endif
}

/**
 * @brief Called when the plugin is paused (i.e should stop running but isn't unloaded).
 */
#ifdef CSSRPG_SOURCEMM
bool CPluginCSSRPG::Pause(char *error, size_t maxlen) {
	return true;
}
#else
void CPluginCSSRPG::Pause(void) {
	return ;
}
#endif

/**
 * @brief Called when the plugin is unpaused (i.e should start executing again).
 */
#ifdef CSSRPG_SOURCEMM
bool CPluginCSSRPG::Unpause(char *error, size_t maxlen) {
	return true;
}
#else
void CPluginCSSRPG::UnPause(void) {
	return ;
}
#endif

/**
 * @brief 
 */
#ifdef CSSRPG_SOURCEMM
void CPluginCSSRPG::AllPluginsLoaded(void) {
	/* We don't really need this for anything other than interplugin
	   communication and that's not used in this plugin.
	   If we really wanted, we could override the factories so other plugins can
	   request interfaces we make.  In this callback, the plugin could be
	   assured that either the interfaces it requires were either loaded in
	   another plugin or not.
	   -- SO SAYS BAILOPANTS
	*/

	return ;
}
#endif

/**
 * @brief Called on level start.
 */
#ifdef CSSRPG_SOURCEMM
bool CPluginCSSRPG::LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background) {
#else
void CPluginCSSRPG::LevelInit(char const *pMapName) {
#endif
	s_gameeventmanager->AddListener(this, "player_footstep", true);
	s_gameeventmanager->AddListener(this, "player_hurt", true);
	s_gameeventmanager->AddListener(this, "player_jump", true);
	s_gameeventmanager->AddListener(this, "item_pickup", true);
	s_gameeventmanager->AddListener(this, "hegrenade_detonate", true);
	s_gameeventmanager->AddListener(this, "flashbang_detonate", true);
	s_gameeventmanager->AddListener(this, "smokegrenade_detonate", true);
	s_gameeventmanager->AddListener(this, "player_spawn", true);
	s_gameeventmanager->AddListener(this, "player_death", true);
	s_gameeventmanager->AddListener(this, "player_say", true);
	s_gameeventmanager->AddListener(this, "round_start", true);
	s_gameeventmanager->AddListener(this, "round_end", true);
	s_gameeventmanager->AddListener(this, "bomb_planted", true);
	s_gameeventmanager->AddListener(this, "bomb_defused", true);
	s_gameeventmanager->AddListener(this, "bomb_exploded", true);
	s_gameeventmanager->AddListener(this, "hostage_rescued", true);
	s_gameeventmanager->AddListener(this, "vip_escaped", true);
	s_gameeventmanager->AddListener(this, "player_team", true);

#ifdef CSSRPG_SOURCEMM
	RETURN_META_VALUE(MRES_IGNORED, true);
#else
	return ;
#endif
}

/**
 * @brief called on level start, when the server is ready to accept client
 *        connections edictCount is the number of entities in the level, 
 *        clientMax is the max client count.
 */
void CPluginCSSRPG::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
	if(!s_esounds->IsSoundPrecached("buttons/button14.wav")) {
		s_esounds->PrecacheSound("buttons/button14.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("buttons/blip2.wav")) {
		s_esounds->PrecacheSound("buttons/blip2.wav", true);
	}

	if(!s_esounds->IsSoundPrecached("physics/glass/glass_impact_bullet1.wav")) {
		s_esounds->PrecacheSound("physics/glass/glass_impact_bullet1.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("physics/glass/glass_impact_bullet2.wav")) {
		s_esounds->PrecacheSound("physics/glass/glass_impact_bullet2.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("physics/glass/glass_impact_bullet3.wav")) {
		s_esounds->PrecacheSound("physics/glass/glass_impact_bullet3.wav", true);
	}

	if(!s_esounds->IsSoundPrecached("physics/glass/glass_sheet_impact_hard1.wav")) {
		s_esounds->PrecacheSound("physics/glass/glass_impact_bullet1.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("physics/glass/glass_sheet_impact_hard2.wav")) {
		s_esounds->PrecacheSound("physics/glass/glass_impact_bullet2.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("physics/glass/glass_sheet_impact_hard3.wav")) {
		s_esounds->PrecacheSound("physics/glass/glass_impact_bullet3.wav", true);
	}

	if(!s_esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet1.wav")) {
		s_esounds->PrecacheSound("physics/surfaces/tile_impact_bullet1.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet2.wav")) {
		s_esounds->PrecacheSound("physics/surfaces/tile_impact_bullet2.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet3.wav")) {
		s_esounds->PrecacheSound("physics/surfaces/tile_impact_bullet3.wav", true);
	}
	if(!s_esounds->IsSoundPrecached("physics/surfaces/tile_impact_bullet4.wav")) {
		s_esounds->PrecacheSound("physics/surfaces/tile_impact_bullet4.wav", true);
	}

	/* Don't check if these are precached, it will always return true */
	s_esounds->PrecacheSound("weapons/physcannon/physcannon_charge.wav", true);
	s_esounds->PrecacheSound("npc/overwatch/cityvoice/fprison_missionfailurereminder.wav", true);

	beamring_sprite = s_engine->PrecacheModel("sprites/lgtning.vmt", true);
	redtrail_sprite = s_engine->PrecacheModel("sprites/combineball_trail_red_1.vmt", true);

	CRPG_Utils::Init();
	CRPG_TextDB::Init();
	CRPG_Player::Init();
	CRPG_Menu::Init();

	#ifdef WIN32
	init_sigs();
	#else
	init_lsym_funcs();
	#endif

	CRPG_ExternProps::Init(s_gamedll);

	Initialize_TE_Pointer(s_effects);

	CRPGI::Init();
	CRPG::DatabaseMaid();
	is_shutdown = 0;
	CRPG::DebugMsg("Initialization done.");

	return ;
}

/**
 * @brief called once per server frame, do recurring work here (like checking
 *        for timeouts)
 */
void CPluginCSSRPG::GameFrame(bool simulating) {
	CRPG_Timer::RunEvents();
	CRPG_Player::AutoSave();
	//CRPG_Player::ProcessKeys();
	CRPGI_LJump::CheckAll();
	CRPGI_IceStab::GameFrame();
	CRPGI_FPistol::GameFrame();
	CRPGI_Denial::NextFrame();
	CRPGI_Impulse::GameFrame();

	return ;
}

/**
 * @brief Called on level end (as the server is shutting down or going to a new map).
 */
void CPluginCSSRPG::LevelShutdown(void) { // !!!!this can get called multiple times per map change
	s_gameeventmanager->RemoveListener(this);

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

/**
 * @brief Called when a client joins a server.
 */
#ifdef CSSRPG_SOURCEMM
bool CPluginCSSRPG::ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return true;
}
#else
PLUGIN_RESULT CPluginCSSRPG::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return PLUGIN_CONTINUE;
}
#endif

/**
 * @brief Called when a client is authenticated.
 */
#ifndef CSSRPG_SOURCEMM /* !CSSRPG_SOURCEMM */
PLUGIN_RESULT CPluginCSSRPG::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
	return PLUGIN_CONTINUE;
}
#endif

/**
 * @brief Called when a client spawns into a server (i.e as they begin to play).
 */
#ifdef CSSRPG_SOURCEMM
void CPluginCSSRPG::ClientActive(edict_t *pEntity, bool bLoadGame) {
#else
void CPluginCSSRPG::ClientActive(edict_t *pEntity) {
#endif
	int index = CRPG::EdicttoIndex(pEntity);
	CRPG_Player *player;

	player = CRPG_Player::AddPlayer(pEntity);
	CRPGI::AddPlayer(player);

	if(CRPG::IsValidIndex(index)) {
		if(!CRPG_GlobalSettings::enable)
			return ;

		CRPG::ChatAreaMsg(index, "\x01This server is running CSS:RPG v%s (\x04http://cssrpg.sf.net\x01).", CSSRPG_VERSION);
		CRPG::ChatAreaMsg(index, TXTDB(player, greeting.msg1));

		CRPGI_Stealth::SetVisibilities();
	}

	return ;
}

/**
 * @brief Client is connected and should be put in the game.
 */
void CPluginCSSRPG::ClientPutInServer(edict_t *pEntity, char const *playername) {
	return ;
}

/**
 * @brief Called when a client leaves a server (or is timed out).
 */
void CPluginCSSRPG::ClientDisconnect(edict_t *pEntity) {
	CRPG_Player *player;
	CRPG_Menu *menu;

	if(!is_shutdown) {
		menu = EdicttoRPGMenu(pEntity);
		if(menu != NULL)
			menu->DelMenu();

		player = EdicttoRPGPlayer(pEntity);
		CRPGI::DelPlayer(player);
		if(player != NULL)
			player->DelPlayer();
	}

	return ;
}

/**
 * @brief Called on level start.
 */
void CPluginCSSRPG::SetCommandClient(int index) {
	m_iClientCommandIndex = index;
	return ;
}

/**
 * @brief Called on level start.
 */
void CPluginCSSRPG::ClientSettingsChanged(edict_t *pEdict) {
	return ;
}

#ifdef CSSRPG_SOURCEMM
#define CMD_CONTINUE return
#define CMD_OVERRIDE RETURN_META(MRES_SUPERCEDE)
#else
#define CMD_CONTINUE return PLUGIN_CONTINUE
#define CMD_OVERRIDE return PLUGIN_STOP
#endif

/**
 * @brief Called when a client types in a command (only a subset of commands
 *        however, not CON_COMMAND's).
 */
#ifdef CSSRPG_SOURCEMM
void CPluginCSSRPG::ClientCommand(edict_t *pEntity) {
#else
PLUGIN_RESULT CPluginCSSRPG::ClientCommand(edict_t *pEntity) {
#endif
	static char rpgstr[] = "rpg";
	char *pcmd;
	CRPG_Menu *menu;
	submenu_t type;
	int i;

	if(!CRPG_GlobalSettings::enable)
		CMD_CONTINUE;

	if(!pEntity || pEntity->IsFree())
		CMD_CONTINUE;

	pcmd = s_engine->Cmd_Argv(0);
	if(!strcmp(pcmd, "menuselect")) {
		menu = EdicttoRPGMenu(pEntity);
		if(menu == NULL) {
			/* menuselect probably for a different plugin */
			CMD_CONTINUE;
		}
		menu->SelectOption(atoi(s_engine->Cmd_Argv(1)));
		CMD_OVERRIDE;
	}
	else if(!strcmp(pcmd, "rpgmenu")) {
		menu = CRPG_Menu::AddMenu(pEntity);
		if(menu == NULL) {
			CRPG::ConsoleMsg("menu = NULL (1)", MTYPE_ERROR);
			CMD_OVERRIDE;
		}
		menu->CreateMenu();
		CMD_OVERRIDE;
	}
	else {
		i = 0;
		while(pcmd) {
			if(*pcmd++ != rpgstr[i++])
				CMD_CONTINUE;
			if(i >= 3)
				break;
		}

		if(!strcmp(pcmd, "stats"))
			type = stats;
		else if(!strcmp(pcmd, "buy"))
			type = upgrades;
		else if(!strcmp(pcmd, "upgrades"))
			type = upgrades;
		else if(!strcmp(pcmd, "help"))
			type = help;
		else if(!strcmp(pcmd, "sell"))
			type = sell;
		else if(!strcmp(pcmd, "rank"))
			type = none;
		else if(!strcmp(pcmd, "top10"))
			type = top10;
		else
			CMD_CONTINUE;

		if(type == none) {
			CRPG_Player *player;

			if(s_engine->Cmd_Argc() < 2) {
				player = EdicttoRPGPlayer(pEntity);
				WARN_IF(player == NULL, CMD_OVERRIDE)

				CRPG_RankManager::ChatAreaRank(player, CRPG::EdicttoIndex(pEntity));
			}
			else {
				player = IndextoRPGPlayer(CRPG::FindPlayer(s_engine->Cmd_Argv(1)));
				if(player == NULL) /* don't warn */
					CMD_OVERRIDE;

				CRPG_RankManager::ChatAreaRank(player, CRPG::EdicttoIndex(pEntity));
			}

			CMD_OVERRIDE;
		}

		menu = CRPG_Menu::AddMenu(pEntity);
		if(menu == NULL) {
			CRPG::ConsoleMsg("menu = NULL (3)", MTYPE_ERROR);
			CMD_OVERRIDE;
		}
		menu->submenu = type;
		if(type == top10)
			menu->header = 0; /* turn off Credits header */

		menu->CreateMenu();
		CMD_OVERRIDE;
	}

	CMD_CONTINUE;
}

/**
 * @brief Called when an event is fired.
 */
void CPluginCSSRPG::FireGameEvent(IGameEvent *event) {
	const char *name = event->GetName();
	const unsigned int name_len = strlen(name);

    if(!strcmp(name, "player_footstep")) {
		CRPGI_LJump::PlayerFootStep(event->GetInt("userid"));
	}
	else if(!strcmp(name, "player_hurt")) {
		CRPG_Player *attacker = UserIDtoRPGPlayer(event->GetInt("attacker"));
		CRPG_Player *victim = UserIDtoRPGPlayer(event->GetInt("userid"));
		int health = event->GetInt("health"), dmg_health = event->GetInt("dmg_health"), dmg_armor = event->GetInt("dmg_armor");
		const char *weapon = event->GetString("weapon");

		if(attacker == NULL) /* probably "world" */
			return ;

		WARN_IF(victim == NULL, return);

		CRPGI_IceStab::LimitDamage(victim, &dmg_health, (char*)weapon); /* limit damage to prevent lame headshots */

		CRPG_StatsManager::PlayerDamage(attacker, victim, weapon, dmg_health, dmg_armor);
		CRPGI_Vamp::PlayerDamage(attacker, victim, dmg_health, dmg_armor);
		if(CRPG::istrcmp((char*)weapon, "hegrenade")) {
			CRPGI_FNade::PlayerDamage(attacker, victim, dmg_health, dmg_armor);
		}
		else if(CRPG::istrcmp((char*)weapon, "knife")) {
			CRPGI_IceStab::PlayerDamage(attacker, victim, dmg_health, dmg_armor);
		}
		else {
			CRPGI_FPistol::PlayerDamage(attacker, victim, (char*)weapon);
			CRPGI_Impulse::PlayerDamage(attacker, victim, (char*)weapon);
		}
	}
	else if(!strcmp(name, "player_jump")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		WARN_IF(player == NULL, return);

		CRPGI_LJump::PlayerJump(player);
	}
	else if(!strcmp(name, "item_pickup")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		WARN_IF(player == NULL, return)

		CRPGI_Denial::ItemPickup(player, (char*)event->GetString("item"));
	}
	else if(CRPG::memrcmp((void*)(name+name_len), (void*)("detonate"+8), 9)) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		WARN_IF(player == NULL, return)

		CRPGI_Denial::NadeDetonate(player, (char*)name);
	}
	else if(!strcmp(name, "player_spawn")) {
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
	else if(!strcmp(name, "player_death")) {
		CRPG_Player *player = UserIDtoRPGPlayer(event->GetInt("userid"));
		WARN_IF(player == NULL, return)

		player->css.isdead = 1;
		CRPG_StatsManager::PlayerKill(event->GetInt("attacker"), player->userid, event->GetBool("headshot"));
	}
	else if(!strcmp(name, "player_say")) {
		if(!CRPG_GlobalSettings::enable)
			return ;

		const int userid = event->GetInt("userid");
		const char *text = event->GetString("text");

		if(CRPG::istrcmp((char*)text, "rpgmenu")) {
			CRPG_Menu *menu;

			menu = CRPG_Menu::AddMenu(CRPG::UserIDtoEdict(userid));
			if(menu == NULL) {
				CRPG::ConsoleMsg("menu = NULL (2)", MTYPE_ERROR);
				return ;
			}
			menu->CreateMenu();
		}
		else if(CRPG::istrcmp((char*)text, "rpghelp")) {
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
		else if(CRPG::istrcmp((char*)text, "rpgtop10")) {
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
		else if(CRPG::istrcmp((char*)text, "rpggaben")) {
			CRPG_Player *player = UserIDtoRPGPlayer(userid);
			if(player == NULL)
				return ;

			CRPG::EmitSound(player->index, "npc/overwatch/cityvoice/fprison_missionfailurereminder.wav");
		}
	}
	else if(!strcmp(name, "round_start")) {
		CRPGI_Denial::round_end = 0;
		CRPGI_Impulse::RoundStart();
	}
	else if(!strcmp(name, "round_end")) {
		CRPG_StatsManager::WinningTeam(event->GetInt("winner"), event->GetInt("reason"));
		CRPG_TeamBalance::RoundEnd();
		CRPGI_Denial::round_end = 1;
	}
	else if(!strcmp(name, "bomb_planted")) {
		CRPG_StatsManager::BombPlanted(event->GetInt("userid"));
	}
	else if(!strcmp(name, "bomb_defused")) {
		CRPG_StatsManager::BombDefused(event->GetInt("userid"));
	}
	else if(!strcmp(name, "bomb_exploded")) {
		CRPG_StatsManager::BombExploded(event->GetInt("userid"));
	}
	else if(!strcmp(name, "hostage_rescued")) {
		CRPG_StatsManager::HostageRescued(event->GetInt("userid"));
	}
	else if(!strcmp(name, "vip_escaped")) {
		CRPG_StatsManager::VipEscaped(event->GetInt("userid"));
	}
	else if(!strcmp(name, "player_team")) {
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
			else if(player->css.team == team_ct)
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
