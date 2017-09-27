/*
 * xdaemon.cpp
 *
 *  Created on: Sep 24, 2015
 *      Author: root
 */
#include "xdaemon.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>


void daemonize(int flag)
{
	int i;
	//int fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rl;
	struct sigaction sa;

	/*
	* Clear file creation mask.
	*/
	if (!(flag & BD_NO_UMASK0))
		umask(0);

	/*
	* Get maximum number of file descriptors.
	*/
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		_exit(0);

	/*
	* Become a session leader to lose controlling TTY.
	*/
	if ((pid = fork()) < 0)
		_exit(0);
	else if (pid != 0) /* parent */
		_exit(0);
	setsid();

	/*
	* Ensure future opens won’t allocate controlling TTYs.
	*/
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		exit(0);
	if ((pid = fork()) < 0)
		exit(0);
	else if (pid != 0) /* parent */
		exit(0);

	/*
	* Change the current working directory to the root so
	* we won’t prevent file systems from being unmounted.
	*/
	if (!(flag & BD_NO_CHDIR))
		if (chdir("/") < 0)
			exit(0);

	/*
	* Close all open file descriptors.
	*/
	if (!(flag & BD_NO_CLOSE_FILES))
	{
		if (rl.rlim_max == RLIM_INFINITY)
			rl.rlim_max = 8192;
		for (i = 0; i < (int)rl.rlim_max; i++)
			close(i);
	}

	/*
	* Attach file descriptors 0, 1, and 2 to /dev/null.
	*/
	if (!(flag & BD_NOREOPEN_STD_FDS))
	{
		close(0);
		close(1);
		close(2);
		int fd0,fd1,fd2;
		fd0 = open("/dev/null", O_RDWR);
		if (fd0 != 0)
			fd0 = dup2(fd0,0);
		fd1 = dup2(fd0,1);
		fd2 = dup2(fd0,2);

		if (fd0 != 0 || fd1 != 1 || fd2 != 2)
			exit(1);
	}

}





