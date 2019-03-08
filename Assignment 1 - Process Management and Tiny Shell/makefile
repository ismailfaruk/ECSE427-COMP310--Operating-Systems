all:
	gcc tiny_shell.c -o tshell

fork:	tiny_shell.c
	gcc -D FORK tiny_shell.c -o tshell

vfork:	tiny_shell.c
	gcc -D VFORK tiny_shell.c -o tshell

clone:	tiny_shell.c
	gcc -D CLONE tiny_shell.c -o tshell

pipe:	tiny_shell.c
	gcc -D PIPE tiny_shell.c -o tshell

hello_seg:	hello.c
	gcc -D SEG hello.c -o hello_seg

hello:		hello.c
	gcc hello.c -o hello

clean:
	rm tshell hello hello_seg
	
