#define _XOPEN_SOURCE 500 /* getopt */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include "file.h"
#include "priority_queue.h"
#include "update_struct.h"
#include "index.h"
#include "globals.h"
#include "stack.h"

void parse_input(int argc, char ** argv, char ** dir, char ** index_file, int * t, int * flg);
void usage(char * msg);
void load(int desc, update * data);
void extract_from_string(char * from, char * to, int start, int max);
void printer(node * head, int count);
int read_from_stdin(char * str);
int check_strtol(char * nptr, char * endptr, long number);
void console(update * data);
void run_indexing(pthread_t * tid, update * data);
void handle_index_request(update * data, int * index_requested, elem ** index_request_stack);
void display_count(update * data);
void print_largerthan(update * data, char * largerthan);
void print_owner(update * data, char * owner_uid);
void print_namepart(update * data, char * namepart);


int main(int argc, char ** argv) {
    // prepare variables
    char * dir = NULL;
    char * index_file = NULL;
    int _t, _flag, _state, _current_size;
    __time_t _last_time;
    file_node * _ARRAY;

    _state        = 0;
    _current_size = 0;
    _last_time    = 0;
    _ARRAY        = NULL;
    // -----------------------------------------------------------------------------

    // prepare mutexes
    pthread_mutex_t _update_mtx;
    if (pthread_mutex_init(&_update_mtx, NULL)) ERR("couldn't initialise mutex!");

    pthread_mutex_t _flag_mtx;
    if (pthread_mutex_init(&_flag_mtx, NULL)) ERR("couldn't initialise mutex!");
    
    pthread_mutex_t _state_mtx;
    if (pthread_mutex_init(&_state_mtx, NULL)) ERR("couldn't initialise mutex!");

    pthread_mutex_t _array_mtx;
    if (pthread_mutex_init(&_array_mtx, NULL)) ERR("couldn't initialise mutex!");

    pthread_mutex_t _time_mtx;
    if (pthread_mutex_init(&_time_mtx, NULL)) ERR("couldn't initialise mutex!");
    // -----------------------------------------------------------------------------

    int index_file_is_on_stack = 1;
    parse_input(argc, argv, &dir, &index_file, &_t, &index_file_is_on_stack);

    // set the flag accordingly to match the user's choice regarding periodic indexing
    _flag = _t;

    // create and initialise the structure which is shared between the threads
    update data;

    data.update_mtx = &_update_mtx;
    data.flag_mtx = &_flag_mtx;
    data.state_mtx = &_state_mtx;
    data.array_mtx = &_array_mtx;
    data.time_mtx = &_time_mtx;

    data.current_size = &_current_size;
    data.last_time = &_last_time;
    data.ARRAY = &_ARRAY;
    data.state = &_state;
    data.flag = &_flag; 
    data.dir = dir;
    data.index_file = index_file;
    data.t = &_t;
    // -----------------------------------------------------------------------------

    // block sigalarm
    sigset_t mask1, oldmask;
    sigemptyset(&mask1);
    sigaddset(&mask1, SIGALRM);

    if (pthread_sigmask(SIG_BLOCK, &mask1, &oldmask)) ERR("pthread_sigmask");
    // -----------------------------------------------------------------------------
    
    console(&data);
    
    // free mutexes
    if (pthread_mutex_destroy(&_update_mtx)) ERR("couldn't destroy mutex!");
    if (pthread_mutex_destroy(&_flag_mtx)) ERR("couldn't destroy mutex!");
    if (pthread_mutex_destroy(&_state_mtx)) ERR("couldn't destroy mutex!");
    if (pthread_mutex_destroy(&_array_mtx)) ERR("couldn't destroy mutex!");
    if (pthread_mutex_destroy(&_time_mtx)) ERR("couldn't destroy mutex!");
    // -----------------------------------------------------------------------------

    free(*data.ARRAY);
    if (index_file_is_on_stack == 0) free(data.index_file);

    return EXIT_SUCCESS;
}

void parse_input(int argc, char ** argv, char ** dir, char ** index_file, int * t, int * flg) {
    *t = 0;

    int i;
    char c;

    for (i = 1; i < argc; i++) {
        while (-1 != (c = getopt(argc, argv, "d:f:t:"))) {
            switch (c) {
                case 'd':
                    *dir = optarg;
                    break;
                case 'f':
                    *index_file = optarg;
                    break;
                case 't':
                    *t = atoi(optarg);

                    if (*t < 30 || *t > 7200) usage("t should be in range [30, 7200]");
                    break;
            }
        }
    }

    // set the dir to the dir in the env variable if not provided
    if (*dir == NULL) {
        char * temp = getenv("MOLE_DIR");
        if (temp == NULL) usage("path to the dir that is to be indexed is not set");
        *dir = temp;
    }

    // set the index file to the env variable or file in home path if not provided
    if (*index_file == NULL) {
        *flg = 0;
        char * temp = getenv("MOLE_INDEX_PATH");
        if (temp == NULL) {
            char * home = getenv("HOME");
            int length = strlen(home) + strlen("/.mole-index") + 1;
            temp = (char *) malloc(length * sizeof(char));
            strcpy(temp, home);
            temp = strcat(temp, "/.mole-index");
        }

        *index_file = temp;
    }
}

void usage(char * msg) {
    fprintf(stderr, "USAGE: %s ([-d path to dir] [-f path to index file] [-t time to index again periodically])\n", msg);
    exit(EXIT_FAILURE);
}


void load(int desc, update * data) {
    struct stat filestat;
    fstat(desc, &filestat);

    if(pthread_mutex_lock(data->update_mtx)) ERR("pthread_mutex_lock");
        // set last_time to the time of the file modification
        if(pthread_mutex_lock(data->time_mtx)) ERR("pthread_mutex_lock");
            (*data->last_time) = filestat.st_mtime;
        if(pthread_mutex_unlock(data->time_mtx)) ERR("pthread_mutex_unlock");

        // read the size of the array from the file
        if (read(desc, (char *) data->current_size, sizeof(int)) == -1) ERR("read");

        // allocate array
        if(pthread_mutex_lock(data->array_mtx)) ERR("pthread_mutex_lock");
            *data->ARRAY = (file_node *) malloc((*data->current_size) * sizeof(file_node));
            if (data->ARRAY == NULL) ERR("malloc");

            if (read(desc, (char *) *data->ARRAY, (*data->current_size) * sizeof(file_node)) == -1) ERR("read");
        if(pthread_mutex_unlock(data->array_mtx)) ERR("pthread_mutex_unlock");

    if(pthread_mutex_unlock(data->update_mtx)) ERR("pthread_mutex_unlock");
}

void extract_from_string(char * from, char * to, int start, int max) {
    int i;
    // clean the string
    for (i = 0; i < max; i++) {
        to[i] = '\0';
    }

    int j = 0;
    // assign appriopriate values
    for (i = start; i < strlen(from); i++) {
        to[j] = from[i];
        j++;
    }
}

void printer(node * head, int count) {
    char * pipeline = getenv("PAGER");
    FILE * write;

    if (pipeline == NULL || count <= 3) write = stdout;
    else write = popen(pipeline, "w");

    char * name = "name";
    char * path = "path";
    char * type = "type";
    char * size = "size";

    fprintf(write, "--------------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(write, "| %-50s | %-5s | %-10s | %-50s | \n", name, type, size, path);
    fprintf(write, "--------------------------------------------------------------------------------------------------------------------------------\n");

    while(head != NULL) {
        fprintf(write, "| %50s | %5c | %10ld | %-50s\n", head->data->filename, head->data->type, head->data->filesize, head->data->path);
        fprintf(write, "--------------------------------------------------------------------------------------------------------------------------------\n");
        head = head->next;
    }
    
    if (pipeline != NULL && count > 3) pclose(write);
}

int read_from_stdin(char * str) {
    //memset(str, '\0', MAX_INPUT_LENGTH + 1);
    char buf[LINE_MAX + 1]; 
    //memset(buf, '\0', LINE_MAX + 1);
    int i;

    // get line from stdin
    if (fgets(buf, LINE_MAX, stdin) == NULL) ERR("fgets");
    for (i = 0; i < LINE_MAX + 1 ; i++) {
        // if the command's size is surplus in length return 1;
        if (i > MAX_INPUT_LENGTH) { str[i-1] = '\0'; message("Too long command!"); return 1; }

        if (buf[i] == 10 || buf[i] == '\0') {
            str[i] = '\0';
            break;
        }  
        str[i] = buf[i];
    }

    // make the rest of the string null
    for(i = i; i < MAX_INPUT_LENGTH + 1; i++) {
        str[i] = '\0';
    }

    return 0;
}

void console(update * data) {

    int index_file_desc = open(data->index_file, O_RDONLY);

    // the main thread which will perform indexing
    pthread_t single_index_thread;
    int indexed_at_start = 0;

    // the thread that will perform additional indexing
    elem * requested_indexing_stack = NULL;
    int index_requested = 0;    

    if (index_file_desc > 0) { 
        // successfully opened the pointed file
        
        load(index_file_desc, data); 
        if(close(index_file_desc) == -1) ERR("close");

        if (*(data->t) > 0) {
            // if periodic indexing is intended by the user, run the indexing thread
            run_indexing(&single_index_thread, data);

            // set flag to indicate that the obligaroty indexing started 
            indexed_at_start = 1;
        }        
    }
    else if (index_file_desc == -1) {
        // file did not open successfully - run the indexing thread
        run_indexing(&single_index_thread, data);
        
        // set flag to indicate that the obligaroty indexing started 
        indexed_at_start = 1;
    }

    // enter the console loop
    while(1) {
        char command[MAX_INPUT_LENGTH + 1];

        // read from stdin
        if (read_from_stdin(command) == 1) continue;

        if (strcmp(command, "exit") == 0) {
            // nudge the currrently indexing thread to finish job 'politely'
            // set the termination flag to 0 
            set_mutable(0, data->flag, data->flag_mtx);
            break;
        }

        if (strcmp(command, "exit!") == 0) {
            // require immediate termination of the indexing job
            if (indexed_at_start == 1) pthread_cancel(single_index_thread);
            
            // indexing thread can't be cancelled while saving progress to the file
            // set flag to 0 so that the thread will exit when possible
            set_mutable(0, data->flag, data->flag_mtx);

            // cancel all the requested indexing threads if any
            elem * p = requested_indexing_stack;
            while (index_requested ==  1 && p != NULL) {
                p = p->next;
            }

            break;
        }

        if (strcmp(command, "index") == 0) {
            handle_index_request(data, &index_requested, &requested_indexing_stack);
            continue;
        }

        if (strcmp(command, "count") == 0) {
            display_count(data);
            continue;
        }

        char * largerthan = strstr(command, "largerthan");
        char * namepart = strstr(command, "namepart");
        char * owner_uid = strstr(command, "owner");

        if (largerthan != NULL) {
            print_largerthan(data, largerthan);
            continue;
        }

        if (namepart != NULL) {
            print_namepart(data, namepart);
            continue;
        }

        if (owner_uid != NULL) {
            print_owner(data, owner_uid);
            continue;
        } 

        message("unknown command!\n");
    }

    // if needed wait for the threads 
    if(indexed_at_start == 1 && pthread_join(single_index_thread, NULL)) {
        if (errno != ESRCH && errno != EINVAL)
            ERR("pthread_join");
    }

    while (index_requested ==  1 && requested_indexing_stack != NULL) {
        if(pthread_join(requested_indexing_stack->thread_id, NULL)) {
            if (errno != ESRCH && errno != EINVAL)
                ERR("pthread_join");
        }
        
        pop(&requested_indexing_stack);
    }
}

void run_indexing(pthread_t * tid, update * data) {
    int ret = pthread_create(tid, NULL, supervisor, (void *) data);
    if (ret) ERR("pthread_create");
}

void handle_index_request(update * data, int * index_requested, elem ** requested_indexing_stack) {
    int temp_state;

    // check whether there is an ongoing indexing process
    get_mutable(&temp_state, data->state, data->state_mtx);

    if (temp_state == 1) {
        // if so alert and do nothing
        message("There is an ongoing indexing process already!");
    }
    else if (temp_state == 0) {
        // perform the additional indexing task
        *index_requested = 1;

        // prepare new temporary data stucture to be shared with the new thread
        // this thread will not index periodically, so set flag and t to 0
        update * temp_data = calloc(1, sizeof(update));
        *temp_data = *data;

        push_new(requested_indexing_stack);
        (*requested_indexing_stack)->val = 0;
        (*requested_indexing_stack)->data = temp_data;
        temp_data->flag = &((*requested_indexing_stack)->val);
        
        // create thread
        run_indexing(&(*requested_indexing_stack)->thread_id, temp_data);
    }
}

void display_count(update * data) {
    int i, d = 0, j = 0, p = 0, g = 0, z = 0;

    // perform operations only if the ARRAY is not currently being created!
    // if there is an ongoing indexing job - present old data
    if(pthread_mutex_lock(data->array_mtx)) ERR("pthread_mutex_lock");

        for (i = 0; i < (*data->current_size); i++) {
            switch((*data->ARRAY)[i].type) {
                case 'd':
                    d++;
                    break;
                case 'j':
                    j++;
                    break;
                case 'p':
                    p++;
                    break;
                case 'g':
                    g++;
                    break;
                case 'z':
                    z++;
                    break;
            }
        }

    if(pthread_mutex_unlock(data->array_mtx)) ERR("pthread_mutex_unlock");

    printf("------------------------\n");
    printf("| type         | count |\n");
    printf("------------------------\n");
    printf("| directories  | %5d |\n", d);
    printf("------------------------\n");
    printf("| JPEGs        | %5d |\n", j);
    printf("------------------------\n");
    printf("| PNGs         | %5d |\n", p);
    printf("------------------------\n");
    printf("| GZIPs        | %5d |\n", g);
    printf("------------------------\n");
    printf("| ZIPs         | %5d |\n", z);
    printf("------------------------\n");
}

void print_largerthan(update * data, char * largerthan) {
    int i;
    char * endptr = NULL;

    char temp_size[MAX_INPUT_LENGTH];
    extract_from_string(largerthan, temp_size, 10, MAX_INPUT_LENGTH);

    long size = strtol(temp_size, &endptr, 10);
    if (check_strtol(temp_size, endptr, size) == 1) { return; }
    
    // perform operations only if the ARRAY is not currently being created!
    // if there is an ongoing indexing job - present old data
    if(pthread_mutex_lock(data->array_mtx)) ERR("pthread_mutex_lock");

        i = 0;
        // find the position from which the UIDs will be greater than the provided value
        while (i < *data->current_size && (*data->ARRAY)[i].filesize <= size) {
            i = i + 1;
        }

        if (i > *data->current_size) { printf("found no files that are greater than %ld in size", size); return; }

        node * temp_head = NULL;
        int count = 0;

        int j;
        // create a temporary queue for printing
        for (j = *data->current_size - 1; j >= i; j--) {
            Insert(&temp_head, &(*data->ARRAY)[j]);
            count++;
        }

        // print matching data
        printer(temp_head, count);

    if(pthread_mutex_unlock(data->array_mtx)) ERR("pthread_mutex_unlock");

    // free the pointers to the temporary queue without affecting the data
    burn_down(&temp_head);
}

void print_namepart(update * data, char * namepart) {
    int i;

    char temp_part[MAX_INPUT_LENGTH];
    extract_from_string(namepart, temp_part, 9, MAX_INPUT_LENGTH);

    // perform operations only if the ARRAY is not currently being created!
    // if there is an ongoing indexing job - present old data
    if(pthread_mutex_lock(data->array_mtx)) ERR("pthread_mutex_lock");
        
        node * temp_head = NULL;
        int count = 0;

        // add matching data to the temporary queue
        for (i = *data->current_size - 1; i >= 0; i--) {
            if (strstr((*data->ARRAY)[i].filename, temp_part) != NULL) {
                Insert(&temp_head, &(*data->ARRAY)[i]); 
                count++;
            }
        }

        // print matching data
        printer(temp_head, count);

    if(pthread_mutex_unlock(data->array_mtx)) ERR("pthread_mutex_unlock");

    // free the pointers to the temporary queue without affecting the data
    burn_down(&temp_head);
}

void print_owner(update * data, char * owner_uid) {
    int i;
    char * endptr = NULL;

    char temp_uid[MAX_INPUT_LENGTH]; 
    extract_from_string(owner_uid, temp_uid, 5, MAX_INPUT_LENGTH);

    long uid = strtol(temp_uid, &endptr, 10);
    if (check_strtol(temp_uid, endptr, uid) == 1) { return; }

    // perform operations only if the ARRAY is not currently being created!
    // if there is an ongoing indexing job - present old data
    if(pthread_mutex_lock(data->array_mtx)) ERR("pthread_mutex_lock");

        node * temp_head = NULL;
        int count = 0;

        // add matching data to the temporary queue
        for (i = *data->current_size - 1; i >= 0; i--) {
            if ((*data->ARRAY)[i].owner_uid == uid) {
                Insert(&temp_head, &(*data->ARRAY)[i]);
                count++;
            }
        }

        // print matching data
        printer(temp_head, count);

    if(pthread_mutex_unlock(data->array_mtx)) ERR("pthread_mutex_unlock");

    // free the pointers to the temporary queue without affecting the data
    burn_down(&temp_head);
}

int check_strtol(char * nptr, char * endptr, long number) {
    if (nptr == endptr)
        { message("provided number is invalid (no digits found)"); return 1; }
    else if (errno == ERANGE && number == LONG_MIN)
        { message("provided number is invalid (underflow occurred)"); return 1; }
    else if (errno == ERANGE && number == LONG_MAX)
        { message("provided number is invalid (overflow occurred)"); return 1; }
    else if (errno != 0 && number == 0)
        { message("provided number is invalid (unspecified error occurred)"); return 1; }

    return 0;
}