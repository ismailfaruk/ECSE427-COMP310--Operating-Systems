#ifndef __A2_LIB_HEADER__
#define __A2_LIB_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

/* -------------------------------------
	Define your own globals here
---------------------------------------- */

#define __KEY_VALUE_STORE_SIZE__	podSpace * numberOfPods     	/*18MB - space of one pod * number of pods*/
#define __KV_WRITERS_SEMAPHORE__	"WRITER_SAADM_260679016"
#define __KV_READERS_SEMAPHORE__	"READER_SAADM_260679016"
#define __KV_STORE_NAME__			"KV_STORE_SAADM_260679016"
#define KV_EXIT_SUCCESS				0
#define KV_EXIT_FAILURE				-1
#define __TEST_SHARED_MEM_NAME__ "/GTX_1080_TI"

#define keySize 32
#define valueSize 256
#define recordSize 288

#define maxAvailableRecords 65536
#define numberOfPods 256
#define podSize 256
#define podSpace podSize*recordSize

//----------------------------------------

typedef struct {
  char key[keySize];
  char value[valueSize];
}KeyValue;

typedef struct{
  int read_count;
  int pod_counters[numberOfPods];
  int records_read[podSize];
  int initialised;
}SharedMemoryBook;


unsigned long generate_hash(const char *str);

int kv_store_create(const char *kv_store_name);
int kv_store_write(const char *key, const char *value);
char *kv_store_read(const char *key);
char **kv_store_read_all(const char *key);
int kv_delete_db();
int SetSharedMemStruct();

#endif
