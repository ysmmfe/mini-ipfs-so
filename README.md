# mini-IPFS — Sistema de Blocos Endereçados por Conteúdo em C

**Disciplina:** Sistemas Operacionais  
**Período:** 2026.1  
**Equipe:**  
Yasmim Ferreira dos Santos - 600736

## Introdução

Este projeto implementa um mini-IPFS em C, escrito para a disciplina de Sistemas
Operacionais. Ele demonstra, na prática, como uma aplicação usa chamadas de sistema (syscalls)
de arquivo e de rede para implementar dois conceitos centrais do IPFS real: endereçamento por
conteúdo (o endereço de um dado é o hash do próprio conteúdo, não uma localização) e troca de
blocos entre nós via rede.

O foco é tornar visível o caminho que uma operação de I/O percorre até o kernel.

## Objetivo geral

Demonstrar como uma aplicação em espaço de usuário utiliza syscalls do Linux para I/O de arquivos
e comunicação em rede, usando o IPFS como caso de estudo.

## Objetivos específicos

- Calcular o CID (identificador de conteúdo) de um arquivo via SHA-256.
- Armazenar e recuperar blocos localmente, endereçados pelo hash do conteúdo.
- Transferir blocos entre dois nós via TCP.
- Mapear cada operação do programa às syscalls correspondentes.

## O que é o IPFS (em um parágrafo)

O IPFS (*InterPlanetary File System*) é um protocolo P2P de armazenamento e distribuição de
arquivos em que cada conteúdo é identificado pelo hash de si mesmo — o CID — em vez de por uma
localização (como uma URL). Isso significa que o mesmo conteúdo tem sempre o mesmo endereço, e
qualquer nó da rede que o possua pode servi-lo a quem pedir.

## Arquitetura

O projeto tem 4 programas independentes, todos operando sobre a mesma pasta `repo/blocks/`:

- **`ipfs_add <arquivo>`**: lê um arquivo, calcula o SHA-256, usa o hash em hexadecimal como
  **CID**, e salva o conteúdo em `repo/blocks/<cid>`.
- **`ipfs_get <cid>`**: abre `repo/blocks/<cid>` e imprime o conteúdo no stdout.
- **`ipfs_daemon`**: sobe um servidor TCP na porta 8080. Recebe um CID, procura o bloco
  localmente e o envia de volta (ou um status de erro, se não tiver).
- **`ipfs_fetch <ip> <cid>`**: conecta a um `ipfs_daemon` remoto, pede um CID, valida a
  integridade do que recebeu e salva em `repo/blocks/<cid>` local.

```mermaid
flowchart LR
    U[Usuario] -->|arquivo.txt| ADD[ipfs_add]
    ADD -->|conteudo| SHA[SHA-256]
    SHA -->|CID hex| ADD
    ADD -->|grava bloco| REPOA[(repo/blocks - No A)]
    U -->|CID| GET[ipfs_get]
    REPOA --> GET
    GET -->|conteudo| U

    subgraph NoA[No A]
        REPOA
        DAEMON[ipfs_daemon :8080]
        REPOA --> DAEMON
    end

    subgraph NoB[No B]
        FETCH[ipfs_fetch]
        REPOB[(repo/blocks - No B)]
        FETCH --> REPOB
    end

    DAEMON <-->|socket TCP :8080| FETCH
```

```mermaid
flowchart TB
    subgraph USER[Espaco de usuario]
        P1[ipfs_add]
        P2[ipfs_get]
        P3[ipfs_daemon]
        P4[ipfs_fetch]
    end

    subgraph KERNEL[Kernel]
        VFS[VFS - Sistema de Arquivos]
        TCP[Pilha TCP/IP]
    end

    DISK[(Disco - repo/blocks)]
    NET((Rede))

    P1 -->|open read write mkdir| VFS
    P2 -->|open read write| VFS
    P3 -->|open read| VFS
    P4 -->|open write mkdir| VFS
    VFS --> DISK

    P3 -->|socket bind listen accept recv send| TCP
    P4 -->|socket connect send recv| TCP
    TCP --> NET
```

## Chamadas de sistema

> **Distinção importante:** `send`/`recv` **são** syscalls de socket. Já `SHA256`, `inet_pton`,
> `htons`, `snprintf`, `malloc` **não são** syscalls — são funções de biblioteca (libc/OpenSSL).

| Chamada de sistema | Arquivos                        | Papel no projeto                | Relação com o SO |
|---------------------|----------------------------------|----------------------------------|--------------------|
| `open`              | add, get, daemon, fetch          | Abrir arquivos/blocos            | Cria descritor via VFS |
| `read`               | add, get, daemon                 | Ler conteúdo do disco            | Transfere bytes do FS para o buffer do processo |
| `write`              | add, get, fetch                  | Gravar bloco/saída               | Escreve do buffer para FS/stdout via kernel |
| `close`              | todos                            | Liberar descritor                | Kernel libera recurso da tabela de fd |
| `mkdir`              | add, fetch                       | Criar `repo/blocks`              | Cria diretório no FS via VFS |
| `fstat`              | add                              | Descobrir tamanho do arquivo     | Consulta metadados do inode |
| `socket`             | daemon, fetch                    | Criar endpoint TCP               | Aloca estrutura de socket no kernel |
| `setsockopt`         | daemon                           | `SO_REUSEADDR`                   | Ajusta opções do socket no kernel |
| `bind`               | daemon                           | Associar à porta 8080            | Registra endereço/porta na pilha TCP/IP |
| `listen`             | daemon                           | Modo de escuta                   | Cria fila de conexões pendentes |
| `accept`             | daemon                           | Aceitar cliente                  | Retira conexão da fila; novo fd |
| `connect`            | fetch                            | Conectar ao daemon               | Inicia handshake TCP (3-way) via kernel |
| `send`               | daemon, fetch                    | Enviar bytes                     | Enfileira dados na pilha TCP/IP |
| `recv`               | daemon, fetch                    | Receber bytes                    | Lê dados do buffer de socket do kernel |

## Compilação

```bash
make          # compila os 4 binarios
make clean    # remove binarios e as pastas repo/ geradas nos testes
```

## Uso de cada programa

```bash
./ipfs_add <arquivo>        # calcula o CID, salva o bloco, imprime o CID
./ipfs_get <cid>             # imprime o conteudo do bloco no stdout
./ipfs_daemon                 # sobe o servidor na porta 8080
./ipfs_fetch <ip> <cid>       # busca um bloco de um daemon remoto
```

## Demonstração com dois nós

`ipfs_daemon` e `ipfs_fetch` só fazem sentido em pastas diferentes, simulando duas máquinas —
cada uma com seu próprio `repo/blocks/`. Nunca rode o daemon e o fetch na mesma pasta, ou o teste
não prova nada (o bloco já estaria lá).

Rode `./demo.sh` para um roteiro guiado (com pausas) que reproduz o cenário completo: content
addressing, determinismo, efeito avalanche, e a troca de blocos entre dois nós via TCP.

## Testes

```bash
make test     # compila e roda a suite de testes automatizados (tests.sh)
```

A suite cobre: erros de uso (arquivo/CID inexistente), determinismo do hash, efeito avalanche,
arquivos grandes e binários (comparados byte a byte via `cmp`/`sha256sum`), criação automática de
diretórios, bloqueio de path traversal e o ciclo completo de rede (fetch sem daemon, transferência,
verificação de integridade, erro tratado sem corromper bloco).

## Limitações conhecidas

- Um arquivo = um bloco (sem chunking); não há Merkle DAG.
- Sem DHT: o IP do peer é sempre informado manualmente.
- Sem deduplicação nem download paralelo.
- Daemon atende um cliente por vez (single-threaded), sem tratamento de sinal para shutdown
  limpo.
- Sem autenticação/criptografia no canal TCP.
