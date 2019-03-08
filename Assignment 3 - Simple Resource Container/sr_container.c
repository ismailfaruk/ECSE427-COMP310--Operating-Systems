/**
 *  @title      :   sr_container.c
 *  @author     :   Shabir Abdul Samadh (shabirmean@cs.mcgill.ca)
 *  @date       :   20th Nov 2018
 *  @purpose    :   COMP310/ECSE427 Operating Systems (Assingment 3) - Phase 2
 *  @description:   A template C code to be filled in order to spawn container instances
 *  @compilation:   Use "make container" with the given Makefile
*/

#include "sr_container.h"

/**
 *  The cgroup setting to add the writing task to the cgroup
 *  '0' is considered a special value and writing it to 'tasks' asks for the wrinting 
 *      process to be added to the cgroup. 
 *  You must add this to all the controls you create so that it is added to the task list.
 *  See the example 'cgroups_control' added to the array of controls - 'cgroups' - below
 **/  
struct cgroup_setting self_to_task = {
	.name = "tasks",
	.value = "0"
};

/**
 *  ------------------------ TODO ------------------------
 *  An array of different cgroup-controllers.
 *  One controller has been been added for you.
 *  You should fill this array with the additional controls from commandline flags as described 
 *      in the comments for the main() below
 *  ------------------------------------------------------
 **/ 
struct cgroups_control *cgroups[6] = {
    //blkio-cgroup controller
	& (struct cgroups_control) {
		.control = CGRP_BLKIO_CONTROL,
		.settings = (struct cgroup_setting *[]) {
            //blkio.weight was just given as an example to use
			& (struct cgroup_setting) {
				.name = "blkio.weight",
				.value = WEIGHT
			},
			&self_to_task,             // must be added to all the new controls added
			NULL                       // NULL at the end of the array
        }
	},  
	NULL                               // NULL at the end of the array
};

/**
 *  ------------------------ TODO ------------------------
 *  The SRContainer by default suppoprts three flags:
 *          1. m : The rootfs of the container
 *          2. u : The userid mapping of the current user inside the container
 *          3. c : The initial process to run inside the container
 *  
 *   You must extend it to support the following flags:
 *          1. C : The cpu shares weight to be set (cpu-cgroup controller)
 *          2. s : The cpu cores to which the container must be restricted (cpuset-cgroup controller)
 *          3. p : The max number of process's allowed within a container (pid-cgroup controller)
 *          4. M : The memory consuption allowed in the container (memory-cgroup controller)
 *          5. r : The read IO rate in bytes (blkio-cgroup controller) - done
 *          6. w : The write IO rate in bytes (blkio-cgroup controller) - done
 *          7. H : The hostname of the container 
 * 
 *   You can follow the current method followed to take in these flags and extend it.
 *   Note that the current implementation necessitates the "-c" flag to be the last one.
 *   For flags 1-6 you can add a new 'cgroups_control' to the existing 'cgroups' array
 *   For 7 you have to just set the hostname parameter of the 'child_config' struct in the header file
 *  ------------------------------------------------------
 **/
int main(int argc, char **argv)
{   
    // two variables self defined
    
    int cgroup_index;
    cgroup_index = 0;

    int blkio_index;
    
    struct child_config config = {0};
    int option = 0;
    int sockets[2] = {0};
    pid_t child_pid = 0;
    int last_optind = 0;
    bool found_cflag = false;

    struct cgroups_control *cgroup;
    struct cgroup_setting *setting;    

    while ((option = getopt(argc, argv, "c:m:u:H:C:s:p:M:r:w:")))
    {
        if (found_cflag)
            break;

        switch (option)
        {
        case 'c':
            config.argc = argc - last_optind - 1;
            config.argv = &argv[argc - config.argc];
            found_cflag = true;
            break;
        case 'm':
            config.mount_dir = optarg;
            break;
        case 'u':
            if (sscanf(optarg, "%d", &config.uid) != 1)
            {
                fprintf(stderr, "UID not as expected: %s\n", optarg);
                cleanup_stuff(argv, sockets);
                return EXIT_FAILURE;
            }
            break;

        //flag implementation for cpu-cgroup controller 
        case 'C':
            cgroup = (struct cgroups_control*) malloc(sizeof(struct cgroups_control));
            //cgroups[++cgroup_index] = (struct cgroups_control*) malloc(sizeof(struct cgroups_control));
            cgroups[++cgroup_index] = cgroup;
            cgroups[cgroup_index+1] = NULL;

            strcpy(cgroups[cgroup_index]->control, CGRP_CPU_CONTROL);

            cgroups[cgroup_index]->settings = malloc(3*sizeof(struct cgroup_setting));
            //struct cgroup_setting *setting = malloc(sizeof(struct cgroup_setting));
            setting = (struct cgroup_setting*) malloc(sizeof(struct cgroup_setting));

            cgroups[cgroup_index]->settings[0] = setting;
            strcpy(cgroups[cgroup_index]->settings[0]->name,"cpu.shares");
            strcpy(cgroups[cgroup_index]->settings[0]->value, optarg);
  
            cgroups[cgroup_index]->settings[1] = &self_to_task;             // must be added to all the new controls added
			cgroups[cgroup_index]->settings[2] = NULL;
           
            break;

        //flag implementation for cpuset-cgroup controller
        case 's':
            cgroup = (struct cgroups_control*) malloc(sizeof(struct cgroups_control));
            cgroups[++cgroup_index] = cgroup;

            cgroups[cgroup_index+1] = NULL;
            strcpy(cgroup->control, CGRP_CPU_SET_CONTROL);

            cgroups[cgroup_index]->settings = malloc(4*sizeof(struct cgroup_setting));
            setting = (struct cgroup_setting*) malloc(sizeof(struct cgroup_setting));
            
            cgroups[cgroup_index]->settings[0] = setting;
            strcpy(cgroups[cgroup_index]->settings[0]->name,"cpuset.cpus");
            strcpy(cgroups[cgroup_index]->settings[0]->value, optarg);

            setting = (struct cgroup_setting*) malloc(sizeof(struct cgroup_setting));

            cgroups[cgroup_index]->settings[1] = setting;
            strcpy(cgroups[cgroup_index]->settings[1]->name,"cpuset.mems");
            strcpy(cgroups[cgroup_index]->settings[1]->value, "0");
  
            cgroups[cgroup_index]->settings[2] = &self_to_task;             // must be added to all the new controls added
			cgroups[cgroup_index]->settings[3] = NULL;         

            break;

        //flag implementation for pid-cgroup controller
        case 'p':
            cgroup = (struct cgroups_control*) malloc(sizeof(struct cgroups_control));
            cgroups[++cgroup_index] = cgroup;
            cgroups[cgroup_index+1] = NULL;

            strcpy(cgroup->control, CGRP_PIDS_CONTROL);
            
            cgroups[cgroup_index]->settings = malloc(3*sizeof(struct cgroup_setting));
            setting = (struct cgroup_setting*) malloc(sizeof(struct cgroup_setting));

            cgroups[cgroup_index]->settings[0] = setting;
            strcpy(cgroups[cgroup_index]->settings[0]->name,"pids.max");
            strcpy(cgroups[cgroup_index]->settings[0]->value, optarg);
  
            cgroups[cgroup_index]->settings[1] = &self_to_task;             // must be added to all the new controls added
			cgroups[cgroup_index]->settings[2] = NULL;

            break;

        //flag implementation for memory-cgroup controller
        case 'M':
            cgroup = (struct cgroups_control*) malloc(sizeof(struct cgroups_control));
            cgroups[++cgroup_index] = cgroup;
            cgroups[cgroup_index+1] = NULL;

            strcpy(cgroup->control, CGRP_MEMORY_CONTROL);
            
            cgroups[cgroup_index]->settings = malloc(3*sizeof(struct cgroup_setting));
            setting = (struct cgroup_setting*) malloc(sizeof(struct cgroup_setting));

            cgroups[cgroup_index]->settings[0] = setting;
            strcpy(cgroups[cgroup_index]->settings[0]->name,"memory.limit_in_bytes");
            strcpy(cgroups[cgroup_index]->settings[0]->value, optarg);
  
            cgroups[cgroup_index]->settings[1] = &self_to_task;             // must be added to all the new controls added
			cgroups[cgroup_index]->settings[2] = NULL;
            break;

        //flag implementation for blkio-cgroup controller read IO
        case 'r':
            blkio_index = 0;
            while(cgroups[0]->settings[blkio_index]!= NULL){
                blkio_index++;
            }

            //cgroups[0]->settings = malloc(3*sizeof(struct cgroup_setting));
            //setting = malloc(sizeof(struct cgroup_setting));
            
            //cgroups[0]->settings[blkio_index] = settingR;
            strcpy(cgroups[0]->settings[blkio_index]->name,"blkio.throttle.read_bps_device");
            strcpy(cgroups[0]->settings[blkio_index]->value, optarg);
            
            cgroups[0]->settings[blkio_index+1] = NULL;

            break;
        
        //flag implementation for blkio-cgroup controller write IO
        case 'w':
            blkio_index = 0;
            while(cgroups[0]->settings[blkio_index]!= NULL){
                blkio_index++;
            }

            //cgroups[0]->settings = malloc(3*sizeof(struct cgroup_setting));
            //struct cgroup_setting *settingW = malloc(sizeof(struct cgroup_setting));
            
            //cgroups[0]->settings[blkio_index] = settingW;
            strcpy(cgroups[0]->settings[blkio_index]->name,"blkio.throttle.write_bps_device");
            strcpy(cgroups[0]->settings[blkio_index]->value, optarg);
            
            cgroups[0]->settings[blkio_index+1] = NULL;

            break;

        //flag implementation for host configuration
        case 'H':
            config.hostname = optarg;
            break;

        default:
            cleanup_stuff(argv, sockets);
            return EXIT_FAILURE;
        }
        last_optind = optind;
    }

    if (!config.argc || !config.mount_dir){
        cleanup_stuff(argv, sockets);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "####### > Checking if the host Linux version is compatible...");
    struct utsname host = {0};
    if (uname(&host))
    {
        fprintf(stderr, "invocation to uname() failed: %m\n");
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    int major = -1;
    int minor = -1;
    if (sscanf(host.release, "%u.%u.", &major, &minor) != 2)
    {
        fprintf(stderr, "major minor version is unknown: %s\n", host.release);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    if (major != 4 || (minor < 7))
    {
        fprintf(stderr, "Linux version must be 4.7.x or minor version less than 7: %s\n", host.release);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    if (strcmp(ARCH_TYPE, host.machine))
    {
        fprintf(stderr, "architecture must be x86_64: %s\n", host.machine);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "%s on %s.\n", host.release, host.machine);

    if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets))
    {
        fprintf(stderr, "invocation to socketpair() failed: %m\n");
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC))
    {
        fprintf(stderr, "invocation to fcntl() failed: %m\n");
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    config.fd = sockets[1];

    /**
     * ------------------------ TODO ------------------------
     * This method here is creating the control groups using the 'cgroups' array
     * Make sure you have filled in this array with the correct values from the command line flags 
     * Nothing to write here, just caution to ensure the array is filled
     * ------------------------------------------------------
     **/
    if (setup_cgroup_controls(&config, cgroups))
    {
        clean_child_structures(&config, cgroups, NULL);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }

    /**
     * ------------------------ TODO ------------------------
     * Setup a stack and create a new child process using the clone() system call
     * Ensure you have correct flags for the following namespaces:
     *      Network, Cgroup, PID, IPC, Mount, UTS (You don't need to add user namespace)
     * Set the return value of clone to 'child_pid'
     * Ensure to add 'SIGCHLD' flag to the clone() call
     * You can use the 'child_function' given below as the function to run in the cloned process
     * HINT: Note that the 'child_function' expects struct of type child_config.
     * ------------------------------------------------------
     **/

        // You code for clone() goes here
        void *stack, *stackTop;
        stack = malloc(STACK_SIZE);
        if(stack == NULL){
            perror("malloc failed");
            exit(-1);
        }
        stackTop = stack + STACK_SIZE;
        child_pid = clone(child_function, stackTop, SIGCHLD | CLONE_NEWCGROUP | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS, &config);
        
    /**
     *  ------------------------------------------------------
     **/ 
    if (child_pid == -1)
    {
        fprintf(stderr, "####### > child creation failed! %m\n");
        clean_child_structures(&config, cgroups, stack);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    close(sockets[1]);
    sockets[1] = 0;

    if (setup_child_uid_map(child_pid, sockets[0]))
    {
        if (child_pid)
            kill(child_pid, SIGKILL);
    }

    int child_status = 0;
    waitpid(child_pid, &child_status, 0);
    int exit_status = WEXITSTATUS(child_status);

    clean_child_structures(&config, cgroups, stack);
    cleanup_sockets(sockets);
    return exit_status;
}

int child_function(void *arg)
{
    struct child_config *config = arg;
    if (sethostname(config->hostname, strlen(config->hostname)) || \
                setup_child_mounts(config) || \
                setup_child_userns(config) || \
                setup_child_capabilities() || \
                setup_syscall_filters()
        )
    {
        close(config->fd);
        return -1;
    }
    if (close(config->fd))
    {
        fprintf(stderr, "invocation to close() failed: %m\n");
        return -1;
    }
    if (execve(config->argv[0], config->argv, NULL))
    {
        fprintf(stderr, "invocation to execve() failed! %m.\n");
        return -1;
    }
    return 0;
}
