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
 * File:   config.h
 * Created on September 30, 2015, 7:30 PM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_BUFF_SIZE                512                     // Config parser buffer

/* Dynamic configuration options */
#define DPT_WEB_IDE_FORK_ON_START       false                   // Don't daemonize by default
#define DPT_WEB_IDE_HTML_PATH           "/www/webide"           // Base path where the IDE's HTML files are stored. 
#define DPT_WEB_IDE_PORT                10000                   // The IDE server port. 

/* Compile time configuration options */
#define DPT_WEB_IDE_HTTP_PATH_BUFF      256                     // Buffer size for filesystem paths
#define DPT_WEB_IDE_HTTP_SEND_BUFF      4096                    // Buffer size for data transfers
#define DPT_WEB_IDE_DEFAULT_FILE        "index.html"            // Default file to serve
#define DPT_WEB_IDE_WEBSOCK_TIMOUT      50                      // Libwebsockets service timeout
#define DPT_WEB_IDE_INTERPRETER_CMD     "/usr/sbin/dpt-js"      // The used interpreter command
#define DPT_WEB_IDE_PROC_READ_BUFF      4096                    // Buffer size for process stdout

/* Configuration structure */
typedef struct{
    bool daemon;
    char* html_path;
    int port;
} config;

/* Application wide configuration */
extern config* conf;

/**
 * Parse configuration file at /etc/config/dpt-web-ide-server
 * @return true when parse was successfull
 */
bool config_parse();

/**
 * Free the parsed configuration data
 */
void config_free();

#endif

