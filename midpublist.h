#ifndef MIDPUBLIST_H
#define MIDPUBLIST_H 1

typedef void (*midpub_iterate_func)(int mid, const char *id_str, const char *addr_str, void *userdata);

extern int midpub_find(int mid, char **id_str, char **addr_str);
extern int midpub_insert(int mid, const char *id_str, const char *addr_str);
extern int midpub_remove(int mid);
extern void midpub_foreach(midpub_iterate_func f, void *userdata);
extern void midpub_print(void);

#endif

