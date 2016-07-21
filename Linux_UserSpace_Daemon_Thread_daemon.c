/*  

查看tmp.txt文件内容 cat /root/tmp.txt

*/



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

void my_daemon() {
    int i, pid, fd;

    // 1.转变为后台进程
    if ((pid = fork()) == -1) exit(1);
    if (pid != 0) exit(0); // 父进程（前台进程）退出

    // 2.离开原先的进程组，会话
    if (setsid() == -1) exit(1); // 开启一个新会话

    // 3.禁止再次打开控制终端
    if ((pid = fork()) == -1) exit(1);
    if (pid != 0) exit(0); // 父进程（会话领头进程）退出

    // 4.关闭打开的文件描述符，避免浪费系统资源
    for (i = 0; i < NOFILE; i++)
        close(i);

    // 5.改变当前的工作目录，避免卸载不了文件系统
    if (chdir("/") == -1) exit(1);

    // 6.重设文件掩码，防止某些属性被父进程屏蔽
    if (umask(0) == -1) exit(1);

    // 7.重定向标准输入，输出，错误流，因为守护进程没有控制终端
    if ((fd = open("/dev/null", O_RDWR)) == -1) exit(1); // 打开一个指向/dev/null的文件描述符
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    // 8.本守护进程的子进程若不需要返回信息，那么交给init进程回收，避免产生僵尸进程
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) exit(1);
}

#define INTERVAL 2

int main(int argc, char *argv[]) {
    my_daemon(); // 首先使之成为守护进程

    int t = 0;
    FILE *fp = fopen("/root/tmp.txt", "a");
    fprintf(fp, "ppid = %d, pid = %d, sid = %d, pgrp = %d\n", getppid(), getpid(), getsid(0), getpgrp());
    fflush(fp);

    do { // 测试此后台进程，每INTERVAL秒打印当前时间t，30秒后退出此后台进程
        fprintf(fp, "%d\n", t);
        fflush(fp); // 输出缓冲区内容到文件中
        sleep(INTERVAL);
        t += INTERVAL;
    } while(t < 30);

    fclose(fp);

    return 0;
}


