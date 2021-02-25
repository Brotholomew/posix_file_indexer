#include "index.h"

// global vars only because nftw is used
node * head;
int size;

// global only because of the nature of signal handling
volatile sig_atomic_t last_signal = 0;

void * supervisor(void * arg) {
    update * data = (update *) arg;
    int flag;

    set_sig_handler(SIGALRM, sig_handler);

    // block sigalarm
    sigset_t mask1, oldmask;
    sigemptyset(&mask1);
    sigaddset(&mask1, SIGALRM);

    if (pthread_sigmask(SIG_UNBLOCK, &mask1, &oldmask)) ERR("pthread_sigmask");
    // -----------------------------------------------------------------------------

    do {
        if(pthread_mutex_lock(data->update_mtx)) ERR("pthread_mutex_lock");
        
            // indicate that the indexing job is currently ongoing
            set_mutable(1, data->state, data->state_mtx);
            
            // push the handler in case of immediate thread termination
            pthread_cleanup_push(cancel_handler, (void *) data); 
            
            size = 0;

            // traverse the file tree - and create the temporary queue with all the data
            if(nftw(data->dir, walk, MAXFD, FTW_PHYS) == 0) { ; }
            else message("bad permissions to the specified path");

            // create the matrix with the indexed data
            write_to_array(data);

            // notify the user about the completion of the indexing job
            printf("[%ld]: Indexing complete\n", pthread_self());
            
            // update time
            set_time(time(0), data->last_time, data->time_mtx);

            // set the flag indicating that there is an ongoing indexing to 0
            set_mutable(0, data->state, data->state_mtx);

            // take down the handler for now - the thread cannot be disturbed while saving to the file
            pthread_cleanup_pop(0);

            // if the user wants to terminate the thread immediately during the saving of the file, the thread will
            // terminate after he has successfully saved the file because the termination flag will be set to 0
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

            int index_file_desc = open(data->index_file, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0777);
            if (index_file_desc > 0) {
                save(index_file_desc, data);
                if(close(index_file_desc) == -1) ERR("close");
            }
            else { fprintf(stderr, "WARNING: bad permissions, will not save to file: %s\n", data->index_file);}

            // restore immediate termination
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        if(pthread_mutex_unlock(data->update_mtx)) ERR("pthread_mutex_unlock");
        
        // restore the cancellation handler
        pthread_cleanup_push(cancel_handler, (void *) data);

        get_mutable(&flag, data->flag, data->flag_mtx);
        
        // actively wait for appropriate time if necessary
        stay_awake_for_tt(data);

        pthread_cleanup_pop(0);
       
    // terminate if flag was set to 0
    } while (flag != 0);

    // in case of errors in counting the number of data, ensure that the temporary queue is freed
    while(head != NULL) DeleteMin(&head);

    return NULL;
}

int walk(const char *name, const struct stat *s, int _type, struct FTW *f) {
        // check for immediate termination requests
        pthread_testcancel();

        // if the filetype is not one p
        char type = _type == FTW_DNR || _type == FTW_D ? 'd' : get_magic_num(name);

        // files of unsupported types WON'T be indexed 
        if (type == '~') return 0;

        // increase size of the to-be-created indexing array
        size++;

        file_node * data = (file_node *) malloc(sizeof(file_node));
        if (data == NULL) ERR("malloc");

        get_name_from_path(name, data->filename);
        safe_copy_string(name, data->path);
        data->filesize = s->st_size,
        data->owner_uid = s->st_uid,
        data->type = type;

        // insert data to the temporary queue
        Insert(&head, data);
        return 0;
}

char get_magic_num(const char * path) {
    /*
    type:
        d - directory
        j - JPEG
        p - PNG
        g - GZIP
        z - ZIP
        ~ - unsupported
    */
    char ret = '~';

    char j[2]  = {0xFF, 0xD8};
    char g[3]  = {0x1F, 0x8B, 0x08}; 
    char p[8]  = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    char z1[4] = {0x50, 0x4B, 0x03, 0x04};
    char z2[4] = {0x50, 0x4B, 0x05, 0x06};
    char z3[4] = {0x50, 0x4B, 0x07, 0x08};
    
    int file = open(path, O_RDONLY);
    if (file <= 0) ERR("open");

    char * first8;
    first8 = malloc(8 * sizeof(char));

    // get first 8 bytes of the file
    pread(file, (void *) first8, 8, 0);

    if (memcmp(first8, j,  2) == 0) ret = 'j';
    if (memcmp(first8, g,  3) == 0) ret = 'g';
    if (memcmp(first8, p,  8) == 0) ret = 'p';
    if (memcmp(first8, z1, 4) == 0) ret = 'z';
    if (memcmp(first8, z2, 4) == 0) ret = 'z';
    if (memcmp(first8, z3, 4) == 0) ret = 'z';
    
    free(first8);
    close(file);

    return ret;
}

void save(int desc, update * data) {
    if (write(desc, (char *) data->current_size, sizeof(int)) == -1) ERR("write");
    if (write(desc, (char *) *data->ARRAY, (*data->current_size) * sizeof(file_node)) == -1) ERR("write");
}

void  get_name_from_path(const char * path, char * name) {
    const char * pos = strrchr(path, '/');
    int shift = 1;
    if (pos == NULL){
        pos = path;
        shift = 0;
    } 

    int i;
    // make the output clean
    for(i  = 0; i <= MAX_FILENAME_LENGTH; i++) {
        name[i] = '\0'; 
    }

    for (i = 0; i <= MAX_FILENAME_LENGTH; i++) {
        name[i] = pos[i + shift];
        if (pos[i + shift] == '\0') {
            break;
        }

        if (i == MAX_FILENAME_LENGTH) {
            name[i] = '\0';
            // too long filename does not exclude the file from the index but it raises a warning
            message("name of file too long, trimmed!");
            break;
        }
    }
} 

void safe_copy_string(const char * in, char * out) {
    int i = 0, k = strlen(in), start = 0, j = 0;

    // make the output clean
    for (i = 0; i <= MAX_PATH_LENGTH; i++)
        out[i] = '\0';

    if (k > 0 && in[0] == '.') {
        // concatenate with absolute pathname if not provided
        getcwd(out, MAX_PATH_LENGTH);
        start = strlen(out);
        j = 1;

        if (k > 1 && in[1] == '.') {
            j = 2;
        }
    }

    
    for (i = start; i - start < k; i++) {
        out[i] = in[j];
        j++;
        
        if (i == MAX_PATH_LENGTH) {
            out[i] = '\0';
            // too long path does not exclude the file from the index but it raises a warning
            message("name of the path too long, trimmed!");
            break;
        }
    } 
}

void cancel_handler(void * arg) {
    update * data = (update *) arg;

    // unlock the mutex if locked
    pthread_mutex_trylock(data->update_mtx);
    pthread_mutex_unlock(data->update_mtx);

    pthread_mutex_trylock(data->time_mtx);
    pthread_mutex_unlock(data->time_mtx);

    pthread_mutex_trylock(data->array_mtx);
    pthread_mutex_unlock(data->array_mtx);

    pthread_mutex_trylock(data->state_mtx);
    pthread_mutex_unlock(data->state_mtx);

    pthread_mutex_trylock(data->flag_mtx);
    pthread_mutex_unlock(data->flag_mtx);

    // free the temporary queue
    while(head != NULL) {
        DeleteMin(&head);
    }
}

void set_sig_handler(int sig, void (*handler)(int)) {
    struct sigaction s1;
    memset(&s1,0,sizeof(struct sigaction));
    s1.sa_handler = handler;
    if(sigaction(sig,&s1,NULL)) ERR("sigaction");
}

void sig_handler(int sig) {
    last_signal = sig;
}

void write_to_array(update * data) {
    int i;

    if(pthread_mutex_lock(data->array_mtx)) ERR("pthread_mutex_lock");

        // update array size
        (*data->current_size) = size;

        // realloc the array
        *data->ARRAY = (file_node *) realloc(*data->ARRAY, (*data->current_size) * sizeof(file_node));
        if ((*data->current_size) > 0 && *data->ARRAY == NULL) ERR("realloc");

        // assign the data
        for (i = 0; i < (*data->current_size); i++) {
            // avoid saving uninitialised data
            memset(&(*data->ARRAY)[i], 0, sizeof((*data->ARRAY)[i]));

            // check for immediate termination requests
            pthread_testcancel();
            strncpy((*data->ARRAY)[i].filename, head->data->filename, MAX_FILENAME_LENGTH + 1);
            strncpy((*data->ARRAY)[i].path, head->data->path, MAX_PATH_LENGTH + 1);
            (*data->ARRAY)[i].filesize = head->data->filesize;
            (*data->ARRAY)[i].owner_uid = head->data->owner_uid;
            (*data->ARRAY)[i].type = head->data->type;

            DeleteMin(&head);
        }

    if(pthread_mutex_unlock(data->array_mtx)) ERR("pthread_mutex_unlock");
}

void stay_awake_for_tt(update * data) {
    long last_time;
    int flag;

    // check the last time of the indexing job
    get_time(&last_time, data->last_time, data->time_mtx);
    get_mutable(&flag, data->flag, data->flag_mtx);

    long tt =  (*data->t) - (time(0) - last_time);

    while(flag != 0 && tt > 0) {
        // check the last time of the indexing job
        get_time(&last_time, data->last_time, data->time_mtx);

        tt =  (*data->t) - (time(0) - last_time);
        printf("[%ld]: next index in: %lds\n", pthread_self(), tt);
        alarm(tt);

        while(last_signal == 0 && tt > 0) {
            pthread_testcancel();

            get_mutable(&flag, data->flag, data->flag_mtx);
            if (flag == 0) break;
        }

        last_signal = 0;
    }
}