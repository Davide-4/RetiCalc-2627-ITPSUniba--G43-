# RetiCalc-2627-ITPSUniba-Gruppo G43-
Prima assegnazione di laboratorio sulle socket TCP. Sistema operativo usato per la prova: Ubuntu

## Compilazione
Su Ubuntu, con GCC:

```bash
  gcc -std=c11 -Wall -Wextra -o serverTCP_G43 TCP/serverTCP_G43/serverTCP_G43.c
  gcc -std=c11 -Wall -Wextra -o clientTCP_G43 TCP/clientTCP_G43/clientTCP_G43.c
```

## Esempio di sessione

Client:

```
  Inserisci l'IP del server (Invio per 127.0.0.1): 127.0.0.1
  Inserisci la porta del server (Invio per 6000): 6000
  connessione avvenuta
  Inserisci tre interi (n1 n2 n3): 10 20 5
  Risposta dal server: 25
  Inserisci tre interi (n1 n2 n3): 90 20 10
  Risposta dal server: QUIT
```

Server:

```
  Server TCP in ascolto sulla porta 6000...
  connesso con il client il cui nome IP: 127.0.0.1
  la somma di 10+20-5=25
  la somma di 90+20-10=100
  Client disconnesso.
```
