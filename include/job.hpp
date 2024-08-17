#ifndef _ICY_JOB_HPP_
#define _ICY_JOB_HPP_

#include <vector>
#include <string>

#include <termios.h>

namespace icy {

struct job;

struct job {
    job(pid_t, pid_t, const std::vector<std::string>&);
    job(pid_t, pid_t, const std::vector<std::string>&, int _in = fileno(stdin), int _out = fileno(stdout), int _err = fileno(stderr));
    job(const job&) = default;
    job& operator=(const job&) = delete;
    ~job() = default;

    bool foreground() const { return _job_serial == 0; }
    bool background() const { return !foreground(); }
    pid_t pid() const { return _pid; }
    pid_t pgid() const { return _pgid; }
    void set_serial(size_t _s = 0) { _job_serial = _s; }
    size_t serial() const { return _job_serial; }
    struct termios _tmode;

private:
    const pid_t _pid; pid_t _pgid;
    size_t _job_serial = 0; // 0 for fg, others for bg
    const std::vector<std::string> _args;
    int _in; int _out; int _err;
    // int _status = 0;
    // bool _completed = false;
    // bool _stopped = false;
};

};

#endif // _ICY_JOB_HPP_