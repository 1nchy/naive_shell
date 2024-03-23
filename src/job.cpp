#include "job.hpp"

namespace asp {
job::job(pid_t _pid, pid_t _pgid, const std::vector<std::string>& _args)
: _pid(_pid), _pgid(_pgid), _args(_args) {}
job::job(pid_t _pid, pid_t _pgid, const std::vector<std::string>& _args, int _in, int _out, int _err)
: _pid(_pid), _pgid(_pgid), _args(_args), _in(_in), _out(_out), _err(_err) {}
}