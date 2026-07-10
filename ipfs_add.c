#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <arquivo>\n", argv[0]);
        return 1;
    }

    // 1. Abrir o arquivo de entrada (syscall open)
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    // 2. Descobrir o tamanho real do arquivo (syscall fstat) para nao
    // truncar arquivos grandes nem alocar buffer maior que o necessario.
    struct stat info;
    if (fstat(fd, &info) < 0) { perror("fstat"); close(fd); return 1; }
    size_t tamanho = (size_t)info.st_size;

    unsigned char *buffer = malloc(tamanho > 0 ? tamanho : 1);
    if (buffer == NULL) {
        fprintf(stderr, "erro: sem memoria para ler o arquivo\n");
        close(fd);
        return 1;
    }

    // 3. Ler o conteudo inteiro (syscall read em loop: read pode
    // devolver menos bytes do que o pedido antes de terminar).
    size_t lidos = 0;
    while (lidos < tamanho) {
        ssize_t n = read(fd, buffer + lidos, tamanho - lidos);
        if (n < 0) { perror("read"); free(buffer); close(fd); return 1; }
        if (n == 0) break;  // fim de arquivo inesperado
        lidos += (size_t)n;
    }
    close(fd);

    // 4. Calcular o hash SHA-256 do conteudo
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(buffer, lidos, hash);

    char hash_hex[CID_TAM + 1];
    hash_para_hex(hash, hash_hex);

    // 5. Criar diretorios do repositorio (syscall mkdir).
    mkdir("repo", 0755);
    mkdir("repo/blocks", 0755);

    // 6. Montar o caminho do bloco: repo/blocks/<hash>
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "repo/blocks/%s", hash_hex);

    // 7. Salvar o bloco (syscalls open + write)
    int out = open(caminho, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { perror("open saida"); free(buffer); return 1; }
    if (escrever_tudo(out, buffer, lidos) < 0) {
        perror("write");
        close(out);
        free(buffer);
        return 1;
    }
    close(out);

    // 8. Imprimir o CID (o hash)
    printf("Arquivo adicionado.\n");
    printf("CID: %s\n", hash_hex);

    free(buffer);
    return 0;
}
