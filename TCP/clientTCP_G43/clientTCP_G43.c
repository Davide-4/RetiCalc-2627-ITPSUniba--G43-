
#if defined(_WIN32) || defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#define closesocket close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 1024

static void ErrorHandler(const char *errorMessage) {
    printf("%s", errorMessage);
}

/* Come ErrorHandler, ma aggiunge la causa di sistema (solo Linux) */
static void SysErrorHandler(const char *errorMessage) {
    printf("%s", errorMessage);
#if !defined(_WIN32) && !defined(WIN32)
    perror("  dettaglio");
#endif
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

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

#if defined(_WIN32) || defined(WIN32)
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        ErrorHandler("Error at WSAStartup()\n");
        return 1;
    }
#endif

    /* 2) Il client legge da tastiera IP dell'host e numero di porta */
    char ipbuf[64];
    char portbuf[16];
    int port;

    printf("Inserisci l'IP del server: ");
    if (fgets(ipbuf, sizeof(ipbuf), stdin) == NULL) {
        ClearWinSock();
        return 1;
    }
    trim_newline(ipbuf);

    printf("Inserisci la porta: ");
    if (fgets(portbuf, sizeof(portbuf), stdin) == NULL) {
        ClearWinSock();
        return 1;
    }
    port = atoi(portbuf);
    if (port <= 0 || port > 65535) {
        ErrorHandler("Numero di porta non valido.\n");
        ClearWinSock();
        return 1;
    }

    /* 3) Richiede la connessione al server */
    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        SysErrorHandler("socket creation failed.\n");
        ClearWinSock();
        return 1;
    }

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_port = htons((unsigned short)port);

    /* Accetta sia un IP (es. 127.0.0.1) sia un nome host (es. localhost) */
    sad.sin_addr.s_addr = inet_addr(ipbuf);
    if (sad.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *host = gethostbyname(ipbuf);
        if (host == NULL || host->h_addrtype != AF_INET) {
            ErrorHandler("Impossibile risolvere l'indirizzo del server.\n");
            closesocket(sock);
            ClearWinSock();
            return 1;
        }
        memcpy(&sad.sin_addr, host->h_addr_list[0], host->h_length);
    }

    if (connect(sock, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        SysErrorHandler("connect() failed.\n");
        closesocket(sock);
        ClearWinSock();
        return 1;
    }

    /* 6) Riceve la stringa "connessione avvenuta" e la stampa */
    char line[BUFSIZE];
    int r = recv_line(sock, line, sizeof(line));
    if (r < 0) {
        ErrorHandler("Connessione chiusa dal server prima del benvenuto.\n");
        closesocket(sock);
        ClearWinSock();
        return 1;
    }
    printf("%s\n", line);

    while (1) {
        /* 7) Legge tre interi e li invia al server */
        printf("Inserisci tre interi (n1 n2 n3): ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        trim_newline(line);

        if (send_line(sock, line) < 0) {
            ErrorHandler("Errore durante l'invio al server.\n");
            break;
        }

        /* 10) Riceve la risposta e la visualizza */
        r = recv_line(sock, line, sizeof(line));
        if (r < 0) {
            printf("Server disconnesso.\n");
            break;
        }

        printf("Risposta dal server: %s\n", line);
        if (strcmp(line, "QUIT") == 0) {
            break;
        }
    }

    closesocket(sock);
    ClearWinSock();
    return 0;
}
