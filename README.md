# A NAIVE SHELL

## 简介

一个稚嫩的 shell 实现，前端实现了终端界面的字符串编辑，后端实现了命令的解析与执行，任务的调度，历史记录管理等功能。支持管道链接、前后台任务调度、tab 补全/建议等功能。

## 概述

### 构建与使用

~~~shell
$ git clone https://github.com/1nchy/asp_shell.git
$ cd asp_shell
$ make
$ ./shell
~~~

`make` 编译过程使用了 c++20 及以上标准。

### shell 提示信息

~~~shell
[directory (0)]>
~~~

方括号内第一个单词 directory 是当前工作目录名，圆括号内是后台任务数量，`>` 是输入提示符。使用者可以修改 `shell_backend::build_information` 函数的实现，以自定义显示信息；修改 `frontend_backend::_prompt`，以自定义输入提示符。

### shell 功能展示

下面是主要的命令执行与任务调度功能演示：

~~~shell
[directory (0)]> cat&
[1](22719)      cat
[directory (1)]> cat | cat | cat |
> grep a
^Z(22885)       grep stopped
[directory (2)]> jobs
[1](22719)      cat     T (stopped)
[2](22885)      grep    T (stopped)
[directory (2)]> kill 1
[directory (1)]> fg 2
abandon
abandon
1nchy
[directory (0)]> 
~~~

补全功能与建议功能的展示以下面的代码配合文字的形式来说明：

> | 模拟终端屏幕上的光标

~~~txt
[directory (0)]> echo $H|           # 键入 tab，触发命令补全
[directory (0)]> echo $HO|          # 键入 tab，触发命令建议
[directory (0)]> echo $HO|ME        # 键入 tab，展示下一个命令建议
[directory (0)]> echo $HO|STTYPE    # 键入 del，取消该建议
[directory (0)]> echo $HO|          # 键入 tab，展示下一个命令建议
[directory (0)]> echo $HO|ME        # 键入 space，采纳该命令建议
[directory (0)]> echo $HOME|        # 完成
~~~

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

shell 在实现中被划分为前后端两部分，关于[前端](doc/frontend.md)与[后端](doc/backend.md)的实现细节，可以在文档中翻阅。

我把较独立的功能都放到了 `third` 文件夹内，以静态链接库的形式提供使用。新增独立功能时，需要在项目根目录下 `makefile` 的 `THIRD_LIB_NAME` 变量后追加独立功能文件夹名（同时也是预期生成的静态链接库的名字）。文件夹内，除了独立功能的源代码，同样需要定义一个 makefile，用以编译生成静态链接库，并放置在 `third/lib` 目录下。

> 以添加 `signal_stack` 功能为例。
> 
> 首先在 `third` 目录下创建 `signal_stack` 目录，源文件放入其中；
> 
> 然后修改项目根目录下的 `makefile` 文件，在 `THIRD_LIB_NAME` 变量后追加 `signal_stack`
> 
> 其次，在 `signal_stack` 文件夹内创建 `makefile` 文件（可以参考 `third` 目录下其他功能的 `makefile` 实现），需要注意输入的静态链接库应该放入 `../lib` 目录，静态链接库的名字应该是 `lib{name}.a`。
> 
> 最后，项目中只需要以 `#include "signal_stack/signal_stack.hpp"` 的方式引用头文件即可。

独立功能中的[信号栈](doc/third/signal_stack.md)与[字典树](doc/third/trie_tree.md)的设计与实现稍复杂，细节详见文档。
