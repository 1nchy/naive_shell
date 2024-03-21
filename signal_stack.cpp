#include "signal_stack.hpp"

namespace asp {

bool signal_stack::empty(unsigned _sig) const {
    if (_data.count(_sig) == 0 || _data.at(_sig).size() <= 1) {
        return true;
    }
    return false;
}
bool signal_stack::build(unsigned _sig, handler_t _h) {
    return build(_sig, 0, _h);
}
bool signal_stack::build(unsigned _sig, int _flag, handler_t _h) {
    struct sigaction _nact;
    _nact.sa_flags = _flag;
    sigemptyset(&_nact.sa_mask);
    _nact.sa_handler = _h;
    return build(_sig, _nact);
}
bool signal_stack::build(unsigned _sig, int _flag, handler_t _h, std::initializer_list<int> _il) {
    struct sigaction _nact;
    _nact.sa_flags = _flag;
    sigemptyset(&_nact.sa_mask);
    for (const auto& _i : _il) {
        sigaddset(&_nact.sa_mask, _i);
    }
    _nact.sa_handler = _h;
    return build(_sig, _nact);
}
bool signal_stack::build(unsigned _sig, struct sigaction _nact) {
    struct sigaction _oact;
    const int _r = sigaction(_sig, &_nact, &_oact);
    if (_r == -1) return false;
    if (_data[_sig].empty()) {
        // _data[_sig] = {_oact};
        _data[_sig].push_back(_oact);
    }
    _data[_sig].push_back(_nact);
    return true;
}
bool signal_stack::restore(unsigned _sig) {
    if (_data.count(_sig) == 0) return true;
    if (_data[_sig].size() <= 1) {
        _data[_sig].clear();
        _data.erase(_sig);
        return true;
    }
    auto& _act = _data[_sig].at(_data[_sig].size() - 2);
    const int _r = sigaction(_sig, &_act, nullptr);
    if (_r == -1) return false;
    _data[_sig].pop_back();
    return true;
}
bool signal_stack::reset(unsigned _sig) {
    if (_data.count(_sig) == 0 || _data[_sig].size() <= 1) {
        return true;
    }
    const int _r = sigaction(_sig, &_data[_sig].front(), nullptr);
    if (_r == -1) return false;
    _data[_sig].push_back(_data[_sig].front());
    return true;
}
bool signal_stack::clear(unsigned _sig) {
    if (_data.count(_sig) == 0) return true;
    if (_data[_sig].size() <= 1) {
        _data.erase(_sig);
        return true;
    }
    const int _r = sigaction(_sig, &_data[_sig].front(), nullptr);
    if (_r == -1) return false;
    _data[_sig].clear();
    _data.erase(_sig);
    return true;
}
};