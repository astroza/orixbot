/* Orixbot - user.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef _USERS_H_
#define _USERS_H_

#include <orix/md5.h>
#include <sqlite3.h>

#define USER_SUCCESS 0
#define USER_ERROR -1

#define USER_CPASSWORD 0
#define USER_CACCESS 1

int user_get_count(sqlite3 *db);
int user_auth(sqlite3 *, const char *, const char *);
int user_get_access(sqlite3 *, const char *);
int user_add(sqlite3 *, const char *, const char *, int);
int user_change(sqlite3 *, const char *, const char *, int);
int user_del(sqlite3 *, const char *);

#endif
