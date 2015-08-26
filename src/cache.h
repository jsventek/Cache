#ifndef _CACHE_H_
#define _CACHE_H_
// headers for some minimal functionality to make working with cache data easier... should eventually include some simplified functions for basic cache operations

typedef struct rtab Rtab;

Rtab *rtab_new();
void rtab_free(Rtab *results);
Rtab *rtab_unpack(char *packed, int len);

int rtab_status(Rtab* r);
int rtab_ncols(Rtab* r);
int rtab_nrows(Rtab* r);
char* rtab_message(Rtab* r);
char* rtab_data(Rtab* r, int row, int col);

#endif
