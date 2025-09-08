#include "httpd_application.h"
// TODO: support UTF-8
bool json_string_encode(const char* str, char* out_str, size_t out_str_len) {
    if(out_str_len==0) return false;
    *out_str = '\0';
    if(--out_str_len==0) return false;
    while(*str) {
        char ch = *str;
        switch(ch) {
            case '\r':
                if(out_str==0) goto done;
                *out_str++='\\';
                if(--out_str_len<=1) goto done;
                *out_str++='r';
                break;
            case '\n':
                if(out_str==0) goto done;
                *out_str++='\\';
                if(--out_str_len<=1) goto done;
                *out_str++='n';
                break;
            case '\"':
                if(out_str==0) goto done;
                *out_str++='\\';
                if(--out_str_len<=1) goto done;
                *out_str++='\"';
                break;
            case '\t':
                if(out_str==0) goto done;
                *out_str++='\\';
                if(--out_str_len<=1) goto done;
                *out_str++='t';
                break;
            default:
                if(out_str==0) goto done;
                *out_str++=ch;
                if(--out_str_len<=1) goto done;
                break;
                
        }
        ++str;
    }
    *out_str = '\0';
    return true;
done:
    *out_str = '\0';
    return false;
}