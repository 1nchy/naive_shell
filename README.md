# ASP_SHELL

## 简介

## 基础能力
1. 基础的命令解释与执行功能
2. 基础的光标移动、历史记录功能
3. 信号处理
4. 通配符与正则表达式功能
5. 内建命令 `cd` `history` `quit` `bg` `fg` `jobs`

## 系统设计
### 初始化
asp_shell 在初始化时，包括以下部分：

1. 输入输出的重定向
2. 信号处理函数的注册
3. cwd 的初始化

### 模块
asp_shell 在实际运行的时候，可按先后顺序大致分为若干模块：
1. 命令字符串编辑模块
2. 命令解析
3. 正则解析
4. 路径解析
5. 命令执行
6. 输入输出重定向
7. 子进程管理

命令字符串编辑模块负责基础的光标移动、历史记录功能。

命令解析模块负责将输入的命令字符串解析成字符串数组，并展开部分正则表达式、通配符、相对路径等。

正则解析模块负责将命令中的正则表达式展开。

路径解析模块负责将命令中的相对路径转换为绝对路径？

命令执行模块负责新建子进程以执行命令，把字符串数组中命令参数部分传递给子程序。

当检测到可能存在输入输出重定向时（例如命令中包含<>等重定向符号时），输入输出重定向模块将重载当前命令子程序的输入输出。

当 asp_shell 接受特定信号时，需要对当前正在执行的子进程进行管理及操作，包括查询、阻塞、挂起等。

## 详细设计

### shell 与 editor 的划分
shell 类的任务分为三部分：指令读入（A）、指令解析（B）、指令执行（C）。其中一二部分几乎混为一体，因为存在我们解析命令时，发现还需要进一步读入命令的情况。每次读入的命令就是我们在屏幕上输入的一行，因此 editor 应该提供一个公有的 `line` 接口，让 shell 读取一行命令。

shell 类在指令读入的时候，需要让 editor 展示一次提示符。因此 editor 应该提供一个公有的 `show_prompt` 接口；同理，还需要提供一个 `show_information` 接口。

shell 与 editor 的输入输出交互：
1. shell A 阶段：shell.in = editor.in, shell.out = editor.out
2. shell B 阶段：shell.in = null, shell.out = editor.out （无输入需求）
3. shell C 阶段：与 A 阶段一致，除非有重定向需求

### 信号处理
对于信号的处理，我们应该放在 asp_shell 的主函数中。我们先对 `SIGINT`，`SIGSTSTP` 信号做出处理。对于 `ctrl+d` 的信号处理，我们放在 命令字符串编辑模块 中。

### 进程管理
我们的进程管理将仿照 BonusShell 的管理方式。即 shell 主进程创建子进程以执行命令。

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

因此我们自然的会在 fork 循环外定义两个管道 `_child_in_fd[2]` 和 `_child_out_fd[2]`，以下分别简称 `in` 和 `out`。这两个管道的分别代表当前创建的子进程的输入管道与输出管道。注意到，在创建顺序中，上一个子进程的输入，来源与下一个子进程的输出。即在当前 fork 循环中的 in 管道，在下一个 fork 循环中就变成 out 管道了。因此在循环开始前需要 swap 两个管道。

|进程|时机|行为|
|:-:|:-:|:-:|
|B|before fork E|pipe(in), pipe(out)|
|E||dup2(out)，dup2(in)|
|B|after fork E|dup2(out)|
|B|before fork D|swap(in, out), pipe(in)|
|D||dup2(out), dup2(in)|
|B|after fork D|close(out), close(E.in)|

### shell 类型
shell 应该保存以下的内容
> cwd 等环境变量