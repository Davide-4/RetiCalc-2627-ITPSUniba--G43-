
#if defined(_WIN32) || defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#define closesocket close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT 6000
#define QLEN 6
#define BUFSIZE 1024

static void ErrorHandler(const char *errorMessage) {
    printf("%s", errorMessage);
}

static void ClearWinSock(void) {
#if defined(_WIN32) || defined(WIN32)
    WSACleanup();
#endif
}

/* Ritorna -1 se la connessione e' chiusa o in errore,
 * altrimenti la lunghezza della riga (0 = riga vuota). */
static int recv_line(int sock, char *buf, int max_len) {
    int pos = 0;
    char c;
    while (pos < max_len - 1) {
        int n = recv(sock, &c, 1, 0);
        if (n <= 0) {
            if (pos == 0) {
                return -1;
            }
            break;
        }
        if (c == '\n') {
            break;
        }
        if (c != '\r') {
            buf[pos++] = c;
        }
    }
    buf[pos] = '\0';
    return pos;
}

static int send_line(int sock, const char *msg) {
    char out[BUFSIZE];
    int len = snprintf(out, sizeof(out), "%s\n", msg);
    if (len < 0 || len >= (int)sizeof(out)) {
        return -1;
    }
    int sent = 0;
    while (sent < len) {
        int n = send(sock, out + sent, len - sent, 0);
        if (n <= 0) {
            return -1;
        }
        sent += n;
    }
    return sent;
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    int port = DEFAULT_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            printf("Numero di porta non valido: %s\n", argv[1]);
            return 1;
        }
    }

#if defined(_WIN32) || defined(WIN32)
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        ErrorHandler("Error at WSAStartup()\n");
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    /* 1) Il server e' avviato su una porta a scelta e resta sempre in ascolto */
    int serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock < 0) {
        ErrorHandler("socket creation failed.\n");
        ClearWinSock();
        return 1;
    }

#if !defined(_WIN32) && !defined(WIN32)
    /* Permette di riavviare subito il server sulla stessa porta */
    int yes = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = htonl(INADDR_ANY);
    sad.sin_port = htons((unsigned short)port);

    if (bind(serverSock, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        ErrorHandler("bind() failed.\n");
        closesocket(serverSock);
        ClearWinSock();
        return 1;
    }

    if (listen(serverSock, QLEN) < 0) {
        ErrorHandler("listen() failed.\n");
        closesocket(serverSock);
        ClearWinSock();
        return 1;
    }

    printf("Server TCP in ascolto sulla porta %d...\n", port);

    while (1) {
        struct sockaddr_in cad;
        socklen_t clientLen = sizeof(cad);
        int clientSock = accept(serverSock, (struct sockaddr *)&cad, &clientLen);
        if (clientSock < 0) {
            ErrorHandler("accept() failed.\n");
            continue;
        }

        /* 4) Visualizza l'IP dell'host del client connesso */
        printf("connesso con il client il cui nome IP: %s\n", inet_ntoa(cad.sin_addr));

        /* 5) Invia al client la stringa "connessione avvenuta" */
        if (send_line(clientSock, "connessione avvenuta") < 0) {
            ErrorHandler("Errore nell'invio di connessione avvenuta.\n");
            closesocket(clientSock);
            continue;
        }

        char line[BUFSIZE];
        while (1) {
            /* 7) Riceve i tre interi n1, n2, n3 */
            int r = recv_line(clientSock, line, sizeof(line));
            if (r < 0) {
                printf("Client disconnesso.\n");
                break;
            }

            int n1, n2, n3;
            char extra;
            if (sscanf(line, "%d %d %d %c", &n1, &n2, &n3, &extra) != 3) {
                if (send_line(clientSock, "ERRORE: inserire esattamente tre interi validi (n1 n2 n3)") < 0) {
                    break;
                }
                continue;
            }

            /* 8) Calcola res = n1 + n2 - n3 e lo visualizza */
            int res = n1 + n2 - n3;
            printf("la somma di %d+%d-%d=%d\n", n1, n2, n3, res);

            /* 9) Se res == 100 invia "QUIT", altrimenti invia res */
            if (res == 100) {
                send_line(clientSock, "QUIT");
                break;
            } else {
                char resp[64];
                snprintf(resp, sizeof(resp), "%d", res);
                if (send_line(clientSock, resp) < 0) {
                    break;
                }
            }
        }

        closesocket(clientSock);
        /* 11) Il server rimane in ascolto */
    }

    closesocket(serverSock);
    ClearWinSock();
    return 0;
}
