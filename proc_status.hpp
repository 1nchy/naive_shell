#ifndef _ASP_PROC_STATUS_HPP_
#define _ASP_PROC_STATUS_HPP_

#include <string>
#include <vector>

#include <fstream>

namespace asp {
namespace proc {
std::vector<std::string> get_status(pid_t _pid, const std::vector<std::string>& _vs) {
    std::string _proc_path = "/proc/" + std::to_string(_pid) + "/status";
    FILE* _fp = fopen(_proc_path.c_str(), "r");
    if (_fp == nullptr) return {};
    std::ifstream _proc_file;
    _proc_file.open(_proc_path);
    size_t _cnt = 0;
    std::vector<std::string> _status_vec(_vs.size());
    while (!_proc_file.eof()) {
        std::string _line;
        std::getline(_proc_file, _line);
        for (size_t _i = 0; _i != _vs.size(); ++_i) {
            if (_line.starts_with(_vs[_i])) {
                size_t _b = _vs[_i].size();
                while (_b < _line.size() && (_line[_b] == ':' || ::isspace(_line[_b]))) { ++_b; }
                _status_vec[_i] = _line.substr(_b);
                // _proc_file >> _status_vec[_i];
                ++_cnt;
            }
        }
        if (_cnt == _vs.size()) break;
    }
    return _status_vec;
}
}
}

#endif // _ASP_PROC_STATUS_HPP_