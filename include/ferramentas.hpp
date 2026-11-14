// Funçõezinhas de ajuda usadas no programa todo.
//
// Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
#pragma once

#include <cstdint>
#include <string>

// true se o número é 1, 2, 4, 8, 16...
bool ehPotenciaDeDois(uint32_t numero);

// quantos bits são necessários pra contar até esse número (log2). Ex: 16 -> 4
uint32_t quantosBitsPrecisaPara(uint32_t potenciaDeDois);

// mostra a mensagem de erro e fecha o programa
[[noreturn]] void pararComErro(const std::string& mensagem);
