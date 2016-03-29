/*
    This file is part of VK/KittenPHP-DB-Engine.

    VK/KittenPHP-DB-Engine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    VK/KittenPHP-DB-Engine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VK/KittenPHP-DB-Engine.  If not, see <http://www.gnu.org/licenses/>.

    This program is released under the GPL with the additional exemption
    that compiling, linking, and/or using OpenSSL is allowed.
    You are free to remove this exemption from derived works.

    Copyright 2016 Equals182
*/

#include <libcouchbase/couchbase.h>
#include <libcouchbase/n1ql.h>
#include <string.h>
#include <stdlib.h>

#include "couchbase.h"

#include "kphp_core.h"
#include "regexp.h"

extern std::string rows;
extern std::string meta;
std::string rows;
std::string meta;

static void rowCallback(lcb_t instance, int cbtype, const lcb_RESPN1QL *resp) {
    if (resp->rc != LCB_SUCCESS) {
        fprintf(stderr, "N1QL request failed!: %s\n", lcb_strerror(instance, resp->rc));
    }
    static const char *regexp = "~\\n(\\s+)?~";
    const string regexpstr = string(regexp, (dl::size_type)(strlen (regexp)));
    const string response = f$preg_replace(regexpstr, string("", 0), string(resp->row, (int)resp->nrow).to_string()).to_string();
    
    if (! (resp->rflags & LCB_RESP_F_FINAL)) {
        //row
        //rows.append(string(",", 1));
        //fprintf(stderr, "ROW: %s\n", response.c_str());
        rows.append(response.c_str());
        rows.push_back(',');
    } else {
        //meta
        //fprintf(stderr, "META: %s\n", response.c_str());
        meta.append(response.c_str());
    }
    /*
    */
}

OrFalse<string> f$couchbase(const array <var, var> &credentials, const string &n1ql) {
    
    lcb_error_t err;
    lcb_t instance;
    
    rows.clear();
    meta.clear();
    
    //fprintf(stderr, "WAT: %s\n", n1ql.c_str());

    struct lcb_create_st create_options;
    
    char * link = (char * )("couchbase://localhost:8091");
    char * bucket = (char * )("default");
    char * password = (char * )("");
    
    for (array <var, var>::const_iterator it = credentials.begin(); it != credentials.end(); ++it) {
        const char * key = it.get_key().to_string().c_str();
        const var &value = it.get_value();
        
        if (strcmp((char*)("link"), key) == 0) {
            link = strdup(value.to_string().c_str());
        }
        if (strcmp((char*)("bucket"), key) == 0) {
            bucket = strdup(value.to_string().c_str());
        }
        if (strcmp((char*)("password"), key) == 0) {
            password = strdup(value.to_string().c_str());
        }
    }
    
    char * conn = link;
    strcat(conn, (char * )"/");
    strcat(conn, (char * )strdup(bucket));
    
    memset(&create_options, 0, sizeof(create_options));
    create_options.version = 3;
    
    if (password && password[0]) {
        create_options.v.v3.passwd = password;
    }
    create_options.v.v3.connstr = conn;
    
    err = lcb_create(&instance, &create_options);
    if (err != LCB_SUCCESS) {
        fprintf(stderr, "Failed to create libcouchbase instance: %s\n", lcb_strerror(NULL, err));
        php_warning ("Failed to create libcouchbase instance");
        return false;
    }
    //Initiate the connect sequence in libcouchbase
    if ((err = lcb_connect(instance)) != LCB_SUCCESS) {
        fprintf(stderr, "Failed to initiate connect: %s\n", lcb_strerror(NULL, err));
        lcb_destroy(instance);
        php_warning ("Failed to initiate connect");
        return false;
    }
    lcb_wait3(instance, LCB_WAIT_NOCHECK);
    if ((err = lcb_get_bootstrap_status(instance)) != LCB_SUCCESS) {
        fprintf(stderr, "Couldn't establish connection to cluster: %s\n", lcb_strerror(NULL, err));
        lcb_destroy(instance);
        php_warning ("Couldn't establish connection to cluster");
        return false;
    }
    
    lcb_CMDN1QL cmd;

    const char *querystr = n1ql.to_string().c_str();
    cmd.query = querystr;
    cmd.nquery = strlen(querystr);
    cmd.callback = rowCallback;
    
    err = lcb_n1ql_query(instance, NULL, &cmd);
    if (err != LCB_SUCCESS) {
        // OOPS!
        //fprintf(stderr, "Fail: %s\n", "wat");
        php_warning ("Query wasnt successful");
        return false;
    }
    // We can release the params object now..
    lcb_wait(instance);
    lcb_destroy(instance);
    
    if ((int)strlen(rows.c_str()) > 0) {
        rows = rows.substr(0, rows.size() - 1);
    }
    
    int r_len = strlen(rows.c_str());
    int m_len = strlen(meta.c_str());
    
    int len = r_len + 19 + m_len;
    
    static_SB.clean();
    static_SB.reserve(len + 1);
    static_SB.append_unsafe((char*)"{\"rows\":[", 9);
    if ((int)strlen(rows.c_str()) > 0) {
        static_SB.append_unsafe(rows.c_str(), r_len);
    }
    static_SB.append_unsafe("],\"meta\":", 9);
    static_SB.append_unsafe(meta.c_str(), m_len);
    static_SB.append_unsafe("}", 1);
    
    rows.clear();
    meta.clear();
    /*
    */
    
    //delete[] cmd;
    //delete[] ;
    
    return static_SB.str();
    /*
    return false;
    */
}