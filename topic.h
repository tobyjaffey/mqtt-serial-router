#ifndef TOPIC_H
#define TOPIC_H 1

typedef enum
{
    TOPIC_NODE_TYPE_STR,
    TOPIC_NODE_TYPE_PLUS,
    TOPIC_NODE_TYPE_HASH
} topic_node_type_t;

struct topic_node
{
    topic_node_type_t type;
    char *str;
    struct topic_node *next;
};
typedef struct topic_node topic_node_t;


extern topic_node_t *topic_create(void);
extern void topic_destroy(topic_node_t *tn);
extern void topic_print(topic_node_t *root);
extern bool topic_match_string(topic_node_t *rule, const char *topicstr);
extern topic_node_t *topic_create_from_string(const char *topicstr);

#endif

