#ifndef _ASP_PROC_STATUS_HPP_
#define _ASP_PROC_STATUS_HPP_

#include <string>
#include <vector>

namespace asp {
namespace proc {
std::vector<std::string> get_status(pid_t _pid, const std::vector<std::string>& _vs);
}
}

#endif // _ASP_PROC_STATUS_HPP_