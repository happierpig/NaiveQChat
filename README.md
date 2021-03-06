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

- **NOTE**: The FRIEND relationship is bilateral, which means $A$ can communicate with $B$ if and only if $A$ is a friend of $B$ and $B$ is a friend of $A$.
- Todo:  Add permission check to stop users from looking others' chat.
- Todo: Add concurrent lock

### Usage

> Though it is useless, it has usage.

- Install [FUSE](https://github.com/libfuse/libfuse)
- Run `make` in shell (as well as `make run` 、`make test`、`make clean`)
- Something worth noting : You cannot delete a group once. You must delete all users under that first.

### Reference

- 关于环境配置[tutorial](https://www.jianshu.com/p/040bb60aa468)

- 关于文件权限 [st_mode](https://www.runoob.com/linux/linux-file-attr-permission.html)

- 关于*FUSE*操作接口的一些解释 [Click here](https://blog.csdn.net/stayneckwind2/article/details/82876330)

- 关于lib红黑树的使用和内核宏`container_of` [Click here](https://blog.csdn.net/stayneckwind2/article/details/82867062).

# FUSE

- 虽然多了一次从内核态到用户态的切换 但仍然优化了运行效率

- 因为$VFS$虽然提供了统一的接口，但通用性带来的损失是对底层实现的不可知导致的难以适应各个$FS$的特点，导致任何$FS$都要经历相同的权限Check

  例如：$setxattr$调用了一大堆$vfs$的权限检查和并行检查的函数之后才最终调用底层的文件系统接口；但也许底层文件系统并不在意并行一致性问题。