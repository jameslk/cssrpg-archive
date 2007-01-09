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

#ifndef CSSRPG_MISC_H
#define CSSRPG_MISC_H

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
    #include <pthread.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <dirent.h>
#endif

#include "cssrpg_interface.h"

class CRPG_Player;
class CRPG_Menu;
class IVEngineServer;
struct edict_t;

#define MTYPE_ERROR "Error"
#define MTYPE_WARNING "Warning"
#define MTYPE_NOTICE "Notice"
#define MTYPE_DBERR "Database Error"

enum motd_type {motd_text = 1, motd_index, motd_url, motd_file};

enum file_type {file_normal, file_dir};
#define END_OF_DIR 2

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

struct file_info {
	char name[MAX_PATH];
	char ext[MAX_PATH];
	char fullpath[MAX_PATH];
	file_type type;
};

class IPlayerInfo;
class IVEngineServe;
class IPlayerInfoManager;
class CGlobalVars;
class IFileSystem;
class CRPG_Utils {
private:
	/* Private Variables */
	static FILE *dlog_fptr;

	static int textmsg;
	static int hinttext;
	static int vguimenu;

public:
	/* Public Functions */
	static int maxClients(void);
	static int currentClients(void);

	static unsigned int IsValidEdict(edict_t *e);
	static unsigned int IsValidIndex(int index);
	static int UserIDtoIndex(int userid);
	static edict_t* UserIDtoEdict(int userid);
	static edict_t* IndextoEdict(int index);
	static int IndextoUserID(int index);
	static int EdicttoIndex(edict_t *e);
	static int EdicttoUserid(edict_t *e);
	static IPlayerInfo* EdicttoPlayerInfo(edict_t *e);
	static const char* EdicttoSteamID(edict_t *e);
	static int UserMessageIndex(char *name);

	static int FindPlayer(char *str); /* used to find a player by their name or part of their name or also by userid */
	static void ChatAreaMsg(int index, char *msg, ...);
	static void CRPG_Utils::ChatAreaMsg(int index, unsigned int key_id, ...);
	static void HintTextMsg(int index, char *msgf, ...);
	static void EmitSound(int index, char *sound_path, float vol = 0.7, CRPG_Player *follow = NULL);
	static void ShowMOTD(int index, char *title, char *msg, motd_type type, char *cmd = NULL);
	static void SetCheats(char enable, char temporary = 1);

	static void ConsoleMsg(char *msgf, char *msg_type, ...);
	static void DebugMsg(char *msg, ...);
	static void DebugMsg(int nolog, char *msgf, ...);

	static unsigned int steamid_check(char *steamid);
	static unsigned long atoul(char *str);
	static unsigned long hextoul(char *hex);
	static unsigned char* ustrncpy(unsigned char *dest, const unsigned char *src, int len);
	static unsigned int istrcmp(char *str1, char *str2);
	static unsigned int memrcmp(void *mem_end1, void *mem_end2, size_t len);
	static char* istrstr(char *str, char *substr);
	static int snprintf(char *buf, size_t length, const char *format, ...);
	static unsigned int traverse_dir(struct file_info &file, char *path, unsigned int position);

	#ifdef WIN32
	unsigned int CRPG_Utils::CreateThread(LPTHREAD_START_ROUTINE func, LPVOID param);
	#else
	unsigned int CRPG_Utils::CreateThread(void*(*func)(void*), void *param);
	#endif

	static void Init(void);
	static void ShutDown(void);
};

#ifdef WIN32
#define THREAD_FUNC(name) DWORD WINAPI name(LPVOID lpParam)
#else
#define THREAD_FUNC(name) void* name(void *param)
#endif

#define WARN_IF(expression, code) \
	if(expression) { \
		CRPG_Utils::DebugMsg("Warning (%s:%d): " #expression, __FILE__, __LINE__); \
		code; \
	}

class CBasePlayer;
/**
 * @brief Player Tracker Template Class
 *
 * @{
 */
template <class T> class CRPG_PlayerClass {
protected:
	static T **nodes;

	static T* new_node(edict_t *e) {
		int index;
		int userid;
		T *node;

		if(!e)
			return NULL;

		userid = CRPG_Utils::EdicttoUserid(e);
		if(userid < 0)
			return NULL;

		node = new T();

		node->index = CRPG_Utils::EdicttoIndex(e);
		node->userid = userid;

		index = node->index-1;
		if(nodes[index] != NULL) {
			CRPG_Utils::DebugMsg("Warning: Deleted an old node for a new node at index: %d", node->index);
			delete nodes[index];
		}

		nodes[index] = node;

		return node;
	}

	static unsigned int del_node(T *node) {
		if(node == NULL)
			return 0;

		nodes[node->index-1] = NULL;
		delete node;

		return 1;
	}

	static void free_nodes(unsigned int node_count) {
		unsigned int i = node_count;

		if(nodes == NULL) {
			CRPG_Utils::DebugMsg("Warning: Attempt to free memory that was already freed.");
			return ;
		}

		while(i--) {
			if(nodes[i] != NULL) {
				delete nodes[i];
				nodes[i] = NULL;
			}
		}

		delete[] nodes;
		nodes = NULL;

		return ;
	}

public:
	int index;
	int userid;

	edict_t* e(void) {
		return s_engine->PEntityOfEntIndex(this->index);
	}

	IPlayerInfo* info(void) {
		return s_playerinfomanager->GetPlayerInfo(s_engine->PEntityOfEntIndex(this->index));
	}

	CBasePlayer* cbp(void) {
		edict_t *e = s_engine->PEntityOfEntIndex(this->index);
		if(CRPG_Utils::IsValidEdict(e))
			return reinterpret_cast<CBasePlayer*>(e->GetUnknown()->GetBaseEntity());
		else
			return NULL;
	}

	char isfake(void) {
		edict_t *e = s_engine->PEntityOfEntIndex(this->index);
		IPlayerInfo *info = s_playerinfomanager->GetPlayerInfo(e);
		if(info != NULL)
			return info->IsFakeClient();
		else
			return false;
	}

	const char* name(void) {
		edict_t *e = s_engine->PEntityOfEntIndex(this->index);
		IPlayerInfo *info = s_playerinfomanager->GetPlayerInfo(e);
		if(info != NULL)
			return info->GetName();
		else
			return NULL;
	}

	const char* steamid(void) {
		edict_t *e = s_engine->PEntityOfEntIndex(this->index);
		IPlayerInfo *info = s_playerinfomanager->GetPlayerInfo(e);
		if(info != NULL)
			return info->GetNetworkIDString();
		else
			return NULL;
	}

	static T* IndextoHandle(int index) {
		if(!CRPG_Utils::IsValidIndex(index))
			return NULL;

		return nodes[index-1];
	}

	static T* EdicttoHandle(edict_t *e) {
		int index;

		if(!CRPG_Utils::IsValidEdict(e))
			return NULL;

		index = CRPG_Utils::EdicttoIndex(e);
		if(index <= 0)
			return NULL;

		return nodes[index-1];
	}
};
/** @} */

/**
 * @brief Static (Double) Linked List Template Class
 * @note Don't forget to call ll_init() or to manually initialize ll_first,
 *       ll_last, and ll_count to 0!
 *
 * @{
 */
template <class T> class CRPG_StaticLinkedList {
protected:
	/* Protected Functions */
	static void ll_init(void) {
		ll_first = NULL;
		ll_last = NULL;
		ll_count = 0;

		return ;
	}

	void ll_add(void) {
		this->ll_next = NULL;

		if(ll_first == NULL) {
			this->ll_prev = NULL;
			ll_first = static_cast<T*>(this);
		}
		else {
			this->ll_prev = ll_last;
			ll_last->ll_next = static_cast<T*>(this);
		}
		ll_last = static_cast<T*>(this);

		ll_count++;
		return ;
	}

	void ll_del(void) {
		if(this->ll_next == NULL) {
			if(this->ll_prev != NULL) {
				/* ...ll_prev <- this -> NULL */
				this->ll_prev->ll_next = NULL;
				ll_last = this->ll_prev;
			}
			else {
				/* NULL <- this -> NULL */
				ll_first = NULL;
				ll_last = NULL;
			}
		}
		else {
			if(this->ll_prev == NULL) {
				/* NULL <- this -> ll_next... */
				ll_first = this->ll_next;
				ll_first->ll_prev = NULL;
			}
			else {
				/* ...ll_prev <- this -> ll_next... */ \
				this->ll_next->ll_prev = this->ll_prev;
				this->ll_prev->ll_next = this->ll_next;
			}
		}

		ll_count--;
		return ;
	}

	void ll_move_after(T *node) {
		if(this == node)
			return ;

		if(this->ll_prev != NULL)
			this->ll_prev->ll_next = this->ll_next;

		if(this->ll_next != NULL)
			this->ll_next->ll_prev = this->ll_prev;

		if(node == NULL) {
			/* Move to the front */
			this->ll_prev = NULL;
			this->ll_next = ll_first;

			ll_first->ll_prev = static_cast<T*>(this);
			if(ll_first->ll_next == NULL) /* this node was also last */
				ll_last = ll_first;

			ll_first = static_cast<T*>(this);
		}
		else {
			this->ll_prev = node;
			this->ll_next = node->ll_next;

			if(this->ll_next != NULL) /* if node is not last */
				node->ll_next->ll_prev = static_cast<T*>(this);

			node->ll_next = static_cast<T*>(this);
		}

		return ;
	}

	typedef bool (comp_func)(T* type1, T* type2);
	static void ll_sort(comp_func *func) {
		T *ptr, *next;
		unsigned int i = ll_count;

		if(func == NULL)
			return ;

		while(i--) {
			for(ptr = ll_first->ll_next;ptr != NULL;ptr = next) {
				next = ptr->ll_next; /* ptr changes its position */
				if(func(ptr, ptr->ll_prev))
					ptr->ll_move_after(ptr->ll_prev->ll_prev);
			}
		}

		return ;
	}

public:
	/* Public Variables */
	static T *ll_first;
	static T *ll_last;
	static unsigned int ll_count;

	T *ll_next;
	T *ll_prev;
};
/** @} */

/**
 * @brief Dynamic (Double) Linked List Template Class
 * @note Don't forget to call ll_init() or to manually initialize ll_first,
 *       ll_last, and ll_count to 0!
 *
 * @{
 */
template <class T> class CRPG_DynamicLinkedList {
private:
	T **first;
	T **last;
	unsigned int *count;

protected:
	/* Protected Functions */
	void ll_init(void) {
		*first = NULL;
		*last = NULL;
		*count = 0;

		return ;
	}

	void assign_pointers(T **ll_first, T **ll_last, unsigned int *ll_count) {
		first = ll_first;
		last = ll_last;
		count = ll_count;
	}

	void ll_add(void) {
		this->ll_next = NULL;

		if(*first == NULL) {
			this->ll_prev = NULL;
			*first = static_cast<T*>(this);
		}
		else {
			this->ll_prev = *last;
			(*last)->ll_next = static_cast<T*>(this);
		}
		*last = static_cast<T*>(this);

		(*count)++;
		return ;
	}

	void ll_add(T *node) {
		node->ll_next = NULL;

		if(*first == NULL) {
			node->ll_prev = NULL;
			*first = static_cast<T*>(node);
		}
		else {
			node->ll_prev = *last;
			(*last)->ll_next = static_cast<T*>(node);
		}
		*last = static_cast<T*>(node);

		(*count)++;
		return ;
	}

	void ll_del(void) {
		if(this->ll_next == NULL) {
			if(this->ll_prev != NULL) {
				/* ...ll_prev <- this -> NULL */
				this->ll_prev->ll_next = NULL;
				*last = this->ll_prev;
			}
			else {
				/* NULL <- this -> NULL */
				*first = NULL;
				*last = NULL;
			}
		}
		else {
			if(this->ll_prev == NULL) {
				/* NULL <- this -> ll_next... */
				*first = this->ll_next;
				(*first)->ll_prev = NULL;
			}
			else {
				/* ...ll_prev <- this -> ll_next... */ \
				this->ll_next->ll_prev = this->ll_prev;
				this->ll_prev->ll_next = this->ll_next;
			}
		}

		(*count)--;
		return ;
	}

	void ll_move_after(T *node) {
		if(this == node)
			return ;

		if(this->ll_prev != NULL)
			this->ll_prev->ll_next = this->ll_next;

		if(this->ll_next != NULL)
			this->ll_next->ll_prev = this->ll_prev;

		if(node == NULL) {
			/* Move to the front */
			this->ll_prev = NULL;
			this->ll_next = *first;

			(*first)->ll_prev = static_cast<T*>(this);
			if((*first)->ll_next == NULL) /* this node was also last */
				*last = *first;

			*first = static_cast<T*>(this);
		}
		else {
			this->ll_prev = node;
			this->ll_next = node->ll_next;

			if(this->ll_next != NULL) /* if node is not last */
				node->ll_next->ll_prev = static_cast<T*>(this);

			node->ll_next = static_cast<T*>(this);
		}

		return ;
	}

	typedef bool (comp_func)(T* type1, T* type2);
	void ll_sort(comp_func *func) {
		T *ptr, *next;
		unsigned int i = *count;

		if(func == NULL)
			return ;

		while(i--) {
			for(ptr = (*first)->ll_next;ptr != NULL;ptr = next) {
				next = ptr->ll_next; /* ptr changes its position */
				if(func(ptr, ptr->ll_prev))
					ptr->ll_move_after(ptr->ll_prev->ll_prev);
			}
		}

		return ;
	}

public:
	/* Public Variables */
	T *ll_next;
	T *ll_prev;
};
/** @} */

typedef void (timer_func)(void *argv[], int argc);
#define TIMER_FUNC(x) void x(void *argv[], int argc)

class CRPG_Timer: public CRPG_StaticLinkedList<CRPG_Timer> {
	/* Private Variables */
	static float nextrun_tm; /* keep track of the next run time */

public:
	/* Public Variables */
	float inc_tm;
	float next_tm;

	unsigned int repeats;
	unsigned int call_count;

	timer_func *func;
	void **argv;
	int argc;

	/* Public Functions */
	static void Init(void);
	static void FreeMemory(void);

	static CRPG_Timer* AddTimer(float secs, unsigned int repeats, timer_func *func, int argc, ...);
	void DelTimer(void);

	static void RunEvents(void);
};

#endif
