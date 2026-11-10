#!/usr/bin/env bash
# Roda os 4 traços de calibração e compara com as respostas do professor.
cd "$(dirname "$0")" || exit 1
make -s simulador || exit 1
mkdir -p saidas

PASTA=tracos/calibracao
QUANTOS_PASSARAM=0

comparar() {
    local nome=$1
    if diff -u "$PASTA/${nome}_esperado.txt" "saidas/${nome}.txt" > "saidas/${nome}.diff"; then
        echo "  OK     $nome"
        QUANTOS_PASSARAM=$((QUANTOS_PASSARAM + 1))
    else
        echo "  FALHOU $nome  (diferencas em saidas/${nome}.diff)"
    fi
}

./simulador --traco $PASTA/C1_mapeamento_direto.txt --capacidade 128 --bloco 16 --vias 1 --escrita WB \
    > saidas/C1_mapeamento_direto.txt
comparar C1_mapeamento_direto

./simulador --traco $PASTA/C2_associativa_LRU.txt --capacidade 64 --bloco 16 --vias 2 --escrita WB \
    > saidas/C2_associativa_LRU.txt
comparar C2_associativa_LRU

# C3 roda duas vezes (WB e WT), separadas por uma linha de "=" como no arquivo de resposta
{
    ./simulador --traco $PASTA/C3_escrita.txt --capacidade 64 --bloco 16 --vias 1 --escrita WB
    echo
    printf '=%.0s' {1..70}; echo
    echo
    ./simulador --traco $PASTA/C3_escrita.txt --capacidade 64 --bloco 16 --vias 1 --escrita WT
} > saidas/C3_escrita.txt
comparar C3_escrita

./simulador --traco $PASTA/C4_classificacao.txt --capacidade 64 --bloco 16 --vias 1 --escrita WB \
    > saidas/C4_classificacao.txt
comparar C4_classificacao

echo "Calibracao: $QUANTOS_PASSARAM de 4"
[ "$QUANTOS_PASSARAM" -eq 4 ]
