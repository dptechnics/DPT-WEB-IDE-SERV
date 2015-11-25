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
 * File:   main.c
 * Created on September 30, 2015, 5:27 PM
 */

#include <libwebsockets.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <bits/signum.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "config.h"
#include "logger.h"
#include "main.h"

/* Flag denoting a forced exit */
static volatile int force_exit = 0;  

/* The websocket context */
static struct libwebsocket_context *context;    

/**
 * Signal handler.
 * @param sig the received signal
 */
static void sighandler(int sig) {
    force_exit = 1;
    libwebsocket_cancel_service(context);
}

/**
 * DPT-Web IDE server main entry point. 
 * @param argc argument count. 
 * @param argv argument data. 
 * @return 0 on success. 
 */
int main(int argc, char** argv)
{
    struct lws_context_creation_info info;
    int n = 0;
    int cur_fd;
    
    log_message(LOG_INFO, "Starting dpt-web-ide server...\r\n");
    
    /* Parse configuration */
    if(!config_parse()) {
        log_message(LOG_WARNING, "Could not parse configuration, falling back to defaults\r\n");
    }
    
    /* fork (if not disabled) */
    if (conf->daemon) {
        switch (fork()) {
            case -1:
                perror("fork()");
                exit(1);

            case 0:
                /* daemon setup */
                if (chdir("/"))
                    perror("chdir()");

                cur_fd = open("/dev/null", O_WRONLY);
                if (cur_fd > 0) {
                    dup2(cur_fd, 0);
                    dup2(cur_fd, 1);
                    dup2(cur_fd, 2);
                }

                break;

            default:
                exit(0);
        }
    }
    
    /* Register the signal handler for interruption */
    signal(SIGINT, sighandler);
    
    /* Ignore child (interpreter) exits so they don't become zombie */
    signal(SIGCHLD, SIG_IGN);
    
    /* Initialize libwebsockets context */
    memset(&info, 0, sizeof(info));
    info.port = conf->port;
    info.iface = NULL;
    info.protocols = protocols;
    info.extensions = libwebsocket_get_internal_extensions();
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;
    info.gid = -1;
    info.uid = -1;
    info.options = 0;
    context = libwebsocket_create_context(&info);
    
    if(context == NULL) {
        log_message(LOG_ERROR, "Could not create libwebsocket context, failed to start\r\n");
        return EXIT_FAILURE;
    } else {
        log_message(LOG_INFO, "Succesfully created libwebsocket context\r\n");
    }
    
    /* Start the main eventloop */
    while(n >= 0 && !force_exit) {
        /* Run the and ide-run process service */
        libwebsocket_callback_on_writable_all_protocol(&protocols[PROTO_IDE_RUN]);
        
        /* Run the websocket service */
        n = libwebsocket_service(context, DPT_WEB_IDE_WEBSOCK_TIMOUT);
    }
    
    /* Close program */
    libwebsocket_context_destroy(context);
    log_message(LOG_INFO, "dpt-web-ide server exited cleanly\r\n");
    
    return EXIT_SUCCESS;
}