set -u
FALHAS=0
TESTES=0

ok()   { TESTES=$((TESTES+1)); echo "  OK  - $1"; }
falha(){ TESTES=$((TESTES+1)); FALHAS=$((FALHAS+1)); echo "FALHA - $1"; }

WORKDIR=$(mktemp -d)
cp ipfs_add ipfs_get ipfs_daemon ipfs_fetch "$WORKDIR/"
cd "$WORKDIR"

echo "=== Testes locais (ipfs_add / ipfs_get) ==="

# T01: arquivo inexistente
if ./ipfs_add naoexiste.txt >/dev/null 2>&1; then falha "T01 arquivo inexistente deveria falhar"; else ok "T01 arquivo inexistente"; fi

# T02: argumentos ausentes
if ./ipfs_add >/dev/null 2>&1; then falha "T02 sem argumentos deveria falhar"; else ok "T02 sem argumentos"; fi

# T03: CID inexistente no get
if ./ipfs_get 0000000000000000000000000000000000000000000000000000000000000000 >/dev/null 2>&1; then
  falha "T03 CID inexistente deveria falhar"
else
  ok "T03 CID inexistente (get)"
fi

# T04: arquivo vazio
: > vazio.txt
CID_VAZIO=$(./ipfs_add vazio.txt | grep CID | cut -d' ' -f2)
if [ -f "repo/blocks/$CID_VAZIO" ]; then ok "T04 arquivo vazio gera bloco"; else falha "T04 arquivo vazio"; fi

# T05: CID tem 64 hex chars
echo "conteudo pequeno" > small.txt
CID_SMALL=$(./ipfs_add small.txt | grep CID | cut -d' ' -f2)
if [ "${#CID_SMALL}" -eq 64 ]; then ok "T05 CID tem 64 caracteres"; else falha "T05 CID tamanho errado (${#CID_SMALL})"; fi

# T06: arquivo grande (5MB) nao trunca
head -c 5000000 /dev/urandom > big.bin 2>/dev/null || dd if=/dev/urandom of=big.bin bs=1000 count=5000 >/dev/null 2>&1
HASH_ESPERADO=$(sha256sum big.bin | cut -d' ' -f1)
CID_BIG=$(./ipfs_add big.bin | grep CID | cut -d' ' -f2)
if [ "$HASH_ESPERADO" = "$CID_BIG" ]; then ok "T06 arquivo grande: hash bate (sem truncar)"; else falha "T06 arquivo grande: hash nao bate"; fi

# T07: binario recuperado e identico byte a byte
./ipfs_get "$CID_BIG" > big_recuperado.bin
if cmp -s big.bin big_recuperado.bin; then ok "T07 binario identico apos get"; else falha "T07 binario diferente apos get"; fi

# T08: determinismo
CID_A=$(./ipfs_add small.txt | grep CID | cut -d' ' -f2)
CID_B=$(./ipfs_add small.txt | grep CID | cut -d' ' -f2)
if [ "$CID_A" = "$CID_B" ]; then ok "T08 determinismo (mesmo conteudo = mesmo CID)"; else falha "T08 determinismo"; fi

# T09: avalanche
echo "conteudo pequeno." > small2.txt
CID_C=$(./ipfs_add small2.txt | grep CID | cut -d' ' -f2)
if [ "$CID_A" != "$CID_C" ]; then ok "T09 avalanche (1 char muda o CID)"; else falha "T09 avalanche"; fi

# T10: criacao de diretorios
rm -rf repo
./ipfs_add small.txt >/dev/null
if [ -d "repo/blocks" ]; then ok "T10 repo/blocks criado automaticamente"; else falha "T10 criacao de diretorios"; fi

# T17: path traversal bloqueado
if ./ipfs_get "../../../etc/passwd" >/dev/null 2>&1; then
  falha "T17 path traversal deveria ser bloqueado"
else
  ok "T17 path traversal bloqueado"
fi

echo
echo "=== Testes de rede (ipfs_daemon / ipfs_fetch) ==="

# T11: daemon indisponivel
if ./ipfs_fetch 127.0.0.1 "$CID_SMALL" >/dev/null 2>&1; then
  falha "T11 fetch sem daemon deveria falhar"
else
  ok "T11 fetch sem daemon rodando"
fi

# Sobe o daemon para os testes de rede
mkdir -p nodeA_repo
cp -r repo nodeA_repo/repo 2>/dev/null
(cd nodeA_repo && ../ipfs_daemon >/dev/null 2>&1 &)
sleep 1

mkdir -p nodeB && cd nodeB
cp ../ipfs_get ../ipfs_fetch .

# T13: transferencia A->B
./ipfs_fetch 127.0.0.1 "$CID_SMALL" >/dev/null 2>&1
if [ -f "repo/blocks/$CID_SMALL" ]; then ok "T13 bloco transferido de A para B"; else falha "T13 transferencia de bloco"; fi

# T14: integridade do bloco transferido
HASH_RECEBIDO=$(sha256sum "repo/blocks/$CID_SMALL" 2>/dev/null | cut -d' ' -f1)
if [ "$HASH_RECEBIDO" = "$CID_SMALL" ]; then ok "T14 integridade do bloco recebido"; else falha "T14 integridade do bloco"; fi

# T15: get local no no B apos fetch
if ./ipfs_get "$CID_SMALL" >/dev/null 2>&1; then ok "T15 get local funciona apos fetch"; else falha "T15 get apos fetch"; fi

# T16: CID inexistente no daemon nao vira bloco corrompido
CID_FALSO="1111111111111111111111111111111111111111111111111111111111111111"
CID_FALSO="${CID_FALSO:0:64}"
if ./ipfs_fetch 127.0.0.1 "$CID_FALSO" >/dev/null 2>&1; then
  falha "T16 CID inexistente deveria falhar no fetch"
else
  if [ -f "repo/blocks/$CID_FALSO" ]; then
    falha "T16 erro foi salvo como bloco (bug M2 voltou)"
  else
    ok "T16 erro do daemon nao vira bloco (protocolo de status ok)"
  fi
fi

pkill -f ipfs_daemon 2>/dev/null

cd /
rm -rf "$WORKDIR"

echo
echo "=== Resultado: $((TESTES-FALHAS))/$TESTES testes passaram ==="
if [ "$FALHAS" -gt 0 ]; then exit 1; fi
