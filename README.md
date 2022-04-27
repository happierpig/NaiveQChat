# QChat

> CS2952-1 OS Class DIY Homework

A tiny communication toy based on *FUSE* (Filesystem in Userspace), offering information exchange like:  Under the mounted root directory, `echo "Hello world" > /bot1/bot2`, we will see *Hello world!* both on `/bot1/bot2` and `/bot2/bot1`.

### Usage

> Though it is useless, it has usage.

- 安装[FUSE](https://github.com/libfuse/libfuse)
- `gcc -Wall QChat.c pkg-config fuse3 --cflags --libs -o hello` 编译

### Reference

- 关于文件权限 [st_mode](https://www.runoob.com/linux/linux-file-attr-permission.html)

- 关于*FUSE*操作接口的一些解释 [Click here](https://blog.csdn.net/stayneckwind2/article/details/82876330)

- 关于lib红黑树的使用和内核宏`container_of` [Click here](https://blog.csdn.net/stayneckwind2/article/details/82867062).