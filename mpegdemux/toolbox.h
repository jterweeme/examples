#ifndef _TOOLBOX_H_
#define _TOOLBOX_H_
class Toolbox
{
public:
    static int str_get_streams (const char *str, unsigned char stm[256], unsigned msk);
    static const char *str_skip_white (const char *str);
    static char *str_clone (const char *str);
};
#endif



