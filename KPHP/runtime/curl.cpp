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

#include "curl.h"
#include "interface.h"
#include "string_functions.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

OrFalse<string> f$requests(const string &url, const string &post, const array<string>& headers) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();

    if (curl) {
        const char* c;
        string str;
        struct curl_slist* _headers = NULL;

         for (array<string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
            str = it.get_value();
            c = str.c_str();
            _headers = curl_slist_append(_headers, c);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, _headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);

        if (strlen(post.c_str()) > 0) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
        }

        res = curl_easy_perform(curl);

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
