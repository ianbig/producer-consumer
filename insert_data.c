/**
 * Create Deposit and Insert Data
    * test exceed maximum item
    * test exceed maximum allow size
    * test input multiple data at one time
    * test input multiple data but give wrong parameter: wrong number of item, wrong data content
 * GetData
    * test lock and without lock
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_deposit.h"

#define DATA_ITEMS 1
#define NUMER_OF_INSERT_ITERATION 3
#define MAX_ITEMS_SIZE 1

int main() {
    SIMPDEP_HANDLE hdl = NULL;
    int data_size_rand = 0;
    char *l_mdata[DATA_ITEMS];
    int l_mdata_len[DATA_ITEMS];
    char *data[DATA_ITEMS];
    int index = 23;
    int input_data_len = 0;

    SIMPDEP_GDATA gd;


    // create testing data
    for(int i = 0; i < DATA_ITEMS; i++) {
        srand(i + 1);
        data_size_rand = rand() * 1024 + rand();
        if( data_size_rand < 0) {
            data_size_rand *= -1; // avoid overflow
        }
        data_size_rand = data_size_rand % MAX_ITEMS_SIZE;
        printf("item %d size is %d\n", i, data_size_rand + 1);
        data[i] = (char*)malloc(sizeof(char) * (data_size_rand + 5));
        int num = 100;
        snprintf(data[i], data_size_rand + 5, "%d", num);
        fprintf(stderr, "Item %d's Content: %s\n", i, data[i]);

        l_mdata[i] = data[i];
        l_mdata_len[i] =  data_size_rand + 5;
    }

    // create memory buffer
    hdl = SIMPDEP_CreateDeposit("memory_buffer", (data_size_rand + 5) * 3, 3, 0);
    int ret = 0;
    if(hdl != NULL) {
        // SIMPDEP_Show(hdl);
        for(int i = 0; i < NUMER_OF_INSERT_ITERATION; i++) {
            input_data_len = SIMPDEP_StoreNewData(hdl, DATA_ITEMS, l_mdata, l_mdata_len); // strange
            ret = SIMPDEP_GetData(hdl, &gd, 1);
        }

        fprintf(stderr, "ret is %d serial is %d status: %d size: %d data: %s\n", ret, gd.ret_data_serial, gd.status, gd.ret_data_sz, gd.ret_data);
        fprintf(stderr, "expect serial: %d\n", gd.expect_serial);
        SIMPDEP_Show(hdl);

        input_data_len = SIMPDEP_StoreNewData(hdl, DATA_ITEMS, l_mdata, l_mdata_len);
        fprintf(stderr, "first write over length: %d\n", input_data_len);
        ret = SIMPDEP_GetData(hdl, &gd, 1); // 3 0
        fprintf(stderr, "fisrt write over: ret is %d serial is %d status: %d size: %d data: %s\n", ret, gd.ret_data_serial, gd.status, gd.ret_data_sz, gd.ret_data);
        fprintf(stderr, "expect serial: %d\n", gd.expect_serial);

        input_data_len = SIMPDEP_StoreNewData(hdl, DATA_ITEMS, l_mdata, l_mdata_len); //4 1
        input_data_len = SIMPDEP_StoreNewData(hdl, DATA_ITEMS, l_mdata, l_mdata_len); //5 2
        input_data_len = SIMPDEP_StoreNewData(hdl, DATA_ITEMS, l_mdata, l_mdata_len); //6 0
        input_data_len = SIMPDEP_StoreNewData(hdl, DATA_ITEMS, l_mdata, l_mdata_len); //7 2
        SIMPDEP_Show(hdl);
        ret = SIMPDEP_GetData(hdl, &gd, 1); // catch 4 but fail, serial turn
        fprintf(stderr, "second write over: ret is %d serial is %d status: %d size: %d data: %s\n", ret, gd.ret_data_serial, gd.status, gd.ret_data_sz, gd.ret_data);
        fprintf(stderr, "expect serial: %d\n", gd.expect_serial);
        ret = SIMPDEP_GetData(hdl, &gd, 1);
        fprintf(stderr, "third write over: ret is %d serial is %d status: %d size: %d data: %s\n", ret, gd.ret_data_serial, gd.status, gd.ret_data_sz, gd.ret_data);
        fprintf(stderr, "expect serial: %d\n", gd.expect_serial);
        // SIMPDEP_GetData(hdl, &gd, 1);
        // fprintf(stderr, "ret is %d\nserial is %d status: %d size: %d data: %s\n", ret, gd.ret_data_serial, gd.status, gd.ret_data_sz, gd.ret_data);
        
        // SIMPDEP_Show(hdl);
    }
    /*
    for(int i = 0; i < NUMER_OF_INSERT_ITERATION; i++) {
        input_data_len = SIMPDEP_StoreNewData(hdl, DATA_ITEMS, l_mdata, l_mdata_len); // strange
            // fprintf(stderr, "iteration %d lenght: %d\n", i , input_data_len);
    }
    SIMPDEP_Show(hdl);
    */
    
    for(int i = 0; i < DATA_ITEMS; i++) {
        free(data[i]);
    }

    /*
    // section for getting data out
    memset(&gd, 0, sizeof(gd));
    for(int i = 0; i < 3; i++) {
        SIMPDEP_GetData(hdl, &gd, 0);
        fprintf(stderr, "ret_data: %s\nret_data_sz: %d\nret_data_serial: %lld\nget_latest: %d\nexpect_serial: %lld\n", \
        gd.ret_data, gd.ret_data_sz, gd.ret_data_serial, gd.get_latest, gd.expect_serial);
        //SIMPDEP_UnlockData(hdl, &gd);
        fprintf(stderr, "+++++++++\n");
    }
    SIMPDEP_UnlockData(hdl, &gd);
    // SIMPDEP_UnlockData(hdl, &gd);
    SIMPDEP_Show(hdl);
    */
    SIMPDEP_DestroyDeposit(hdl);
}