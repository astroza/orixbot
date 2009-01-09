/* Orixbot - util.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <orix/util.h>
#include <stdlib.h>
#include <time.h>

void gen_str(char *s, int size)
{
        int i=0;
        char l;

        srand(time(NULL));
        do {
                l = rand()%122;
                if(l > 97 && l < 122 && (s[i]?s[i]!=l:1))
                s[i++] = l;
        } while ( i < (size-1) );
	s[i] = 0;
}
