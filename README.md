# QChat

> CS2952-1 OS Class DIY Homework

A tiny communication toy based on *FUSE* (Filesystem in Userspace), offering information exchange like:  Under the mounted root directory, `echo "Hello world" > /bot1/bot2`, we will see *Hello world!* both on `/bot1/bot2` and `/bot2/bot1`.

### Feature

- Instruction Supported: cd ls touch cat echo mkdir rmdir rm 

- Support **grouping** (multi-level directory) like : 

```
├─ QChat
├─ QChatRoom
│  ├─ Alice
│  │  ├─ Cleo
│  │  └─ Friends
│  │     └─ Bob
│  ├─ Bob
│  │  ├─ Baby
│  │  │  └─ Cleo
│  │  └─ Sons
│  │     └─ Alice
│  ├─ Cleo
│  │  ├─ Alice
│  │  └─ Bob
│  └─ log
```

- Friendly chatting interface : 

```shell
$ echo "Why are you so sad, my son?" >> happypig/Sons/sadpiggy 
$ echo "Even if the pain is more fun togther." >> sadpiggy/Fathers/happypig 
$ echo "I'm made of L O V E." >> happypig/Sons/sadpiggy 
$ echo "Can you just not see the truth, my papa?" >> sadpiggy/Fathers/happypig 
$ cat happypig/Sons/sadpiggy 
Welcome to QChat!
[happypig]
Why are you so sad, my son?
[sadpiggy]
Even if the pain is more fun togther.
[happypig]
I'm made of L O V E.
[sadpiggy]
Can you just not see the truth, my papa?
```

- todo:  Add permission check to stop users from looking others' chat.

### Usage

> Though it is useless, it has usage.

- Install [FUSE](https://github.com/libfuse/libfuse)
- Run `make` in shell (as well as `make run` 、`make test`、`make clean`)
- Something worth noting : You cannot delete a group once. You must delete all users under that first.

### Reference

- 关于文件权限 [st_mode](https://www.runoob.com/linux/linux-file-attr-permission.html)

- 关于*FUSE*操作接口的一些解释 [Click here](https://blog.csdn.net/stayneckwind2/article/details/82876330)

- 关于lib红黑树的使用和内核宏`container_of` [Click here](https://blog.csdn.net/stayneckwind2/article/details/82867062).