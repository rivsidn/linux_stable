#ifndef _UAPI_LINUX_STAT_H
#define _UAPI_LINUX_STAT_H


#if defined(__KERNEL__) || !defined(__GLIBC__) || (__GLIBC__ < 2)

/*
 * umode_t 是一个unsigned short 数，总共16bits.
 */
#define S_IFMT  00170000	//8进制数，总共16bits.
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
/*
 * 特殊权限位.
 *
 * S_ISUID (Set-user-ID 位)，设置之后，执行该文件时会将effective user id 设置
 * 为文件所有者的user id.
 *
 * S_ISGID (Set-group-ID 位)，设置之后，执行该文件时会将effective group id 设置
 * 为文件所有者的group id.
 *
 * S_ISVTX (Sticky 位)，目录设置此位后，只有文件所有者、目录所有者或
 * root 才能删除或重命名目录中的文件.
 */
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

/*
 * S_I 是前缀.
 *
 * S_IRUSR	usr 读权限
 * S_IWUSR	usr 写权限
 * S_IXUSR	usr 执行权限
 * S_IRWXU	usr 读写执行权限
 */
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#endif


#endif /* _UAPI_LINUX_STAT_H */
