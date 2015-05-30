
#include "ts_fetcher.h"

char * http_method_str[] =  {
    "DUMMY",
    "GET",
    "POST",
    "CONNECT",
    "DELETE",
    "HEAD",
    "PURGE",
    "PUT",
    "LAST"
};

int ts_http_fetch_handler(TSCont contp, TSEvent event, void *edata);
int ts_http_fetcher_process_write(http_fetcher *fch, TSEvent event);
int ts_http_fetcher_process_read(http_fetcher *fch, TSEvent event);
int ts_http_fetcher_parse_header(http_fetcher *fch);
void ts_http_fetcher_release(http_fetcher *fch);
int ts_http_fetcher_callback_sm(http_fetcher *fch, TSEvent event);
int ts_http_fetcher_filter_body(http_fetcher *fch, int ev);
void ts_http_fetcher_extract(http_fetcher *fch);
void ts_http_fetcher_setup_filter(http_fetcher *fch);

static int ts_http_fetcher_transfer(http_fetcher *fch, int code);
static int ts_http_fetcher_verify_chunked(http_fetcher *fch, int code);
static int ts_http_fetcher_verify_cl(http_fetcher *fch, int code);
static int chunked_extract_frag_len(chunked_info *ci, const char *start, int64_t n);
static int chunked_extract_return(chunked_info *ci, const char *start, int64_t n, int tail);
static void chunked_info_init(chunked_info *ci, int dechunk);


http_fetcher *
ts_http_fetcher_create(TSCont contp, struct sockaddr *addr, int flags)
{
    http_fetcher    *fch;

    fch = (http_fetcher*)TSmalloc(sizeof(http_fetcher));

    fch->flags = flags;
    fch->contp = contp;
    fch->aip = *addr;

    if (flags & TS_FLAG_FETCH_USE_NEW_LOCK) {
        fch->mutexp = TSMutexCreate();
    } else {
        fch->mutexp = TSContMutexGet(contp);
    }

    fch->req_buffer = TSIOBufferCreate();
    fch->req_reader = TSIOBufferReaderAlloc(fch->req_buffer);

    /* initialize */
    fch->http_vc = NULL;
    fch->read_vio = NULL;
    fch->write_vio = NULL;
    fch->method = 0;

    fch->http_parser = NULL;
    fch->hdr_bufp = NULL;
    fch->hdr_loc = NULL;
    fch->hdr_buffer = NULL;
    fch->hdr_reader = NULL;
    fch->resp_buffer = NULL;
    fch->resp_reader = NULL;
    fch->body_buffer = NULL;
    fch->body_reader = NULL;
    fch->flow_buffer = NULL;
    fch->flow_reader = NULL;

    fch->fetch_contp = NULL;
    fch->action = NULL;

    fch->resp_cl = -1;
    fch->resp_already = 0;
    fch->post_cl = 0;
    fch->post_already = 0;

    fch->ctx = NULL;
    fch->status_code = 0;
    fch->ref = 0;

    fch->header_done = 0;
    fch->body_done = 0;
    fch->deleted = 0;
    fch->chunked = 0;
    fch->error = 0;
    fch->launched = 0;

    return fch;
}

void
ts_http_fetcher_destroy(http_fetcher *fch)
{
    if (fch->action) {
        TSActionCancel(fch->action);
        fch->action = NULL;
    }

    fch->deleted = 1;

    if (fch->ref)
        return;

    ts_http_fetcher_release(fch);
}

void
ts_http_fetcher_init(http_fetcher *fch, const char *method, int method_len, const char *uri, int uri_len)
{
    char    buf[2048];
    int     n;

    if (uri_len >= sizeof(buf) - 64)
        uri_len = sizeof(buf) - 64;

    if (method_len == TS_HTTP_LEN_GET && !strncasecmp(method, TS_HTTP_METHOD_GET, TS_HTTP_LEN_GET)) {
        fch->method = TS_FETCH_HTTP_METHOD_GET;
        n = sprintf(buf, "GET %.*s HTTP/1.1\r\n", uri_len, uri);

    } else if (method_len == TS_HTTP_LEN_POST && !strncasecmp(method, TS_HTTP_METHOD_POST, TS_HTTP_LEN_POST)) {
        fch->method = TS_FETCH_HTTP_METHOD_POST;
        n = sprintf(buf, "POST %.*s HTTP/1.1\r\n", uri_len, uri);

    } else if (method_len == TS_HTTP_LEN_CONNECT && !strncasecmp(method, TS_HTTP_METHOD_CONNECT, TS_HTTP_LEN_CONNECT)) {
        fch->method = TS_FETCH_HTTP_METHOD_CONNECT;
        n = sprintf(buf, "CONNECT %.*s HTTP/1.1\r\n", uri_len, uri);

    } else if (method_len == TS_HTTP_LEN_DELETE && !strncasecmp(method, TS_HTTP_METHOD_DELETE, TS_HTTP_LEN_DELETE)) {
        fch->method = TS_FETCH_HTTP_METHOD_DELETE;
        n = sprintf(buf, "DELETE %.*s HTTP/1.1\r\n", uri_len, uri);

    } else if (method_len == TS_HTTP_LEN_HEAD && !strncasecmp(method, TS_HTTP_METHOD_HEAD, TS_HTTP_LEN_HEAD)) {
        fch->method = TS_FETCH_HTTP_METHOD_HEAD;
        n = sprintf(buf, "HEAD %.*s HTTP/1.1\r\n", uri_len, uri);

    } else if (method_len == TS_HTTP_LEN_PURGE && !strncasecmp(method, TS_HTTP_METHOD_PURGE, TS_HTTP_LEN_PURGE)) {
        fch->method = TS_FETCH_HTTP_METHOD_PURGE;
        n = sprintf(buf, "PURGE %.*s HTTP/1.1\r\n", uri_len, uri);

    } else if (method_len == TS_HTTP_LEN_PUT && !strncasecmp(method, TS_HTTP_METHOD_PUT, TS_HTTP_LEN_PUT)) {
        fch->method = TS_FETCH_HTTP_METHOD_PUT;
        n = sprintf(buf, "PUT %.*s HTTP/1.1\r\n", uri_len, uri);

    } else {

        if (method_len >= 16)
            method_len = 16;

        n = sprintf(buf, "%.*s %.*s HTTP/1.1\r\n", method_len, method, uri_len, uri);
    }

    TSIOBufferWrite(fch->req_buffer, buf, n);
}

void
ts_http_fetcher_init_common(http_fetcher *fch, int method, const char *uri, int uri_len)
{
    char    buf[2048];
    int     n;

    fch->method = method;

    if (uri_len >= sizeof(buf) - 64)
        uri_len = sizeof(buf) - 64;

    n = sprintf(buf, "%s %.*s HTTP/1.1\r\n", http_method_str[method], uri_len,uri);

    TSIOBufferWrite(fch->req_buffer, buf, n);
}

void
ts_http_fetcher_add_header(http_fetcher *fch, const char *name, int name_len, const char *value, int value_len)
{
    if ((name_len == TS_MIME_LEN_CONTENT_LENGTH) &&
            (strncasecmp(name, TS_MIME_FIELD_CONTENT_LENGTH, name_len) == 0)) {      // for POST
        fch->post_cl = atoll(value);
    }

    TSIOBufferWrite(fch->req_buffer, name, name_len);
    TSIOBufferWrite(fch->req_buffer, ": ", sizeof(": ")-1);
    TSIOBufferWrite(fch->req_buffer, value, value_len);
    TSIOBufferWrite(fch->req_buffer, "\r\n", sizeof("\r\n")-1);
}

void
ts_http_fetcher_launch(http_fetcher *fch)
{
    int64_t     hdr_len;
    // post body , content-length

    TSIOBufferWrite(fch->req_buffer, "\r\n", sizeof("\r\n")-1);
    hdr_len = TSIOBufferReaderAvail(fch->req_reader);

    fch->http_vc = TSHttpConnect(&(fch->aip));

    fch->hdr_buffer = TSIOBufferSizedCreate(TS_IOBUFFER_SIZE_INDEX_8K);
    fch->hdr_reader = TSIOBufferReaderAlloc(fch->hdr_buffer);
    fch->hdr_bufp = TSMBufferCreate();
    fch->hdr_loc = TSHttpHdrCreate(fch->hdr_bufp);

    fch->resp_buffer = TSIOBufferCreate();
    fch->resp_reader = TSIOBufferReaderAlloc(fch->resp_buffer);
    fch->http_parser = TSHttpParserCreate();

    fch->fetch_contp = TSContCreate(ts_http_fetch_handler, fch->mutexp);
    TSContDataSet(fch->fetch_contp, fch);

    fch->read_vio = TSVConnRead(fch->http_vc, fch->fetch_contp, fch->resp_buffer, INT64_MAX);

    if ((fch->method == TS_FETCH_HTTP_METHOD_POST || fch->method == TS_FETCH_HTTP_METHOD_PUT) && fch->post_cl >= 0) {
        fch->write_vio = TSVConnWrite(fch->http_vc, fch->fetch_contp, fch->req_reader, hdr_len + fch->post_cl);

    } else {
        fch->write_vio = TSVConnWrite(fch->http_vc, fch->fetch_contp, fch->req_reader, hdr_len);
    }

    fch->launched = 1;
}

void
ts_http_fetcher_consume_resp_body(http_fetcher *fch, int64_t len)
{
    if (!fch->header_done)
        return;

    TSIOBufferReaderConsume(fch->body_reader, len);

    if (!fch->deleted && !fch->action &&
            TSIOBufferReaderAvail(fch->body_reader) < TS_MARK_FETCH_BODY_LOW_WATER) {
        fch->action = TSContSchedule(fch->fetch_contp, 0, TS_THREAD_POOL_DEFAULT);
    }
}

void
ts_http_fetcher_set_ctx(http_fetcher *fch, void *ctx)
{
    fch->ctx = ctx;
}

void *
ts_http_fetcher_get_ctx(http_fetcher *fch)
{
    return fch->ctx;
}

void
ts_http_fetcher_append_data(http_fetcher *fch, const char *data, int len)
{
    if (len <= 0 || fch->body_done)
        return;

    TSIOBufferWrite(fch->req_buffer, data, len);

    if (fch->launched)
        TSVIOReenable(fch->write_vio);
}

void
ts_http_fetcher_copy_data(http_fetcher *fch, TSIOBufferReader readerp, int64_t length, int64_t offset)
{
    if (length <= 0 || fch->body_done)
        return;

    TSIOBufferCopy(fch->req_buffer, readerp, length, offset);

    if (fch->launched)
        TSVIOReenable(fch->write_vio);
}

int
ts_http_fetcher_process_write(http_fetcher *fch, TSEvent event)
{
    switch (event) {

        case TS_EVENT_VCONN_WRITE_READY:
            if (TSIOBufferReaderAvail(fch->req_reader) > 0)
                TSVIOReenable(fch->write_vio);

            break;

        case TS_EVENT_VCONN_WRITE_COMPLETE:
            break;

        case TS_EVENT_ERROR:
        default:
            return ts_http_fetcher_callback_sm(fch, TS_EVENT_FETCH_ERROR);
    }

    return 0;
}

int
ts_http_fetcher_process_read(http_fetcher *fch, TSEvent event)
{
    int         ret;

    switch (event) {

        case TS_EVENT_VCONN_READ_READY:
        case TS_EVENT_VCONN_READ_COMPLETE:
        case TS_EVENT_VCONN_EOS:

            if (!fch->header_done) {
                ret = ts_http_fetcher_parse_header(fch);
                if (ret) {
                    return ts_http_fetcher_callback_sm(fch, TS_EVENT_FETCH_ERROR);

                } else if (fch->header_done) {
                    if (!(fch->flags & TS_FLAG_FETCH_IGNORE_HEADER_DONE)) {
                        ret = ts_http_fetcher_callback_sm(fch, TS_EVENT_FETCH_HEADER_DONE);
                        if (ret)
                            return -1;
                    }
                }
            }

            if (event == TS_EVENT_VCONN_READ_READY) {

                if (fch->header_done) {

                    if (fch->status_code == TS_HTTP_STATUS_NOT_MODIFIED || fch->status_code == TS_HTTP_STATUS_NO_CONTENT) {
                        ret = ts_http_fetcher_filter_body(fch, BODY_COMPLETE);
                    } else {
                        ret = ts_http_fetcher_filter_body(fch, BODY_READY);
                    }

                    if (ret != TS_EVENT_FETCH_BODY_READY || !(fch->flags & TS_FLAG_FETCH_IGNORE_BODY_READY)) {
                        if (ts_http_fetcher_callback_sm(fch, ret))
                            return -1;
                    }

                } else {
                    TSVIOReenable(fch->read_vio);
                }

            } else if (fch->header_done) {
                ret = ts_http_fetcher_filter_body(fch, BODY_COMPLETE);
                return ts_http_fetcher_callback_sm(fch, ret);

            } else {
                return ts_http_fetcher_callback_sm(fch, TS_EVENT_FETCH_ERROR);
            }

            break;

        case TS_EVENT_ERROR:
        default:
            return ts_http_fetcher_callback_sm(fch, TS_EVENT_FETCH_ERROR);
    }

    return 0;
}

int
ts_http_fetcher_parse_header(http_fetcher *fch)
{
    int64_t         avail, used;
    const char      *start, *guard, *end;
    int             ret;
    TSIOBufferBlock blk;

    blk = TSIOBufferReaderStart(fch->resp_reader);

    while (blk) {
        guard = start = TSIOBufferBlockReadStart(blk, fch->resp_reader, &avail);
        end = start + avail;

        ret = TSHttpHdrParseResp(fch->http_parser, fch->hdr_bufp, fch->hdr_loc, &start, end);

        switch (ret) {

            case TS_PARSE_ERROR:
                return -1;

            case TS_PARSE_DONE:
                used = start - guard;
                TSIOBufferCopy(fch->hdr_buffer, fch->resp_reader, used, 0);
                TSIOBufferReaderConsume(fch->resp_reader, used);

                ts_http_fetcher_extract(fch);
                // ts_http_fetcher_setup_filter(fch);

                fch->header_done = 1;

                return 0;

            default:
                TSIOBufferCopy(fch->hdr_buffer, fch->resp_reader, avail, 0);
                TSIOBufferReaderConsume(fch->resp_reader, avail);
                break;
        }

        blk = TSIOBufferBlockNext(blk);
    }

    return 0;
}

void
ts_http_fetcher_extract(http_fetcher *fch)
{
    TSMLoc      cl_loc, te_loc;
    const char  *val;
    int         val_len;
    int         i, n;

    fch->status_code = TSHttpHdrStatusGet(fch->hdr_bufp, fch->hdr_loc);

    cl_loc = TSMimeHdrFieldFind(fch->hdr_bufp, fch->hdr_loc, TS_MIME_FIELD_CONTENT_LENGTH, TS_MIME_LEN_CONTENT_LENGTH);
    if (cl_loc)
        fch->resp_cl = TSMimeHdrFieldValueInt64Get(fch->hdr_bufp, fch->hdr_loc, cl_loc, -1);
    else
        fch->resp_cl = -1;

    te_loc = TSMimeHdrFieldFind(fch->hdr_bufp, fch->hdr_loc, TS_MIME_FIELD_TRANSFER_ENCODING, TS_MIME_LEN_TRANSFER_ENCODING);
    if (te_loc) {
        n = TSMimeHdrFieldValuesCount(fch->hdr_bufp, fch->hdr_loc, te_loc);
        for (i = 0; i < n; i++) {
            val = TSMimeHdrFieldValueStringGet(fch->hdr_bufp, fch->hdr_loc, te_loc, i, &val_len);
            if ((val_len == TS_HTTP_LEN_CHUNKED) && (strncasecmp(val, TS_HTTP_VALUE_CHUNKED, val_len) == 0)) {
                fch->chunked = 1;
                break;
            }
        }
    }

    if (cl_loc)
        TSHandleMLocRelease(fch->hdr_bufp, fch->hdr_loc, cl_loc);

    if (te_loc) {

        if (fch->flags & TS_FLAG_FETCH_FORCE_DECHUNK)
            TSMimeHdrFieldDestroy(fch->hdr_bufp, fch->hdr_loc, te_loc);

        TSHandleMLocRelease(fch->hdr_bufp, fch->hdr_loc, te_loc);
    }

    if (fch->chunked) {
        if (fch->flags & TS_FLAG_FETCH_FORCE_DECHUNK) {
            chunked_info_init(&fch->cinfo, 1);
        } else {
            chunked_info_init(&fch->cinfo, 0);
        }

    } else if (fch->resp_cl >= 0) {
        fch->resp_already = 0;
    }

    fch->flow_buffer = TSIOBufferCreate();
    fch->flow_reader = TSIOBufferReaderAlloc(fch->flow_buffer);

    fch->body_buffer = TSIOBufferCreate();
    fch->body_reader = TSIOBufferReaderAlloc(fch->body_buffer);
}

int
ts_http_fetcher_filter_body(http_fetcher *fch, int ev)
{
    int         ret;
    int64_t     resp_avail, body_avail, wavail, need;

    if (fch->action) {
        TSActionCancel(fch->action);
        fch->action = NULL;
    }

    need = 0;
    resp_avail = TSIOBufferReaderAvail(fch->resp_reader);

    if (ev != BODY_COMPLETE) {

        body_avail = TSIOBufferReaderAvail(fch->body_reader);
        wavail = TS_MARK_FETCH_BODY_HIGH_WATER - body_avail;

        if (wavail <= 0)
            return TS_EVENT_FETCH_BODY_QUIET;

        need = resp_avail > wavail ? wavail : resp_avail;

        TSIOBufferCopy(fch->flow_buffer, fch->resp_reader, need, 0);
        TSIOBufferReaderConsume(fch->resp_reader, need);

        if (TSIOBufferReaderAvail(fch->resp_reader) < TS_MARK_FETCH_RESP_LOW_WATER)
            TSVIOReenable(fch->read_vio);

    } else  {
        TSIOBufferCopy(fch->flow_buffer, fch->resp_reader, resp_avail, 0);
        TSIOBufferReaderConsume(fch->resp_reader, resp_avail);
        need = resp_avail;
    }

    if (fch->chunked) {
        ret = ts_http_fetcher_verify_chunked(fch, ev);

    } else if (fch->resp_cl >= 0) {
        ret = ts_http_fetcher_verify_cl(fch, ev);

    } else {
        ret = ts_http_fetcher_transfer(fch, ev);
    }

    switch (ret) {

        case BODY_READY:
            if (need == 0)
                return TS_EVENT_FETCH_BODY_QUIET;

            return TS_EVENT_FETCH_BODY_READY;

        case BODY_COMPLETE:
            fch->body_done = 1;
            return TS_EVENT_FETCH_BODY_COMPLETE;

        default:
            break;
    }

    fch->error = 1;
    return TS_EVENT_FETCH_ERROR;
}

int
ts_http_fetcher_callback_sm(http_fetcher *fch, TSEvent event)
{
    if (fch->deleted && !fch->ref) {
        ts_http_fetcher_release(fch);
        return -1;
    }

    if (event == TS_EVENT_FETCH_BODY_QUIET)
        return 0;

    fch->ref++;

    if (fch->flags & TS_FLAG_FETCH_USE_NEW_LOCK)
        TSMutexLock(TSContMutexGet(fch->contp));

    TSContCall(fch->contp, event, fch);

    if (fch->flags & TS_FLAG_FETCH_USE_NEW_LOCK)
        TSMutexUnlock(TSContMutexGet(fch->contp));

    fch->ref--;

    if (fch->deleted && !fch->ref) {
        ts_http_fetcher_release(fch);
        return -1;
    }

    return 0;
}

void
ts_http_fetcher_release(http_fetcher *fch)
{
    if (fch->http_vc) {
        TSVConnClose(fch->http_vc);
        fch->http_vc = NULL;
    }

    if (fch->http_parser)
        TSHttpParserDestroy(fch->http_parser);

    if (fch->hdr_loc) {
        TSMimeHdrDestroy(fch->hdr_bufp, fch->hdr_loc);
        TSHandleMLocRelease(fch->hdr_bufp, TS_NULL_MLOC, fch->hdr_loc);
    }

    if (fch->hdr_bufp)
        TSMBufferDestroy(fch->hdr_bufp);

    if (fch->hdr_reader)
        TSIOBufferReaderFree(fch->hdr_reader);
    if (fch->hdr_buffer)
        TSIOBufferDestroy(fch->hdr_buffer);

    if (fch->resp_reader)
        TSIOBufferReaderFree(fch->resp_reader);
    if (fch->resp_buffer)
        TSIOBufferDestroy(fch->resp_buffer);

    if (fch->body_reader)
        TSIOBufferReaderFree(fch->body_reader);
    if (fch->body_buffer)
        TSIOBufferDestroy(fch->body_buffer);

    if (fch->flow_reader)
        TSIOBufferReaderFree(fch->flow_reader);
    if (fch->flow_buffer)
        TSIOBufferDestroy(fch->flow_buffer);

    if (fch->req_reader)
        TSIOBufferReaderFree(fch->req_reader);
    if (fch->req_buffer)
        TSIOBufferDestroy(fch->req_buffer);

    TSContDestroy(fch->fetch_contp);

    TSfree(fch);
    fch = NULL;
}

int
ts_http_fetch_handler(TSCont contp, TSEvent event, void *edata)
{
    int     ret;
    http_fetcher *fch = (http_fetcher*)TSContDataGet(contp);

    if (event == TS_EVENT_IMMEDIATE) {
        ret = ts_http_fetcher_filter_body(fch, BODY_READY);
        ts_http_fetcher_callback_sm(fch, (TSEvent)ret);

    } else if (edata == fch->read_vio) {
        ts_http_fetcher_process_read(fch, event);

    } else if (edata == fch->write_vio) {
        ts_http_fetcher_process_write(fch, event);

    } else {
        ts_http_fetcher_callback_sm(fch, TS_EVENT_FETCH_ERROR);
    }

    return 0;
}

static int
ts_http_fetcher_transfer(http_fetcher *fch, int code)
{
    int64_t     left;

    left = TSIOBufferReaderAvail(fch->flow_reader);

    TSIOBufferCopy(fch->body_buffer, fch->flow_reader, left, 0); 
    TSIOBufferReaderConsume(fch->flow_reader, left);

    return code;
}

static int
ts_http_fetcher_verify_cl(http_fetcher *fch, int code)
{
    int64_t need, left;

    if (fch->resp_already >= fch->resp_cl)
        return BODY_COMPLETE;

    left = TSIOBufferReaderAvail(fch->flow_reader);

    if (fch->resp_already + left >= fch->resp_cl) {
        need = fch->resp_cl - fch->resp_already;
    } else {
        need = left;
    }   

    TSIOBufferCopy(fch->body_buffer, fch->flow_reader, need, 0); 
    TSIOBufferReaderConsume(fch->flow_reader, need);

    fch->resp_already += left;

    if (fch->resp_already >= fch->resp_cl)
        return BODY_COMPLETE;
    else if (code == BODY_COMPLETE)
        return BODY_ERROR;

    return BODY_READY;
}

static int
ts_http_fetcher_verify_chunked(http_fetcher *fch, int code)
{
    int         n;
    int64_t     avail, need;
    int64_t     blk_len;
    const char  *start;
    const char  *cur;
    TSIOBufferBlock blk, next_blk;

    chunked_info *ci = &fch->cinfo;
    blk = TSIOBufferReaderStart(fch->flow_reader);

    while (blk) {

        next_blk = TSIOBufferBlockNext(blk);

        start = TSIOBufferBlockReadStart(blk, fch->flow_reader, &blk_len);
        avail = blk_len;

        if (avail) {

            do {
                cur = start + blk_len - avail;

                switch (ci->state) {

                    case CHUNK_WAIT_LENGTH:
                        n = chunked_extract_frag_len(ci, cur, avail);
                        if (n < 0)
                            return BODY_ERROR;

                        avail -= n;

                        if (!ci->dechunk_enabled)
                            TSIOBufferCopy(fch->body_buffer, fch->flow_reader, n, 0);

                        TSIOBufferReaderConsume(fch->flow_reader, n);
                        break;

                    case CHUNK_WAIT_RETURN:
                        n = chunked_extract_return(ci, cur, avail, 0);
                        avail -= n;

                        if (!ci->dechunk_enabled)
                            TSIOBufferCopy(fch->body_buffer, fch->flow_reader, n, 0);

                        TSIOBufferReaderConsume(fch->flow_reader, n);
                        break;

                    case CHUNK_WAIT_DATA:
                        if (ci->frag_len + avail <= ci->frag_total) {
                            TSIOBufferCopy(fch->body_buffer, fch->flow_reader, avail, 0);
                            TSIOBufferReaderConsume(fch->flow_reader, avail);
                            ci->frag_len += avail;
                            avail = 0;
                            break;
                        } else {
                            need = ci->frag_total - ci->frag_len;
                            if (need) {
                                TSIOBufferCopy(fch->body_buffer, fch->flow_reader, need, 0);
                                TSIOBufferReaderConsume(fch->flow_reader, need);
                                ci->frag_len += need;
                                avail -= need;
                            }

                            ci->cr = 0;
                            ci->state = CHUNK_WAIT_RETURN_END;
                        }

                        break;

                    case CHUNK_WAIT_RETURN_END:
                        n = chunked_extract_return(ci, cur, avail, 1);
                        avail -= n;

                        if (!ci->dechunk_enabled)
                            TSIOBufferCopy(fch->body_buffer, fch->flow_reader, n, 0);

                        TSIOBufferReaderConsume(fch->flow_reader, n);
                        break;

                    case CHUNK_DATA_DONE:

                        if (!ci->dechunk_enabled)
                            TSIOBufferCopy(fch->body_buffer, fch->flow_reader, avail, 0);

                        TSIOBufferReaderConsume(fch->flow_reader, avail);
                        avail = 0;
                        ci->done = 1;
                        break;

                    default:
                        break;
                }
            } while (avail > 0);
        }

        if (ci->done)
            break;

        blk = next_blk;
    }

    if (ci->done) {
        return BODY_COMPLETE;

    } else if (code == BODY_COMPLETE) {
        return BODY_ERROR;

    } else {
        return BODY_READY;
    }
}

static int
chunked_extract_frag_len(chunked_info *ci, const char *start, int64_t n)
{
    const char *ptr = start;
    const char *end = start + n;

    while (ptr < end) {

        if ((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'a' && *ptr <= 'f') || (*ptr >= 'A' && *ptr <= 'F')) {

            ci->frag_buf[ci->frag_pos++] = *ptr;

            if (ci->frag_pos > sizeof(ci->frag_buf) - 1)
                return -1;

            ptr++;

        } else {

            if (ci->frag_pos == 0)
                return -1;

            ci->frag_buf[ci->frag_pos++] = 0;
            sscanf(ci->frag_buf, "%x", &(ci->frag_total));
            ci->frag_len = 0;

            if (ci->frag_total == 0)
                ci->state = CHUNK_DATA_DONE;
            else {
                ci->cr = 0;
                ci->state = CHUNK_WAIT_RETURN;
            }

            break;
        }
    }

    return ptr - start;
}

static int 
chunked_extract_return(chunked_info *ci, const char *start, int64_t n, int tail)
{
    const char *ptr = start;
    const char *end = start + n;

    if (!n)
        return 0;

    do {
        if (ci->cr && *ptr ++ == '\n') {
            if (tail) {
                ci->frag_pos = 0;
                ci->state = CHUNK_WAIT_LENGTH;
            } else {
                ci->frag_len = 0;
                ci->state = CHUNK_WAIT_DATA;
            }

            break;
        }

        ptr = (char*)memchr(ptr, '\r', end - ptr);
        if (ptr) {
            ci->cr = 1;
            ptr++;
        }

    } while (ptr < end);

    return ptr - start;
}

static void
chunked_info_init(chunked_info *ci, int dechunk)
{
    ci->state = CHUNK_WAIT_LENGTH;
    ci->frag_total = 0;
    ci->frag_len = 0;
    ci->frag_pos = 0;
    ci->done = 0;
    ci->cr = 0;
    ci->dechunk_enabled = dechunk;
}
