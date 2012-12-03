#ifndef ADDRSUBLIST_H
#define ADDRSUBLIST_H 1

typedef void (*addrsub_iterate_func)(const char *addr_str, const char *topic_str, void *userdata);

extern int addrsub_insert(const char *addr_str, const char *topic_str);
extern int addrsub_remove(const char *addr_str, const char *topic_str);
extern void addrsub_foreach(addrsub_iterate_func f, void *userdata);
extern void addrsub_print(void);
extern int addrsub_count_topic(const char *topic);

#endif

