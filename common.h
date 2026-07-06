#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <sys/types.h>

#define CID_TAM 64  // SHA-256 em hexadecimal = 64 caracteres

// Converte SHA256_DIGEST_LENGTH (32) bytes de hash em string hexadecimal.
// 'saida' precisa ter pelo menos CID_TAM + 1 bytes.
void hash_para_hex(const unsigned char *hash, char *saida);

// Verifica se 'cid' e uma string com exatamente CID_TAM caracteres
// hexadecimais (0-9a-f). Usado para bloquear path traversal: um CID
// invalido nunca deve virar parte de um caminho de arquivo.
// Retorna 1 se valido, 0 se invalido.
int validar_cid(const char *cid);

// write()/send() podem gravar menos bytes do que o pedido numa unica
// chamada. Esta funcao repete a chamada ate escrever tudo ou falhar.
// Retorna 0 em sucesso, -1 em erro.
int escrever_tudo(int fd, const void *buf, size_t tamanho);

// Mesma ideia para o socket: send() pode enviar parcialmente.
int enviar_tudo(int sockfd, const void *buf, size_t tamanho);

#endif
