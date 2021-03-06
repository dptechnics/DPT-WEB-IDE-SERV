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
 * File:   ide-run.c
 * Created on October 2, 2015, 3:55 AM
 */

#include <libwebsockets.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>

#include "logger.h"
#include "process.h"
#include "config.h"
#include "ide-run.h"

/**
 * Dump a constant string in a file and close it. 
 * @param dmp the string to dump. 
 * @param filename the filename to write to. 
 * @return true on success false on error. 
 */
static bool _dump_to_file(const char* dmp, const char* filename)
{
    FILE *f = fopen(filename, "w");
    if(f == NULL)
    {
        log_message(LOG_ERROR, "Could not open file %s\r\n", filename);
        return false;
    }
    
    if(fputs(dmp, f) < 0) {
        fclose(f);
        log_message(LOG_ERROR, "Could not write to file %s\r\n", filename);
        return false;
    }
    
    fclose(f);
    return true;
}

/**
 * This handles ide_run protocol requests. 
 * @param context the context of the request. 
 * @param wsi the websocket currently used. 
 * @param reason the callback reason. 
 * @param user the user function. 
 * @param in the in function. 
 * @param len the length. 
 * @return returns 0 on success or -1 on error.
 */
int ide_run_callback(struct libwebsocket_context *context, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
    struct ide_run_session *sess = (struct ide_run_session*) user;
    int b_read;
    
    switch(reason) {
        case LWS_CALLBACK_ESTABLISHED:
            log_message(LOG_INFO, "ide-run websocket connection established\r\n");
            break;
        
        case LWS_CALLBACK_CLOSED:
            log_message(LOG_INFO, "ide-run websocket connection closed\r\n");
            if(sess->pid > 0) {
                log_message(LOG_DEBUG, "Killing process: %d\r\n", (int) sess->pid);
                process_stop(sess->pfstream, sess->pid);
                sess->pid = -1;
            }        
            break;
            
        case LWS_CALLBACK_SERVER_WRITEABLE:
            /* Forward the process output to the browser if any */
            if(sess->pid > 0) {
                errno = 0;
                b_read = read(sess->pfd, sess->pbuff + LWS_SEND_BUFFER_PRE_PADDING, DPT_WEB_IDE_PROC_READ_BUFF - LWS_SEND_BUFFER_PRE_PADDING - LWS_SEND_BUFFER_POST_PADDING);
                if(b_read == -1 && errno != EAGAIN) {
                    // The read call failed, close
                    log_message(LOG_ERROR, "Could not read from interpreter stdout: %s (pfd = %d)\r\n", strerror(errno), sess->pfd);
                    return -1;
                }
                
                if(b_read > 0) {
                    libwebsocket_write(wsi, sess->pbuff + LWS_SEND_BUFFER_PRE_PADDING, b_read, LWS_WRITE_TEXT);
                }
            }
            break;
            
        case LWS_CALLBACK_RECEIVE:     
            if(strncmp("STOP", (const char*) in, 4) == 0) {
                // Stop current process
                if(sess->pid > 0) {
                    log_message(LOG_DEBUG, "Killing process: %d\r\n", (int) sess->pid);
                    process_stop(sess->pfstream, sess->pid);
                    sess->pid = -1;
                }
            }else {
            
                // Dump source code into file
                _dump_to_file((const char*) in, "/tmp/dptwebide_tmp154968.js");

                // Kill previous process if any
                if(sess->pid > 0) {
                    log_message(LOG_DEBUG, "Killing process: %d\r\n", (int) sess->pid);
                    process_stop(sess->pfstream, sess->pid);
                    sess->pid = -1;
                }

                // Open interpreter process and set to non blocking read
                sess->pfstream = process_start("dpt-js /tmp/dptwebide_tmp154968.js 2>&1", &(sess->pid));
                sess->pfd = fileno(sess->pfstream);
                fcntl(sess->pfd, F_SETFL, O_NONBLOCK);
            }
            break;
     
        default:
            break;
    }
    
    return 0;
}