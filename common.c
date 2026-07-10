#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

// Converte SHA256_DIGEST_LENGTH (32) bytes de hash em string hexadecimal.
// 'saida' precisa ter pelo menos CID_TAM + 1 bytes.
void hash_para_hex(const unsigned char *hash, char *saida) {
    for (int i = 0; i < 32; i++)
        snprintf(saida + (i * 2), 3, "%02x", hash[i]);
    saida[64] = '\0';
}

// Verifica se 'cid' e uma string com exatamente CID_TAM caracteres
// hexadecimais (0-9a-f). Usado para bloquear path traversal: um CID
// invalido nunca deve virar parte de um caminho de arquivo.
// Retorna 1 se valido, 0 se invalido.
int validar_cid(const char *cid) {
    size_t len = 0;
    while (cid[len] != '\0') {
        if (len >= CID_TAM) return 0;
        if (!isxdigit((unsigned char)cid[len])) return 0;
        len++;
    }
    return len == CID_TAM;
}

// write()/send() podem gravar menos bytes do que o pedido numa unica
// chamada. Esta funcao repete a chamada ate escrever tudo ou falhar.
// Retorna 0 em sucesso, -1 em erro.
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

// Mesma ideia para o socket: send() pode enviar parcialmente.
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
