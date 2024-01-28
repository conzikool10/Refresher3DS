#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>

#include "assert.h"

#include "scetool.h"

bool valid_digest(char *digest)
{
    int i = 0;

    while (digest[i])
    {
        if (!isalnum(digest[i]) &&
            digest[i] != '!' &&
            digest[i] != '@' &&
            digest[i] != '#' &&
            digest[i] != '$' &&
            digest[i] != '%' &&
            digest[i] != '^' &&
            digest[i] != '&' &&
            digest[i] != '*' &&
            digest[i] != '(' &&
            digest[i] != ')' &&
            digest[i] != '?' &&
            digest[i] != '/' &&
            digest[i] != '<' &&
            digest[i] != '>' &&
            digest[i] != '~' &&
            digest[i] != '[' &&
            digest[i] != ']' &&
            digest[i] != '\\')
        {
            return false;
        }
        i++;
    }

    return true;
}