
#include "ts_lua_util.h"


static int ts_lua_transform_handler(TSCont contp, ts_lua_transform_ctx *transform_ctx);


int
ts_lua_transform_entry(TSCont contp, TSEvent event, void *edata)
{
    TSVIO       input_vio;

    ts_lua_transform_ctx *transform_ctx = (ts_lua_transform_ctx*)TSContDataGet(contp);

    if (TSVConnClosedGet(contp)) {
        TSContDestroy(contp);
        TSfree(transform_ctx);
        return 0;
    }

    switch (event) {

        case TS_EVENT_ERROR:
            input_vio = TSVConnWriteVIOGet(contp);
            TSContCall(TSVIOContGet(input_vio), TS_EVENT_ERROR, input_vio);
            break;

        case TS_EVENT_VCONN_WRITE_COMPLETE:
            TSVConnShutdown(TSTransformOutputVConnGet(contp), 0, 1);
            break;

        case TS_EVENT_VCONN_WRITE_READY:
        default:
            ts_lua_transform_handler(contp, transform_ctx);
            break;
    }

    return 0;
}

static int
ts_lua_transform_handler(TSCont contp, ts_lua_transform_ctx *transform_ctx)
{
    TSVConn             output_conn;
    TSVIO               input_vio;
    TSIOBufferReader    input_reader;
    TSIOBufferBlock     blk;
    int64_t             towrite, blk_len, upstream_done, avail, left;
    const char          *start;
    const char          *res;
    size_t              res_len;
    int                 ret, eos;
    lua_State           *L;

    L = transform_ctx->hctx->lua;

    output_conn = TSTransformOutputVConnGet(contp);
    input_vio = TSVConnWriteVIOGet(contp);
    input_reader = TSVIOReaderGet(input_vio);

    if (!transform_ctx->output_buffer) {
        transform_ctx->output_buffer = TSIOBufferCreate();
        transform_ctx->output_reader = TSIOBufferReaderAlloc(transform_ctx->output_buffer);
        transform_ctx->output_vio = TSVConnWrite(output_conn, contp, transform_ctx->output_reader, INT64_MAX);
    }

    if (!TSVIOBufferGet(input_vio)) {
        TSVIONBytesSet(transform_ctx->output_vio, transform_ctx->total);
        TSVIOReenable(transform_ctx->output_vio);
        return 1;
    }

    if (transform_ctx->eos) {
        return 1;
    }

    left = towrite = TSVIONTodoGet(input_vio);
    upstream_done = TSVIONDoneGet(input_vio);
    avail = TSIOBufferReaderAvail(input_reader);
    eos = 0;

    if (left <= avail)
        eos = 1;

    if (towrite > avail)
        towrite = avail;

    blk = TSIOBufferReaderStart(input_reader);

    do {
        start = TSIOBufferBlockReadStart(blk, input_reader, &blk_len);

        lua_pushlightuserdata(L, transform_ctx);
        lua_rawget(L, LUA_GLOBALSINDEX);                /* push function */

        if (towrite > blk_len) {
            lua_pushlstring(L, start, (size_t)blk_len);
            towrite -= blk_len;
        } else {
            lua_pushlstring(L, start, (size_t)towrite);
            towrite = 0;
        }

        if (!towrite && eos) {
            lua_pushinteger(L, 1);                          /* second param, not finish */ 
        } else {
            lua_pushinteger(L, 0);                          /* second param, not finish */ 
        }

        if (lua_pcall(L, 2, 2, 0)) {
            fprintf(stderr, "lua_pcall failed: %s\n", lua_tostring(L, -1));
        }

        ret = lua_tointeger(L, -1);                         /* 0 is not finished, 1 is finished */
        res = lua_tolstring(L, -2, &res_len);

        if (res && res_len) {
            TSIOBufferWrite(transform_ctx->output_buffer, res, res_len);
            transform_ctx->total += res_len;
        }

        lua_pop(L, 2);

        if (ret || eos) {            // EOS
            eos = 1;
            break;
        }

        blk = TSIOBufferBlockNext(blk);

    } while (blk && towrite > 0);

    TSIOBufferReaderConsume(input_reader, avail);
    TSVIONDoneSet(input_vio, upstream_done + avail);

    if (eos) {
        transform_ctx->eos = 1;
        TSVIONBytesSet(transform_ctx->output_vio, transform_ctx->total);
        TSVIOReenable(transform_ctx->output_vio);
        TSContCall(TSVIOContGet(input_vio), TS_EVENT_VCONN_WRITE_COMPLETE, input_vio);
    } else {
        TSVIOReenable(transform_ctx->output_vio);
        TSContCall(TSVIOContGet(input_vio), TS_EVENT_VCONN_WRITE_READY, input_vio);
    }

    return 1;
}

