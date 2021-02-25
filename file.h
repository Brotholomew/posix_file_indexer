#include <sys/types.h>

#include "globals.h"

#ifndef file_structure
#define file_structure

typedef struct file_node {
    char filename[MAX_FILENAME_LENGTH + 1];
    char path[MAX_PATH_LENGTH + 1];
    off_t filesize;
    uid_t owner_uid;
    char type;
} file_node;

#endif

/*
type:
    d - directory
    j - JPEG
    p - PNG
    g - GZIP
    z - ZIP
*/