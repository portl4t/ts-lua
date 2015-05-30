
#ifndef _TS_FETCHER_H
#define _TS_FETCHER_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ts/ts.h>
#include <ts/experimental.h>

#define TS_EVENT_FETCH_HEADER_DONE          63000
#define TS_EVENT_FETCH_BODY_READY           63001
#define TS_EVENT_FETCH_BODY_COMPLETE        63002
#define TS_EVENT_FETCH_ERROR                63003
#define TS_EVENT_FETCH_BODY_QUIET           63004

#define TS_FLAG_FETCH_FORCE_DECHUNK         (1<<0)
#define TS_FLAG_FETCH_IGNORE_HEADER_DONE    (1<<1)
#define TS_FLAG_FETCH_IGNORE_BODY_READY     (1<<2)
#define TS_FLAG_FETCH_USE_NEW_LOCK          (1<<3)

#define TS_MARK_FETCH_RESP_LOW_WATER        (4*1024)        // we should reenable read_vio when resp_buffer data less than this
#define TS_MARK_FETCH_BODY_LOW_WATER        (4*1024)        // we should move data from resp_buffer to body_buffer when body_buffer data less than this
#define TS_MARK_FETCH_BODY_HIGH_WATER       (16*1024)       // we should not move data from resp_buffer to body_buffer more than this


typedef enum {
    TS_FETCH_HTTP_METHOD_DUMMY,
    TS_FETCH_HTTP_METHOD_GET,
    TS_FETCH_HTTP_METHOD_POST,
    TS_FETCH_HTTP_METHOD_CONNECT,
    TS_FETCH_HTTP_METHOD_DELETE,
    TS_FETCH_HTTP_METHOD_HEAD,
    TS_FETCH_HTTP_METHOD_PURGE,
    TS_FETCH_HTTP_METHOD_PUT,
    TS_FETCH_HTTP_METHOD_LAST
} http_method;

typedef enum {
    BODY_READY = 0,
    BODY_COMPLETE,
    BODY_ERROR,
} http_body_state;

enum ChunkedState
{
    CHUNK_WAIT_LENGTH,
    CHUNK_WAIT_RETURN,
    CHUNK_WAIT_DATA,
    CHUNK_WAIT_RETURN_END,
    CHUNK_DATA_DONE
};


typedef struct {
    int             state;

    int             frag_total;
    int             frag_len;
    char            frag_buf[16];
    unsigned char   frag_pos;

    unsigned        done:1;
    unsigned        cr:1;
    unsigned        dechunk_enabled:1;
} chunked_info;

typedef struct {
    TSVConn         http_vc;
    TSVIO           read_vio;
    TSVIO           write_vio;

    http_method     method;

    TSHttpParser    http_parser;                // http response parser

    TSMBuffer       hdr_bufp;                   // HTTPHdr, parse result of response header
    TSMLoc          hdr_loc;                    // HTTPHdr->m_http

    TSIOBuffer          hdr_buffer;             // buffer to store the response header, migration from resp_buffer
    TSIOBufferReader    hdr_reader;

    TSIOBuffer          resp_buffer;            // buffer to store the raw response
    TSIOBufferReader    resp_reader;

    TSIOBuffer          body_buffer;            // buffer to store the response body, migration from resp_buffer
    TSIOBufferReader    body_reader;

    TSIOBuffer          flow_buffer;            // buffer to control flow for response body
    TSIOBufferReader    flow_reader;

    TSIOBuffer          req_buffer;             // buffer to store the request
    TSIOBufferReader    req_reader;

    TSCont          contp;
    TSMutex         mutexp;

    TSCont          fetch_contp;
    TSAction        action;

    struct sockaddr aip;

    int64_t         resp_cl;                         // content-length of resp
    int64_t         resp_already;                    // verify content-length

    int64_t         post_cl;
    int64_t         post_already;

    void            *ctx;
    int             status_code;
    int             flags;
    int             ref;

    chunked_info    cinfo;
    unsigned        header_done:1;
    unsigned        body_done:1;
    unsigned        deleted:1;
    unsigned        chunked:1;
    unsigned        error:1;
    unsigned        launched:1;
//    unsigned        init_with_str:1;
} http_fetcher;


http_fetcher * ts_http_fetcher_create(TSCont contp, struct sockaddr *addr, int flags);
void ts_http_fetcher_destroy(http_fetcher *fch);

void ts_http_fetcher_init(http_fetcher *fch, const char *method, int method_len, const char *uri, int uri_len);
void ts_http_fetcher_init_common(http_fetcher *fch, int method, const char *uri, int uri_len);
void ts_http_fetcher_add_header(http_fetcher *fch, const char *name, int name_len, const char *value, int value_len);
void ts_http_fetcher_append_data(http_fetcher *fch, const char *data, int len);
void ts_http_fetcher_copy_data(http_fetcher *fch, TSIOBufferReader readerp, int64_t length, int64_t offset);
void ts_http_fetcher_launch(http_fetcher *fch);
void ts_http_fetcher_set_high_water(http_fetcher *fch, int64_t max);
void ts_http_fetcher_consume_resp_body(http_fetcher *fch, int64_t len);

void ts_http_fetcher_set_ctx(http_fetcher *fch, void *ctx);
void * ts_http_fetcher_get_ctx(http_fetcher *fch);

#ifdef  __cplusplus
}
#endif

#endif
