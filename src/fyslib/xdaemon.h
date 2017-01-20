/*
 * xdaemon.h
 *
 *  Created on: Sep 24, 2015
 *      Author: root
 */

#ifndef HELLOLINUX_FYSLIB_XDAEMON_H_
#define HELLOLINUX_FYSLIB_XDAEMON_H_

#define BD_NO_CHDIR				0x1
#define BD_NO_CLOSE_FILES		0x2
#define BD_NOREOPEN_STD_FDS		0x4
#define BD_NO_UMASK0			0x8

void daemonize(int flag);


#endif /* HELLOLINUX_FYSLIB_XDAEMON_H_ */
