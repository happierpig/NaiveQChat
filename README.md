# Q Baby

> FUSE : Filesystem in Userspace

## struct fuse_operations

- 结构体成员是一堆函数指针，定义了钩子函数--即内核*VFS*通过调用这些函数实现的用户态的文件系统
- 所有函数的实现都是可选的，实现特定功能想要的组合函数即可。
- *permisson check*



- [st_mode](https://www.runoob.com/linux/linux-file-attr-permission.html)

 