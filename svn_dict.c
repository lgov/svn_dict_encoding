//
//  svn_dict.c
//  svn_dict_encoding
//
//  Created by Lieven Govaerts on 24/09/12.
//  Copyright (c) 2012 Lieven Govaerts. All rights reserved.
//

#include <stdio.h>

#include "apr.h"
#include "apr_pools.h"

#include <zlib.h>

#include "serf.h"
#include "serf_bucket_types.h"

static const char *propfind_resp =
"<?xml version='1.0' encoding='utf-8'?>"
"<D:multistatus xmlns:D='DAV:' xmlns:ns0='DAV:'>"
"<D:response xmlns:S='http://subversion.tigris.org/xmlns/svn/' xmlns:C='http://subversion.tigris.org/xmlns/custom/' xmlns:V='http://subversion.tigris.org/xmlns/dav/' xmlns:lp1='DAV:' xmlns:lp3='http://subversion.tigris.org/xmlns/dav/' xmlns:lp2='http://apache.org/dav/props/'>"
"<D:href>/repos/asf/!svn/bln/1295006</D:href>"
"<D:propstat>"
"<D:prop>"
"<S:eol-style>native</S:eol-style>"
"<lp1:resourcetype/>"
"<lp1:getcontentlength>6457</lp1:getcontentlength>"
"<lp1:getcontenttype>text/plain</lp1:getcontenttype>"
"<lp1:getetag>'1079591//subversion/trunk/packages/windows-WiX/BuildSubversion/LicensesCommon.wxi'</lp1:getetag>"
"<lp1:creationdate>2011-03-08T22:44:23.367404Z</lp1:creationdate>"
"<lp1:getlastmodified>Tue, 08 Mar 2011 22:44:23 GMT</lp1:getlastmodified>"
"<lp1:version-name>1079591</lp1:version-name>"
"<lp1:creator-displayname>ebswift</lp1:creator-displayname>"
"<lp3:md5-checksum>cc0fae4e1e023e707ec86e53eb342059</lp3:md5-checksum>"
"<lp3:repository-uuid>13f79535-47bb-0310-9956-ffa450edef68</lp3:repository-uuid>"
"<D:supportedlock>"
"<D:lockentry>"
"<D:lockscope><D:exclusive/></D:lockscope>"
"<D:locktype><D:write/></D:locktype>"
"</D:lockentry>"
"</D:supportedlock>"
"<D:lockdiscovery/>"
"</D:prop>"
"<D:status>HTTP/1.1 200 OK</D:status>"
"</D:propstat>"
"</D:response>"
"</D:multistatus>";

static int
compress_str(apr_pool_t *pool, serf_bucket_alloc_t *bkt_alloc,
             const char* orig, int len, const char* dict,
             serf_bucket_t **out_bkt)
{
    int zerr;
    z_stream zstr;
    apr_size_t buf_size, write_len;
    apr_pool_t *subpool;
    void *write_buf;

    *out_bkt = serf_bucket_aggregate_create(bkt_alloc);
    /* zstream must be NULL'd out. */
    memset(&zstr, 0, sizeof(z_stream));

    zerr = deflateInit(&zstr, Z_DEFAULT_COMPRESSION);

    if (dict) {
        deflateSetDictionary(&zstr, dict, strlen(dict));
    }

    /* The largest buffer we should need is 0.1% larger than the
     compressed data, + 12 bytes. This info comes from zlib.h.  */
    buf_size = len + (len / 1000) + 13;
    apr_pool_create(&subpool, pool);

    write_buf = apr_palloc(subpool, buf_size);

    zstr.next_in = (Bytef *)orig;  /* Casting away const! */
    zstr.avail_in = (uInt) len;

    zerr = Z_OK;
    while (zstr.avail_in > 0 && zerr != Z_STREAM_END)
    {
        serf_bucket_t *tmp;

        zstr.next_out = write_buf;
        zstr.avail_out = (uInt) buf_size;

        zerr = deflate(&zstr, Z_FINISH);
        if (zerr < 0)
            return -1;
        write_len = buf_size - zstr.avail_out;
        if (write_len > 0) {
            tmp = SERF_BUCKET_SIMPLE_STRING_LEN(write_buf, write_len, bkt_alloc);
            serf_bucket_aggregate_append(*out_bkt, tmp);
        }
    }
    
    apr_pool_destroy(subpool);

    return zstr.total_out;
}
int main(void)
{
    apr_pool_t *global_pool;
    apr_status_t status;
    serf_context_t *serf;
	serf_bucket_alloc_t *bkt_alloc;
    serf_bucket_t *bkt, *defl_bkt;

    int orig_len, gzip_len, svn_gzip_len, gzip_dict_len;
    
    /* Initialize the Apache portable runtime library. */
    apr_initialize();
    atexit(apr_terminate);

    apr_pool_create(&global_pool, NULL);

    serf = serf_context_create(global_pool);
    bkt_alloc = serf_bucket_allocator_create(global_pool,
                                             /* unfreed cb, not used */
                                             NULL, NULL);

    orig_len = strlen(propfind_resp);
    gzip_len = compress_str(global_pool, bkt_alloc,
                            propfind_resp, orig_len,
                            0l, /* no dictionary */
                            &defl_bkt);

    gzip_dict_len = compress_str(global_pool, bkt_alloc,
                                propfind_resp, orig_len,
                                 "<?xml version='1.0' encoding='utf-8'?><D:multistatus xmlns:D='DAV:' xmlns:ns0='DAV:'>"
                                 "<D:response xmlns:S='http://subversion.tigris.org/xmlns/svn/' xmlns:C='http://subversion.tigris.org/xmlns/custom/' xmlns:V='http://subversion.tigris.org/xmlns/dav/' xmlns:lp1='DAV:' xmlns:lp3='http://subversion.tigris.org/xmlns/dav/' xmlns:lp2='http://apache.org/dav/props/'><D:href>"
                                 "<D:propstat><D:prop><S:eol-style>"
                                 "</lp1:getcontentlength><lp1:getcontenttype>"
                                 "</lp1:getetag><lp1:creationdate></lp1:creationdate><lp1:getlastmodified>"
                                 "</lp1:version-name><lp1:creator-displayname></lp3:md5-checksum><lp3:repository-uuid>"
                                 "<D:supportedlock><D:lockentry><D:lockscope><D:locktype>"
                                 "</D:lockentry></D:supportedlock><D:lockdiscovery/></D:prop>"
                                 "</D:status></D:propstat></D:response></D:multistatus>",
                                &defl_bkt);
    printf("orig_len: %d, gzip_len: %d, gzip_dict_len: %d\n", orig_len, gzip_len,
           gzip_dict_len);
    printf("gzip_len vs orig_len: %d%%\n", (100 * gzip_len / orig_len));
    printf("gzip_dict_len vs orig_len: %d%%\n", (100 * gzip_dict_len / orig_len));
    printf("gzip_dict_len vs gzip_len: %d%%\n", (100 * gzip_dict_len / gzip_len));
    printf("gzip_dict_len vs gzip_len: %d bytes\n", gzip_dict_len - gzip_len);

    
    /* Cleanup */
    apr_pool_destroy(global_pool);

    return 0;
}
