#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_addr()

int main()
{
    int sock_cli = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    
    if(sock_cli == -1){
        printf("socket error");
        return -1;
    }

    int port = 8080;
    char ip[] = "127.0.0.1";
    
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET; /*使用IPv4协议*/
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);
    
    if(connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr))  == -1){
        printf("connect error");
        return -1;
    }

    char request[100];
    sprintf(request, "GET / HTTP/1.1\r\nHost: %s:%d\r\n\r\n", ip, port);
    write(sock_cli, request, strlen(request));

    char response[1024];
    read(sock_cli, response, 1024);

    printf(response);

    close(sock_cli);

    return 0;
}


