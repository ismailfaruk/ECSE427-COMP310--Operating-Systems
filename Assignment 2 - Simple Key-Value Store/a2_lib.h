#ifndef A2_LIB_H
#define A2_LIB_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/types.h>
#include <stdbool.h>

//Pod Definitions
#define numberOfPods 16
#define podSize 8

//limit on key and value size taken from handout
#define keySize 32
#define valueSize 256

//record size = key size + value size
#define recordSize keySize+valueSize

//total memory space per pod, row*column
#define podSpace podSize*recordSize

//Available Records = numberOfPods*podSize
#define AvailableRecords numberOfPods*podSize

#define KV_STORE_SIZE	podSpace * numberOfPods
#define TEST_SHARED_MEM_NAME "/GTX_1080_TI"

/* Some semaphores used
#define KV_WRITERS_SEMAPHORE  "W_ifaruk_260663521"
#define KV_READERS_SEMAPHORE  "R_ifaruk_260663521"
#define KV_STORE_NAME         "KV_STORE_ifaruk_260663521"
*/

typedef struct{
  int read_count;
  int pod_counter[numberOfPods];
  int records_read[podSize];
  int initializeFlag;
}SharedMemoryStruct;

//function declaration of a2_lib
unsigned long hash(unsigned char *str);
int kv_store_create(char *kv_store_name);
int kv_store_write(char *key, char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);
int kv_delete_db();
int bookkeeping();

#endif