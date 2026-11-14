// =====================================================================
//  Simulador de Cache — M3 de Organização de Computadores (UNIVALI)
//  Prof. Thiago Felski Pereira
//
//  Autores: José Gabriel Santos Gomes
//           Matheus Pompeo Dias
//           Nathan Gustavo Reichert
//
//  Como usar:
//    ./simulador --traco ARQUIVO --capacidade BYTES --bloco BYTES
//                --vias N|total --escrita WB|WT [--resumo]
//
//  Regras seguidas (todas do enunciado):
//    - endereço de 32 bits, cada acesso é uma palavra de 4 bytes
//    - substituição LRU dentro do conjunto (acerto também conta como uso)
//    - escrita com alocação nas duas políticas (falta de escrita traz o bloco)
//    - write-through: toda escrita manda 4 bytes pra memória
//    - write-back: escrita liga o bit "modificado"; expulsar bloco modificado
//      gera writeback do bloco inteiro; no fim, descarrega os que sobraram
//    - classificação das faltas usando uma cache "sombra" totalmente
//      associativa, com a mesma capacidade e o mesmo tamanho de bloco
// =====================================================================

#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "cache.hpp"
#include "leitor_de_traco.hpp"
#include "parametros.hpp"

// tudo que vai pro resumo do final
struct ContadoresDaSimulacao {
    uint32_t totalDeAcessos = 0;
    uint32_t totalDeAcertos = 0;
    uint32_t faltasCompulsorias = 0;
    uint32_t faltasDeConflito = 0;
    uint32_t faltasDeCapacidade = 0;
    uint32_t escritasWriteThrough = 0;        // cada uma = 4 bytes
    uint32_t writebacksDuranteAExecucao = 0;  // cada um = um bloco inteiro
    uint32_t blocosDescarregadosNoFinal = 0;  // cada um = um bloco inteiro

    uint32_t totalDeFaltas() const {
        return faltasCompulsorias + faltasDeConflito + faltasDeCapacidade;
    }
};

static void imprimirCabecalho(const Cache& cache) {
    printf("CONFIGURACAO  capacidade=%u B  bloco=%u B  vias=%u  conjuntos=%u  escrita=%s\n",
           cache.capacidadeEmBytes, cache.tamanhoDoBlocoEmBytes, cache.numeroDeVias,
           cache.numeroDeConjuntos, cache.usaWriteBack ? "WB" : "WT");
    printf("CAMPOS        rotulo=%u bits  indice=%u bits  offset=%u bits\n",
           cache.bitsDoRotulo, cache.bitsDoIndice, cache.bitsDoOffset);
}

// uma linha por acesso, no mesmo formato dos arquivos de resposta da calibração
static void imprimirLinhaDoAcesso(const Cache& cache, uint32_t numeroDoAcesso,
                                  const AcessoDoTraco& acesso, const ResultadoDoAcesso& resultado,
                                  const std::string& tipoDaFalta) {
    int digitosHexaDoRotulo = (int)(cache.bitsDoRotulo + 3) / 4;

    printf("%4u  %c 0x%08X  rot=0x%0*X  idx=%-4uoff=%-4u", numeroDoAcesso,
           acesso.ehEscrita ? 'W' : 'R', acesso.endereco, digitosHexaDoRotulo,
           cache.pegarRotulo(acesso.endereco), cache.pegarIndice(acesso.endereco),
           cache.pegarOffset(acesso.endereco));

    if (resultado.foiAcerto) {
        printf("ACERTO\n");
    } else if (!resultado.expulsouUmBloco) {
        printf("FALTA  %s\n", tipoDaFalta.c_str());
    } else {
        printf("FALTA  %-11s expulsa 0x%08X%s\n", tipoDaFalta.c_str(),
               resultado.enderecoDoBlocoExpulso,
               resultado.expulsaoGerouWriteback ? " (writeback)" : "");
    }
}

static void imprimirResumo(const Cache& cache, const ContadoresDaSimulacao& contadores) {
    double taxaDeAcertoEmPorcentagem =
        contadores.totalDeAcessos == 0 ? 0.0
                                       : 100.0 * contadores.totalDeAcertos / contadores.totalDeAcessos;
    uint32_t blocosLidosDaMemoria = contadores.totalDeFaltas();  // toda falta traz um bloco

    printf("\n");
    printf("acessos ............. %u\n", contadores.totalDeAcessos);
    printf("acertos ............. %u\n", contadores.totalDeAcertos);
    printf("faltas .............. %u  (compulsorias %u, conflito %u, capacidade %u)\n",
           contadores.totalDeFaltas(), contadores.faltasCompulsorias, contadores.faltasDeConflito,
           contadores.faltasDeCapacidade);
    printf("taxa de acerto ...... %.2f%%\n", taxaDeAcertoEmPorcentagem);
    printf("blocos lidos da mem.  %u  (%u B)\n", blocosLidosDaMemoria,
           blocosLidosDaMemoria * cache.tamanhoDoBlocoEmBytes);

    if (cache.usaWriteBack) {
        uint32_t totalDeEscritas =
            contadores.writebacksDuranteAExecucao + contadores.blocosDescarregadosNoFinal;
        printf("escritas na memoria  %u  (%u B)  [writebacks %u + descarga final %u]\n",
               totalDeEscritas, totalDeEscritas * cache.tamanhoDoBlocoEmBytes,
               contadores.writebacksDuranteAExecucao, contadores.blocosDescarregadosNoFinal);
    } else {
        printf("escritas na memoria  %u  (%u B)\n", contadores.escritasWriteThrough,
               contadores.escritasWriteThrough * 4);
    }
}

int main(int quantidadeDeArgumentos, char** argumentos) {
    ParametrosDoUsuario parametros = lerParametros(quantidadeDeArgumentos, argumentos);
    std::vector<AcessoDoTraco> acessosDoTraco = lerArquivoDeTraco(parametros.caminhoDoTraco);

    Cache cacheDeVerdade(parametros.capacidade, parametros.tamanhoDoBloco, parametros.vias,
                         parametros.usaWriteBack);

    // cache sombra: totalmente associativa, mesma capacidade e mesmo bloco.
    // não aparece na saída, só serve pra separar falta de conflito de falta de capacidade.
    uint32_t blocosQueCabemNaCache = parametros.capacidade / parametros.tamanhoDoBloco;
    Cache cacheSombraTotalmenteAssociativa(parametros.capacidade, parametros.tamanhoDoBloco,
                                           blocosQueCabemNaCache, parametros.usaWriteBack);

    // blocos de memória que já apareceram alguma vez no traço (pra saber se é compulsória)
    std::set<uint32_t> blocosQueJaForamAcessados;

    ContadoresDaSimulacao contadores;
    contadores.totalDeAcessos = (uint32_t)acessosDoTraco.size();

    imprimirCabecalho(cacheDeVerdade);
    if (!parametros.mostrarSoOResumo) {
        printf("\n");
    }

    for (uint32_t posicao = 0; posicao < acessosDoTraco.size(); posicao++) {
        const AcessoDoTraco& acesso = acessosDoTraco[posicao];
        uint32_t enderecoDoBloco = acesso.endereco & ~(parametros.tamanhoDoBloco - 1);

        // as duas caches recebem TODOS os acessos
        ResultadoDoAcesso resultado = cacheDeVerdade.acessar(acesso.endereco, acesso.ehEscrita);
        ResultadoDoAcesso resultadoNaSombra =
            cacheSombraTotalmenteAssociativa.acessar(acesso.endereco, acesso.ehEscrita);

        bool primeiraVezQueVejoEsseBloco = (blocosQueJaForamAcessados.count(enderecoDoBloco) == 0);
        blocosQueJaForamAcessados.insert(enderecoDoBloco);

        // classificação da falta
        std::string tipoDaFalta;
        if (resultado.foiAcerto) {
            contadores.totalDeAcertos++;
        } else if (primeiraVezQueVejoEsseBloco) {
            tipoDaFalta = "compulsoria";   // nunca tinha visto esse bloco
            contadores.faltasCompulsorias++;
        } else if (resultadoNaSombra.foiAcerto) {
            tipoDaFalta = "conflito";      // a totalmente associativa teria acertado
            contadores.faltasDeConflito++;
        } else {
            tipoDaFalta = "capacidade";    // nem a totalmente associativa acertaria
            contadores.faltasDeCapacidade++;
        }

        // tráfego de escrita pra memória
        if (acesso.ehEscrita && !parametros.usaWriteBack) {
            contadores.escritasWriteThrough++;
        }
        if (resultado.expulsaoGerouWriteback) {
            contadores.writebacksDuranteAExecucao++;
        }

        if (!parametros.mostrarSoOResumo) {
            imprimirLinhaDoAcesso(cacheDeVerdade, posicao + 1, acesso, resultado, tipoDaFalta);
        }
    }

    if (parametros.usaWriteBack) {
        contadores.blocosDescarregadosNoFinal = cacheDeVerdade.contarBlocosModificadosQueSobraram();
    }

    imprimirResumo(cacheDeVerdade, contadores);
    return 0;
}
