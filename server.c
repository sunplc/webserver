/**
 * multiple processes webserver
 */

#include <stdio.h>			// perror()
#include <stdlib.h>			// malloc() free() exit()
#include <string.h>
#include <unistd.h>			// execl() access() fork() dup2() STDOUT_FILENO sleep()
#include <limits.h>         // PATH_MAX

#include <errno.h>          // ECHILD
#include <signal.h>         // kill() SIGTERM SIGKILL
#include <fcntl.h>			// open() O_RDONLY
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>		// stat() S_ISDIR S_IXUSR
#include <sys/wait.h>		// waitpid()

#include "functions.h"

#define BUF_SIZE 1024 * 1024	// maximum number of characters of read line
#define SERV_PORT 8080          // HTTP port 

#define DOCUMENT_ROOT "/www"    // web root path
#define LOG_PATH "/log"         // log path

#define WORKER_COUNT 5          // number of child processes
#define MAX_REQUEST 1000        // number of maximum requests for per child

int main(int argc, char *argv[])
{
    char filename[PATH_MAX], cwd[PATH_MAX];
    pid_t pid;

    // Get current working directory.
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        PerrorExit("getcwd()");
    }


    if (argc == 2) {
        if (strcmp(argv[1], "start") == 0) {

        } else if (strcmp(argv[1], "stop") == 0) {

            Concat(filename, cwd, "/webserver.pid");
            int master_pid = ReadPidFile(filename);
            if (master_pid <= 0) {
                puts("webserver is not running");
            }

            // send SIGKILL signal to every process 
            // in the process group whose ID is -pid
            puts("Stopping...");
            kill(master_pid * -1, SIGTERM);
            unlink(filename);
            printf("Stopped process group #%d.\n", master_pid);

            exit(EXIT_SUCCESS);

        } else if (strcmp(argv[1], "status") == 0) {

            Concat(filename, cwd, "/webserver.pid");
            int master_pid = ReadPidFile(filename);
            if (master_pid <= 0) {
                puts("webserver is not running");
            } else {
                puts("webserver is running");
            }

            exit(EXIT_SUCCESS);

        } else {

            printf("Usage:\n webserver start\n"
                    " webserver stop\n webserver status\n");
            exit(EXIT_SUCCESS);
        }

    } else {
        printf("Usage:\n webserver start\n"
                " webserver stop\n webserver status\n");
        exit(EXIT_SUCCESS);
    }



    Concat(filename, cwd, LOG_PATH "/server.log");
    int errfd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (errfd < 0) {
        PerrorExit("open() errfd");
    }

    pid = Daemonize(errfd);

    Concat(filename, cwd, "/webserver.pid");
    WritePidFile(filename, pid);

    Concat(filename, cwd, LOG_PATH "/access.log");
    FILE *accfp = fopen(filename, "a+");
    if (accfp == NULL)
        PerrorExit("open access.log file");

    Concat(filename, cwd, LOG_PATH "/server.log");
    FILE *srvfp = fopen(filename, "a+");
    if (srvfp == NULL)
        PerrorExit("open server.log file");

    Log(srvfp, "Server started, master pid = %d", (int)pid);
    fflush(srvfp);

    int listenfd, connfd;
    listenfd = Socket(SERV_PORT);

    int i, index, status;
    int workers[WORKER_COUNT];

    // Fork n child process, storage their pid in array.
    for (i = 0; i < WORKER_COUNT; ++i) {
        if ((pid = fork()) < 0) {
            PerrorExit("failed to fork");
        } else if (pid > 0) {
            workers[i] = pid;
        } else {
            break;
        }
        Log(srvfp, "Fork Worker #%d", (int)pid);
        fflush(srvfp);
    }


    // Parent process logic.
    if (pid > 0) {

        // Loop and block to wait child process terminate.
        for (i = 0; ; ++i) {

            if ((pid = wait(&status)) == -1) {
                if (errno == ECHILD) {
                    Log(srvfp, "ECHILD");
                } else {
                    Log(srvfp, "wait() error");
                }

            } else {

                // Child process terminated normally from exit or _exit.
                if (WIFEXITED(status)) {
                    Log(srvfp, "child #%d exited with status %d",
                            pid, WEXITSTATUS(status));

                } else { // Terminated abnormally.

                    Log(srvfp, "child #%d terminated abnormally"
                            " with status %d", (int)pid, status);
                }

                // Get the index of terminated child.
                for (index = 0; index < WORKER_COUNT; ++index) {
                    if (workers[index] == (int)pid)
                        break;
                }

                if (index == WORKER_COUNT) {
                    Log(srvfp, "worker's pid error!");
                }

                fflush(srvfp);

                if ((pid = fork()) < 0) {
                    Log(srvfp, "failed to fork");
                } else if (pid > 0) {
                    Log(srvfp, "#%d create proc #%d", getpid(), pid);
                    //sleep(1);
                    workers[index] = pid;
                } else {
                    break;
                }
            }

        }

        // Master end.
        if (pid > 0) {
            return 0;
        }
    }


    // Child worker process logic.
    fclose(srvfp);

    char delimiter[2] = " \0";
    char *token, *extension;

    char *protocol = malloc(256);
    char *http_method = malloc(256);
    char *uri = malloc(256);
    char *content_type = malloc(1024);

    char buf[BUF_SIZE];
    char str[INET_ADDRSTRLEN];

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    // Loop and accept TCP connection request.
    for (i = 0; i < MAX_REQUEST; ++i) {

        client_addr_len = sizeof(client_addr);
        // Accept TCP connection.
        connfd = Accept(listenfd, (struct sockaddr *)&client_addr, 
                &client_addr_len);


        // Parse HTTP header
        if (Readline(connfd, buf, BUF_SIZE) == 0) {
            break;
        }

        // If error while get the first header.
        if (buf == NULL || strcmp(buf, "\r\n") == 0) {
            break;
        }

        token = strtok(buf, delimiter);
        strcpy(http_method, token);

        token = strtok(NULL, delimiter);
        strcpy(uri, token);

        token = strtok(NULL, delimiter);
        strcpy(protocol, token);
        rtrim(protocol, '\n');
        rtrim(protocol, '\r');

        // Blank line stands for the end of headers.
        while (Readline(connfd, buf, BUF_SIZE) != 0) {
            if (strcmp(buf, "\r\n") == 0) {
                break;
            }
        }

        // Default MIME type.
        strcpy(content_type ,"content-type: text/html\r\n");

        // Get the extension of the URI to determine the MIME type.
        if ((extension = strrchr(uri, '.')) != NULL) {
            ++extension;
            //printf("extension=%s\n", extension);
            if (strcmp(extension, "jpg") == 0) {
                sprintf(content_type, "content-type: %s/%s\r\n", "image", "jpeg");
            }
        }

        // Returns the index file if the root directory is requested.
        if (strcmp(uri, "/") == 0) {
            Concat(filename, cwd, DOCUMENT_ROOT "/index.html");
        } else {
            strcpy(filename, uri);
            strcpy(uri, DOCUMENT_ROOT);
            strcat(uri, filename);
            Concat(filename, cwd, uri);
        }

        // Check if the requested file exists.
        if (access(filename, F_OK) != -1) { 

            // Check if file is openable and have execute permission.
            struct stat filebuf;
            if (stat(filename, &filebuf) == 0 && !S_ISDIR(filebuf.st_mode) 
                    && filebuf.st_mode & S_IXUSR) {

                // Response 200 OK.
                Write(connfd, "HTTP/1.1 200 OK\r\n", 17);
                Write(connfd, content_type, strlen(content_type));
                // Send blank line, indicates the end of response headers.
                Write(connfd, "\r\n", 2);

                // Close STDOUT_FILENO, copy connfd to STDOUT_FILENO.
                dup2(connfd, STDOUT_FILENO);
                execl(filename, "nothing", NULL);

            } else {

                // Response 200
                Write(connfd, "HTTP/1.1 200 OK\r\n", 17);
                Write(connfd, content_type, strlen(content_type));
                // Send blank line, indicates the end of response headers
                Write(connfd, "\r\n", 2);

                // Open file, read content as http response body.
                int fd;
                if ((fd = open(filename, O_RDONLY)) < 0) {
                    PerrorExit("failed open URI file");
                }

                while ( (i = Read(fd, buf, BUF_SIZE)) > 0) {
                    // printf("i=%d, BUF_SIZE=%d\n", i, BUF_SIZE);
                    Write(connfd, buf, i);
                }

                Close(fd);
            }

        } else {

            strcpy(content_type ,"content-type: text/html\r\n");

            // Response 404 Not Found.
            Write(connfd, "HTTP/1.1 404 Not Found\r\n", 24);
            Write(connfd, content_type, strlen(content_type));
            // Send blank line, indicates the end of response headers.
            Write(connfd, "\r\n", 2);

            // Open 404.html, read content as http response body.
            Concat(filename, cwd, DOCUMENT_ROOT "/404.html");
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                PerrorExit("failed open 404.html file");
            }

            while ( (i = Readline(fd, buf, BUF_SIZE)) > 0) {
                Write(connfd, buf, i);
            }

            Close(fd);
        }

        // Get Client ip and port.
        inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str));

        Log(accfp, "HTTP request from %s:%d, %s %s %s", 
                str, ntohs(client_addr.sin_port), protocol, http_method, uri);

        fflush(accfp);

        // Close TCP connection.
        Close(connfd);
    }


    free(protocol);
    free(http_method);
    free(uri);
    free(content_type);

    return 0; // Every child process terminated here.
}

