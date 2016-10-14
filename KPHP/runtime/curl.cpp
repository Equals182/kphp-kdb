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

    Copyright 2012-2013 Vkontakte Ltd
              2012-2013 Arseny Smirnov
              2012-2013 Aliaksei Levin
                   2015 Alexander Rizaev
*/

#include <curl/curl.h>
#include <curl/multi.h>
#include <typeinfo>
#include <stdlib.h>
#include <map>

#include "curl.h"
#include "interface.h"
#include "string_functions.h"
#include "array.h"
#include "array_functions.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

OrFalse<string> f$requests(const string &url, const string &post, const array<string>& headers, const array <var, var> &extras) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();

    if (curl) {
        const char* c;
        string str;
        struct curl_slist* _headers = NULL;
        char errbuf[CURL_ERROR_SIZE];

        for (array<string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
            str = it.get_value();
            c = str.c_str();
            _headers = curl_slist_append(_headers, c);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, _headers);
        
        for (array <var, var>::const_iterator it = extras.begin(); it != extras.end(); ++it) {
            const char* key = it.get_key().to_string().c_str();
            const var &value = it.get_value();

            if (strcmp((char*)("CURLOPT_SSLCERT"), key) == 0) {
                curl_easy_setopt(curl, CURLOPT_SSLCERT, value.to_string().c_str());
            }
            if (strcmp((char*)("CURLOPT_SSLKEY"), key) == 0) {
                curl_easy_setopt(curl, CURLOPT_SSLKEY, value.to_string().c_str());
            }
            if (strcmp((char*)("CURLOPT_KEYPASSWD"), key) == 0) {
                curl_easy_setopt(curl, CURLOPT_KEYPASSWD, value.to_string().c_str());
            }
            if (strcmp((char*)("CURLOPT_STDERR"), key) == 0) {
                FILE *filep = fopen(value.to_string().c_str(), "wb");
                curl_easy_setopt(curl, CURLOPT_STDERR, filep);
            }
            if (strcmp((char*)("CURLOPT_VERBOSE"), key) == 0) {
                curl_easy_setopt(curl, CURLOPT_VERBOSE, value.to_int());
            }
            if (strcmp((char*)("CURLOPT_SSL_VERIFYPEER"), key) == 0) {
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, value.to_int());
            }
            if (strcmp((char*)("CURLOPT_SSL_VERIFYHOST"), key) == 0) {
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, value.to_int());
            }
            
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);

        if (strlen(post.c_str()) > 0) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
        }

        errbuf[0] = 0;
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            size_t len = strlen(errbuf);
            fprintf(stderr, "\nlibcurl: (%d) ", res);
            if(len)
                fprintf(stderr, "%s%s", errbuf,
                    ((errbuf[len - 1] != '\n') ? "\n" : ""));
            else
                fprintf(stderr, "%s\n", curl_easy_strerror(res));
        }

        static_SB.clean();

        int len = strlen(readBuffer.c_str());
        static_SB.reserve(len + 1);
        static_SB.append_unsafe(readBuffer.c_str(), len);

        curl_easy_cleanup(curl);
    } else {
        php_warning ("requests() error");
        return 228;
    }

    return static_SB.str();
}

extern std::map<char*, std::string> mcurl_results;
std::map<char*, std::string> mcurl_results;

static size_t multi_cb(char *d, size_t n, size_t l, void *p) {
    /* take care of the data here, ignored in this example */
    size_t realsize = n * l;
    ((std::string*)p)->append(d, realsize);
    return realsize;
}

static void init_one(CURLM *cm, var parameters) {
    const array <var,var> &p = parameters.to_array();
    if (p.isset(string("url", 3))) {
        CURL *eh = curl_easy_init();
        const string &url_string = p.get_value(string("url", 3)).to_string();
        const char *url = url_string.c_str();
        
        curl_easy_setopt(eh, CURLOPT_URL, url);
        
        if (p.isset(string("headers", 7))) {
            struct curl_slist* _headers = NULL;
            const array <var, var> &headers = p.get_value(string("headers", 7)).to_array();
            for (array <var, var>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
                _headers = curl_slist_append(_headers, it.get_value().to_string().c_str());
            }
            curl_easy_setopt(eh, CURLOPT_HTTPHEADER, _headers);
        }
        if (p.isset(string("private", 7))) {
            if (p.get_value(string("private", 7)).is_bool() == true && p.get_value(string("private", 7)).to_bool() == true) {
                curl_easy_setopt(eh, CURLOPT_PRIVATE, url);
            } else if (p.get_value(string("private", 7)).is_string() == true) {
                curl_easy_setopt(eh, CURLOPT_PRIVATE, p.get_value(string("private", 7)).to_string().c_str());
            }
        }
        if (p.isset(string("data", 4))) {
            curl_easy_setopt(eh, CURLOPT_POST, 1);
            curl_easy_setopt(eh, CURLOPT_POSTFIELDS, p.get_value(string("data", 4)).to_string().c_str());
        }
        if (p.isset(string("timeout", 7))) {
            curl_easy_setopt(eh, CURLOPT_TIMEOUT, p.get_value(string("timeout", 7)).to_int());
        }
        if (p.isset(string("sslcert", 7))) {
            curl_easy_setopt(eh, CURLOPT_SSLCERT, p.get_value(string("sslcert", 7)).to_string().c_str());
        }
        if (p.isset(string("sslkey", 6))) {
            curl_easy_setopt(eh, CURLOPT_SSLKEY, p.get_value(string("sslkey", 6)).to_string().c_str());
        }
        if (p.isset(string("keypasswd", 9))) {
            curl_easy_setopt(eh, CURLOPT_KEYPASSWD, p.get_value(string("keypasswd", 9)).to_string().c_str());
        }
        if (p.isset(string("stderr", 6))) {
            FILE *filep = fopen(p.get_value(string("stderr", 6)).to_string().c_str(), "wb");
            curl_easy_setopt(eh, CURLOPT_STDERR, filep);
        }
        if (p.isset(string("verbose", 7)) && p.get_value(string("verbose", 7)).is_bool() == true) {
            if (p.get_value(string("verbose", 7)).to_bool() == true) {
                curl_easy_setopt(eh, CURLOPT_VERBOSE, 1);
            } else {
                curl_easy_setopt(eh, CURLOPT_VERBOSE, 0);
            }
        }
        if (p.isset(string("ssl_verifypeer", 14)) && p.get_value(string("ssl_verifypeer", 14)).is_bool() == true) {
            if (p.get_value(string("ssl_verifypeer", 14)).to_bool() == true) {
                curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 1);
            } else {
                curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0);
            }
        }
        if (p.isset(string("ssl_verifyhost", 14)) && p.get_value(string("ssl_verifyhost", 14)).is_bool() == true) {
            if (p.get_value(string("ssl_verifyhost", 14)).to_bool() == true) {
                curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 1);
            } else {
                curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 0);
            }
        }
        char *_key = strdup(url);
        if (p.isset(string("id", 2))) {
            _key = strdup(p.get_value(string("id", 2)).to_string().c_str());
        }
        fprintf(stderr, "id: %s\n", _key);
        if(mcurl_results.count(_key) < 1) {
            mcurl_results.insert( std::pair<char*,std::string>(_key,(std::string)"") );
            curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, multi_cb);
            curl_easy_setopt(eh, CURLOPT_WRITEDATA, &mcurl_results[_key]);
        }
        
        curl_multi_add_handle(cm, eh);
    }
}

OrFalse< array<var, var> > f$multi_requests(const array <var, var> &parameters, const array <var, var> &curl_parameters) {
    CURLM *cm=NULL;
    CURL *eh=NULL;
    CURLMsg *msg=NULL;
    CURLcode return_code;
    int still_running=0, msgs_left=0;
    int http_status_code;
    const char *szUrl;
    const int max_wait_time = 30*1000;
    
    curl_global_init(CURL_GLOBAL_ALL);

    cm = curl_multi_init();
    
    for (array <var, var>::const_iterator it = curl_parameters.begin(); it != curl_parameters.end(); ++it) {
        const char* key = it.get_key().to_string().c_str();
        const var &value = it.get_value();

        if (strcmp((char*)("CURLMOPT_CHUNK_LENGTH_PENALTY_SIZE"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_CHUNK_LENGTH_PENALTY_SIZE, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_CONTENT_LENGTH_PENALTY_SIZE"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_CONTENT_LENGTH_PENALTY_SIZE, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_MAXCONNECTS"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_MAX_HOST_CONNECTIONS"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_MAX_HOST_CONNECTIONS, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_MAX_PIPELINE_LENGTH"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_MAX_PIPELINE_LENGTH, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_MAX_TOTAL_CONNECTIONS"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_MAX_TOTAL_CONNECTIONS, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_PIPELINING"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_PIPELINING, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_PIPELINING_SERVER_BL"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_PIPELINING_SERVER_BL, value.to_string().c_str());
        }
        else if (strcmp((char*)("CURLMOPT_PIPELINING_SITE_BL"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_PIPELINING_SITE_BL, value.to_string().c_str());
        }
        /*
        // NOT DONE!!!! //
        else if (strcmp((char*)("CURLMOPT_PUSHDATA"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_PUSHDATA, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_PUSHFUNCTION"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_PUSHFUNCTION, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_SOCKETDATA"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_SOCKETDATA, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_SOCKETFUNCTION"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_SOCKETFUNCTION, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_TIMERDATA"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_TIMERDATA, value.to_int());
        }
        else if (strcmp((char*)("CURLMOPT_TIMERFUNCTION"), key) == 0) {
            curl_multi_setopt(cm, CURLMOPT_TIMERFUNCTION, value.to_int());
        }
        */
    }
    
    for (array <var, var>::const_iterator it = parameters.begin(); it != parameters.end(); ++it) {
        init_one(cm, it.get_value());
    }
    
    curl_multi_perform(cm, &still_running);
    do {
        int numfds=0;
        int res = curl_multi_wait(cm, NULL, 0, max_wait_time, &numfds);
        if (res != CURLM_OK) {
            fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
            return false;
        }
        curl_multi_perform(cm, &still_running);
    } while(still_running);
    
    while ((msg = curl_multi_info_read(cm, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            eh = msg->easy_handle;

            return_code = msg->data.result;
            if (return_code!=CURLE_OK) {
                fprintf(stderr, "CURL error code: %d\n", msg->data.result);
                continue;
            }

            // Get HTTP status code
            http_status_code=0;
            szUrl = NULL;

            curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);
            curl_easy_getinfo(eh, CURLINFO_PRIVATE, &szUrl);

            if (http_status_code==200) {
                printf("200 OK for %s\n", szUrl);
            } else {
                fprintf(stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code);
            }

            curl_multi_remove_handle(cm, eh);
            curl_easy_cleanup(eh);
        }
        else {
            fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
        }
    }

    curl_multi_cleanup(cm);
    
    array <var, var> multi_response;
    for (std::map<char*, std::string>::iterator it=mcurl_results.begin(); it!=mcurl_results.end(); ++it) {
        multi_response.set_value(string(it->first, strlen(it->first)), string((it->second).c_str(), (it->second).size()));
    }
    mcurl_results.clear();
    return multi_response;
}