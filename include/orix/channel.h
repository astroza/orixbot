/* Orixbot - channel.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <ocore/hash.h>

typedef struct {
	unsigned int flags;
	char reserved[4];
} user_status;

typedef struct {
        int mode;
	ocore_hash users;
} orix_channel;

orix_channel *channel_find(const char *name, ocore_hash *channels);

#endif
