#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include "common.h"

#define PORTA 8080
#define STATUS_OK   0x00
#define STATUS_ERRO 0x01

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "uso: %s <ip> <cid>\n", argv[0]);
        return 1;
    }

    // 0. Validar o CID antes de usa-lo em qualquer caminho de arquivo
    // ou de manda-lo pela rede (mesma defesa do lado do cliente).
    if (!validar_cid(argv[2])) {
        fprintf(stderr, "CID invalido: precisa ter %d caracteres hexadecimais\n", CID_TAM);
        return 1;
    }

    // 1. Criar o socket (syscall socket)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    // 2. Configurar o endereco do servidor
    struct sockaddr_in servidor;
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PORTA);
    if (inet_pton(AF_INET, argv[1], &servidor.sin_addr) != 1) {
        fprintf(stderr, "IP invalido: %s\n", argv[1]);
        close(sock);
        return 1;
    }

    // 3. Conectar (syscall connect)
    if (connect(sock, (struct sockaddr*)&servidor, sizeof(servidor)) < 0) {
        perror("connect"); close(sock); return 1;
    }

    // 4. Enviar o CID pedido (syscall send completo)
    if (enviar_tudo(sock, argv[2], CID_TAM) < 0) {
        perror("send"); close(sock); return 1;
    }

    // 5. Ler o byte de status ANTES de decidir se ha algo para salvar.
    unsigned char status;
    ssize_t lido_status = recv(sock, &status, 1, 0);
    if (lido_status <= 0) {
        fprintf(stderr, "Conexao encerrada antes da resposta\n");
        close(sock);
        return 1;
    }
    if (status == STATUS_ERRO) {
        fprintf(stderr, "Bloco nao encontrado no peer remoto: %s\n", argv[2]);
        close(sock);
        return 1;
    }

    // 6. Receber o conteudo em um buffer de memoria (nao grava direto em
    //    disco ainda: precisamos verificar o hash antes de confirmar).
    size_t capacidade = 65536, tamanho = 0;
    unsigned char *conteudo = malloc(capacidade);
    if (conteudo == NULL) {
        fprintf(stderr, "erro: sem memoria\n");
        close(sock);
        return 1;
    }

    char buffer[4096];
    ssize_t n;
    while ((n = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        if (tamanho + (size_t)n > capacidade) {
            capacidade *= 2;
            unsigned char *novo = realloc(conteudo, capacidade);
            if (novo == NULL) {
                fprintf(stderr, "erro: sem memoria\n");
                free(conteudo);
                close(sock);
                return 1;
            }
            conteudo = novo;
        }
        memcpy(conteudo + tamanho, buffer, (size_t)n);
        tamanho += (size_t)n;
    }
    close(sock);

    // 7. Verificar integridade: o hash do que chegou precisa bater com o
    // CID pedido.
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(conteudo, tamanho, hash);
    char hash_hex[CID_TAM + 1];
    hash_para_hex(hash, hash_hex);

    if (strcmp(hash_hex, argv[2]) != 0) {
        fprintf(stderr, "ERRO: integridade falhou (hash recebido nao bate com o CID pedido)\n");
        free(conteudo);
        return 1;
    }

    // 8. Só agora grava em disco: dado ja verificado.
    mkdir("repo", 0755);
    mkdir("repo/blocks", 0755);
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "repo/blocks/%s", argv[2]);
    int out = open(caminho, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { perror("open saida"); free(conteudo); return 1; }
    if (escrever_tudo(out, conteudo, tamanho) < 0) {
        perror("write");
        close(out);
        free(conteudo);
        return 1;
    }
    close(out);

    printf("Bloco %s recebido, verificado e salvo.\n", argv[2]);
    free(conteudo);
    return 0;
}
