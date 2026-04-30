#pragma once

#include "device/fixed_string.hpp"

namespace casio::device
{

template <int MaxLines, int LineLen>
class LineBuffer
{
public:
    void clear()
    {
        count_ = 0;
        overflow_ = false;
        for(int i = 0; i < MaxLines; i++) lines_[i].clear();
    }

    bool add(const char *text)
    {
        if(count_ >= MaxLines) {
            overflow_ = true;
            return false;
        }
        bool ok = lines_[count_].assign(text);
        if(!ok) overflow_ = true;
        count_++;
        return ok;
    }

    FixedString<LineLen> &next()
    {
        if(count_ >= MaxLines) {
            overflow_ = true;
            return scratch_;
        }
        lines_[count_].clear();
        return lines_[count_++];
    }

    int count() const { return count_; }
    bool overflow() const { return overflow_; }

    const char *line(int index) const
    {
        if(index < 0 || index >= count_) return "";
        return lines_[index].c_str();
    }

private:
    FixedString<LineLen> lines_[MaxLines];
    FixedString<LineLen> scratch_;
    int count_ = 0;
    bool overflow_ = false;
};

using OutputLines = LineBuffer<32, 96>;

} // namespace casio::device
