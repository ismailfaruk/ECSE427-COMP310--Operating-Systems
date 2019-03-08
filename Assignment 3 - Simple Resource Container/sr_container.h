/**
 *  @title      :   sr_container.h
 *  @author     :   Shabir Abdul Samadh (shabirmean@cs.mcgill.ca)
 *  @purpose    :   COMP310/ECSE427 Operating Systems (Assingment 3) - Phase 2
 *  @description:   Header file for the template C code of A3
*/

#ifndef __SRCONTAINER__
#define __SRCONTAINER__

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sched.h>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <linux/capability.h>
#include <linux/limits.h>
#include <stdbool.h>


#define ARCH_TYPE "x86_64"
#define MEMORY "1073741824"         // default limit on the memory cgroup controller
#define CPU_SHARES "256"            // default shares limit on cpu cgroup controller
#define PIDS "64"                   // default pids limit on the pid cgroup controller
#define WEIGHT "10"                 // default weight limit on the block IO cgroup controller
#define FD_COUNT 64                 // default FD_COUNT
#define USERNS_OFFSET 10000         // default UID offset from the parent USER namespaces
#define USERNS_COUNT 2000           // default users count in the new USER namespace
#define SCMP_FAIL SCMP_ACT_ERRNO(EPERM) // Seccomp rule to set errno to EPERM on matchin filter
#define STACK_SIZE (1024 * 1024)    // Stack size for the cloned child


/**
 *  Set of CGROUPS-Controllers that must be set
 **/
#define CGRP_MEMORY_CONTROL "memory"
#define CGRP_CPU_CONTROL "cpu"
#define CGRP_CPU_SET_CONTROL "cpuset"
#define CGRP_PIDS_CONTROL "pids"
#define CGRP_BLKIO_CONTROL "blkio"

/**
 *  Struct to hold configurations related to the child process that 
 *  will be spawned in the container environment
 **/
struct child_config
{
    int argc;
    uid_t uid;
    int fd;
    char *hostname;
    char **argv;
    char *mount_dir;
};

/**
 *  Struct that holds the the sttings for a specific CGROUP controller 
 **/
struct cgroups_control {
	char control[256];          // the name of the cgroup-controller (eg: memory)
	struct cgroup_setting {       // an array of cgroup-controller settings
		char name[256];             // the name of the setting  (eg: pids.max)
		char value[256];            // the value to be set      (eg: 50)
	} **settings;
};

int child_function(void *arg);
int setup_child_capabilities();
int switch_child_root(const char *new_root, const char *put_old);
int setup_child_mounts(struct child_config *config);
int setup_syscall_filters();
int setup_cgroup_controls(struct child_config *config, struct cgroups_control **cgrps);
int free_cgroup_controls(struct child_config *config, struct cgroups_control **cgrps);
int setup_child_uid_map(pid_t child_pid, int fd);
int setup_child_userns(struct child_config *config);
int choose_child_hostname(char *buff, size_t len);

void clean_child_structures(struct child_config *config, struct cgroups_control **cgrps, char *child_stack);
void cleanup_stuff(char *argv[], int sockets[2]);
void print_usage(char *argv[]);
void cleanup_sockets(int sockets[2]);

#endif 