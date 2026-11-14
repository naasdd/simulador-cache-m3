// Parâmetros da linha de comando e validação da configuração.
//
// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#pragma once

#include <cstdint>
#include <string>

struct ParametrosDoUsuario {
    std::string caminhoDoTraco;
    uint32_t capacidade = 0;
    uint32_t tamanhoDoBloco = 0;
    uint32_t vias = 0;
    bool usaWriteBack = true;
    bool mostrarSoOResumo = false;
};

// lê --traco, --capacidade, --bloco, --vias, --escrita e --resumo
// (e já recusa combinações impossíveis)
ParametrosDoUsuario lerParametros(int quantidadeDeArgumentos, char** argumentos);
