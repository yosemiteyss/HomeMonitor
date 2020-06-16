#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE         100
#define SERVER_PORT     12345

int main(int argc, char *argv[])
{
    int sockfd, n, bytes_wrt, len;
    struct sockaddr_in servaddr;
    char rcv_buff[MAXLINE];

    memset(&rcv_buff, 0, sizeof(rcv_buff));

    /* init socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error: init socket\n");
        return 0;
    }

    /* init server address (IP : port) */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.128.145");
    servaddr.sin_port = htons(SERVER_PORT);

    /* connect to the server */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Error: connect\n");
        return 0;
    }

    /* read the response */
    while (1) {
        n = read(sockfd, rcv_buff, sizeof(rcv_buff) - 1);

        if (n <= 0) {
            break;
        } else {
            rcv_buff[n] = 0;        /* 0 terminate */
            printf("%s", rcv_buff);
            fflush(stdout);
        }
    }

    /* close the connection */
    close(sockfd);

    return 0;
}