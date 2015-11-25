/* 
 * Copyright (c) 2014, Daan Pape
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *     1. Redistributions of source code must retain the above copyright 
 *        notice, this list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright 
 *        notice, this list of conditions and the following disclaimer in the 
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * File:   process.c
 * Created on October 1, 2015, 12:06 PM
 */

#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "process.h"
#include "logger.h"
#include "config.h"

/**
 * Start a process and get a FILE to read the process
 * output. 
 * @param command the command to start. 
 * @param pid the PID of the spawned process.
 * @return the FILE handle to read from the process.
 */
FILE* process_start(const char* command, pid_t* pid)
{
    FILE *fp;
    int pipe_fd[2];
    int parent_fd;
    int child_fd;
    pid_t p;
    
    if(pipe(pipe_fd)) {
        log_message(LOG_ERROR, "Could not create child process pipe\r\n");
        return NULL;
    }
    
    child_fd = pipe_fd[1];
    parent_fd = pipe_fd[0];
    
    if(!(fp = fdopen(parent_fd, "r"))) {
        close(parent_fd);
        close(child_fd);
        return NULL;
    }
    
    if((p = vfork()) == 0) {
        close(parent_fd);
        if(child_fd != 1) {
            dup2(child_fd, 1);
            close(child_fd);
        }
        
        execl("/bin/sh", "sh", "-c", command, (char *)0);
        _exit(127);
    }

    close(child_fd);
    
    if(p > 0) {
        log_message(LOG_DEBUG, "Process succesfully started\r\n");
        *pid = p;
        return fp;
    }
    
    fclose(fp);
    return NULL;
}

/**
 * Stop a child process and close it's stdin and stdout. 
 * @param p the child process to stop.
 */
void process_stop(FILE* stream, pid_t pid)
{
    fclose(stream);
    kill(pid, SIGINT);
    kill(pid, SIGKILL);
}