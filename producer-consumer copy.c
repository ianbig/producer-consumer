#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <semaphore.h>
#include <string.h>
#include <jpeglib.h>
#include <openssl/md5.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "producer-consumer.h"


#define SHARED_BTW_THREAD 0
#define NUMBEER_OF_ITEMS 10
#define IMG_MAX 100000
#define MD5_SUM 32


SIMPDEP_HANDLE hndl;
pthread_mutex_t mutex;
int producer_record[NUMBEER_OF_ITEMS];
int consumer_record[NUMBEER_OF_ITEMS];
unsigned char *answer[NUMBEER_OF_ITEMS];

typedef struct {
    unsigned char md5;
    int width;
    int height;
} JPEG_INFO_S;

void signalhandler(int signum) {
    fprintf(stderr, "Info for Producer\n");
    for(int i = 0; i < NUMBEER_OF_ITEMS; i++) {
        fprintf(stderr, "Image %d is placed into buffer %d times\n", i + 1, producer_record[i]);
    }

    fprintf(stderr, "Info for Consumer\n");
    for(int i = 0; i < NUMBEER_OF_ITEMS; i++) {
        fprintf(stderr, "Image %d is retrieve from buffer %d times\n", i + 1, consumer_record[i]);
    }  
    exit(1);
}

void img_to_md5(const char *file_path, unsigned char md5_ptr[]) {
    unsigned char c[MD5_DIGEST_LENGTH];
    FILE *inFile = fopen(file_path, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char fdata[1024];


    if (inFile == NULL) {
        printf ("%s can't be opened\n", file_path);
        return;
    }

    MD5_Init(&mdContext);
    while ((bytes = fread (fdata, 1, 1024, inFile)) != 0) {
        MD5_Update(&mdContext, fdata, bytes);
    }
    MD5_Final(c,&mdContext);

    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) md5_ptr[i] = c[i];

    fclose(inFile);
}

int md5_string_cmp(unsigned char *arg1, unsigned char *arg2) {
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if(arg1[i] != arg2[i]) return 0;
    }
    return 1;
}

void find_file(SIMPDEP_GDATA *gd) {
     // check which file
    for(int i = 0; i < NUMBEER_OF_ITEMS; i++) {
        if(gd->ret_data != NULL) {
            if( md5_string_cmp((unsigned char*)gd->ret_data, answer[i]) ) {
                consumer_record[i] += 1;
            }
        }
    }
}

void* producer(void *arg) {


    fprintf(stderr, "Producer Thread is running\n");
    int current_index = 0;
    int iteration = 0;
    while(1) {
        DIR *folder = NULL;
        struct dirent *entry = NULL;
        char file_path[IMG_MAX];
        char *data[1];
        int data_len[1] = {0};

        folder = opendir("/home/ian/image");
        if(folder == NULL) {
            fprintf(stderr, "ERROR Open folder in producer\n");
            return NULL;
        }

        data[0] = (char*)malloc(1024 * sizeof(char));
        while((entry = readdir(folder))) {
            if(entry->d_name[0] == '.') {
                continue;
            }
            snprintf(file_path, IMG_MAX, "/home/ian/image/%s", entry->d_name);

            // convert to md5
            img_to_md5(file_path, (unsigned char*)data[0]);
            //for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", (unsigned char)data[0][i]);
            //printf(" %s\n", file_path);
            data_len[0] = MD5_DIGEST_LENGTH;

            // put into buffer
            SIMPDEP_StoreNewData(hndl, 1, data, data_len); // test without lock
            usleep(1000);
            memset(file_path, 0, IMG_MAX);
            memset(data[0], 0, 1024 * 1);
            
            current_index = atoi(&entry->d_name[6]);
            producer_record[current_index - 1] += 1;
            // fprintf(stderr, "%s: %d\n", entry->d_name, current_index);
        }

        closedir(folder);
        // SIMPDEP_Show(hndl);
        free(data[0]);
    }
    
}

void* consumer(void *arg) {
    fprintf(stderr, "Consumer thread is running\n");
    int current_index = 0;
    int iteration = 0;
    int i = 0;
    int ret = 1; // getData 0 = success -1 = fail

    SIMPDEP_GDATA gd;

    while(1) {

        // get data from buffer
        ret = SIMPDEP_GetData(hndl, &gd, 1);

        if(ret == -1) {
            if(gd.status == SIMPDEP_GSTATUS_TOOSLOW_CATCHOLDESTP1) continue; // data being overwrite
            usleep(500); // not exist yet
        }

        find_file(&gd);
        SIMPDEP_UnlockData(hndl, &gd);
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    pthread_t consumer_thraed, producer_thread;
    CONSUMER_ARG_S consumer_arg;
    PRODUCER_ARG_S producer_arg;

    DIR *folder = NULL;
    struct dirent *entry = NULL;

    int file_index = -1;

    signal(SIGINT, signalhandler);

    // get related folder image: number of img, max_img_size
    // also need to create a sample answer for consumer to test valid of data
    folder = opendir("/home/ian/image"); // refactor to store in variable
    if(folder == NULL) {
        fprintf(stderr, "ERROR: unable to access folder\n");
        exit(EXIT_FAILURE);
    }

    // hndl = SIMPDEP_CreateDeposit("memory_buf", MD5_DIGEST_LENGTH * NUMBEER_OF_ITEMS, img_num, 0); // test
    // crreate answer
    fprintf(stderr, "==== Answer ====\n");
    while((entry = readdir(folder))) {
        int data_len[1];
        data_len[0] = MD5_DIGEST_LENGTH + 5;
        char file_path[IMG_MAX];

        if(entry->d_name[0] == '.') {
            continue;
        }
        snprintf(file_path, IMG_MAX, "/home/ian/image/%s", entry->d_name);
        file_index = atoi(&entry->d_name[6]);
        answer[file_index - 1] = (unsigned char *)malloc(sizeof(unsigned char) * (MD5_DIGEST_LENGTH + 5) );
        img_to_md5(file_path, answer[file_index - 1]);
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", answer[file_index-1][i]);
        printf(" %s\n", file_path);
    }
    closedir(folder);

    // create a deposit
    hndl = SIMPDEP_CreateDeposit("memory_buf", MD5_DIGEST_LENGTH * NUMBEER_OF_ITEMS, 3, 0);
    pthread_mutex_init(&mutex, NULL);
    
    pthread_create(&consumer_thraed, NULL, consumer, NULL);
    pthread_create(&producer_thread, NULL, producer, NULL);

    pthread_join(consumer_thraed, NULL);
    pthread_join(producer_thread, NULL);


    SIMPDEP_DestroyDeposit(hndl);  
    
   return 0;
}
