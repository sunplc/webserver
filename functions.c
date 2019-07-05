#include <stdio.h> // perror
#include <stdlib.h> // atoi
#include <string.h>
#include <unistd.h> // close fork sleep
#include <fcntl.h>  // open O_RDWR
#include <errno.h> // EINTR
#include <sys/socket.h>
#include <netinet/in.h> // IPPROTO_TCP
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>   // struct timeval.tv_usec
#include <limits.h>         // PATH_MAX

void PerrorExit(const char *s)
{
	perror(s);
	exit(1);
}

int Socket(int port)
{
	int sockfd;

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		PerrorExit("socket error");
    }

    int optval = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
            (char *)&optval, sizeof(optval));

	struct sockaddr_in server_addr;

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &server_addr,
                sizeof(server_addr)) <  0) {
		PerrorExit("bind error");
    }

    // start listen, backlog is 20
	if (listen(sockfd, 100) < 0) {
		PerrorExit("listen error");
    }

	return sockfd;
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	int n;
	again:
	if ( (n = accept(fd, sa, salenptr)) < 0) {
		if ((errno == ECONNABORTED) || (errno == EINTR))
			goto again;
		else
			PerrorExit("accept error");
	}
	return n;
}

ssize_t Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t n;
	again:
	if ( (n = read(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
	ssize_t n;
	again:
	if ( (n = write(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

// 带缓冲区的 getchar(), static 限定本函数的作用域在本文件之内
static ssize_t Readchar(int fd, char *ptr)
{
	static int count;
	static char buf[1024];
	static char *bufp;

	if (count <= 0) {
		again:

		if ( (count = read(fd, buf, 1024)) < 0) {
			if (errno == EINTR)
				goto again;
			return -1;
		} else if (count == 0)
			return 0;

		bufp = (char *)buf;
	}

	count--;
	*ptr = *bufp++;
	return 1;
}

/**
 * 读取一行字符串，以 \n 作为换行符标志
 */
ssize_t Readline(int fd, char *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c;

	for (n = 1; n < maxlen; n++) {
		if ( (rc = Readchar(fd, &c)) == 1) {
			*vptr++ = c;
			if (c == '\n')
				break;

		} else if (rc == 0) {
			*vptr = 0;
			return n - 1;

		} else {
			*vptr = 0;
			return -1;
		}
	}

	*vptr = 0;
	return n;
}

// 关闭一个文件描述符
void Close(int fd)
{
	if (close(fd) == -1)
		PerrorExit("close error");
}

// 去除字符串末尾的指定字符
int rtrim(char *str, char c)
{
	int len = strlen(str);
	if (len < 1)
		return 0;

	char *end = str + len - 1;
	if (*end == c) {
		*end = '\0';
		return 1;
	}

	return 0;
}

// print current date time milliseconds
void PrintTime()
{
    time_t timer;
    time(&timer); // 获取当前时间; 等同于 timer = time(NULL);

    struct tm *p = localtime(&timer);

    struct timeval te;  // 获取微秒 μs
    gettimeofday(&te, NULL); // get current time

    printf("[%d-%02d-%02d %02d:%02d:%02d.%03ld]\n",
            (1900+p->tm_year), (1+p->tm_mon), p->tm_mday,
            p->tm_hour, p->tm_min, p->tm_sec, te.tv_usec / 1000);
}

// 写日志到日志文件
void Log(FILE *fp, char *fmt, ...)
{
    time_t timer;
    time(&timer); // 获取当前时间; 等同于 timer = time(NULL);

    struct tm *p = localtime(&timer);

    struct timeval te;  // 获取微秒 μs
    gettimeofday(&te, NULL); // get current time

    va_list args;
    va_start(args, fmt);

    fprintf(fp, "[%d-%02d-%02d %02d:%02d:%02d.%06ld] ",
            (1900+p->tm_year), (1+p->tm_mon), p->tm_mday,
            p->tm_hour, p->tm_min, p->tm_sec, te.tv_usec);

    vfprintf(fp, fmt, args);
    fprintf(fp, "\n");

    va_end(args);
}

// 将当前进程转变为守护进程
int Daemonize(int errfd)
{
    // fork child process insure current process isn't group leader
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork()");
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }

    // create new Session, let current process become 
    // new session leader and process group leader
    pid = setsid();

    // change work dir to /
    if (chdir("/") < 0) {
        perror("chdir()");
        exit(1);
    }

    close(0);
    open("/dev/null", O_RDWR);
    dup2(0, 1);
    dup2(errfd, 2);

    return (int)pid;
}

// concatenate string s1 and s2, storage result to dest
void Concat(char *dest, char *s1, char *s2)
{
    bzero(dest, 1);
    strcpy(dest, s1);
    strcat(dest, s2);
}

void WritePidFile(const char *pidfile, int pid)
{
    // open file, read only, create if not exists, truncate old content
    int pidfilefd = open(pidfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (pidfilefd < 0) {
        perror("open pid file");
        exit(1);
    }

    char pidstr[8];
    sprintf(pidstr, "%d\n", pid);
    write(pidfilefd, pidstr, strlen(pidstr));
    close(pidfilefd);
}

int ReadPidFile(const char *pidfile)
{
    // open file, read only, create if not exists, truncate old content
    int pidfilefd = open(pidfile, O_RDONLY);
    if (pidfilefd < 0) {
        return 0;
    }

    char pidstr[8];
    read(pidfilefd, pidstr, sizeof(pidstr) - 1);
    close(pidfilefd);

    int pid = atoi(pidstr);
    return pid;
}

