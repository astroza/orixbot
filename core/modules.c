/* Orixbot - modules.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <dlfcn.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <orix/message.h>
#include <orix/private.h>
#include <orix/modules.h>
#include <orix/log.h>
#include <ocore/hash.h>

static ocore_dlist *mods_list;

static char *mods_path = "/usr/share/orix/modules";
static void free_module(void *data);

static void INIT object_init()
{
	mods_list = ocore_dlist_new();
	ocore_list_set_free_func(OCORE_LIST(mods_list), free_module);
}

/* Se recorre la lista de modulos, buscando el modulo por su nombre */
orix_module *mod_find_by_name(const char *name)
{
	orix_module *mod;
	if(!name)
		return NULL;

	mod = ocore_list_goto_first(OCORE_LIST(mods_list));
	while(mod != NULL) {
		if(strcmp(mod->name, name) == 0)
			return mod;

		mod = ocore_list_next(OCORE_LIST(mods_list));
	}

	return NULL;
}

/* Se recorre la lista de modulos, buscando el modulo por su direccion de handle */
orix_module *mod_find_by_handle(void *handle)
{
	orix_module *mod;

	if(!handle)
		return NULL;

	mod = ocore_list_goto_first(OCORE_LIST(mods_list));
	while(mod != NULL) {
		if(mod->handle == handle)
			return mod;

		mod = ocore_list_next(OCORE_LIST(mods_list));
	}

	return NULL;
}

static int search_name_ptr(ocore_hash *handle, const char *name)
{
	ocore_hash_node *node;

	node = ocore_hash_get_node(handle, name);
	if(node && node->name == name)
		return 1;

	return 0;
}

static inline int search_ref_and_remove(ocore_dlist *l, void *ref)
{
	void *value;

	value = ocore_list_goto_first(OCORE_LIST(l));

	while(value && value != ref)
		value = ocore_list_next(OCORE_LIST(l));

	if(value) {
		ocore_dlist_remove(l); /* Elimina el nodo actual */
		return 1;
	}

	return 0;
}

static void do_mod_unload(orix_module *mod)
{
	if(mod->referenced)
		mod->referenced--;

	if(!mod->referenced)
		ocore_dlist_remove(mods_list); /* Elimina el nodo actual */
}

unsigned int mod_unload(orix_bot *bot)
{
	unsigned int i, count = 0;
	char *symbol;
	void *ref;
	orix_module *mod = ocore_list_current(OCORE_LIST(mods_list));

	/* Busca features del modulo cargadas en el bot */
	for(i = 0; i < 2; i++) {
		symbol = ocore_list_goto_first(mod->features_list[i]);
		while(symbol) {
			if(search_name_ptr(&bot->cmds[i], symbol) && ocore_hash_remove(&bot->cmds[i], symbol))
				count++;

			symbol = ocore_list_next(mod->features_list[i]);
		}
	}

	for(i = 0; i < MESSAGE_TYPES; i++) {
		/* mod->features_list[2 + i] y bot->parsers[i] son equivalentes en el tipo de analizador.
		 * Buscaremos referencias de la lista mod->features_list[2 + i] en bot->parsers[i], si existe una
		 * la borra y aumenta un contador.
		 */
		ref = ocore_list_goto_first(mod->features_list[2 + i]);
		while(ref) {
			if(search_ref_and_remove(bot->parsers[i], ref))
				count++;

			ref = ocore_list_next(mod->features_list[2 + i]);
		}
	}

	if(count)
		do_mod_unload(mod);

	return count;
}

void mod_unload_all_from_bot(orix_bot *bot)
{
	orix_module *mod;

	mod = ocore_list_goto_first(OCORE_LIST(mods_list));
	while(mod) {
		mod_unload(bot); /* Tries to unload module from "bot" */
		if(mod == ocore_list_current(OCORE_LIST(mods_list)))
			mod = ocore_list_next(OCORE_LIST(mods_list));
		else
			mod = ocore_list_current(OCORE_LIST(mods_list));
	}
}

static void free_module(void *data)
{
	orix_module *mod = data;
	unsigned int i;

	dlclose(mod->handle);
#ifdef ENABLE_DEBUG
        orix_log(DEBUG, "%s(): (name=%s) freed", __FUNCTION__, mod->name);
#endif
	free(mod->name);
	if(mod->author)
		free(mod->author);

	for(i = 0; i < (2 + MESSAGE_TYPES); i++)
		ocore_list_destroy(mod->features_list[i]);

	free(mod);
}

static void free_command(void *data)
{
#if ENABLE_DEBUG
	orix_log(DEBUG, "%s(): command \"%s\" freed", __FUNCTION__, (void *)data);
#endif
	free(data);
}

/* do_module_load:
 */
static orix_module *do_mod_load(const char *modname)
{
	xmlDocPtr doc;
	xmlNodePtr child, root;
	orix_module mod, *ret_mod;
	const char *so_path;
	char *symbol, *on,  xmlfile[255], *purename;
	void *symbol_ref;
	unsigned int i, used, where;
	int cmd_id;

	purename = rindex(modname, '/');
	if(purename)
		purename++;
	else
		purename = (char *)modname;

	snprintf(xmlfile, sizeof(xmlfile), "%s/%s.xml", mods_path, purename);
	doc = xmlParseFile(xmlfile);
	if(doc == NULL) {
		orix_log(ERROR, "%s(%s): Can't open xmlfile", __FUNCTION__, xmlfile);
		return NULL;
	}

	root = xmlDocGetRootElement(doc);
	if(!root) {
		orix_log(ERROR, "%s(%s): Empty file", __FUNCTION__, xmlfile);
		goto error;
	}

	if(xmlStrcmp(root->name, (const xmlChar *) "module") != 0) {
		orix_log(ERROR, "%s(%s): Root tag is not \"module\"", __FUNCTION__, xmlfile);
		goto error;
	}

	so_path = (char *)xmlGetProp(root, (xmlChar *)"so");
	if(!so_path) {
		orix_log(ERROR, "%s(%s): Shared object not given", __FUNCTION__, xmlfile);
		goto error;
	}

	mod.author = (char *)xmlGetProp(root, (xmlChar *)"author");

	mod.handle = dlopen(so_path, RTLD_NOW);
	if(!mod.handle) {
		orix_log(ERROR, "%s(%s): %s:%s", __FUNCTION__, xmlfile, so_path, dlerror());
		goto error;
	}
	xmlFree((void *)so_path);

	for(i = 0; i < (2 + MESSAGE_TYPES); i++)
		mod.features_list[i] = NULL;

	for(child = root->xmlChildrenNode; child != NULL; child = child->next) {
		symbol = (char *)xmlGetProp(child, (xmlChar *)"symbol");
		if(!symbol)
			continue;

		symbol_ref = dlsym(mod.handle, symbol);
		if(!symbol_ref) {
			orix_log(ERROR, "%s(%s): %s", __FUNCTION__, xmlfile, dlerror());
			continue;
		}

		on = (char *)xmlGetProp(child, (xmlChar *)"on");

		if(xmlStrcmp(child->name, (xmlChar *)"command") == 0) {
			if(!on) 
				where = 0;
			else {
				if(strcmp(on, "server") == 0)
					where = 1;
				else
					where = 0;

				xmlFree(on);
			}

			if(mod.features_list[where] == NULL) {
				mod.features_list[where] = ocore_list_new();
				ocore_list_set_free_func(mod.features_list[where], free_command);
			}

			ocore_list_new_node(mod.features_list[where], (void *)symbol);

		} else {
			xmlFree(symbol);
			if(xmlStrcmp(child->name, (xmlChar *)"parser") == 0) {
				if(on) {
					cmd_id = get_cmd_id(on);
					xmlFree(on);
					if(cmd_id == 0) {
						orix_log(ERROR, "%s(%s): on \"%s\" not supported in line %d\n", __FUNCTION__, xmlfile, on, child->line);
						continue;
					}
				} else {
					orix_log(ERROR, "%s(%s): missing \"on\" attribute in line %d\n", __FUNCTION__, xmlfile, child->line);
					continue;
				}

				if(mod.features_list[cmd_id + 2] == NULL)
					mod.features_list[cmd_id + 2] = ocore_list_new();

				ocore_list_new_node(mod.features_list[cmd_id + 2], (void *)symbol_ref);
			}
		}
	}

	xmlFreeDoc(doc);
	used = 0;

	for(i = 0; i < (2 + MESSAGE_TYPES); i++)
		if(mod.features_list[i]) {
			used = 1;
			break;
		}

	if(!used) {
		orix_log(ERROR, "%s(%s): isn't used, ORIX discards this module", __FUNCTION__, xmlfile);
		dlclose(mod.handle);
		return NULL;
	}

	ret_mod = malloc(sizeof(orix_module)); /* Necesitamos un malloc's wrapper que en caso de no disponer de memoria FINALIZE orix */
	ret_mod->name = strdup(modname);
	ret_mod->handle = mod.handle;
	ret_mod->author = mod.author; /* author apunta a una cadena creada dinamicamente por libxml2 */
	ret_mod->referenced = 0;
	for(i = 0; i < (2 + MESSAGE_TYPES); i++)
		ret_mod->features_list[i] = mod.features_list[i];

	ocore_dlist_new_node(mods_list, ret_mod);

	return ret_mod;
error:
	xmlFreeDoc(doc);
	return NULL;
}

unsigned int mod_load(orix_bot *bot, const char *modname)
{
	orix_module *mod;
	unsigned int i, count = 0;
	char *symbol;
	void *ref;

	mod = mod_find_by_name(modname);
	if(!mod)
		mod = do_mod_load(modname);

	if(!mod)
		return 0;

	for(i = 0; i < 2; i++) {
		symbol = ocore_list_goto_first(mod->features_list[i]);
		while(symbol) {

			/* Es imposible que esto falle */
			ref = dlsym(mod->handle, symbol);
			if(ocore_hash_add(&bot->cmds[i], symbol, ref, 0))
				count++;

			symbol = ocore_list_next(mod->features_list[i]);
		}
	}

	for(; i < (2 + MESSAGE_TYPES); i++) {
		ref = ocore_list_goto_first(mod->features_list[i]);

		if(ref && bot->parsers[i-2] == NULL)
			bot->parsers[i-2] = ocore_dlist_new();

		while(ref) {
			if(ocore_dlist_new_node(bot->parsers[i-2], ref))
				count++;

			ref = ocore_list_next(mod->features_list[i]);
		}
	}

	if(count)
		mod->referenced++;

	if(!mod->referenced)
		do_mod_unload(mod);

#if ENABLE_DEBUG
	orix_log(DEBUG, "%s(%s): %d features loaded", __FUNCTION__, modname, count);
#endif
	return count;
}

/* module_get_list(): Consigue la lista de modulos
 */
orix_module *mod_get_first()
{
	return ocore_list_goto_first(OCORE_LIST(mods_list));
}

orix_module *mod_get_next()
{
	return ocore_list_next(OCORE_LIST(mods_list));
}

orix_module *mod_get_current()
{
	 return ocore_list_current(OCORE_LIST(mods_list));
}

void set_mods_path(char *dir)
{
	/* Hara falta un verificador de ruta ? */
	if(dir)
		mods_path = dir;
}
