#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <sys/types.h>

#define CID_TAM 64  // SHA-256 em hexadecimal = 64 caracteres

void hash_para_hex(const unsigned char *hash, char *saida);

int validar_cid(const char *cid);

int escrever_tudo(int fd, const void *buf, size_t tamanho);

int enviar_tudo(int sockfd, const void *buf, size_t tamanho);

#endif
