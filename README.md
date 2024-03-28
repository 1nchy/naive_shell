# A NAIVE SHELL

## 简介

一个稚嫩的 shell 实现，前端实现了终端界面的字符串编辑，后端实现了命令的解析与执行，任务的调度，历史记录管理等功能。支持管道链接、前后台任务调度、tab 补全/建议等功能。

## 概述

### 构建与使用

~~~shell
$ make
$ ./shell
~~~

`make` 编译过程使用了 c++20 及以上标准。

### shell 提示信息

~~~shell
[directory (0)]>
~~~

方括号内第一个单词 directory 是当前工作目录名，圆括号内是后台任务数量，`>` 是输入提示符。使用者可以修改 `shell_backend::build_information` 函数的实现，以自定义显示信息；修改 `frontend_backend::_prompt`，以自定义输入提示符。

## 项目结构

~~~txt
asp_shell
├── .gitignore
├── makefile
├── LICENSE.md
├── README.md
├── doc                         # 项目文档
│   ├── backend.md              # 后端实现文档
│   ├── frontend.md             # 前端实现文档
│   └── third                   # 独立功能实现文档
│       ├── signal_stack.md
│       └── trie_tree.md
├── include                     # 头文件
│   └── ...
├── src                         # 源文件
│   └── ...
└── third                       # 独立功能
    ├── lib                     # 独立功能生成的静态链接库
    ├── file_system             # 文件操作
    ├── output                  # 日志输出
    ├── proc_status             # 进程状态获取
    ├── signal_stack            # 信号栈
    └── trie_tree               # 字典树
~~~

