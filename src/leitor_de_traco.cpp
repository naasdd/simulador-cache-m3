// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#include "leitor_de_traco.hpp"

#include <cstdio>
#include <fstream>

#include "ferramentas.hpp"

std::vector<AcessoDoTraco> lerArquivoDeTraco(const std::string& caminhoDoArquivo) {
    std::ifstream arquivo(caminhoDoArquivo);
    if (!arquivo) {
        pararComErro("nao consegui abrir o traco '" + caminhoDoArquivo + "'");
    }

    std::vector<AcessoDoTraco> todosOsAcessos;
    std::string linhaDoArquivo;
    int numeroDaLinha = 0;

    while (std::getline(arquivo, linhaDoArquivo)) {
        numeroDaLinha++;
        std::string ondeEsta = "linha " + std::to_string(numeroDaLinha) + " do traco";

        // tudo depois de # é comentário
        size_t posicaoDoComentario = linhaDoArquivo.find('#');
        if (posicaoDoComentario != std::string::npos) {
            linhaDoArquivo = linhaDoArquivo.substr(0, posicaoDoComentario);
        }

        char letraDaOperacao = 0;
        char textoDoEndereco[64] = {0};
        int quantosCamposLeu =
            std::sscanf(linhaDoArquivo.c_str(), " %c %63s", &letraDaOperacao, textoDoEndereco);

        if (quantosCamposLeu <= 0) {
            continue;  // linha vazia ou só comentário
        }
        if (quantosCamposLeu != 2) {
            pararComErro(ondeEsta + ": esperava 'R|W endereco'");
        }

        AcessoDoTraco acesso;

        if (letraDaOperacao == 'R' || letraDaOperacao == 'r') {
            acesso.ehEscrita = false;
        } else if (letraDaOperacao == 'W' || letraDaOperacao == 'w') {
            acesso.ehEscrita = true;
        } else {
            pararComErro(ondeEsta + ": operacao deve ser R ou W");
        }

        unsigned long valorDoEndereco = 0;
        try {
            valorDoEndereco = std::stoul(textoDoEndereco, nullptr, 16);
        } catch (...) {
            pararComErro(ondeEsta + ": endereco hexadecimal invalido");
        }
        if (valorDoEndereco > 0xFFFFFFFFul) {
            pararComErro(ondeEsta + ": endereco passa de 32 bits");
        }
        acesso.endereco = (uint32_t)valorDoEndereco;

        // cada acesso é uma palavra de 4 bytes alinhada
        if (acesso.endereco % 4 != 0) {
            pararComErro(ondeEsta + ": endereco nao esta alinhado em 4 bytes");
        }

        todosOsAcessos.push_back(acesso);
    }
    return todosOsAcessos;
}
