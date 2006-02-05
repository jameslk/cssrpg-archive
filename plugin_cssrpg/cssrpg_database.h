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

#ifndef CSSRPG_DATABASE_H
#define CSSRPG_DATABASE_H

#ifndef sqlite_callback
typedef int (sqlite_callback)(void*, int, char**, char**);
#endif

#define DB_FUNC(x) int x(void *arg, int argc, char **argv, char **col_name)

#define DB_OK 0;
#define DB_ERROR 1;

struct tbl_result {
	int row_count;
	int col_count;

	char ***array;
};

struct sqlite3;
class CRPG_Database {
	/* Private Variables */
	sqlite3 *db;
	char *db_name;
	char *db_path;
	int db_status;

public:
	/* Public Functions */
	CRPG_Database(char *name);
	~CRPG_Database(void);

	unsigned int Query(char *queryf, ...);
	unsigned int Query(struct tbl_result **result, char *queryf, ...);
	unsigned int Query(sqlite_callback *func, void *arg, char *queryf, ...);

	int GetInsertKey(void);

	int TableExists(char *table);
	int ColExists(char *col, char *table);

	int RowCount(char *from, char *where = NULL);
};

void FreeResult(struct tbl_result *result);
char* GetCell(struct tbl_result *result, char *col_name, int row_num = 1);

#endif
