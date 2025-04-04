#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client {
    int     id;
    char    msg[290000];
}   t_client;

t_client    clis[1024];
fd_set      read_set, write_set, cur_set;
int         maxfd = 0, gid = 0;
char        send_buffer[300000], recv_buffer[300000];

void err(char  *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

void send_to_all(int sender) {
    for (int fd = 0; fd <= maxfd; fd++) {
        if  (FD_ISSET(fd, &write_set) && fd != sender && send(fd, send_buffer, strlen(send_buffer), 0) == -1) err("Fatal error\n");
    }
}

int main(int ac, char **av) {
    if (ac != 2) err("Wrong number of arguments\n");

    struct sockaddr_in  serveraddr = {0};
    socklen_t           len;
    int servfd = socket(AF_INET, SOCK_STREAM, 0); if (servfd == -1) err("Fatal error\n");
    maxfd = servfd;

    FD_ZERO(&cur_set);
    FD_SET(servfd, &cur_set);
    bzero(clis, sizeof(clis));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(av[1]));

    if (bind(servfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1 || listen(servfd, 100) == -1) err("Fatal error\n");

    while (1) {
        read_set = write_set = cur_set;
        if (select(maxfd + 1, &read_set, &write_set, 0, 0) == -1) continue;
        for (int fd = 0; fd <= maxfd; fd++) {
            if (FD_ISSET(fd, &read_set)) {
                if (fd == servfd) {
                    int clifd = accept(servfd, (struct sockaddr *)&serveraddr, &len);
                    if (clifd == -1) continue;
                    if (clifd > maxfd) maxfd = clifd;
                    clis[clifd].id = gid++;
                    FD_SET(clifd, &cur_set);
                    sprintf(send_buffer, "server: client %d just arrived\n", clis[clifd].id);
                    send_to_all(clifd);
                }
                else {
                    int ret = recv(fd, recv_buffer, sizeof(recv_buffer), 0);
                    if (ret <= 0) {
                        sprintf(send_buffer, "server: client %d just left\n", clis[fd].id);
                        send_to_all(fd);
                        FD_CLR(fd, &cur_set);
                        close(fd);
                        bzero(clis[fd].msg, strlen(clis[fd].msg));
                    }
                    else
                        for (int i = 0, j = strlen(clis[fd].msg); i < ret; i++, j++) {
                            clis[fd].msg[j] = recv_buffer[i];
                            if (clis[fd].msg[j] == '\n') {
                                clis[fd].msg[j] = '\0';
                                sprintf(send_buffer, "client %d: %s\n", clis[fd].id, clis[fd].msg);
                                send_to_all(fd);
                                bzero(clis[fd].msg, strlen(clis[fd].msg));
                                j = -1;
                            }
                        }
                }
                break;
            }
        }
    }
    return (0);
}