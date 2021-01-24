CC = g++
CFLAG = -pthread  -w -g
LIB_PATH = ~/cubo_fw/src/yun_sys/deposite
LIB_PATH_SOURCE = $(LIB_PATH)/simple_deposit.cpp

PROJECT_TARGET = deposit producer-consumer
PROJECT_EXEC = producer-consumer
PROJECT_OBJ = simple_deposit.o producer-consumer.o

TEST_TARGET = deposit insert_data
TEST_EXEC = insert_data
TEST_OBJ = insert_data.o simple_deposit.o

all: $(PROJECT_TARGET)
	$(CC) $(CFLAG) $(PROJECT_OBJ) -o producer-consumer -lcrypto -lssl -ljpeg
test: $(TEST_TARGET)
	$(CC) $(CFLAG) $(TEST_OBJ) -o insert_data
deposit: $(LIB_PATH_SOURCE)
	$(CC) $(CFLAG) -c $(LIB_PATH_SOURCE)
producer-consumer: producer-consumer.c producer-consumer.h
	$(CC) $(CFLAG) -c -I $(LIB_PATH) producer-consumer.c
insert_data: insert_data.c
	$(CC) $(CFLAG) -I $(LIB_PATH) -c insert_data.c
test_jpeg: test_jpeg.c
	gcc test_jpeg.c -ljpeg -o test_jpeg
clean:
	rm *.o
	rm $(TEST_EXEC) $(PROJECT_EXEC)
#Q: where is .o file of deposit