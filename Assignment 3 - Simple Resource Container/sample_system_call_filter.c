/**
 *          THIS IS JUST A SAMPLE CODE FOR SYSTEM CALL FILTERING
 **/

/**
 *  Initialize a seccomp context
 *      seccomp_init() is the method used
 *      SCMP_ACT_ALLOW - flag indicates that by default we want to ALLOW all system calls
 **/
scmp_filter_ctx seccomp_ctx = seccomp_init(SCMP_ACT_ALLOW);     
if (!seccomp_ctx) {
    fprintf(stderr, "seccomp initialization failed: %m\n");
    return EXIT_FAILURE;
}

/**
 *  Now we must set up filters for certain system calls for which we want different behavior
 *      Lets say we want to block the following system_calls. As in we want to kill any thread that tries to do them
 *          move_pages  - moves memory pages of a process to different blocks
 *          ptrace      - observe, track and control execution of another process
 * 
 *          seccomp_rule_add() is the method used to add a filtering rule
 *          SCMP_FAIL is the action to take when there is match on this filter (IE. KILL the thread - no mercy!!!)
 *          SCMP_SYS(move_pages) -  the 3rd argument is the system-call number to which this filter applies. 
 *                                  we use the SCMP_SYS() macro to get the correct number based on the underlying architecture
 *          4th argument - You use this argument if you dont want to capture all calls to the system call but want to capture
 *                          calls only when the system call has certain matching arguments.
 *                          Ex: the read() system call is called with the first argument being 0 (STDOUT)
 **/ 
int filter_set_status = seccomp_rule_add(
                                            seccomp_ctx,            // the context to which the rule applies
                                            SCMP_FAIL,          // action to take on rule match
                                            SCMP_SYS(move_pages),   // get the sys_call number using SCMP_SYS() macro
                                            0                       // any additional argument matches
                                        );
if (filter_set_status) {
    if (seccomp_ctx)
        seccomp_release(seccomp_ctx);
    fprintf(stderr, "seccomp could not add KILL rule for 'move_pages': %m\n");
    return EXIT_FAILURE;
}

filter_set_status = seccomp_rule_add(seccomp_ctx, SCMP_FAIL, SCMP_SYS(ptrace), 0);
if (filter_set_status) {
    if (seccomp_ctx)
        seccomp_release(seccomp_ctx);
    fprintf(stderr, "seccomp could not add KILL rule for 'ptrace': %m\n");
    return EXIT_FAILURE;
}

/**
 *  As another example lets say you want to disallow the 'unshare' system call
 *  But say that you want to disallow it ONLY if the 'CLONE_NEWUSER' flag is set in its argument
 *  Now we observe the signature for unshare() is as follows:
 *                  int unshare(int flags);
 *  Here flags can be an OR'ed combination of CLONE_FILES, CLONE_FS, CLONE_NEWCGROUP, CLONE_NEWUSER and many
 *  So we want to just capture calls to unshare() only if the 'CLONE_NEWUSER' flag is OR'ed in the argument.
 *  So seccomp_rule_add() would be as follows 
 **/
filter_set_status = seccomp_rule_add(
                                    seccomp_ctx,                    // the context to which the rule applies
                                    SCMP_FAIL,                  // action to take on rule match
                                    SCMP_SYS(unshare),              // get the sys_call number using SCMP_SYS() macro
                                    1,                              // any additional argument matches
                                    SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)
                                    );
if (filter_set_status) {
    if (seccomp_ctx)
        seccomp_release(seccomp_ctx);
    fprintf(stderr, "seccomp could not add KILL rule for 'unshare': %m\n");
    return EXIT_FAILURE;
}


/**
 *  Set the 'SCMP_FLTATR_CTL_NNP' attribute on the newly created context
 *  This attribute is used to ensure that the NO_NEW_PRIVS functionality is enabled 
 *  This is to control previledge escalation of child processes spawned with exec() in a parent with lesser priviledge
 *  You can just copy this and use it at the end of all seccomp rules.
 **/
filter_set_status = seccomp_attr_set(seccomp_ctx, SCMP_FLTATR_CTL_NNP, 0);
if (filter_set_status) {
    if (seccomp_ctx)
        seccomp_release(seccomp_ctx);
    fprintf(stderr, "seccomp could not set attribute 'SCMP_FLTATR_CTL_NNP': %m\n");
    return EXIT_FAILURE;
}


/**
 *  Finally load the created context into the kernel and release it from the current process memory
 **/ 
filter_set_status = seccomp_load(seccomp_ctx);
if (filter_set_status) {
    if (seccomp_ctx)
        seccomp_release(seccomp_ctx);               // release from current process memory.
    fprintf(stderr, "seccomp could not load the new context: %m\n");
    return EXIT_FAILURE;
}