#ifndef __PRODUCER_CONSUMER_H__
#define __PRODUCER_CONSUMER_H__

#include "simple_deposit.h"

typedef struct {
    char *file_path;
} PRODUCER_ARG_S;

typedef struct {
    char *file_path;
} CONSUMER_ARG_S;

void producer(char *file_path);
void consumer(char *file_path);
void img_to_md5(const char *file_path, unsigned char md5_ptr[]);
int md5_string_cmp(unsigned char *arg1, unsigned char *arg2);
void find_file(SIMPDEP_GDATA *gd);

#endif