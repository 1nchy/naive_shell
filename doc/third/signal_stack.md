# SIGNAL_STACK

## 摘要

传统的 C++ 信号操作较繁琐，涉及较多语句及变量，容易产生混淆乃至错误。我们提出的 `signal_stack` 在保留原有信号操作功能的基础上，大幅简化了信号操作所需要的步骤，并提供了基于栈的信号处理管理方式，适用于大多数信号处理的场景，能让开发者专注于主要功能的实现。

## 概述

`signal_stack` 是封装了信号处理数据的容器，它提供了一系列的方法，对特定信号（的处理函数）进行操作。它的接口主要部分如下：

~~~cpp
class signal_stack {
public:
    signal_stack() = default;
    signal_stack(const signal_stack&) = delete;
    signal_stack& operator=(const signal_stack&) = delete;
    ~signal_stack() = default;
    typedef void(*handler_t)(int);
    size_t size() const { return _data.size(); }
    // whether the signal handler is set
    bool empty(unsigned) const;
    // set signal handler
    bool build(unsigned, handler_t);
    // set signal handler with flag
    bool build(unsigned, int, handler_t);
    // set signal handler with flag and mask
    bool build(unsigned, int, handler_t, std::initializer_list<int>);
    // set signal handler
    bool build(unsigned, struct sigaction);
    // restore previous signal handler
    bool restore(unsigned);
    // set initial handler
    bool reset(unsigned);
    // clear all signal handler in stack
    bool clear(unsigned);
}
~~~

### 构造函数

`signal_stack` 仅使用默认构造函数。注意到，它的拷贝构造函数与赋值构造函数都被定义为删除，其背后的原因是显而易见的。一般来说，倘若对同一个进程，存在两个或多个不同的 `signal_stack` 对其信号处理进行管理，很容易引发错误，特别是当两个或多个 `signal_stack` 被**交替使用**时。因此，我们建议通过**全局变量**的形式来使用该容器，如果我们不得不使用多个 `signal_stack` 管理信号时，请一定按栈的顺序来创建、销毁与操作。

以我们的 `asp_shell` 项目为例，`backend.cpp` 中使用了其他文件不可见的 `signal_stack` 全局变量管理信号；而 `frontend.cpp` 中同样使用了其他文件不可见的 `signal_stack` 全局变量管理信号。注意到，在 `frontend.cpp` 中，`signal_stack` 全局变量是以栈的顺序被操作的：忽略 `SIGCHLD` 信号前，`_signal_stack` 为空，在 `_signal_stack` 重置 `SIGCHLD` 信号前，项目的单线程模型保证了没有其他的 `signal_stack` 对信号处理有更新。

~~~cpp
void shell_frontend::_M_cooked() {
    _signal_stack.build(SIGCHLD, nullptr);
    system("stty cooked");
    _signal_stack.restore(SIGCHLD);
}
void shell_frontend::_M_raw() {
    _signal_stack.build(SIGCHLD, nullptr);
    system("stty raw");
    _signal_stack.restore(SIGCHLD);
}
~~~

### 成员函数

|函数名|操作|
|:-:|:-:|
|empty|判断该信号是否被设置了非默认的处理函数|
|build|为该信号设置处理函数<br/>（可以设置 flag、mask 等信息）|
|restore|恢复到上一个信号处理函数|
|reset|将该信号覆盖为默认处理函数<br/>（如果该信号没有被设置任何非默认的处理函数，则无操作）|
|clear|将该信号设置为默认处理函数，并清空历史记录<br/>（如果该信号没有被设置任何非默认的处理函数，则无操作）|

## 实现

为了实现**恢复默认/初始处理函数**功能，我们在第一次设置某信号处理函数时，有必要保存 `sigaction` 第三个参数的结果——即默认处理数据。

这里需要注意到信号处理数据管理的一个细节：当某个信号对应堆栈仅有一个数据时，该数据一定是默认的信号处理函数。