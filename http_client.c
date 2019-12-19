#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_addr()

char server_ip[] = "127.0.0.1";
int server_port = 8080;
char *server_domain_name = "127.0.0.1";

int main()
{
    int sock_cli = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    
    if(sock_cli == -1){
        printf("socket error");
        return -1;
    }
    
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET; /*使用IPv4协议*/
    servaddr.sin_port = htons(server_port);
    servaddr.sin_addr.s_addr = inet_addr(server_ip);
    
    if(connect(sock_cli, (struct sockaddr *)&servaddr,
                sizeof(servaddr))  == -1){
        printf("connect error");
        return -1;
    }

    char request[1024];
    sprintf(request, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", server_domain_name);

    write(sock_cli, request, strlen(request));

    char response[1025];
    int len = 0;

    while((len = read(sock_cli, response, 1024))) {
        printf("%.*s", len, response);
    }

    close(sock_cli);
    return 0;
}


