#!/usr/bin/env bash
# Roda todas as configurações dos experimentos E1–E6, do E3 modificado e do Desafio.
#   - a saída completa (por acesso) de cada execução vai pra resultados/<experimento>/
#   - as tabelas prontas pro relatório vão pra resultados/tabelas.md
#
# Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
cd "$(dirname "$0")" || exit 1
make -s simulador || exit 1

EXPERIMENTOS=tracos/experimentos
NOSSOS=tracos/nossos
TABELAS=resultados/tabelas.md
mkdir -p resultados
: > "$TABELAS"

# roda o simulador, guarda a saída e devolve os números do resumo numa linha só:
# acertos taxa compulsorias conflito capacidade bytes_lidos escritas bytes_escritos
rodar() {
    local traco=$1 capacidade=$2 bloco=$3 vias=$4 escrita=$5 arquivoDeSaida=$6
    mkdir -p "$(dirname "$arquivoDeSaida")"
    ./simulador --traco "$traco" --capacidade "$capacidade" --bloco "$bloco" \
                --vias "$vias" --escrita "$escrita" > "$arquivoDeSaida" || exit 1
    awk '
        /^acertos/             { acertos = $3 }
        /^faltas/              { gsub(/[(),]/, ""); compulsorias = $5; conflito = $7; capacidade = $9 }
        /^taxa de acerto/      { taxa = $5; sub(/\./, ",", taxa) }
        /^blocos lidos/        { gsub(/[()]/, ""); bytesLidos = $6 }
        /^escritas na memoria/ { gsub(/[()]/, ""); escritas = $4; bytesEscritos = $5 }
        END { print acertos, taxa, compulsorias, conflito, capacidade, bytesLidos, escritas, bytesEscritos }
    ' "$arquivoDeSaida"
}

escrever() { echo "$*" >> "$TABELAS"; }

# tabela com o tamanho do bloco variando (E1 e E2)
tabelaVariandoBloco() {
    local titulo=$1 traco=$2 pasta=$3
    escrever "## $titulo"
    escrever ""
    escrever "| Bloco (B) | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Bytes lidos |"
    escrever "|---:|---:|---:|---:|---:|---:|---:|"
    for bloco in 4 8 16 32 64; do
        read -r ac tx co cf cp bl es be < <(rodar "$traco" 256 "$bloco" 1 WB "resultados/$pasta/bloco_${bloco}.txt")
        escrever "| $bloco | $ac | $tx | $co | $cf | $cp | $bl |"
    done
    escrever ""
}

# tabela com o número de vias variando (E3, E4, E5)
tabelaVariandoVias() {
    local titulo=$1 traco=$2 pasta=$3
    shift 3
    local listaDeVias=("$@")
    escrever "## $titulo"
    escrever ""
    escrever "| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |"
    escrever "|---:|---:|---:|---:|---:|---:|"
    for vias in "${listaDeVias[@]}"; do
        read -r ac tx co cf cp bl es be < <(rodar "$traco" 256 16 "$vias" WB "resultados/$pasta/vias_${vias}.txt")
        escrever "| $vias | $ac | $tx | $co | $cf | $cp |"
    done
    escrever ""
}

# tabela com a política de escrita variando (E6)
tabelaVariandoEscrita() {
    local titulo=$1 traco=$2 pasta=$3
    escrever "## $titulo"
    escrever ""
    escrever "| Política | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Escritas | Bytes escritos | Bytes lidos |"
    escrever "|---|---:|---:|---:|---:|---:|---:|---:|---:|"
    for escrita in WT WB; do
        read -r ac tx co cf cp bl es be < <(rodar "$traco" 256 16 2 "$escrita" "resultados/$pasta/${escrita}.txt")
        escrever "| $escrita | $ac | $tx | $co | $cf | $cp | $es | $be | $bl |"
    done
    escrever ""
}

escrever "# Tabelas dos experimentos (geradas por rodar_experimentos.sh)"
escrever ""
escrever "Capacidade 256 B em todos. A saída por acesso de cada linha está em \`resultados/<experimento>/\`."
escrever ""

tabelaVariandoBloco   "E1 — varredura (1 via, WB)"            $EXPERIMENTOS/E1_varredura.txt   E1
tabelaVariandoBloco   "E2 — registros (1 via, WB)"            $EXPERIMENTOS/E2_registros.txt   E2
tabelaVariandoVias    "E3 — dois vetores (bloco 16 B, WB)"    $EXPERIMENTOS/E3_dois_vetores.txt E3 1 2 4 16
tabelaVariandoVias    "E3 modificado — segundo vetor em 0x10000080 (bloco 16 B, WB)" \
                      $NOSSOS/E3_dois_vetores_modificado.txt  E3_modificado 1
tabelaVariandoVias    "E4 — varredura (bloco 16 B, WB)"       $EXPERIMENTOS/E1_varredura.txt   E4 1 2 4 16
tabelaVariandoVias    "E5 — laço de 80 palavras (bloco 16 B, WB)" $EXPERIMENTOS/E5_laco_80.txt E5_80 1 2 4 16
tabelaVariandoVias    "E5 — laço de 60 palavras (bloco 16 B, WB)" $EXPERIMENTOS/E5_laco_60.txt E5_60 1 2 4 16
tabelaVariandoEscrita "E6 — acumulador (bloco 16 B, 2 vias)"  $EXPERIMENTOS/E6_acumulador.txt  E6_acumulador
tabelaVariandoEscrita "E6 — cópia (bloco 16 B, 2 vias)"       $EXPERIMENTOS/E6_copia.txt       E6_copia
tabelaVariandoEscrita "E6 — escritas esparsas (bloco 16 B, 2 vias)" $EXPERIMENTOS/E6_esparsa.txt E6_esparsa
tabelaVariandoVias    "Desafio (bloco 16 B, WB)"              $NOSSOS/desafio.txt              Desafio 1 2 4

cat "$TABELAS"
