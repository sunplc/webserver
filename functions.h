#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <sys/socket.h>

void PerrorExit(const char *s);

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

void Bind(int fd, const struct sockaddr *sa, socklen_t salen);

void Listen(int fd, int backlog);

int Socket();

ssize_t Read(int fd, void *ptr, size_t nbytes);

ssize_t Write(int fd, const void *ptr, size_t nbytes);

//ssize_t Readchar(int fd, char *ptr);

ssize_t Readline(int fd, void *vptr, size_t maxlen);

void Close(int fd);

int rtrim(char *str, char c);

void PrintTime();

void Log(FILE *fp, char *fmt, ...);

int Daemonize(int);

void Concat(char *dest, char *s1, char *s2);

void WritePidFile(const char *pidfile, int pid);

int ReadPidFile(const char *pidfile);

#endif /* FUNCTIONS_H */
