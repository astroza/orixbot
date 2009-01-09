/* Orixbot - modules.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef __MODULES_H_
#define __MODULES_H_

#include <mplx2/mplx2.h>
#include <ocore/list.h>
#include <orix/parse.h>
#include <orix/bot.h>
#include <orix/message.h>

typedef struct {
        char *name;
	void *handle;
	char *author;

	unsigned int referenced;
	ocore_list *features_list[2 + MESSAGE_TYPES];

} orix_module;

#define CMDLENMIN 2
#define CMDLENMAX 32

typedef void (*bot_cmd)(orix_msg *, int);
typedef int (*server_cmd)(struct mplx_socket *, int, char **);
typedef void (*bot_parser)(orix_msg *);

orix_module *mod_find_by_name(const char *name);
orix_module *mod_find_by_handle(void *handle);

unsigned int mod_unload(orix_bot *bot);
unsigned int mod_load(orix_bot *bot, const char *modname);
void mod_unload_all_from_bot(orix_bot *bot);

orix_module *mod_get_first();
orix_module *mod_get_next();
orix_module *mod_get_current();

void set_mods_path(char *dir);

#endif
