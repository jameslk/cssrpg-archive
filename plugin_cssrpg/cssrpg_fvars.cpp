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

#include <stdlib.h>

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#include "cssrpg.h"
#include "cssrpg_fvars.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template class CRPG_StaticLinkedList<CRPG_FileVar>;
template<> CRPG_FileVar* CRPG_StaticLinkedList<CRPG_FileVar>::ll_first = NULL;
template<> CRPG_FileVar* CRPG_StaticLinkedList<CRPG_FileVar>::ll_last = NULL;
template<> unsigned int CRPG_StaticLinkedList<CRPG_FileVar>::ll_count = 0;

/**
 * @brief Sets the variable to the specified value.
 */
void CRPG_FileVar::set_value(char *val) {
	if(val == NULL)
		val = "0";

	value.s = (char*)calloc(strlen(val)+1, sizeof(char));
	strcpy(value.s, val);
	
	value.l = atol(val);
	value.ul = CRPG::atoul(val);
	
	value.f = atof(val);
	value.uf = abs(atof(val));

	if(strlen(val) == 1) {
		if(*val == '0')
			value.b = false;
		else
			value.b = true;
	}
	else {
		if(CRPG::istrcmp(val, "true"))
			value.b = true;
		else
			value.b = false;
	}

	return ;
}

/**
 * @brief Scans the KeyValues tree for a key with the name name.
 */
class KeyValues* CRPG_FileVar::scan_kvtree(class KeyValues *kv, char *name) {
	KeyValues *subkey;

	for(subkey = kv;subkey != NULL;subkey = subkey->GetNextTrueSubKey()) {
		if(CRPG::istrcmp((char*)subkey->GetName(), name))
			return subkey;
	}

	return NULL;
}

/**
 * @brief Parses the file variable database for matching variables and update
 *        their value.
 */
void CRPG_FileVar::parse_fvdb(class KeyValues *kv) {
	unsigned int i;
	char *value;
	CRPG_FileVar *fvar;
	KeyValues *subkey = NULL;

	WARN_IF(kv == NULL, return);

	for(fvar = ll_first;fvar != NULL;fvar = fvar->ll_next) {
		subkey = kv;
		i = 0;
		while(subkey != NULL) {
			subkey = scan_kvtree(subkey, fvar->path+i);
			if(subkey == NULL)
				break;

			/* Find the end of this segment of the path */
			while((fvar->path[i] != '\0') && (i < fvar->pathlen))
				i++;

			i++; /* increment passed the '\0' character */
			if(i >= fvar->pathlen) {
				value = (char*)subkey->GetString(fvar->key);
				if((value != NULL) && strlen(value)) {
					fvar->set_value(value);
					CRPG::DebugMsg("Found a matching file variable for: %s (%s)", fvar->name, value);
				}
				break;
			}

			subkey = subkey->GetFirstSubKey();
		}
	}

	return ;
}

/**
 * @brief Default constructor.
 */
CRPG_FileVar::CRPG_FileVar(char *n, char *dv, char *p): path(NULL), key(NULL), pathlen(0) {
	unsigned int dvlen, i;

	memset(&value, 0, sizeof(value));

	WARN_IF((n == NULL) || (p == NULL) || !strlen(p), return);

	name = (char*)calloc(strlen(n)+1, sizeof(char));
	strcpy(name, n);

	/* Set the default value for this file variable */
	if((dv != NULL) && (dvlen = strlen(dv))) {
		set_value(dv);
	}
	else {
		set_value(NULL);
	}

	pathlen = strlen(p);

	/* Scan backwards for the first '/', everything after it is the key name */
	i = pathlen;
	while(i--) {
		if(p[i] == '/')
			break;
	}
	i++;

	WARN_IF(i <= 1, return);

	key = (char*)calloc(pathlen-i+1, sizeof(char));
	strcpy(key, p+i);

	pathlen = i-1;
	path = (char*)calloc(pathlen+1, sizeof(char));
	strncpy(path, p, pathlen);

	/* break up the path so it can be parsed */
	for(i = 0;i < pathlen;i++) {
		if(path[i] == '/')
			path[i] = '\0';
	}

	ll_add();
}

/**
 * @brief Destructor.
 */
CRPG_FileVar::~CRPG_FileVar(void) {
	if(value.s != NULL)
		free(value.s);

	if(key != NULL)
		free(key);

	if(path != NULL)
		free(path);
}

/**
 * @brief Initializes all file variables from the file variable database.
 */
void CRPG_FileVar::LoadFVars(void) {
	char *fvdb_path, *buf;
	unsigned int fsize;
	FILE *fptr;
	KeyValues *kv;

	if(!ll_count)
		return;

	fvdb_path = (char*)calloc(512, sizeof(char));
	s_engine->GetGameDir(fvdb_path, 256);

	#ifdef WIN32
	CRPG::snprintf(fvdb_path, 512, "%s\\cfg\\cssrpg\\", fvdb_path);
	#else
	CRPG::snprintf(fvdb_path, 512, "%s/cfg/cssrpg/", fvdb_path);
	#endif

	s_filesystem->CreateDirHierarchy(fvdb_path);
	CRPG::snprintf(fvdb_path, 512, "%s%s", fvdb_path, FVAR_DBNAME);

	fptr = fopen(fvdb_path, "rb");
	if(!fptr) {
		CRPG::ConsoleMsg("No file variable database located at \"%s\", using default values", MTYPE_WARNING, fvdb_path);
		free(fvdb_path);
		return ;
	}

	/* Get the file variable database size for allocation */
	fseek(fptr, 0, SEEK_END);
	fsize = ftell(fptr);
	rewind(fptr);

	buf = (char*)calloc(fsize+1, sizeof(char));
	fread(buf, sizeof(char), fsize, fptr);
	fclose(fptr);

	/* Check for "EF BB BF" UTF-8 BOM */
	if(!memcmp(buf, "\xEF\xBB\xBF", 3))
		buf += 3;

	/* Load the file variable database into a KeyValues class */
	kv = new KeyValues(FVAR_DBNAME);
	kv->UsesEscapeSequences(true);
	if(!kv->LoadFromBuffer(FVAR_DBNAME, buf)) {
		CRPG::ConsoleMsg("Failed to parse file variable database \"%s\", using default values", MTYPE_WARNING, fvdb_path);
		kv->deleteThis();
		free(buf);
		free(fvdb_path);
		return ;
	}

	parse_fvdb(kv);

	kv->deleteThis();
	free(buf);
	free(fvdb_path);
	return ;
}