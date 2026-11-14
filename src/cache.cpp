// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#include "cache.hpp"

#include "ferramentas.hpp"

Cache::Cache(uint32_t capacidade, uint32_t tamanhoDoBloco, uint32_t vias, bool escritaEhWriteBack) {
    capacidadeEmBytes = capacidade;
    tamanhoDoBlocoEmBytes = tamanhoDoBloco;
    numeroDeVias = vias;
    usaWriteBack = escritaEhWriteBack;

    numeroDeConjuntos = capacidade / (tamanhoDoBloco * vias);

    bitsDoOffset = quantosBitsPrecisaPara(tamanhoDoBloco);
    bitsDoIndice = quantosBitsPrecisaPara(numeroDeConjuntos);
    bitsDoRotulo = 32 - bitsDoIndice - bitsDoOffset;

    // a cache começa vazia: todas as linhas com validade 0
    conjuntos.assign(numeroDeConjuntos, std::vector<LinhaDaCache>(numeroDeVias));
}

// ---------------------------------------------------------------------
//  Divisão do endereço
//
//  | rótulo (o resto) | índice (bitsDoIndice) | offset (bitsDoOffset) |
// ---------------------------------------------------------------------

uint32_t Cache::pegarOffset(uint32_t endereco) const {
    return endereco & (tamanhoDoBlocoEmBytes - 1);
}

uint32_t Cache::pegarIndice(uint32_t endereco) const {
    return (endereco >> bitsDoOffset) & (numeroDeConjuntos - 1);
}

uint32_t Cache::pegarRotulo(uint32_t endereco) const {
    // uso 64 bits pra não dar problema se o deslocamento for 32
    return (uint32_t)((uint64_t)endereco >> (bitsDoOffset + bitsDoIndice));
}

// endereço do primeiro byte do bloco (o caminho contrário: junta rótulo e índice, offset 0)
uint32_t Cache::montarEnderecoDoBloco(uint32_t rotulo, uint32_t indice) const {
    uint64_t endereco = ((uint64_t)rotulo << (bitsDoOffset + bitsDoIndice))
                      | ((uint64_t)indice << bitsDoOffset);
    return (uint32_t)endereco;
}

// ---------------------------------------------------------------------
//  Acesso
// ---------------------------------------------------------------------

ResultadoDoAcesso Cache::acessar(uint32_t endereco, bool ehEscrita) {
    ResultadoDoAcesso resultado;
    uint32_t indice = pegarIndice(endereco);
    uint32_t rotulo = pegarRotulo(endereco);
    std::vector<LinhaDaCache>& conjuntoDoEndereco = conjuntos[indice];

    relogio++;

    // 1) procura o bloco no conjunto
    for (LinhaDaCache& linha : conjuntoDoEndereco) {
        if (linha.estaValida && linha.rotulo == rotulo) {
            resultado.foiAcerto = true;
            linha.momentoDoUltimoUso = relogio;  // acerto também conta como uso (LRU)
            if (ehEscrita && usaWriteBack) {
                linha.estaModificada = true;
            }
            return resultado;
        }
    }

    // 2) faltou: escolhe onde colocar o bloco novo
    LinhaDaCache& linhaEscolhida = escolherLinhaParaSubstituir(conjuntoDoEndereco);

    if (linhaEscolhida.estaValida) {
        resultado.expulsouUmBloco = true;
        resultado.enderecoDoBlocoExpulso = montarEnderecoDoBloco(linhaEscolhida.rotulo, indice);
        resultado.expulsaoGerouWriteback = linhaEscolhida.estaModificada;
    }

    // 3) traz o bloco. Vale pra leitura E escrita (write-allocate)
    linhaEscolhida.estaValida = true;
    linhaEscolhida.rotulo = rotulo;
    linhaEscolhida.estaModificada = (ehEscrita && usaWriteBack);
    linhaEscolhida.momentoDoUltimoUso = relogio;

    return resultado;
}

// primeiro procura uma linha vazia; se não tiver, pega a usada há mais tempo (LRU)
LinhaDaCache& Cache::escolherLinhaParaSubstituir(std::vector<LinhaDaCache>& conjunto) {
    for (LinhaDaCache& linha : conjunto) {
        if (!linha.estaValida) {
            return linha;
        }
    }

    LinhaDaCache* linhaUsadaHaMaisTempo = &conjunto[0];
    for (LinhaDaCache& linha : conjunto) {
        if (linha.momentoDoUltimoUso < linhaUsadaHaMaisTempo->momentoDoUltimoUso) {
            linhaUsadaHaMaisTempo = &linha;
        }
    }
    return *linhaUsadaHaMaisTempo;
}

uint32_t Cache::contarBlocosModificadosQueSobraram() const {
    uint32_t quantidade = 0;
    for (const std::vector<LinhaDaCache>& conjunto : conjuntos) {
        for (const LinhaDaCache& linha : conjunto) {
            if (linha.estaValida && linha.estaModificada) {
                quantidade++;
            }
        }
    }
    return quantidade;
}
