#pragma once

// CasioSDK compatibility header - provides minimal C++ stdlib for PrizmSDK
// Use this header instead of standard C++ headers when targetting Prizm

#if defined(TARGET_PRIZM)

// PrizmSDK: minimal compat
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

// Minimal C++ type stubs
namespace std {

// Import C types/functions into std namespace
using ::size_t;
using ::tolower;
using ::isalpha;
using ::isdigit;
using ::sqrt;
using ::exp;
using ::snprintf;

// string_view - used extensively
class string_view {
    const char* ptr_ = nullptr;
    std::size_t len_ = 0;
public:
    string_view() = default;
    string_view(const char* s) : ptr_(s), len_(s ? strlen(s) : 0) {}
    string_view(const char* s, std::size_t n) : ptr_(s), len_(n) {}
    const char* data() const { return ptr_; }
    std::size_t size() const { return len_; }
    const char* begin() const { return ptr_; }
    const char* end() const { return ptr_ + len_; }
    char operator[](std::size_t i) const { return ptr_[i]; }
    bool empty() const { return len_ == 0; }
    int compare(string_view other) const {
        std::size_t minlen = len_ < other.len_ ? len_ : other.len_;
        int r = strncmp(ptr_, other.ptr_, minlen);
        if(r != 0) return r;
        if(len_ < other.len_) return -1;
        if(len_ > other.len_) return 1;
        return 0;
    }
};

inline bool operator==(string_view a, string_view b) { return a.compare(b) == 0; }
inline bool operator!=(string_view a, string_view b) { return a.compare(b) != 0; }

// string - minimal wrapper around char[]
class string {
    char buf_[256];
    std::size_t len_ = 0;
    static constexpr std::size_t capacity_ = 255;
public:
    string() { buf_[0] = 0; }
    string(const char* s) { strncpy(buf_, s, capacity_); len_ = strlen(s); if(len_ > capacity_) len_ = capacity_; }
    string(const char* s, std::size_t n) { len_ = n < capacity_ ? n : capacity_; strncpy(buf_, s, len_); buf_[len_] = 0; }
    const char* c_str() const { return buf_; }
    const char* data() const { return buf_; }
    std::size_t size() const { return len_; }
    std::size_t length() const { return len_; }
    bool empty() const { return len_ == 0; }
    char operator[](std::size_t i) const { return buf_[i]; }
    char& operator[](std::size_t i) { return buf_[i]; }
    void clear() { buf_[0] = 0; len_ = 0; }
    void resize(std::size_t n) { if(n < capacity_) len_ = n; buf_[len_] = 0; }
    int compare(const string& other) const { return strcmp(buf_, other.buf_); }
    int compare(const char* other) const { return strcmp(buf_, other); }
};

// optional - minimal implementation
template<typename T>
struct optional {
    T val;
    bool has = false;
    optional() {}
    optional(T v) : val(v), has(true) {}
    explicit operator bool() const { return has; }
    T& operator*() { return val; }
    const T& operator*() const { return val; }
};

// unique_ptr - minimal stub
template<typename T>
class unique_ptr {
    T* p = nullptr;
public:
    unique_ptr() {}
    unique_ptr(T* ptr) : p(ptr) {}
    ~unique_ptr() { if(p) delete p; }
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
    T* get() const { return p; }
    T& operator*() const { return *p; }
    T* operator->() const { return p; }
    void reset(T* ptr) { if(p) delete p; p = ptr; }
    T* release() { T* r = p; p = nullptr; return r; }
};

// vector - minimal stub (not usable, just for type compatibility)
template<typename T>
class vector {
    T arr_[16];
    std::size_t sz_ = 0;
public:
    vector() {}
    vector(std::size_t n) { sz_ = n < 16 ? n : 16; }
    T& operator[](std::size_t i) { return arr_[i < 16 ? i : 0]; }
    const T& operator[](std::size_t i) const { return arr_[i < 16 ? i : 0]; }
    std::size_t size() const { return sz_; }
    void push_back(const T& v) { if(sz_ < 16) arr_[sz_++] = v; }
    void clear() { sz_ = 0; }
    T* begin() { return arr_; }
    T* end() { return arr_ + sz_; }
};

// map - minimal stub
template<typename K, typename V>
class map {
public:
    using value_type = std::pair<K, V>;
    map() {}
    V& operator[](const K& k) { static V nullv; return nullv; }
    std::size_t size() const { return 0; }
    bool empty() const { return true; }
};

// unordered_map - minimal stub
template<typename K, typename V>
class unordered_map {
public:
    using value_type = std::pair<K, V>;
    unordered_map() {}
    V& operator[](const K& k) { static V nullv; return nullv; }
    std::size_t size() const { return 0; }
    bool empty() const { return true; }
};

// deque - minimal stub
template<typename T>
class deque {
    T arr_[16];
    std::size_t sz_ = 0;
public:
    deque() {}
    void push_back(const T& v) { if(sz_ < 16) arr_[sz_++] = v; }
    void push_front(const T& v) {}
    void pop_front() { if(sz_ > 0) sz_--; }
    void pop_back() { if(sz_ > 0) sz_--; }
    T& front() { return arr_[0]; }
    const T& front() const { return arr_[0]; }
    T& back() { return arr_[sz_ > 0 ? sz_ - 1 : 0]; }
    std::size_t size() const { return sz_; }
    bool empty() const { return sz_ == 0; }
    void clear() { sz_ = 0; }
    T& operator[](std::size_t i) { return arr_[i < 16 ? i : 0]; }
};

// nullptr_t
using nullptr_t = decltype(nullptr);

// Numeric limits
using size_t = decltype(sizeof(int));
using int64_t = ::int64_t;
using int32_t = ::int32_t;
using uint32_t = ::uint32_t;

} // namespace std

// Casio-specific type aliases
namespace casio {
    using int64 = int64_t;
    using int32 = int32_t;
    using uint32 = uint32_t;
}

#else

// Host build: use standard C++ library
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <climits>
#include <cctype>
#include <string>
#include <string_view>
#include <optional>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include <functional>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace casio {
    using int64 = int64_t;
    using int32 = int32_t;
    using uint32 = uint32_t;
}

#endif