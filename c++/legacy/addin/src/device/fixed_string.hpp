#pragma once

#include <stddef.h>

namespace casio::device
{

template <size_t N>
class FixedString
{
public:
    FixedString() { clear(); }

    void clear()
    {
        len_ = 0;
        truncated_ = false;
        buf_[0] = '\0';
    }

    bool assign(const char *text)
    {
        clear();
        return append(text);
    }

    bool append(const char *text)
    {
        if(text == nullptr) return true;
        bool ok = true;
        for(size_t i = 0; text[i] != '\0'; i++) {
            if(!append_char(text[i])) ok = false;
        }
        return ok;
    }

    bool append_char(char c)
    {
        if(len_ + 1 >= sizeof(buf_)) {
            truncated_ = true;
            return false;
        }
        buf_[len_++] = c;
        buf_[len_] = '\0';
        return true;
    }

    bool append_int(int value)
    {
        char tmp[16];
        int n = 0;
        long v = value;
        if(v == 0) {
            return append_char('0');
        }
        if(v < 0) {
            append_char('-');
            v = -v;
        }
        while(v > 0 && n < (int)sizeof(tmp)) {
            tmp[n++] = char('0' + (v % 10));
            v /= 10;
        }
        bool ok = true;
        while(n > 0) {
            if(!append_char(tmp[--n])) ok = false;
        }
        return ok;
    }

    const char *c_str() const { return buf_; }
    size_t size() const { return len_; }
    bool empty() const { return len_ == 0; }
    bool truncated() const { return truncated_; }

private:
    char buf_[N + 1];
    size_t len_;
    bool truncated_;
};

inline int cstr_len(const char *text)
{
    if(text == nullptr) return 0;
    int n = 0;
    while(text[n] != '\0') n++;
    return n;
}

inline bool cstr_eq(const char *a, const char *b)
{
    if(a == nullptr || b == nullptr) return a == b;
    int i = 0;
    while(a[i] != '\0' || b[i] != '\0') {
        if(a[i] != b[i]) return false;
        i++;
    }
    return true;
}

inline bool starts_with(const char *text, const char *prefix)
{
    if(text == nullptr || prefix == nullptr) return false;
    for(int i = 0; prefix[i] != '\0'; i++) {
        if(text[i] != prefix[i]) return false;
    }
    return true;
}

inline char lower_ascii(char c)
{
    if(c >= 'A' && c <= 'Z') return char(c - 'A' + 'a');
    return c;
}

inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

inline bool is_alpha(char c)
{
    c = lower_ascii(c);
    return c >= 'a' && c <= 'z';
}

inline void copy_cstr(char *dst, int cap, const char *src)
{
    if(dst == nullptr || cap <= 0) return;
    int i = 0;
    if(src != nullptr) {
        while(i + 1 < cap && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

} // namespace casio::device
