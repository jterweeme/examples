#include "toolbox.h"
#include <cstring>
#include <cstdlib>

char *Toolbox::str_clone (const char *str)
{
    char *ret = (char *)malloc (strlen (str) + 1);

    if (ret == NULL)
        return (NULL);

    strcpy (ret, str);
    return (ret);
}

const char *Toolbox::str_skip_white (const char *str)
{
    while ((*str == ' ') || (*str == '\t'))
        str += 1;

    return str;
}

int Toolbox::str_get_streams (const char *str, unsigned char stm[256], unsigned msk)
{
    unsigned i;
    int      incl;
    char     *tmp;
    unsigned stm1, stm2;

    incl = 1;

    while (*str != 0) {
        str = str_skip_white (str);

        if (*str == '+') {
            str += 1;
            incl = 1;
        }
        else if (*str == '-') {
            str += 1;
            incl = 0;
        }
        else {
            incl = 1;
        }

        if (strncmp (str, "all", 3) == 0) {
            str += 3;
            stm1 = 0;
            stm2 = 255;
        }
        else if (strncmp (str, "none", 4) == 0) {
            str += 4;
            stm1 = 0;
            stm2 = 255;
            incl = !incl;
        }
        else {
            stm1 = (unsigned) strtoul (str, &tmp, 0);
            if (tmp == str) {
                return (1);
            }

            str = tmp;

            if (*str == '-') {
                str += 1;
                stm2 = (unsigned) strtoul (str, &tmp, 0);
                if (tmp == str) {
                    return (1);
                }
                str = tmp;
            }
            else {
                stm2 = stm1;
            }
        }
        if (incl) {
            for (i = stm1; i <= stm2; i++) {
                stm[i] |= msk;
            }
        }
        else {
            for (i = stm1; i <= stm2; i++) {
                stm[i] &= ~msk;
            }
        }

        str = str_skip_white (str);

        if (*str == '/') {
            str += 1;
        }
    }

    return (0);
}



