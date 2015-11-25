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
 * File:   http.c
 * Created on September 30, 2015, 7:29 PM
 */

#include <libwebsockets.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "http.h"
#include "config.h"
#include "mimetypes.h"
#include "logger.h"

/**
 * Lookup the mimetype of a file based on the file extension.
 * @param path the full filepath.
 * @return when no mimetype is assigned return the default "application/octet-stream" 
 */
static const char* _http_get_mimetype(const char *path)
{
    const struct mimetype *m = &mime_types[0];
    const char *e;

    while (m->extn) {
        e = &path[strlen(path) - 1];

        while (e >= path) {
            if ((*e == '.' || *e == '/') && !strcasecmp(&e[1], m->extn))
                return m->mime;
            --e;
        }
        ++m;
    }

    return "application/octet-stream";
}

/**
 * This handles HTTP protocol requests. 
 * @param context the context of the request. 
 * @param wsi the websocket currently used. 
 * @param reason the callback reason. 
 * @param user the user function. 
 * @param in the in function. 
 * @param len the length. 
 * @return returns 0 on success or -1 on error.
 */
int http_callback(struct libwebsocket_context *context, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
    char path_buffer[DPT_WEB_IDE_HTTP_PATH_BUFF];                                   /* Buffer to store the real filepath in */
    const char* request = (const char*) in;                                         /* The request part of the URL */
    static unsigned char buffer[DPT_WEB_IDE_HTTP_SEND_BUFF];                        /* Data transfer buffer */
    struct http_session *sess = (struct http_session*) user;                        /* The HTTP session data */
    int n, m;                                                                       /* Working variables */                               
    
    switch(reason) {
        case LWS_CALLBACK_HTTP:
            /* Check the request header */
            if(len < 1) {
                log_message(LOG_ERROR, "File request is to short, bad request\r\n");
                libwebsockets_return_http_status(context, wsi, HTTP_STATUS_BAD_REQUEST, NULL);
                goto finish;
            }
            
            /* Proceed to accept data if this is a POST request */
            if(lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI)) {
                return 0;
            }
            
            /* Add the base directory to the file path */
            strncpy(path_buffer, conf->html_path, DPT_WEB_IDE_HTTP_PATH_BUFF);
            
            /* Check if a filename is given */
            if(strcmp(request, "/")) {
                /* Remove query string if any by null terminating on the question mark */
                char *last = strrchr(request, '?');
                if(last != NULL) {
                    *last = '\0';
                }
                
                /* Append correct filename */
                if(request[0] == '/') {
                    strncat(path_buffer, "/", DPT_WEB_IDE_HTTP_PATH_BUFF - strlen(path_buffer));
                }
                strncat(path_buffer, request, DPT_WEB_IDE_HTTP_PATH_BUFF - strlen(path_buffer));
            } else {
                /* Serve default file */
                strncat(path_buffer, "/" DPT_WEB_IDE_DEFAULT_FILE, DPT_WEB_IDE_HTTP_PATH_BUFF - strlen(path_buffer));
            }
            path_buffer[DPT_WEB_IDE_HTTP_PATH_BUFF - 1] = '\0';
            
            /* Lookup the mimetype of the file */
            const char* mimetype = _http_get_mimetype(path_buffer);
            
            // Serve the file asynchronously
            n = libwebsockets_serve_http_file(context, wsi, path_buffer, mimetype, NULL, 0);
            if (n < 0 || ((n > 0) && lws_http_transaction_completed(wsi))) {
                log_message(LOG_ERROR, "Can't reuse the connection, close socket\r\n");
                return -1;
            }
    
            break;
            
        case LWS_CALLBACK_HTTP_BODY:
            strncpy(path_buffer, request, 20);
            path_buffer[len < 20 ? len : 20] = '\0';
            
            lwsl_notice("LWS_CALLBACK_HTTP_BODY: %s... len %d\n", (const char*) path_buffer, (int) len);
            break;
            
        case LWS_CALLBACK_HTTP_BODY_COMPLETION:
            lwsl_notice("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
            libwebsockets_return_http_status(context, wsi, HTTP_STATUS_OK, NULL);
            goto finish;
            
        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            // Close the connection after the body is complete
            goto finish;
            
        case LWS_CALLBACK_HTTP_WRITEABLE:
            // Proceed with transmitting data
            do {
                n = DPT_WEB_IDE_HTTP_SEND_BUFF - LWS_SEND_BUFFER_PRE_PADDING;
                m = lws_get_peer_write_allowance(wsi);
                
                if(m == 0) {
                    goto afterwrite;
                }
                
                if(m != -1 && m < n) {
                    n = m;
                }
                
                // Read from the buffer and close connection on problem
                n = read(sess->fd, buffer + LWS_SEND_BUFFER_PRE_PADDING, n);
                if(n < 0) {
                    log_message(LOG_ERROR, "Problem with reading from HTTP connection\r\n");
                    goto close_conn;
                }
                if(n == 0) {
                    goto flush_close_conn;
                }
                
                // Now try to write and support HTTP2 and close connection on problem
                m = libwebsocket_write(wsi, buffer + LWS_SEND_BUFFER_PRE_PADDING, n, LWS_WRITE_HTTP);
                if((m < 0) || (m != n && lseek(sess->fd, m - n, SEEK_CUR) < 0)) {
                    goto close_conn;
                }
                if(m) {
                    libwebsocket_set_timeout(wsi, PENDING_TIMEOUT_HTTP_CONTENT, 5);
                }
                
                // If the buffer is full, wait for another call
                if(lws_partial_buffered(wsi)) {
                    break;
                }
            } while(!lws_send_pipe_choked(wsi));
            
afterwrite:
            libwebsocket_callback_on_writable(context, wsi);    
            break;

flush_close_conn:
            if(lws_partial_buffered(wsi)) {
                libwebsocket_callback_on_writable(context, wsi);
                break;
            }
            close(sess->fd);
            goto finish;
            
close_conn:
            close(sess->fd);
            return -1;
            
        default:
            break;
    }
    
    return 0;

finish:
    return lws_http_transaction_completed(wsi) ? -1 : 0;
}