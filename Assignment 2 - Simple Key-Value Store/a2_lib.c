
#include "a2_lib.h"

//global variables used in multiple methods
char *addr;
sem_t *write_lock;
sem_t *read_lock;
SharedMemoryStruct* SMS;

// hash function to generate a long number
unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while (c = *str++){
        hash = ((hash << 5) + hash) + c;
    }
    hash = (hash > 0) ? hash : -(hash);
	return hash % numberOfPods;		//return will directly map 
}

int kv_store_create(char *kv_store_name){

	int fd = shm_open(kv_store_name, O_CREAT | O_RDWR, S_IRWXU);	/*0_CREAT will create shm if it does not exist
																	0_CREAT does nothing if shm exist
																	function opens the existing store
																	0_RDWR is activated regardless new shm is created
																	or the existing one is being used*/

	//error in case shared mem does not open
	if(fd < 0){
		perror("Error opening shared memor\n");
		return -1;
	}
	//casted onto char* because mmap returns void pointers, just to keep consistency
	addr = (char*)mmap(NULL, sizeof(SharedMemoryStruct) + KV_STORE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
																			
	if(addr == MAP_FAILED){
		perror("Mapping Failed");
		return -1;
	}

	ftruncate(fd, sizeof(SharedMemoryStruct) + KV_STORE_SIZE);
	
	//bookkeeping
	bookkeeping();

	close(fd);
	//rest of the clear up done using kv_delete_db()

	return 0;
}

int kv_store_write(char *key, char *value){

	// check if key size is within its alloted size
	if (!(strlen(key) <= keySize)){
		perror("Key Size exceeded");
		return -1;
	}

	// check if value size is within its alloted size
	else if (!(strlen(value) <= valueSize)){
		perror("Value Size exceeded");
		return -1;
	}

	else{
	//obtain write lock
	sem_wait(write_lock);
	//using hash map to find the pod_index_pointer corresponding to the key
	int pod_index_pointer = hash(key);

	//shift to point to the correct pod
	int pod_pointer_shift = podSpace * pod_index_pointer;

	//shift to point to the empty row inside pod
	int record_pointer_shift = recordSize * SMS->pod_counter[pod_index_pointer];

	//copy key and value from input memory location to shared memory location
	memcpy(addr + sizeof(SharedMemoryStruct) + pod_pointer_shift + record_pointer_shift, key, keySize);
	memcpy(addr + sizeof(SharedMemoryStruct) + pod_pointer_shift + record_pointer_shift + keySize, value, valueSize);

	//incrememnt counter of the pod to state another kv-pair has been written to it
	SMS->pod_counter[pod_index_pointer]++;

	//Wrap around to the first value in case the pod is full
	//replace oldest value if there is not enough space
	SMS->pod_counter[pod_index_pointer] = SMS->pod_counter[pod_index_pointer] % 256;

	//release write lock
	sem_post(write_lock);
	}
	return 0;
	
/*
- How will you keep track of this value?
- If that is also not enough?
*/
}

char *kv_store_read(char *key){

	int pod_index_pointer = hash(key);

	//find the correct pod in memory
	int pod_pointer_shift = podSpace * pod_index_pointer;
	int record_pointer_shift;

	//initizing returnValue
	char* returnValue = NULL;

	/** Following the algorithm- the first reader obtains the write-lock **/
	sem_wait(read_lock);

	SMS->read_count++;
	if(SMS->read_count == 1){
		sem_wait(write_lock);
	}

	sem_post(read_lock);

	int i;
	for(i = 0; i < podSize ; i++){
		//shift pointer to the address of the next kvpair in the pod to read
		record_pointer_shift = recordSize * SMS->records_read[pod_index_pointer];

		//if the key we read is the same as the one passed into the method
		if(memcmp(addr + sizeof(SharedMemoryStruct) + pod_pointer_shift + record_pointer_shift, key, strlen(key))	== 0){

			//copy the value to a string we defined, which we will return
			returnValue = strdup((char*) (addr + record_pointer_shift + pod_pointer_shift +
					sizeof(SharedMemoryStruct) + keySize));

			//increment record read counter
			SMS->records_read[pod_index_pointer]++;
			SMS->records_read[pod_index_pointer] = 	SMS->records_read[pod_index_pointer] % 256;

			sem_wait(read_lock);

			SMS->read_count -= 1;
			if(SMS->read_count == 0){
				sem_post(write_lock);
			}

			sem_post(read_lock);

			//return the value of the key if found
			return returnValue;

		} else{
			SMS->records_read[pod_index_pointer]++;
			SMS->records_read[pod_index_pointer] = 	SMS->records_read[pod_index_pointer] % 256;
		}
	}

	sem_wait(read_lock);

	SMS->read_count--;
	if(SMS->read_count == 0){
		sem_post(write_lock);
	}

	sem_post(read_lock);

	return NULL;

/*
- if there are many records which one to return?
*/
}

char **kv_store_read_all(char *key){
    //
	int detect = 0;
	int readCount;
	int pod_index_pointer = hash(key);
	
	//return stack declare
	char** values = malloc(sizeof(char*));

	char* temp_string;
	int kvpair_index;

	sem_wait(read_lock);

	SMS->read_count++;
	readCount = SMS->read_count;
	if(readCount == 1){
		sem_wait(write_lock);
	}

	sem_post(read_lock);
	pod_index_pointer

	//shift pointer to the requried Pod
	int pod_pointer_shift = podSpace * pod_index_pointer;

	//obtain the number of records read to make sure all are read
	int record_index = 0;

	int i;
	for(i = 0; i < podSize ; i++){
		//get the address of the next kvpair in the pod
		kvpair_index = record_index * recordSize;

		//increment the index to read the next pair in the next iteration of the loop
		record_index++;
		record_index = record_index % podSize;

		//check if the string we are reading has the same key as the one passed in
		if(memcmp(addr + sizeof(SharedMemoryStruct) + pod_pointer_shift + kvpair_index, key, strlen(key)) == 0){
			detect++;

			//resize the string array to make room for the string we add
			values = realloc(values, sizeof(char*)*(detect+1));

			//copy the string into the array
			temp_string = strdup(addr + sizeof(SharedMemoryStruct) + pod_pointer_shift + kvpair_index + keySize);
			values[detect-1] = temp_string;

		}
	}

	sem_wait(read_lock);

	SMS->read_count--;
	readCount = SMS->read_count;
	if(readCount == 0){
		sem_post(write_lock);
	}

	sem_post(read_lock);

	//the last value in the array should be null
	values[detect] = NULL;

	//if nothing was found
	if(detect==0){
		return (NULL);
	}
	return values;
}

int bookkeeping(){

	//Initialise SharedMemoryStruct object
	SMS = (SharedMemoryStruct*) addr;

	//open the semaphores
	write_lock = sem_open("write_lock", O_CREAT, 0777, 1);
	read_lock = sem_open("read_lock", O_CREAT, 0777, 1);


	//incase open fails
	if(write_lock == SEM_FAILED || read_lock == SEM_FAILED){
		perror("Error opening sempahores");
		return -1;
	}

	//initialize count varables in SMS
	if(SMS->initializeFlag == 0){
		for(int i = 0; i < numberOfPods; i++){
			SMS->pod_counter[i] = 0;
			SMS->records_read[i] = 0;
		}

		SMS->read_count = 0;

		//variable to show the book has been initializeFlag
		SMS->initializeFlag = 1;	//wanted to use enum, just ran out of time
	}

	return 0;
}

//implemeted for tester, although not mentioned in the question..maybe optional
int kv_delete_db(){
	sem_unlink("write_lock");
	sem_unlink("read_lock");
	if(munmap(addr,  KV_STORE_SIZE + sizeof(SharedMemoryStruct)) == -1){
		perror("Failed to unmap");
		return -1;
	}
	shm_unlink(TEST_SHARED_MEM_NAME);

	return 0;
}

int main(int argc, char **argv){
    return 0;   
}