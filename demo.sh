set -e

pausa() {
    echo
    read -rp ">>> ENTER para continuar... " _
}

echo "=== 0. Compilando (make clean && make) ==="
make clean >/dev/null 2>&1 || true
make
pausa

echo "=== 1. Content addressing: mesmo conteudo = mesmo CID ==="
rm -rf repo teste.txt teste2.txt
echo "Sistemas Operacionais e IPFS" > teste.txt
./ipfs_add teste.txt
echo "--- rodando de novo, no MESMO arquivo ---"
CID=$(./ipfs_add teste.txt | grep CID | cut -d' ' -f2)
echo "CID identico nas duas vezes (determinismo)"
pausa

echo "=== 2. Efeito avalanche: 1 caractere muda tudo ==="
echo "Sistemas Operacionais e IPFs" > teste2.txt
echo "(note o 's' minusculo no final, unica diferenca)"
./ipfs_add teste2.txt
echo "--- CID completamente diferente do anterior ---"
pausa

echo "=== 3. Recuperacao local (ipfs_get) ==="
./ipfs_get "$CID"
pausa

echo "=== 4. Seguranca: CID invalido e rejeitado (path traversal) ==="
./ipfs_get "../../../etc/passwd" || true
pausa

echo "=== 5. Rede: dois nos trocando blocos via TCP ==="
rm -rf /tmp/mini_ipfs_demo
mkdir -p /tmp/mini_ipfs_demo/nodeA /tmp/mini_ipfs_demo/nodeB
cp ipfs_add ipfs_get ipfs_daemon ipfs_fetch /tmp/mini_ipfs_demo/nodeA/
cp ipfs_add ipfs_get ipfs_daemon ipfs_fetch /tmp/mini_ipfs_demo/nodeB/
cp repo/blocks/"$CID" /tmp/mini_ipfs_demo/nodeA/ 2>/dev/null || true
mkdir -p /tmp/mini_ipfs_demo/nodeA/repo/blocks
cp repo/blocks/"$CID" /tmp/mini_ipfs_demo/nodeA/repo/blocks/

echo "Subindo o daemon no 'no A' em background..."
(cd /tmp/mini_ipfs_demo/nodeA && ./ipfs_daemon &)
sleep 1

echo "--- No 'no B', o bloco NAO existe ainda: ---"
(cd /tmp/mini_ipfs_demo/nodeB && ./ipfs_get "$CID") || true
pausa

echo "--- Buscando do no A via rede (ipfs_fetch) ---"
(cd /tmp/mini_ipfs_demo/nodeB && ./ipfs_fetch 127.0.0.1 "$CID")
echo "--- Agora o no B consegue recuperar localmente ---"
(cd /tmp/mini_ipfs_demo/nodeB && ./ipfs_get "$CID")
pausa

echo "=== 6. Encerrando o daemon ==="
pkill -f ipfs_daemon 2>/dev/null || true
echo "Demo concluida."
