# BACKEND

## 摘要

shell 后端主要负责命令的解析与执行，任务的调度，历史记录管理等，继承为前端提供接口的纯虚类。

## 简述

### 需求分析与设计

从命令执行的流程来看，shell 后端有三个主要功能，命令解析、命令处理、命令执行，这些功能分别对应 `parse`、`compile`、`execute` 三个函数。

前端的部分效果，需要后端支持，包括命令补全/建议，历史记录。后端的 `build_tab_next` 函数支持命令补全；后端的 `build_tab_list` 函数支持命令建议。后端的三个 `*_history` 函数用于支持历史记录的管理。

后端还需要负责进程的信号处理以及任务调度，这些都决定了后端必须以单例的形式运行，因此 `backend_interface` 的拷贝构造函数与赋值构造函数被定义为删除。

### 成员函数

|函数名|操作|
|:-:|:-:|
|parse|解析命令字符串<br/>返回值：-1错误；0完成；1需要更多输入|
|compile|处理字符串<br/>（处理特殊字符、展开环境变量等）|
|execute|执行命令|
|build_information|构造shell展示信息|
|build_tab_next|解析待补全命令，并返回补全字符串|
|build_tab_list|解析待补全命令，并返回补全建议|
|prev_history|返回上一个历史命令|
|next_history|返回下一个历史命令|
|append_history|追加命令记录|
|kill_all_process|杀死所有任务|

## 实现

这里我们特别把单独的不可拆分的可执行单元称为**指令**，把用管道符号`|`连接的若干指令称为**命令**，把只包含一条指令的命令称为单指令。容易注意到，命令是由`||`、`&&`、`;`、`&`这些命令分隔符隔开的，而指令往往是由管道符号`|`隔开。我们把由命令分隔符连接的若干命令称为**命令序列**。

### 命令解析相关

因为指令存在隔行输入的情况，所以 `shell_backend::parse` 应该在指令不完整的情况下提醒前端继续接受输入。`shell_backend::parse` 应该仅做最基本的指令字符串划分工作，并不该处理特殊字符、展开环境变量等。

`shell_backend::parse` 首先处理异常的读入状态（例如上一次读入的命令不完整），然后简单处理特殊字符（引号、空格等），之后用命令符字典树检查当前位置的输入，检查当前字符是普通字符还是命令符号，并选择对应的处理方式。

~~~cpp
struct command {
    std::vector<std::vector<std::string>> _instructions;
    std::string _redirect_in; std::string _redirect_out;
    short _redirect_out_type = 0; // 0 none, 1 create, 2 append
    bool _background = false;
    const std::vector<std::string>& main_ins() const;
    void append_word(const std::string& _s);
    void create_instruction();
};
~~~

`command` 类型被用于管理命令，其内部主要保存指令字符串集合、重定向信息等。

~~~cpp
struct command_sequence {
    std::vector<command> _parsed_command;
    std::vector<std::string> _command_relation;
    void build_word(const std::string& _s);
    void build_instruction();
    void build_command(const std::string& _relation, bool _bg = false);
};
~~~

`command_sequence` 被用于管理命令集合，定义的三种函数 `build_word`、`build_instruction`、`build_command` 分别负责构建词汇、构建指令、构建命令。

`shell_backend::compile` 负责特殊字符的进一步处理（解析反斜杠，展开引号、展开环境变量）。

### 命令执行相关

`shell_backend::execute` 负责执行命令。我们的进程管理将仿照 BonusShell 的管理方式。即 shell 主进程创建子进程，并在子进程中执行命令。

我们下面以一个管道命令作为范例，来说明我们是如何组织管道通信的。

~~~shell
$ ls | grep hpp | sort -r | cat [< out_file > in_file]
~~~

为执行该命令，除开 shell 本身所在的进程 A，我们需要创建 4 个子进程。

|进程代号|进程名|父进程|输入来源|输出方向|
|:-:|:-:|:-:|:-:|:-:|
|A|shell||stdin|stdout|
|B|cat|A|E.out|A.out/out_file|
|C|ps|B|A.in/in_file|D.in|
|D|grep|B|C.out|E.in|
|E|sort|B|D.out|B.in|

注意到 E 进程是唯一需要和主子进程 B 进行管道通信的，为了方便，我们将按 E、D、C 的顺序创建子进程。

为了让 E 和 B 构建管道通信，在 E 被 fork 之前，就需要 pipe(E,B)。<br/>
为了让 D 和 E 构建管道通信，在 D 被 fork 之前，就需要 pipe(D,E)。<br/>
为了让 C 和 D 构建管道通信，在 C 被 fork 之前，就需要 pipe(C,D)。

注意到，为了保证 B、D、E 三进程之间的通信，我们需要在创建 E 进程之前，创建两个管道，分别用于 B、E 通信以及（保留至下一个 fork 循环）D、E 通信。用于后者的管道需要在 D、E 通信建立后，在 B 进程中关闭它。

因此我们自然的会在 fork 循环外定义两个管道 `_child_in_fd[2]` 和 `_child_out_fd[2]`，以下分别简称 `in` 和 `out`。这两个管道的分别代表当前创建的子进程的输入管道与输出管道。其中下标为 0 的是读端，下标为 1 的是写端。注意到，在创建顺序中，上一个子进程的输入，来源与下一个子进程的输出。即在当前 fork 循环中的 in 管道，在下一个 fork 循环中就变成 out 管道了。因此在循环开始前需要 swap 两个管道。

|进程|时机|行为|
|:-:|:-:|:-:|
|B|before fork E|pipe(in), pipe(out)|
|E||dup2(out)，dup2(in)|
|B|after fork E|dup2(out)|
|B|before fork D|swap(in, out), pipe(in)|
|D||dup2(out), dup2(in)|
|B|after fork D|close(out), close(E.in)|

注意到第一个被主子进程创建出来的子进程 E 的输出将作为主子进程的输入，但主子进程不能一上来就执行 `dup2` 将 E 的输出管道绑定到自己的读入，因为后续创建的子进程会继承主子进程的输入输出管道，如果我们一上来就把 E 的输出与主子进程的输入绑定，在创建后续子进程时，还需要恢复回去，增加额外工作，因此我们必须延迟绑定。

对于中间创建的子进程，他们的输入输出管道都被另外两个进程拥有：对于输入管道，它还被主子进程和上一个子进程拥有；对于输出管道，它还被主子进程和下一个子进程拥有，因此对于中间创建的每个 `fd[2]`，都需要关闭四次——主子进程关闭两次。

### 命令补全/建议相关

补全主要有以下几种类型：当前路径补全，文件（路径）补全，可执行文件补全，环境变量补全。

我们首先要补全解析前端的输入字符串，判断补全类型。例如，指令的第一个词汇一般是可执行命令；`$` 后面的词汇一般是环境变量等；其余的词汇一般是文件类型。

~~~cpp
enum tab_type : short { // bitmap
    none = 0x0, file = 0x1 << 0,
    program = 0x1 << 1, env = 0x1 << 2,
    // cwd = 0x1 << 3,
};
trie_tree _program_dict, _file_dict, _cwd_dict, _env_dict;
void parse_tab(const std::string& _line);
std::string build_program_tab_next(const std::string&);
std::string build_file_tab_next(const std::string&);
std::string build_env_tab_next(const std::string&);
std::vector<std::string> build_program_tab_list(const std::string&);
std::vector<std::string> build_file_tab_list(const std::string&);
std::vector<std::string> build_env_tab_list(const std::string&);
void fetch_program_dict();
void fetch_file_dict(const std::filesystem::path&);
void fetch_env_dict();
void fetch_cwd_dict();
~~~

根据四种补全类型，我们定义了对应的四个字典树。而在枚举与函数部分，我们把文件补全和当前路径补全合并了。当前端有命令补全与建议的需要时，后端会根据命令解析结果，获取相应类型的补全与建议，并合并成一个最终结果。

### 任务调度相关

对于信号的处理，我们应该放在 asp_shell 的主函数中。我们先对 `SIGINT`，`SIGSTSTP` 信号做出处理。对于 `ctrl+d` 的信号处理，我们放在 命令字符串编辑模块 中。

我们需要显式处理的信号有三个：`SIGINT`，`SIGTSTP`，`SIGCHLD`。其中前两者是我们主动发出的信号，后面那个是我们被动处理的信号。和信号相关的内置函数有 `jobs`，`fg`，`bg`。

此外，为了维护前台与后台进程/任务，我们用 `fg` 和 `bg` 来表示前台/后台进程集合。

关于IO，shell 的读写仅应该由前台进程组执行，我们每创建一些进程以执行一个命令的时候，都应该把这些进程放入一个新的进程组中，无论该命令是前台/后台运行的。

注意到每个命令执行时，都有一个主指令，其对应的进程将作为进程组组长。父/子进程都需要同时把子进程/自己放入指定进程组内[1]。

|动作|SIGINT|SIGTSTP|fg|bg|
|:-:|:-:|:-:|:-:|:-:|
|触发信号|SIGINT|SIGTSTP|SIGCONT|SIGCONT|
|fg集|kill(SIGINT)|kill(SIGSTOP)|-|-|
|bg集|-|-|kill(SIGCONT)|kill(SIGCONT)|
|other|-|-|-|-|

|SIGCHLD action|WIFSTOPPED|WIFEXITED|
|:-:|:-:|:-:|
|fg|fg.del<br/>bg.add|fg.del|
|bg|-|fg.del<br/>bg.del|
|other|-|-|

为了完全实现 `fg`、`bg` 等功能，需要对进程进行分组管理，我们抽象出 `job` 类，用来保存进程数据，包括参数、装填、返回值、进程组信息等。

### 内建命令相关

~~~cpp
using builtin_instruction_handler = void(shell_backend::*)(const std::vector<std::string>&);
struct builtin_instruction_detail {
    builtin_instruction_handler _handler;
    size_t _min_args = 1; size_t _max_args = 1;
};
void _M_cd(const std::vector<std::string>&);
const std::unordered_map<std::string, builtin_instruction_detail> _builtin_instruction = {
    {"cd", {&shell_backend::_M_cd, 1, 2}},
};
~~~

内建命令由哈希表管理，`builtin_instruction_detail` 类型中包含了处理函数的指针、参数要求等信息。`_min_args` 表示参数最少个数；`_max_args` 表示参数最多个数，为 0 表示可以是任意多个。

## 引用

[1] [Launching Jobs](https://www.gnu.org/software/libc/manual/html_node/Launching-Jobs.html)
