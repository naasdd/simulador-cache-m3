// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#include "parametros.hpp"

#include <cstdlib>
#include <iostream>

#include "ferramentas.hpp"

static void mostrarComoUsar() {
    std::cerr << "Uso: ./simulador --traco ARQUIVO --capacidade BYTES --bloco BYTES\n"
                 "                 --vias N|total --escrita WB|WT [--resumo]\n"
                 "\n"
                 "  --vias total   deixa a cache totalmente associativa\n"
                 "  --resumo       mostra so o resumo final (sem a saida por acesso)\n";
}

static uint32_t lerNumero(const std::string& texto, const std::string& nomeDoParametro) {
    try {
        return (uint32_t)std::stoul(texto);
    } catch (...) {
        pararComErro(nomeDoParametro + " precisa ser um numero, veio '" + texto + "'");
    }
}

// recusa combinações impossíveis
static void conferirSeAConfiguracaoFazSentido(const ParametrosDoUsuario& parametros) {
    if (!ehPotenciaDeDois(parametros.capacidade)) pararComErro("capacidade precisa ser potencia de 2");
    if (!ehPotenciaDeDois(parametros.tamanhoDoBloco)) pararComErro("bloco precisa ser potencia de 2");
    if (!ehPotenciaDeDois(parametros.vias)) pararComErro("numero de vias precisa ser potencia de 2");

    if (parametros.tamanhoDoBloco < 4) {
        pararComErro("bloco precisa ter pelo menos 4 bytes (uma palavra)");
    }
    if (parametros.tamanhoDoBloco > parametros.capacidade) {
        pararComErro("bloco maior que a capacidade da cache");
    }

    uint32_t quantidadeDeBlocosNaCache = parametros.capacidade / parametros.tamanhoDoBloco;
    if (parametros.vias > quantidadeDeBlocosNaCache) {
        pararComErro("mais vias (" + std::to_string(parametros.vias) + ") do que blocos na cache (" +
                     std::to_string(quantidadeDeBlocosNaCache) + ")");
    }
}

ParametrosDoUsuario lerParametros(int quantidadeDeArgumentos, char** argumentos) {
    ParametrosDoUsuario parametros;
    bool passouEscrita = false;
    bool pediuTotalmenteAssociativa = false;

    for (int i = 1; i < quantidadeDeArgumentos; i++) {
        std::string nome = argumentos[i];

        if (nome == "--ajuda" || nome == "-h") {
            mostrarComoUsar();
            std::exit(0);
        }
        if (nome == "--resumo") {
            parametros.mostrarSoOResumo = true;
            continue;
        }

        // os outros parâmetros têm um valor logo depois do nome
        if (i + 1 >= quantidadeDeArgumentos) {
            mostrarComoUsar();
            pararComErro("faltou o valor de " + nome);
        }
        std::string valor = argumentos[++i];

        if (nome == "--traco") {
            parametros.caminhoDoTraco = valor;
        } else if (nome == "--capacidade") {
            parametros.capacidade = lerNumero(valor, nome);
        } else if (nome == "--bloco") {
            parametros.tamanhoDoBloco = lerNumero(valor, nome);
        } else if (nome == "--vias") {
            if (valor == "total") {
                pediuTotalmenteAssociativa = true;
            } else {
                parametros.vias = lerNumero(valor, nome);
            }
        } else if (nome == "--escrita") {
            passouEscrita = true;
            if (valor == "WB" || valor == "wb") {
                parametros.usaWriteBack = true;
            } else if (valor == "WT" || valor == "wt") {
                parametros.usaWriteBack = false;
            } else {
                pararComErro("--escrita deve ser WB ou WT");
            }
        } else {
            mostrarComoUsar();
            pararComErro("parametro desconhecido: " + nome);
        }
    }

    bool faltouAlgumParametro = parametros.caminhoDoTraco.empty() || parametros.capacidade == 0 ||
                                parametros.tamanhoDoBloco == 0 || !passouEscrita ||
                                (parametros.vias == 0 && !pediuTotalmenteAssociativa);
    if (faltouAlgumParametro) {
        mostrarComoUsar();
        pararComErro("faltam parametros obrigatorios");
    }

    // totalmente associativa = um conjunto só, com uma via pra cada bloco
    if (pediuTotalmenteAssociativa && parametros.tamanhoDoBloco != 0) {
        parametros.vias = parametros.capacidade / parametros.tamanhoDoBloco;
    }

    conferirSeAConfiguracaoFazSentido(parametros);
    return parametros;
}
