#ifndef MIDSUBLIST_H
#define MIDSUBLIST_H 1

typedef void (*midsub_iterate_func)(int mid, const char *id_str, const char *addr_str, const char *topic_str, void *userdata);

extern int midsub_find(int mid, char **id_str, char **addr_str, char **topic_str);
extern int midsub_insert(int mid, const char *id_str, const char *addr_str, const char *topic_str);
extern int midsub_remove(int mid);
extern void midsub_foreach(midsub_iterate_func f, void *userdata);
extern void midsub_print(void);

#endif

