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

char        buf_snd[300000], buf_rcv[300000];
int         fdmax = 0, gid = 0;
fd_set      setRd, setWr, setCr;
socklen_t   len;
t_client    clis[1024] = {0};

void err(char  *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

void send_to_all(int sender) {
    for (int fd = 0; fd <= fdmax; fd++)
        if  (FD_ISSET(fd, &setWr) && fd != sender && send(fd, buf_snd, strlen(buf_snd), 0) == -1) err("Fatal error\n");
}

int main(int ac, char **av) {
    if (ac != 2) err("Wrong number of arguments\n");
    int fdserv = socket(AF_INET, SOCK_STREAM, 0); if (fdserv == -1) err("Fatal error\n");
    fdmax = fdserv;
    FD_ZERO(&setCr);
    FD_SET(fdserv, &setCr);

    struct sockaddr_in  serveraddr = {0};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(av[1]));

    if (bind(fdserv, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1 || listen(fdserv, 100) == -1) err("Fatal error\n");

    while (1) {
        setRd = setWr = setCr;
        if (select(fdmax + 1, &setRd, &setWr, 0, 0) == -1) continue;
        for (int fd = 0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &setRd)) {
                if (fd == fdserv) {
                    int fdcli = accept(fdserv, (struct sockaddr *)&serveraddr, &len);
                    if (fdcli == -1) continue;
                    if (fdcli > fdmax) fdmax = fdcli;
                    clis[fdcli].id = gid++;
                    FD_SET(fdcli, &setCr);
                    sprintf(buf_snd, "server: client %d just arrived\n", clis[fdcli].id);
                    send_to_all(fdcli);
                }
                else {
                    int ret = recv(fd, buf_rcv, sizeof(buf_rcv), 0);
                    if (ret <= 0) {
                        sprintf(buf_snd, "server: client %d just left\n", clis[fd].id);
                        send_to_all(fd);
                        FD_CLR(fd, &setCr);
                        close(fd);
                        bzero(clis[fd].msg, strlen(clis[fd].msg));
                    }
                    else
                        for (int i = 0, j = strlen(clis[fd].msg); i < ret; i++, j++) {
                            clis[fd].msg[j] = buf_rcv[i];
                            if (clis[fd].msg[j] == '\n') {
                                clis[fd].msg[j] = '\0';
                                sprintf(buf_snd, "client %d: %s\n", clis[fd].id, clis[fd].msg);
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