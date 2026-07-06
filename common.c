#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

void hash_para_hex(const unsigned char *hash, char *saida) {
    for (int i = 0; i < 32; i++)
        snprintf(saida + (i * 2), 3, "%02x", hash[i]);
    saida[64] = '\0';
}

int validar_cid(const char *cid) {
    size_t len = 0;
    while (cid[len] != '\0') {
        if (len >= CID_TAM) return 0;
        if (!isxdigit((unsigned char)cid[len])) return 0;
        len++;
    }
    return len == CID_TAM;
}

int escrever_tudo(int fd, const void *buf, size_t tamanho) {
    const char *p = buf;
    size_t restante = tamanho;
    while (restante > 0) {
        ssize_t n = write(fd, p, restante);
        if (n < 0) return -1;
        p += n;
        restante -= (size_t)n;
    }
    return 0;
}

int enviar_tudo(int sockfd, const void *buf, size_t tamanho) {
    const char *p = buf;
    size_t restante = tamanho;
    while (restante > 0) {
        ssize_t n = send(sockfd, p, restante, 0);
        if (n < 0) return -1;
        p += n;
        restante -= (size_t)n;
    }
    return 0;
}
