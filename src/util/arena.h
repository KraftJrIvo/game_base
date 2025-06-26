#include <cstddef>
#include <vector>
#include "zpp_bits.h"

template <size_t CAP, typename T>
class Arena 
{
    friend zpp::bits::access;
	using serialize = zpp::bits::members<3>;

    std::vector<T> _data;
    std::vector<size_t> _counters;
    size_t _firstAvailableIdx;

    void _allocate() 
    {
        _data.resize(CAP);
        _counters.resize(CAP, 1);
        for (size_t i = 0; i < CAP; ++i)
                _counters[i] += CAP - i;
        if (_firstAvailableIdx >= CAP)
            _firstAvailableIdx = CAP;
    }

public:
    
    Arena() :
        _firstAvailableIdx(0)
    {
        _allocate();
    }

    T* data() {
        return _data.data();
    }

    size_t size() {
        return CAP * sizeof(T);
    }

    size_t acquire(const T& obj, size_t count = 1) {
        auto _candidx = _firstAvailableIdx; 
        while (_counters[_candidx] < count) {
            _candidx++;
        }
        bool enough = _counters[_candidx] >= count;
        if (enough) {
            auto idx = _candidx;
            _data[idx] = obj;
            for (size_t i = 0; i < count; ++i)
                _counters[idx + i] = 0;
            if (idx == _firstAvailableIdx)
                _firstAvailableIdx += count;
            while (_firstAvailableIdx < CAP && !_counters[_firstAvailableIdx])
                _firstAvailableIdx++;
            return idx;
        }
        return -1;        
    }

    T& at(size_t idx) {
        return _data[idx];
    }    

    const T& get(size_t idx) const {
        return _data[idx];
    }

    size_t count() {return _firstAvailableIdx;}
    size_t capacity() {return CAP;}

};