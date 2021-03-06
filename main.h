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
 * File:   main.h
 * Created on September 30, 2015, 7:21 PM
 */

#ifndef MAIN_H
#define	MAIN_H

#include "http.h"
#include "ide-run.h"

/**
 * The protocols this server supports. 
 */
enum protocols {
    PROTO_HTTP = 0,
    PROTO_IDE_RUN
};

/**
 * Mapping of protocols and callbacks
 */
static struct libwebsocket_protocols protocols[] = {
    {
        "http",
        http_callback,
        sizeof(struct http_session),
        0
    },
    {
        "ide-run",
        ide_run_callback,
        sizeof(struct ide_run_session),
        0
    },
    {
        NULL, NULL, 0, 0
    }
};


/**
 * DPT-Web IDE server main entry point. 
 * @param argc argument count. 
 * @param argv argument data. 
 * @return 0 on success. 
 */
int main(int argc, char** argv);

#endif

