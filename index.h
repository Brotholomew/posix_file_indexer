#define _XOPEN_SOURCE 500
#define MAXFD 100

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ftw.h>
#include <signal.h>

#include "priority_queue.h"
#include "file.h"
#include "update_struct.h"
#include "globals.h"

#ifndef index
#define index

int walk(const char *name, const struct stat *s, int type, struct FTW *f);
char get_magic_num(const char * path);
void * supervisor(void * arg);
void save(int desc, update * data);
void get_name_from_path(const char * path, char * name);
void safe_copy_string(const char * in, char * out);
void cancel_handler(void * arg);
void set_sig_handler(int sig, void (*handler)(int));
void sig_handler(int sig);
void write_to_array(update * data);
void stay_awake_for_tt(update * data);

#endif