// A cache: conjuntos, vias, LRU, write-back / write-through.
//
// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#pragma once

#include <cstdint>
#include <vector>

// Uma linha (via) da cache
struct LinhaDaCache {
    bool estaValida = false;          // bit de validade (V)
    bool estaModificada = false;      // bit de modificação (D), só liga no write-back
    uint32_t rotulo = 0;
    uint64_t momentoDoUltimoUso = 0;  // pra saber quem é o LRU (menor = usado há mais tempo)
};

// O que aconteceu num acesso
struct ResultadoDoAcesso {
    bool foiAcerto = false;
    bool expulsouUmBloco = false;
    uint32_t enderecoDoBlocoExpulso = 0;
    bool expulsaoGerouWriteback = false;
};

class Cache {
public:
    Cache(uint32_t capacidadeEmBytes, uint32_t tamanhoDoBlocoEmBytes, uint32_t numeroDeVias,
          bool usaWriteBack);

    // faz um acesso (leitura ou escrita) e conta o que aconteceu
    ResultadoDoAcesso acessar(uint32_t endereco, bool ehEscrita);

    // divisão do endereço: rótulo | índice | offset
    uint32_t pegarRotulo(uint32_t endereco) const;
    uint32_t pegarIndice(uint32_t endereco) const;
    uint32_t pegarOffset(uint32_t endereco) const;

    // quantos blocos ainda estão com D = 1 no fim do traço (descarga final)
    uint32_t contarBlocosModificadosQueSobraram() const;

    // configuração (só leitura depois de criada)
    uint32_t capacidadeEmBytes;
    uint32_t tamanhoDoBlocoEmBytes;
    uint32_t numeroDeVias;
    uint32_t numeroDeConjuntos;
    bool usaWriteBack;

    uint32_t bitsDoOffset;
    uint32_t bitsDoIndice;
    uint32_t bitsDoRotulo;

private:
    std::vector<std::vector<LinhaDaCache>> conjuntos;  // conjuntos[indice][via]
    uint64_t relogio = 0;                              // aumenta 1 a cada acesso

    LinhaDaCache& escolherLinhaParaSubstituir(std::vector<LinhaDaCache>& conjunto);
    uint32_t montarEnderecoDoBloco(uint32_t rotulo, uint32_t indice) const;
};
