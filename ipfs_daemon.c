#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>   // sockets
#include "common.h"

#define PORTA 8080
#define STATUS_OK   0x00
#define STATUS_ERRO 0x01

// Le o CID enviado pelo cliente. O CID tem tamanho fixo (CID_TAM), mas o
// TCP pode entregar os bytes fragmentados em varios recv(); por isso lemos
// em loop ate completar CID_TAM bytes (ou a conexao fechar).
static int receber_cid(int cliente, char *cid) {
    size_t recebidos = 0;
    while (recebidos < CID_TAM) {
        ssize_t n = recv(cliente, cid + recebidos, CID_TAM - recebidos, 0);
        if (n <= 0) return -1;  // conexao fechada ou erro
        recebidos += (size_t)n;
    }
    cid[CID_TAM] = '\0';
    return 0;
}

int main() {
    // Saida sem buffer: garante que as mensagens aparecam na hora,
    // mesmo se a saida for redirecionada para um arquivo.
    setbuf(stdout, NULL);

    // 1. Criar o socket (syscall socket)
    int servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor < 0) { perror("socket"); return 1; }

    // Permitir reutilizar a porta rapidamente
    int opt = 1;
    if (setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }

    // 2. Configurar e associar o endereco (syscall bind)
    struct sockaddr_in endereco;
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORTA);

    if (bind(servidor, (struct sockaddr*)&endereco, sizeof(endereco)) < 0) {
        perror("bind"); close(servidor); return 1;
    }

    // 3. Comecar a escutar (syscall listen)
    if (listen(servidor, 5) < 0) { perror("listen"); close(servidor); return 1; }
    printf("Daemon escutando na porta %d...\n", PORTA);

    while (1) {
        // 4. Aceitar uma conexao (syscall accept)
        int cliente = accept(servidor, NULL, NULL);
        if (cliente < 0) continue;

        // 5. Receber o CID pedido (syscall recv, em loop)
        char cid[CID_TAM + 1];
        if (receber_cid(cliente, cid) < 0 || !validar_cid(cid)) {
            // CID invalido ou incompleto: nunca chega a tocar no disco.
            // Isso e o que bloqueia o path traversal (M1).
            unsigned char status = STATUS_ERRO;
            enviar_tudo(cliente, &status, 1);
            fprintf(stderr, "Pedido rejeitado: CID invalido\n");
            close(cliente);
            continue;
        }
        printf("Pedido recebido: %s\n", cid);

        // 6. Procurar o bloco no repositorio local
        char caminho[256];
        snprintf(caminho, sizeof(caminho), "repo/blocks/%s", cid);
        int fd = open(caminho, O_RDONLY);

        if (fd < 0) {
            // Protocolo: 1 byte de status ANTES do payload. Erro nunca
            // e confundido com conteudo do lado do cliente (corrige M2).
            unsigned char status = STATUS_ERRO;
            enviar_tudo(cliente, &status, 1);
        } else {
            unsigned char status = STATUS_OK;
            if (enviar_tudo(cliente, &status, 1) == 0) {
                // 7. Enviar o conteudo do bloco (syscalls read + send completo)
                char buffer[4096];
                ssize_t n;
                while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
                    if (enviar_tudo(cliente, buffer, (size_t)n) < 0) break;
                }
            }
            close(fd);
        }
        close(cliente);
    }

    close(servidor);
    return 0;
}
