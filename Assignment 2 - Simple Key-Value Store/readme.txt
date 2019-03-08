I had to coninuously perform read lock then check for read count and then do write lock or unlock write depending on read count and then unlock read as well thus I skipped commenting it everytime and writing how it works here. Repeated below.

	SMS->read_count++;

	readCount = SMS->read_count;
	if(readCount == 1){
		sem_wait(write_lock);
	}

	sem_post(read_lock);