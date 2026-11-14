// Leitura do arquivo de traço (uma linha por acesso: "R 0x10000000" ou "W 0x10000004").
//
// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct AcessoDoTraco {
    bool ehEscrita;
    uint32_t endereco;
};

// lê o arquivo inteiro; para o programa com erro se alguma linha estiver errada
std::vector<AcessoDoTraco> lerArquivoDeTraco(const std::string& caminhoDoArquivo);
