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

/* NOTICE: Some of the code featured here is from the SQLite Database Library
   LINK:
   http://www.sqlite.org/
*/

#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

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
#include "cssrpg_database.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void FreeResult(struct tbl_result *result) {
	int nrow, ncol;

	if(result == NULL)
		return ;

	nrow = result->row_count;
	while(nrow--) {
		ncol = result->col_count;
		while(ncol--) {
			free(result->array[nrow][ncol]);
		}
		free(result->array[nrow]);
	}
	
	free(result->array);
	free(result);

	return ;
}

int GetColIndex(struct tbl_result *result, char *col_name) {
	unsigned int i;

	WARN_IF((result == NULL) || (col_name == NULL), return NULL)

	i = result->col_count;

	while(i--) {
		if(CRPG::istrcmp(result->array[0][i], col_name))
			return i;
	}

	CRPG::ConsoleMsg("GetColIndex() failed to find a match for col_name: \"%s\"", MTYPE_WARNING, col_name);

	return -1;
}

char* GetCell(struct tbl_result *result, char *col_name, int row_num) {
	unsigned int i;

	WARN_IF((result == NULL) || (col_name == NULL), return NULL)

	i = result->col_count;

	while(i--) {
		if(CRPG::istrcmp(result->array[0][i], col_name))
			return result->array[row_num][i];
	}

	CRPG::ConsoleMsg("GetCell() failed to find a match for col_name: \"%s\"", MTYPE_WARNING, col_name);
	return "0";
}

CRPG_Database::CRPG_Database(char *name) {
	#ifndef WIN32 /* !WIN32 */
	struct stat buf;
	unsigned int result;
	#endif

	db_name = (char*)calloc(128, sizeof(char));;
	memset(db_name, 0, 128);
	strncpy(db_name, name, 32);

	db_path = new char[512];
	s_engine->GetGameDir(db_path, 256);

	#ifdef WIN32
	CRPG::snprintf(db_path, 512, "%s\\cfg\\cssrpg\\", db_path);
	#else
	CRPG::snprintf(db_path, 512, "%s/cfg/cssrpg/", db_path);
	#endif

	s_filesystem->CreateDirHierarchy(db_path);
	CRPG::snprintf(db_path, 512, "%s%s", db_path, db_name);

	#ifndef WIN32 /* !WIN32 */
	result = stat("cssrpg.db", &buf);
	if(!result) { /* fix an old problem */
		CRPG::ConsoleMsg("Database located in the srcds dir, moving it to %s", MTYPE_NOTICE, db_path);
		rename("cssrpg.db", db_path);
	}
	#endif

	db_status = sqlite3_open(db_path, &db);
	if(db_status != SQLITE_OK)
		CRPG::ConsoleMsg("Failed to open database '%s'", MTYPE_DBERR, db_name);
}

CRPG_Database::~CRPG_Database(void) {
	free(db_name);
	free(db_path);
	sqlite3_close(db);
}

unsigned int CRPG_Database::Query(char *queryf, ...) {
	va_list ap;
	int retval;
	char *error, *query;

	if(db_status != SQLITE_OK)
		return 0;

	va_start(ap, queryf);
	query = sqlite3_vmprintf(queryf, ap);
	va_end(ap);

	CRPG::DebugMsg(1, "Query: %s", query);
	retval = sqlite3_exec(db, query, NULL, NULL, &error);
	sqlite3_free(query);

	if(retval != SQLITE_OK) {
		if(error != NULL)
			CRPG::ConsoleMsg("%s", MTYPE_DBERR, error);
		else
			sqlite3_free(error);

		return 0;
	}

	return 1;
}

unsigned int CRPG_Database::Query(struct tbl_result **result, char *queryf, ...) {
	va_list ap;
	int retval, nrow, ncol, i, ii;
	char *error, **result_array, *query, *str;

	if(db_status != SQLITE_OK)
		return 0;

	va_start(ap, queryf);
	query = sqlite3_vmprintf(queryf, ap);
	va_end(ap);

	CRPG::DebugMsg(1, "Query: %s", query);
	retval = sqlite3_get_table(db, query, &result_array, &nrow, &ncol, &error);
	sqlite3_free(query);

	if(retval != SQLITE_OK) {
		if(error != NULL)
			CRPG::ConsoleMsg("%s", MTYPE_DBERR, error);
		else
			sqlite3_free(error);

		*result = NULL;
		return 0;
	}

	nrow++; /* result_array comes with a row for the column names */
	*result = NULL;
	if(nrow <= 1)
		return 1;

	*result = new struct tbl_result;

	(*result)->row_count = nrow;
	(*result)->col_count = ncol;
	(*result)->array = (char***)malloc(nrow*sizeof(char**));
	for(i = 0;i < nrow;i++) (*result)->array[i] = (char**)malloc(ncol*sizeof(char*));

	for(i = 0;i < nrow;i++) {
		for(ii = 0;ii < ncol;ii++) {
			str = result_array[(ncol*i)+(ii)];
			if(str == NULL) str = "0";
			(*result)->array[i][ii] = (char*)calloc(strlen(str)+1, sizeof(char));
			strcpy((*result)->array[i][ii], str);
		}
	}

	return 1;
}

unsigned int CRPG_Database::Query(sqlite_callback *func, void *arg, char *queryf, ...) {
	va_list ap;
	int result;
	char *error, *query;

	if(db_status != SQLITE_OK)
		return 0;

	va_start(ap, queryf);
	query = sqlite3_vmprintf(queryf, ap);
	va_end(ap);

	CRPG::DebugMsg(1, "Query: %s", query);
	result = sqlite3_exec(db, query, func, arg, &error);
	sqlite3_free(query);

	if(result != SQLITE_OK) {
		if(error != NULL)
			CRPG::ConsoleMsg("%s", MTYPE_DBERR, error);
		else
			sqlite3_free(error);

		return 0;
	}

	return 1;
}

int CRPG_Database::GetInsertKey(void) {
	return sqlite3_last_insert_rowid(db);
}

/* A very cheap way of doing things but there is no other alternative */
#define NO_SUCH_TBL "no such table"
int CRPG_Database::TableExists(char *table) {
	char *error, *query;
	int result;

	query = sqlite3_mprintf("SELECT * FROM %s", table);
	result = sqlite3_exec(db, query, NULL, NULL, &error);
	sqlite3_free(query);

	if(result != SQLITE_OK) {
		if(error == NULL)
			return -1;

		if(!memcmp(error, NO_SUCH_TBL, strlen(NO_SUCH_TBL))) {
			sqlite3_free(error);
			return 0;
		}
		else {
			CRPG::ConsoleMsg("%s", MTYPE_DBERR, error);
			sqlite3_free(error);
			return -1;
		}
	}

	return 1;
}

#define NO_SUCH_COL "no such column"
int CRPG_Database::ColExists(char *col, char *table) {
	char *error, *query;
	int result;

	query = sqlite3_mprintf("SELECT %s FROM %s", col, table);
	result = sqlite3_exec(db, query, NULL, NULL, &error);
	sqlite3_free(query);

	if(result != SQLITE_OK) {
		if(error == NULL)
			return -1;

		if(!memcmp(error, NO_SUCH_COL, strlen(NO_SUCH_COL))) {
			sqlite3_free(error);
			return 0;
		}
		else {
			CRPG::ConsoleMsg("%s", MTYPE_DBERR, error);
			sqlite3_free(error);
			return -1;
		}
	}

	return 1;
}

int CRPG_Database::RowCount(char *from, char *where) {
	struct tbl_result *result;
	int retval, count;
	char *ptr;

	if(where == NULL)
		retval = this->Query(&result, "SELECT COUNT(*) as c FROM %s", from);
	else
		retval = this->Query(&result, "SELECT COUNT(*) as c FROM %s WHERE %s", from, where);

	if(!retval || result == NULL)
		return -1;

	ptr = GetCell(result, "c");
	if(ptr == NULL)
		return -1;

	count = atoi(ptr);

	FreeResult(result);
	return count;
}
