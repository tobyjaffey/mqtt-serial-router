#ifndef MIDUNSUBLIST_H
#define MIDUNSUBLIST_H 1

typedef void (*midunsub_iterate_func)(int mid, const char *id_str, const char *addr_str, const char *topic_str, void *userdata);

extern int midunsub_find(int mid, char **id_str, char **addr_str, char **topic_str);
extern int midunsub_insert(int mid, const char *id_str, const char *addr_str, const char *topic_str);
extern int midunsub_remove(int mid);
extern void midunsub_foreach(midunsub_iterate_func f, void *userdata);
extern void midunsub_print(void);

#endif

