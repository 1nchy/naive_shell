# FRONTEND

## 摘要

shell 前端主要负责命令字符串的读入，终端界面的操作与展示等，继承了为外部提供接口的纯虚类。

## 简述

### 需求分析与设计

从命令执行的流程来看，shell 前端有两个主要功能：字符读入，特殊字符处理。

前端需要专门负责的效果展示包括：命令补全/建议功能，历史记录，字符串编辑。这三部分都可以归类于特殊字符处理。

前端需要负责命令读写与 IO，同时也兼顾着极少量的信号管理工作，最重的是，它需要与后端单例进行交互。这些都决定了前端必须同样以单例的形式运行，因此 `frontend_interface` 的拷贝构造函数与赋值构造函数被定义为删除。

### 成员函数

|函数名|操作|
|:-:|:-:|
|show_information|展示自定义的信息|
|show_prompt|显示输入提示|
|run|循环等待输入并执行|

## 实现

### 字符读入

~~~cpp
enum struct extended_char : short { /*omitted*/ };
short _M_read_character();
using character_handler = void(shell_frontend::*)(short);
void home_handler(short);
const std::unordered_map<short, character_handler> _char_handler_map = {
    {(short)extended_char::ec_home, &shell_frontend::home_handler},
};
~~~

`shell_frontend::_M_read_character` 负责读入一个字符。这里的一个字符，可能是某些需要特殊处理的字符，例如方向键等。这些输入需要交给程序来处理，而不是终端自行处理，因此在读入前后需要分别调用 `system raw` 和 `system cooked`。`system` 函数执行时，会触发一个 SIGCHLD 信号，这是应该被忽略的，需要特殊处理。

存在一些特殊字符/键，一次按键可能需要若干次 `read` 才能读取完输入的数据。这里以“上方向键”为例：`^[[A` 是上方向键在屏幕上的回显，需要三次 `read` 才能将其从输入缓冲区完全读出。但需要注意的是，第二、三次读入需要非阻塞 `read`，这样才能将这些特殊字符与 `esc` 键区分开。为了减少覆盖屏幕上特殊字符的额外工作（光标回退、覆盖空白、光标回退、覆写 `_back`，光标回退），我们关闭了终端的回显功能。

> ~~这里其实处理得不太满意，但暂时也没有其他更好的办法，就先这样叭~~

读取字符后，在 `_char_handler_map` 中调用对应的处理函数。

### 效果展示

所有需要展示的效果中，命令补全/建议稍难，这里只阐述该功能。

~~~cpp
bool has_tab_next(); // 是否已对当前命令进行补全
bool has_tab_list(); // 是否已对当前命令进行建议
void switch_tab_list(); // 切换建议列表
size_t front_signature() const; // 生成命令签名
~~~

>                      (press tab)
> 
>                           ↓
>
>                    `has_tab_next()`
>
>                 0↓                  ↓1
>
>  `build_tab_next().empty()` -1→ `has_tab_list()`
>
>        0↓                        0↓          ↓1
>
> (write _tab_next)  `build_tab_list()`  →  `switch_tab_list()`

上面是 tab 功能的流程图，由于后端的补全/建议功能较复杂，耗时较多，因此前端采用了签名的方法，尽量减少后端功能的调用。
