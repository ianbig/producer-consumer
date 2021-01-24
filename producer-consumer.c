#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
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
#include <sys/stat.h>


#include "producer-consumer.h"


#define SHARED_BTW_THREAD 0
#define NUMBEER_OF_ITEMS 10
#define STORAGE_SIZE 1000000
#define MD5_SUM 32
#define STRUCT_LENGTH 4
#define MAX_FILE_LEN 1000


SIMPDEP_HANDLE hndl;
pthread_mutex_t mutex;
int producer_record[NUMBEER_OF_ITEMS];
int consumer_record[NUMBEER_OF_ITEMS];
unsigned char *answer[NUMBEER_OF_ITEMS];
pthread_t consumer_thraed, producer_thread;

typedef struct {
    unsigned char *md5;
    int width;
    int height;
    unsigned char *bmp_buffer;
    unsigned long bmp_size; // for allocation
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
    pthread_cancel(producer_thread);
    pthread_cancel(consumer_thraed);
}

void img_to_md5(const char *file_path, unsigned char md5_ptr[]) {
    unsigned char c[MD5_DIGEST_LENGTH];
    FILE *inFile = fopen(file_path, "r");
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

    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) 
        md5_ptr[i] = c[i];

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

// -1: FAILED to stat source jpg
// 0:  File does not seem to be a normal JPEG
// 1: normal
int get_jpeg (char *file_path, JPEG_INFO_S *data) {
	int rc, i, j;

	// Variables for the source jpg
	struct stat file_info;
	unsigned long jpg_size;
	unsigned char *jpg_buffer;

	// Variables for the decompressor itself
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// Variables for the output buffer, and how long each row is
	unsigned long bmp_size;
	unsigned char *bmp_buffer;
	int row_stride, width, height, pixel_size;

	rc = stat(file_path, &file_info);
	if (rc) {
		fprintf(stderr, "FAILED to stat source jpg");
		return -1;
	}
	jpg_size = file_info.st_size;
	jpg_buffer = (unsigned char*) malloc(jpg_size + 100);

	int fd = open(file_path, O_RDONLY);
	i = 0;
	while (i < jpg_size) {
		rc = read(fd, jpg_buffer + i, jpg_size - i);
		i += rc;
	}
	close(fd);


	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_decompress(&cinfo);
    // read info in buffer into cinfo
	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);
    // read header
	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		fprintf(stderr, "File does not seem to be a normal JPEG");
		return 0;
	}

	// By calling jpeg_start_decompress, you populate cinfo
	// and can then allocate your output bitmap buffers for
	// each scanline.
	jpeg_start_decompress(&cinfo);
	
	width = cinfo.output_width;
	height = cinfo.output_height;
	pixel_size = cinfo.output_components;


	bmp_size = width * height * pixel_size;
	bmp_buffer = (unsigned char*) malloc(bmp_size);

	// The row_stride is the total number of bytes it takes to store an
	// entire scanline (row). 
	row_stride = width * pixel_size;

	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}

    data->height = height;
    data->width = width;
    data->bmp_buffer = (unsigned char*)malloc(bmp_size);
    for(int i = 0; i < bmp_size; i++) {
        data->bmp_buffer[i] = bmp_buffer[i];
    }
    data->bmp_size = bmp_size;
    
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(jpg_buffer);

	
    /*
	// Write the decompressed bitmap out to a ppm file, just to make sure 
	// it worked. 
	fd = open("output.ppm", O_CREAT | O_WRONLY, 0666);
	char buf[1024];


	rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	write(fd, buf, rc); // Write the PPM image header before data
	write(fd, bmp_buffer, bmp_size); // Write out all RGB pixel data

	close(fd);
    */
    free(bmp_buffer);
	return 1;
}

void* producer(void *arg) {
    fprintf(stderr, "Producer Thread is running\n");
    int current_index = 0;
    int iteration = 0;
    while(1) {
        DIR *folder = NULL;
        struct dirent *entry = NULL;
        char file_path[MAX_FILE_LEN];
        JPEG_INFO_S jpeg_data;
        char *data[4];
        int data_len[4] = {0};

        folder = opendir("/home/ian/image");
        if(folder == NULL) {
            fprintf(stderr, "ERROR Open folder in producer\n");
            return NULL;
        }

        while((entry = readdir(folder))) {
            if(entry->d_name[0] == '.') {
                continue;
            }
            snprintf(file_path, MAX_FILE_LEN, "/home/ian/image/%s", entry->d_name);
            //fprintf(stderr, "placing %s\n", file_path);
            // convert to md5
            data[0] = (char*)malloc(sizeof(char) * MD5_DIGEST_LENGTH);
            img_to_md5(file_path, (unsigned char*)data[0]);
            data_len[0] = MD5_DIGEST_LENGTH + 1;
            
            // get jpeg info
            get_jpeg(file_path, &jpeg_data);
            
            data[1] = (char*)malloc(sizeof(char) * (1 + 1)); // height
            data_len[1] = 1 + 1;
            data[2] = (char*)malloc(sizeof(char) * (1 + 1)); // width
            data_len[2] = 1 + 1;
            data[3] = (char*)malloc(sizeof(char) * jpeg_data.bmp_size); // content
            data_len[3] = jpeg_data.bmp_size;

            //fprintf(stderr, "%d\n", jpeg_data.bmp_size);

            snprintf(data[1], 1 + 1, "%d", jpeg_data.width);
            snprintf(data[2], 1 + 1, "%d", jpeg_data.height);
            snprintf(data[3], jpeg_data.bmp_size, "%s", jpeg_data.bmp_buffer);
            for(int i = 0; i < jpeg_data.bmp_size; i++) {
                data[3][i] = jpeg_data.bmp_buffer[i];
            }
            
            free(jpeg_data.bmp_buffer);
            SIMPDEP_GDATA gd;
            
            // put into buffer
            SIMPDEP_StoreNewData(hndl, 4, data, data_len);
            SIMPDEP_GetData(hndl, &gd, 1);

            usleep(1000);
            memset(file_path, 0, MAX_FILE_LEN);
            
            current_index = atoi(&entry->d_name[6]);
            producer_record[current_index - 1] += 1;
            // fprintf(stderr, "%s: %d\n", entry->d_name, current_index);
            
            for(int i = 0; i < STRUCT_LENGTH; i++) {
                free(data[i]);
            }
            
        }
        closedir(folder);
        //SIMPDEP_Show(hndl);
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
        // free bmp_buffer after getting data
        
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    
    
    CONSUMER_ARG_S consumer_arg;
    PRODUCER_ARG_S producer_arg;

    DIR *folder = NULL;
    struct dirent *entry = NULL;

    clock_t start, elapse;

    int file_index = -1;

    signal(SIGINT, signalhandler);

    // also need to create a sample answer for consumer to test valid of data
    folder = opendir("/home/ian/image"); // refactor to store in variable
    if(folder == NULL) {
        fprintf(stderr, "ERROR: unable to access folder\n");
        exit(EXIT_FAILURE);
    }
    
    // crreate answer
    {
        int data_len[1];
        char file_path[MAX_FILE_LEN];
        fprintf(stderr, "==== Answer ====\n");
        while((entry = readdir(folder))) {
            data_len[0] = MD5_DIGEST_LENGTH + 5;        

            if(entry->d_name[0] == '.') {
                continue;
            }
            snprintf(file_path, MAX_FILE_LEN, "/home/ian/image/%s", entry->d_name);
            file_index = atoi(&entry->d_name[6]);
            answer[file_index - 1] = (unsigned char *)malloc(sizeof(unsigned char) * (MD5_DIGEST_LENGTH + 5) );
            img_to_md5(file_path, answer[file_index - 1]);
            for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", answer[file_index-1][i]);
            printf(" %s\n", file_path);
        }
    }
    closedir(folder);
    

    // create a deposit
    hndl = SIMPDEP_CreateDeposit("memory_buf", STORAGE_SIZE, 3, 0);
    pthread_mutex_init(&mutex, NULL);
    
    pthread_create(&consumer_thraed, NULL, consumer, NULL);
    pthread_create(&producer_thread, NULL, producer, NULL);

    memset(&elapse, 0, sizeof(clock_t));

    start = clock();
    double time_taken = 0.0;
    while(time_taken < 3.0) {
        fprintf(stderr, "%f\n", time_taken);
        elapse = clock() - start;
        time_taken = (double)elapse / CLOCKS_PER_SEC;
    }

    pthread_join(consumer_thraed, NULL);
    pthread_join(producer_thread, NULL);
    
    printf("successfully recycle threads\n");

    SIMPDEP_DestroyDeposit(hndl);  
    
   return 0;
}
