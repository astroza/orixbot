/* Orixbot - user.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <orix/md5.h>
#include <orix/user.h>
#include <orix/log.h>

/* SQL para crear la tabla de usuarios */
static const char CREATE_TABLE_STRING[]="CREATE TABLE users(id integer primary key autoincrement, \n"
				 "username text UNIQUE, \n"
				 "password blob, \n"
				 "access integer)";

/***************************************/

static struct md5_return *encrypt_md5(const char *word)
{
	struct md5_return *passwd;
	md5_state_t state;
	md5_byte_t digest[16];
	int d;

	if(!word)
		return NULL;

	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)word, strlen(word));
	md5_finish(&state, digest);

	passwd = malloc(sizeof(struct md5_return));

	for (d = 0; d < 16; d++)
		sprintf(passwd->md5 + d * 2, "%02x", digest[d]);

	return passwd;
}

static int user_create_table(sqlite3 *db, char **errmsg)
{
	orix_log(DEBUG, "%s(): Creating table 'users'", __FUNCTION__);
	if(sqlite3_exec(db, CREATE_TABLE_STRING, NULL, NULL, errmsg) != SQLITE_OK)
		return USER_ERROR;

	return USER_SUCCESS;
}

/* user_get_count(): Consigue el numero de usuarios */
int user_get_count(sqlite3 *db)
{
	const char *errmsg;
	sqlite3_stmt *stmt;
	const char query[] = "SELECT id FROM users";
	int count = 0, ret;

	if(!db)
		return USER_ERROR;

	/* Podria dejar el byte-code generado con sqlite3_prepare() para
	 * ahorrarme sucesivas compilaciones de las instrucciones en SQL y solo llamar a
	 * a sqlite3_reset() para hacer la consulta de nuevo 
	 */

	if(sqlite3_prepare(db, query, sizeof(query), &stmt, NULL) !=  SQLITE_OK) {
		orix_log(ERROR, "%s(): %s", __FUNCTION__, sqlite3_errmsg(db));
		return USER_ERROR;
	}

	ret = sqlite3_step(stmt);
	switch(ret) {
		case SQLITE_DONE:
			count = 0;
			break;
		case SQLITE_ROW:
			count = sqlite3_data_count(stmt);
			break;
		case SQLITE_ERROR:
		default:
			errmsg = sqlite3_errmsg(db);
			orix_log(ERROR, "%s(): %s (errid=%d)", errmsg? errmsg : "NULL", ret);
			count = USER_ERROR;
			break;
	}

	sqlite3_finalize(stmt);
	return count;
}

static int get_access_value(void *access, int num_column, char **value, char **column_name)
{
	/* Como la columna username es 'unique', solo habra un resultado */
	*((int *)access) = atoi(value[0]);
	return 0;
}

/* user_auth(): Retorna el nivel de acceso solo si existe el usuario. */
int user_auth(sqlite3 *db, const char *username, const char *password) 
{
	struct md5_return *encrypted;
	char *query;
	char *errmsg = NULL;
	int ret, access = USER_ERROR;

	if(!username || !password)
		return USER_ERROR;

	encrypted = encrypt_md5(password);
	query = sqlite3_mprintf("SELECT access FROM users WHERE username='%s' and password='%s'", username, encrypted);
	ret = sqlite3_exec(db, query, get_access_value, &access, &errmsg);
	free(encrypted);
	sqlite3_free(query);

	if(ret == SQLITE_OK)
		return access;

	orix_log(ERROR, "%s(): %s", __FUNCTION__, errmsg);
	sqlite3_free(errmsg);

	return USER_ERROR;
}

/* user_get_access(): Consigue el nivel de acceso de un usuario */
int user_get_access(sqlite3 *db, const char *username)
{
	char *query;
	char *errmsg = NULL;
	int ret, access = USER_ERROR;

	if(!username)
		return USER_ERROR;

	query = sqlite3_mprintf("SELECT access FROM users WHERE username='%s'", username);
	ret = sqlite3_exec(db, query, get_access_value, &access, &errmsg);
	sqlite3_free(query);

	if(ret == SQLITE_OK)
		return access;

	orix_log(ERROR, "%s(): %s", __FUNCTION__, errmsg);
	sqlite3_free(errmsg);

	return USER_ERROR;
}

int user_add(sqlite3 *db, const char *username, const char *password, int access)
{
	struct md5_return *encrypted;
	char *query, *errmsg = NULL;
	int ret;

	if(!username || !password)
		return USER_ERROR;

        encrypted = encrypt_md5(password);
	query = sqlite3_mprintf("INSERT INTO users(username, password, access) VALUES('%s', '%s', %d)", username, encrypted, access);
	free(encrypted);

	sqlexec:
	ret = sqlite3_exec(db, query, NULL, NULL, &errmsg);

	if(ret == SQLITE_ERROR) {
		orix_log(ERROR, "%s(): Unable to add entry :%s", __FUNCTION__, errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
		if(user_create_table(db, &errmsg) == USER_ERROR) {
			orix_log(ERROR, "%s(): %s", __FUNCTION__, errmsg);
			sqlite3_free(errmsg);
			ret = USER_ERROR;
		} else
			goto sqlexec;

	} else if(ret == SQLITE_OK)
		ret = USER_SUCCESS;
	else {
		orix_log(ERROR, "%s(): %s", __FUNCTION__, errmsg);
		sqlite3_free(errmsg);
		ret = USER_ERROR;
	}

	sqlite3_free(query);
	return ret;
}

/* user_change(): Para cambiar informacion del usuario
 */
int user_change(sqlite3 *db, const char *username, const char *new_value, int column_idx)
{
	const char *scheme = "UPDATE users SET %s='%s' WHERE username='%s'";
	char *query;
	sqlite3_stmt *stmt;
	const char *column_name[]={"password", "access"};
	int ret;

	if(!db || !username || !new_value || column_idx > 1 || column_idx < 0)
		return USER_ERROR;

	query = sqlite3_mprintf(scheme, column_name[column_idx], new_value, username);

	if(sqlite3_prepare(db, query, strlen(query), &stmt, NULL) != SQLITE_OK) {
		orix_log(ERROR, "%s(): %s", __FUNCTION__, sqlite3_errmsg(db));
		ret = USER_ERROR;
	} else {
		if(sqlite3_step(stmt) != SQLITE_DONE) {
			orix_log(ERROR, "%s(): %s", sqlite3_errmsg(db));
			ret = USER_ERROR;
		} else
			ret = USER_SUCCESS;

		sqlite3_finalize(stmt);
	}

	sqlite3_free(query);
	return ret;
}

int user_del(sqlite3 *db, const char *username)
{
	sqlite3_stmt *stmt;
	char *query;
	int ret;

	if(!db || !username)
		return USER_ERROR;

	query = sqlite3_mprintf("DELETE FROM users WHERE username='%s'", username);
	if(sqlite3_prepare(db, query, 35 + strlen(username), &stmt, NULL) != SQLITE_OK) {
		orix_log(ERROR, "%s(): %s", __FUNCTION__, sqlite3_errmsg(db));
		ret = USER_ERROR;
	} else {
		if(sqlite3_step(stmt) != SQLITE_DONE) {
			orix_log(ERROR, "%s(): %s", __FUNCTION__, sqlite3_errmsg(db));
			ret = USER_ERROR;
		} else
			ret = USER_SUCCESS;

		sqlite3_finalize(stmt);
	}

	sqlite3_free(query);
	return ret;
}
