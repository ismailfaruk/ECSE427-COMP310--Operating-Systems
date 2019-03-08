#Enter Make test1 for test 1
#Enter Make test2 for test 2

CC=clang
LIBS=-lrt -lpthread
CFLAGS=-g
SOURCE1=a2_lib.c comp310_a2_test1.c
SOURCE2=a2_lib.c comp310_a2_test2.c

EXEC1=os_test1 
EXEC2=os_test2

test1: $(SOURCE1)
	$(CC) -o $(EXEC1) $(CFLAGS) $(SOURCE1) $(LIBS)

test2: $(SOURCE2)
	$(CC) -o $(EXEC2) $(CFLAGS) $(SOURCE2) $(LIBS)

clean:
	rm $(EXEC1) $(EXEC2)
