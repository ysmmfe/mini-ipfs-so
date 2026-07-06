#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // open
#include <unistd.h>     // read, write, close
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <cid>\n", argv[0]);
        return 1;
    }

    // 0. Validar o CID: precisa ser exatamente 64 caracteres hexadecimais.
    //    Isso impede que algo como "../../etc/passwd" vire caminho de arquivo.
    if (!validar_cid(argv[1])) {
        fprintf(stderr, "CID invalido: precisa ter %d caracteres hexadecimais\n", CID_TAM);
        return 1;
    }

    // 1. Montar o caminho do bloco a partir do hash
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "repo/blocks/%s", argv[1]);

    // 2. Abrir o bloco (syscall open)
    int fd = open(caminho, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Bloco nao encontrado: %s\n", argv[1]);
        return 1;
    }

    // 3. Ler e escrever na saida padrao (syscalls read + write completo)
    unsigned char buffer[4096];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        if (escrever_tudo(STDOUT_FILENO, buffer, (size_t)n) < 0) {
            perror("write");
            close(fd);
            return 1;
        }
    }
    if (n < 0) { perror("read"); close(fd); return 1; }

    close(fd);
    return 0;
}
