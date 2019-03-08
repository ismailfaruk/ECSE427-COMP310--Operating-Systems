
#include "a2_lib.h"

char* addr;
sem_t *db;
sem_t *mutex;
SharedMemoryBook* SMB;

//This function was provided to us by the TA
unsigned long generate_hash(const char * str) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++)){
		hash = ((hash << 5) + hash) + c;
	}

	hash = (hash > 0) ? hash : -(hash);

	return hash % 256;
}

int kv_store_create(const char *kv_store_name){
	// implement your create method code here

	int fd;

	fd = shm_open(kv_store_name, O_CREAT | O_RDWR, S_IRWXU);

	//print an error and exit if shared memory failed to open
	if(fd == -1){
		perror("Error opening shared memory");
		return -1;
	}

	//make sure shared memory is the appropriate size (total number of pods* size of one pod, plus the size of the book)
	ftruncate(fd, sizeof(SharedMemoryBook) + __KEY_VALUE_STORE_SIZE__);

	addr =  (char*)mmap (NULL, sizeof(SharedMemoryBook) + __KEY_VALUE_STORE_SIZE__,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(addr == MAP_FAILED){
		perror("Error in mapping memory");
		return -1;
	}

	/**initialise bookkeeping**/
	SetSharedMemStruct();
	
	close(fd);
	return 0;
}


int kv_store_write(const char *key, const char *value){
	// implement your create method code here

	//obtain writing lock
	sem_wait(db);
	int index_of_pod = generate_hash(key) % numberOfPods;

	//move to the correct index of the key-value pair in memory
	size_t locationOfRecord = recordSize * SMB->pod_counters[index_of_pod];

	//move to the correct pod in memory
	size_t locationOfPod = podSpace * index_of_pod;

	//copy the key and the value to the correct address, offset using the values defined above
	memcpy(addr + locationOfRecord + locationOfPod + sizeof(SharedMemoryBook), key, keySize);
	memcpy(addr + locationOfRecord + locationOfPod + sizeof(SharedMemoryBook) + keySize, value, valueSize);

	//incrememnt counter of the pod to state another kv-pair has been written to it
	SMB->pod_counters[index_of_pod]++;

	//Wrap around to the first value in case the pod is full
	SMB->pod_counters[index_of_pod] = SMB->pod_counters[index_of_pod] % 256;

	sem_post(db);

	return 0;
}

char *kv_store_read(const char *key){
	// implement your cre  ate method code here
	int index_of_pod = generate_hash(key) % numberOfPods;

	//find the correct pod in memory
	size_t locationOfPod = podSpace * index_of_pod;
	size_t locationOfRecord;
	int i;
	char* stringToReturn = NULL;

	/** Following the algorithm- the first reader obtains the write-lock **/
	sem_wait(mutex);

	SMB->read_count += 1;
	if(SMB->read_count == 1){
		sem_wait(db);
	}

	sem_post(mutex);

	for(i = 0; i < numberOfPods ; i++){
		//find the address of the next kvpair in the pod to read
		locationOfRecord = recordSize * SMB->records_read[index_of_pod];

		//if the key we read is the same as the one passed into the method
		if(memcmp(addr + locationOfRecord + locationOfPod + sizeof(SharedMemoryBook) , key, strlen(key))	== 0){

			//copy the value to a string we defined, which we will return
			stringToReturn = strdup((char*) (addr + locationOfRecord + locationOfPod +
					sizeof(SharedMemoryBook) + keySize));

			/**indicate that another record has been read in this pod, and wrap around if more than number of records**/
			SMB->records_read[index_of_pod]++;
			SMB->records_read[index_of_pod] = 	SMB->records_read[index_of_pod] % 256;

			/**Following the algorithm again, release write-lock if this is the last reader **/
			sem_wait(mutex);

			SMB->read_count -= 1;
			if(SMB->read_count == 0){
				sem_post(db);
			}

			sem_post(mutex);

			//return the value of the key
			return stringToReturn;

		} else{
			SMB->records_read[index_of_pod]++;
			SMB->records_read[index_of_pod] = 	SMB->records_read[index_of_pod] % 256;
		}
	}

	sem_wait(mutex);

	SMB->read_count -= 1;
	if(SMB->read_count == 0){
		sem_post(db);
	}

	sem_post(mutex);

	//if we reach here, the key was not found in the pod, so we return nothing
	return NULL;
}

char **kv_store_read_all(const char *key){
	// implement your create method code here
	int counter;
	int readCount;
	int index_of_pod;
	int locationOfPod;

	//define the array of strings that we will return
	char** valuesInKey = malloc(sizeof(char*));
	int i;
	int comparing;
	char* copied_string;
	size_t kvpair_index;

	sem_wait(mutex);

	/**Following the algorithm, the first reader obtains the write-lock**/
	SMB->read_count++;

	readCount = SMB->read_count;
	if(readCount == 1){
		sem_wait(db);
	}

	sem_post(mutex);
	//this counter will be used to index the string array, and add the values to the array
	counter = 0;
	index_of_pod = generate_hash(key) % numberOfPods;

	//read the correct pod
	locationOfPod = podSpace * index_of_pod;

	//obtain the number of records read to make sure all are read
	int record_index = 0;

	for(i = 0; i < numberOfPods ; i++){
		//get the address of the next kvpair in the pod
		kvpair_index = record_index * (288);

		//increment the index to read the next pair in the next iteration of the loop
		record_index++;
		record_index = record_index % numberOfPods;

		//check if the string we are reading has the same key as the one passed in
		comparing = memcmp(addr + kvpair_index + locationOfPod + sizeof(SharedMemoryBook) , key, strlen(key));
		if(comparing == 0){
			counter++;

			//resize the string array to make room for the string we add
			valuesInKey = realloc(valuesInKey, sizeof(char*)*(counter+1));

			//copy the string into the array
			copied_string = strdup(addr + kvpair_index + locationOfPod + sizeof(SharedMemoryBook) + keySize);
			valuesInKey[counter-1] = copied_string;

		}
	}

	sem_wait(mutex);

	SMB->read_count --;
	readCount = SMB->read_count;
	if(readCount == 0){
		sem_post(db);
	}

	sem_post(mutex);

	//the last value in the array should be null
	valuesInKey[counter] = NULL;

	//if the counter was never incremented, i.e. we did not find any matching keys, we return null
	if(counter==0){
		return (NULL);
	}
	return valuesInKey;
}


int SetSharedMemStruct(){

	//Initialise SharedMemoryBook object declared globally to have the correct address (where we call mmap)
	SMB = (SharedMemoryBook*) addr;

	/**Open the semaphores i.e the write and read lock respectively **/
	db = sem_open("db", O_CREAT, 0777, 1);
	mutex = sem_open("mutex", O_CREAT, 0777, 1);


	//Print an error and return if they could not be opened
	if(db == SEM_FAILED || mutex == SEM_FAILED){
		perror("Error opening sempahores");
		return -1;
	}

	//Set all the values in the book to 0
	if(SMB->initialised == 0){
		for(int i = 0; i < numberOfPods; i++){
			SMB->pod_counters[i] = 0;
			SMB->records_read[i] = 0;
		}

		SMB->read_count = 0;

		//variable to show the book has been initialised
		SMB->initialised = 1;
	}

	return 0;
}

int kv_delete_db(){
	sem_unlink("db");
	sem_unlink("mutex");
	if(munmap(addr,  __KEY_VALUE_STORE_SIZE__ + sizeof(SharedMemoryBook)) == -1){
		perror("Failed to unmap");
		return -1;
	}
	shm_unlink(__TEST_SHARED_MEM_NAME__);

	return(0);
}

/* -------------------------------------------------------------------------------
	MY MAIN:: Use it if you want to test your impementation (the above methods)
	with some simple tests as you go on implementing it (without using the tester)
	------------------------------------------------------------------------------- */
/*
int main() {

}
 */
