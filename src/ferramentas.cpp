// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#include "ferramentas.hpp"

#include <cstdlib>
#include <iostream>

bool ehPotenciaDeDois(uint32_t numero) {
    // potência de 2 tem um único bit ligado; (n & (n-1)) apaga esse bit e dá 0
    return numero != 0 && (numero & (numero - 1)) == 0;
}

uint32_t quantosBitsPrecisaPara(uint32_t potenciaDeDois) {
    uint32_t quantidadeDeBits = 0;
    while ((1u << quantidadeDeBits) < potenciaDeDois) {
        quantidadeDeBits++;
    }
    return quantidadeDeBits;
}

void pararComErro(const std::string& mensagem) {
    std::cerr << "ERRO: " << mensagem << "\n";
    std::exit(1);
}
