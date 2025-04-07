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

int       fdsrv, fdcli, fdmax, gid = 0;   //
socklen_t len;
fd_set    setRd, setWr, setCr;            //
char      bufSnd[300000], bufRcv[300000];
t_client  clis[1024] = {0};

void err(char *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

void send_to_all(int sender) {
    for (int fd = 0; fd <= fdmax; fd++)
        if  (FD_ISSET(fd, &setWr) && fd != sender && send(fd, bufSnd, strlen(bufSnd), 0) == -1) err("Fatal error\n");
}

int main(int ac, char **av) {
    if (ac != 2) err("Wrong number of arguments\n");
    fdsrv = socket(AF_INET, SOCK_STREAM, 0); if (fdsrv == -1) err("Fatal error\n");
    fdmax = fdsrv;
    FD_ZERO(&setCr);
    FD_SET(fdsrv, &setCr);
    struct sockaddr_in adrSrv = {0};
    adrSrv.sin_family         = AF_INET;
    adrSrv.sin_addr.s_addr    = htonl(INADDR_ANY);
    adrSrv.sin_port           = htons(atoi(av[1]));
    if (bind(fdsrv, (const struct sockaddr *)&adrSrv, sizeof(adrSrv)) == -1) err("Fatal error\n");
    if (listen(fdsrv, 100)                                            == -1) err("Fatal error\n");
    while (1) {
        setRd = setWr = setCr;
        if (select(fdmax + 1, &setRd, &setWr, 0, 0) == -1) continue;
        for (int fd = 0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &setRd)) {
                if (fd == fdsrv) {
                    fdcli = accept(fdsrv, (struct sockaddr *)&adrSrv, &len); if (fdcli == -1) continue;
                    if (fdcli > fdmax) fdmax = fdcli;
                    clis[fdcli].id = gid++;
                    FD_SET(fdcli, &setCr);
                    sprintf(bufSnd, "server: client %d just arrived\n", clis[fdcli].id);
                    send_to_all(fdcli);
                }
                else {
                    int ret = recv(fd, bufRcv, sizeof(bufRcv), 0);
                    if (ret <= 0) {
                        sprintf(bufSnd, "server: client %d just left\n", clis[fd].id);
                        send_to_all(fd);
                        FD_CLR(fd, &setCr);
                        close(fd);
                        memset(clis[fd].msg, 0, sizeof(clis[fd].msg));
                    } else {
                        for (int i = 0, j = strlen(clis[fd].msg); i < ret; i++, j++) {
                            clis[fd].msg[j] = bufRcv[i];
                            if (clis[fd].msg[j] == '\n') {
                                clis[fd].msg[j] = '\0';
                                sprintf(bufSnd, "client %d: %s\n", clis[fd].id, clis[fd].msg);
                                send_to_all(fd);
                                memset(clis[fd].msg, 0, sizeof(clis[fd].msg));
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