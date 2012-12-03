#ifndef CMD_H
#define CMD_H 1

int cmd_process(const char *orig_line);
void cmd_publish_cb(int mid);
void cmd_subscribe_cb(int mid);
void cmd_message_cb(const char *topic, const char *msg);
void cmd_unsubscribe_cb(int mid);

#endif

