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

#ifndef CSSRPG_INTERFACE_H
#define CSSRPG_INTERFACE_H

#include "igameevents.h"

/**
 * @brief Builds the SourceMM CSS:RPG version.
 */
//#define CSSRPG_SOURCEMM

/**
 * @brief Interfaces from the engine.
 *
 * @{
 */
extern class IVEngineServer *s_engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern class IFileSystem *s_filesystem; // file I/O 
extern class IGameEventManager2 *s_gameeventmanager; // game events interface /* 2 */
extern class IPlayerInfoManager *s_playerinfomanager; // game dll interface to interact with players
extern class IBotManager *s_botmanager; // game dll interface to interact with bots
extern class IServerPluginHelpers *s_helpers; // special 3rd party plugin helpers from the engine
extern class IUniformRandomStream *s_randomStr;
extern class IServerGameClients *s_serverclients;
extern class IEngineTrace *s_enginetrace;
extern class IServerGameDLL *s_gamedll;
extern class IServerGameEnts *s_gameents;
extern class IEffects *s_effects; // fx	

extern class IVoiceServer *s_voiceserver;
extern class ICvar *s_cvar;
extern class INetworkStringTableContainer *s_networkstringtable;
extern class IStaticPropMgrServer *s_staticpropmgr;
extern class IEngineSound *s_enginesound;
extern class ISpatialPartition *s_partition;
extern class IVModelInfo *s_modelinfo;
extern class IDataCache *s_dataCache;
extern class IEngineSound *s_esounds; // sound 
extern class ISoundEmitterSystemBase *s_soundemitterbase;
extern class IClientEntityList *s_entitylist; //CBaseEntityList

extern class CGlobalVars *s_globals;
/** @} */

#ifdef CSSRPG_SOURCEMM
/**
 * @brief SourceMM Plugin Specific
 *
 * @{
 */
	#include "ISmmPlugin.h"

	class CPluginCSSRPG: public ISmmPlugin, public IGameEventListener2 {
		/* Private Variables */
		int m_iClientCommandIndex;
		bool is_shutdown;

		/* Private Functions */
		void* InterfaceSearch(CreateInterfaceFn factory, char *name);

	public:
		/* Public Functions */
		CPluginCSSRPG();
		~CPluginCSSRPG();

		const char *GetDescription();
		const char *GetVersion();

		bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
		bool Unload(char *error, size_t maxlen);
		bool Pause(char *error, size_t maxlen);
		bool Unpause(char *error, size_t maxlen);
		void AllPluginsLoaded(void);
		bool LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background);
		void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
		void GameFrame(bool simulating);
		void LevelShutdown(void);
		bool ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
		void ClientActive(edict_t *pEntity, bool bLoadGame);
		void ClientPutInServer(edict_t *pEntity, char const *playername);
		void ClientDisconnect(edict_t *pEntity);
		void SetCommandClient(int index);
		void ClientSettingsChanged(edict_t *pEdict);
		void ClientCommand(edict_t *pEntity); /* Use RETURN_META(MRES_SUPERCEDE) to override the gamedll */

		/* Some extra crap SourceMM requires */
		const char *GetName() {
			return "CSS:RPG";
		}

		const char *GetAuthor() {
			return "In Soviet Russia, there is no author!";
		}

		const char *GetURL() {
			return "http://cssrpg.sourceforge.net/";
		}

		const char *GetLicense() {
			return "zlib/libpng";
		}

		const char *GetDate() {
			return __DATE__;
		}

		const char *GetLogTag() {
			return "CSSRPG";
		}

		/* IGameEventListener Interface */
		void FireGameEvent(IGameEvent *eventt); /* KeyValues *event */
	};

	extern SourceHook::CallClass<IVEngineServer> *s_Engine_CC;

	PLUGIN_GLOBALVARS();
/** @} */
#else
/**
 * @brief Valve Server Plugin Specific
 *
 * @{
 */
	#include "engine/iserverplugin.h"

	class CPluginCSSRPG: public IServerPluginCallbacks, public IGameEventListener2 {
		/* Private Variables */
		int m_iClientCommandIndex;
		bool is_shutdown;

		/* Private Functions */
		void* InterfaceSearch(CreateInterfaceFn factory, char *name);

	public:
		CPluginCSSRPG();
		~CPluginCSSRPG();

		// IServerPluginCallbacks methods
		virtual const char     *GetPluginDescription(void);
		virtual bool			Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
		virtual void			Unload(void);
		virtual void			Pause(void);
		virtual void			UnPause(void);   
		virtual void			LevelInit(char const *pMapName);
		virtual void			ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
		virtual void			GameFrame(bool simulating);
		virtual void			LevelShutdown(void);
		virtual PLUGIN_RESULT	ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
		virtual PLUGIN_RESULT	NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
		virtual void			ClientActive(edict_t *pEntity);
		virtual void			ClientPutInServer(edict_t *pEntity, char const *playername);
		virtual void			ClientDisconnect(edict_t *pEntity);
		virtual void			SetCommandClient(int index);
		virtual void			ClientSettingsChanged(edict_t *pEdict);
		virtual PLUGIN_RESULT	ClientCommand(edict_t *pEntity);

		// IGameEventListener Interface
		virtual void FireGameEvent(IGameEvent *event); //KeyValues *event

		virtual int GetCommandIndex() { return m_iClientCommandIndex; }
	};
/** @} */
#endif

extern CPluginCSSRPG g_CSSRPGPlugin;

#endif /* CSSRPG_INTERFACE_H */
