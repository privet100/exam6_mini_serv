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

int       fdS, fdC, fdM, gid = 0;   //
socklen_t len;
fd_set    setRcv, setSnd, setCur;            //
char      bufSnd[300000], bufRcv[300000];
t_client  cli[1024] = {0};

void err(char *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

void send_to_all(int sender) {
    for (int fd = 0; fd <= fdM; fd++)
        if  (FD_ISSET(fd, &setSnd) && fd != sender && send(fd, bufSnd, strlen(bufSnd), 0) == -1) err("Fatal error\n");
}

int main(int ac, char **av) {
    if (ac != 2) err("Wrong number of arguments\n");
    fdS = socket(AF_INET, SOCK_STREAM, 0); if (fdS == -1) err("Fatal error\n");
    fdM = fdS;
    FD_ZERO(&setCur);
    FD_SET(fdS, &setCur);
    struct sockaddr_in adrSrv = {0};
    adrSrv.sin_family         = AF_INET;
    adrSrv.sin_addr.s_addr    = htonl(INADDR_ANY);
    adrSrv.sin_port           = htons(atoi(av[1]));
    if (bind(fdS, (const struct sockaddr *)&adrSrv, sizeof(adrSrv)) == -1) err("Fatal error\n");
    if (listen(fdS, 100)                                            == -1) err("Fatal error\n");
    while (1) {
        setRcv = setSnd = setCur;
        if (select(fdM + 1, &setRcv, &setSnd, 0, 0) == -1) continue;
        for (int fd = 0; fd <= fdM; fd++) {
            if (FD_ISSET(fd, &setRcv)) {
                if (fd == fdS) {
                    fdC = accept(fdS, (struct sockaddr *)&adrSrv, &len); if (fdC == -1) continue;
                    if (fdC > fdM) fdM = fdC;
                    cli[fdC].id = gid++;
                    FD_SET(fdC, &setCur);
                    sprintf(bufSnd, "server: client %d just arrived\n", cli[fdC].id);
                    send_to_all(fdC);
                }
                else {
                    int ret = recv(fd, bufRcv, sizeof(bufRcv), 0);
                    if (ret <= 0) {
                        sprintf(bufSnd, "server: client %d just left\n", cli[fd].id);
                        send_to_all(fd);
                        FD_CLR(fd, &setCur);
                        close(fd);
                        memset(cli[fd].msg, 0, sizeof(cli[fd].msg));
                    } else {
                        for (int i = 0, j = strlen(cli[fd].msg); i < ret; i++, j++) {
                            cli[fd].msg[j] = bufRcv[i];
                            if (cli[fd].msg[j] == '\n') {
                                cli[fd].msg[j] = '\0';
                                sprintf(bufSnd, "client %d: %s\n", cli[fd].id, cli[fd].msg);
                                send_to_all(fd);
                                memset(cli[fd].msg, 0, sizeof(cli[fd].msg));
                                j = -1;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    return (0);
}