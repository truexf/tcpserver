/*
 * os_file_io.h
 *
 *  Created on: Apr 10, 2015
 *      Author: root
 */

#ifndef HELLOLINUX_FYSLIB_OS_FILE_IO_H_
#define HELLOLINUX_FYSLIB_OS_FILE_IO_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

namespace fyslib
{

/*
 * open() //打开/创建文件
 * lseek() //定位文件指针
 * read() //读文件
 * write() //写文件
 * pread() //原子定位读
 * pwrite //原子定位写
 * close() //关闭文件
 * sync() //将所有文件脏缓存送入写队列
 * fsync() //将指定文件脏缓存写入磁盘直至完成
 * fdatasync() //将指定文件脏缓存的数据（不包括文件状态信息）写入磁盘直至返回
 * fcntl() //对打开的文件的情太进行获取或控制：阻塞/非阻塞，只读，o_append,同步/异步，缓存/非缓存。。。等等
 * ioctl() //各种io的杂项控制
 * */


} //endof namespace



#endif /* HELLOLINUX_FYSLIB_OS_FILE_IO_H_ */
