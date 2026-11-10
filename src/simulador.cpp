// =====================================================================
//  Simulador de Cache — M3 de Organização de Computadores (UNIVALI)
//  Integrantes: (preencher)
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

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std;

// ---------------------------------------------------------------------
//  Funçõezinhas de ajuda
// ---------------------------------------------------------------------

bool ehPotenciaDeDois(uint32_t numero) {
    return numero != 0 && (numero & (numero - 1)) == 0;
}

// log2 de uma potência de 2 (ex: 16 -> 4)
uint32_t quantosBitsPrecisaPara(uint32_t potenciaDeDois) {
    uint32_t quantidadeDeBits = 0;
    while ((1u << quantidadeDeBits) < potenciaDeDois) {
        quantidadeDeBits++;
    }
    return quantidadeDeBits;
}

void pararComErro(const string& mensagem) {
    cerr << "ERRO: " << mensagem << "\n";
    exit(1);
}

// ---------------------------------------------------------------------
//  Uma linha (via) da cache
// ---------------------------------------------------------------------

struct LinhaDaCache {
    bool estaValida = false;          // bit de validade (V)
    bool estaModificada = false;      // bit de modificação (D), só no write-back
    uint32_t rotulo = 0;
    uint64_t momentoDoUltimoUso = 0;  // pra saber quem é o LRU
};

// ---------------------------------------------------------------------
//  O que aconteceu num acesso
// ---------------------------------------------------------------------

struct ResultadoDoAcesso {
    bool foiAcerto = false;
    bool expulsouUmBloco = false;
    uint32_t enderecoDoBlocoExpulso = 0;
    bool expulsaoGerouWriteback = false;
};

// ---------------------------------------------------------------------
//  A cache em si
// ---------------------------------------------------------------------

class Cache {
public:
    uint32_t capacidadeEmBytes;
    uint32_t tamanhoDoBlocoEmBytes;
    uint32_t numeroDeVias;
    uint32_t numeroDeConjuntos;
    bool usaWriteBack;

    uint32_t bitsDoOffset;
    uint32_t bitsDoIndice;
    uint32_t bitsDoRotulo;

    Cache(uint32_t capacidade, uint32_t tamanhoDoBloco, uint32_t vias, bool escritaEhWriteBack) {
        capacidadeEmBytes = capacidade;
        tamanhoDoBlocoEmBytes = tamanhoDoBloco;
        numeroDeVias = vias;
        usaWriteBack = escritaEhWriteBack;

        numeroDeConjuntos = capacidade / (tamanhoDoBloco * vias);

        bitsDoOffset = quantosBitsPrecisaPara(tamanhoDoBloco);
        bitsDoIndice = quantosBitsPrecisaPara(numeroDeConjuntos);
        bitsDoRotulo = 32 - bitsDoIndice - bitsDoOffset;

        // cache começa vazia: todas as linhas com validade 0
        conjuntos.assign(numeroDeConjuntos, vector<LinhaDaCache>(numeroDeVias));
    }

    uint32_t pegarOffset(uint32_t endereco) const {
        return endereco & (tamanhoDoBlocoEmBytes - 1);
    }

    uint32_t pegarIndice(uint32_t endereco) const {
        return (endereco >> bitsDoOffset) & (numeroDeConjuntos - 1);
    }

    uint32_t pegarRotulo(uint32_t endereco) const {
        // uso 64 bits pra não dar problema se o deslocamento for 32
        return (uint32_t)((uint64_t)endereco >> (bitsDoOffset + bitsDoIndice));
    }

    // endereço do primeiro byte do bloco (offset zerado)
    uint32_t montarEnderecoDoBloco(uint32_t rotulo, uint32_t indice) const {
        uint64_t endereco = ((uint64_t)rotulo << (bitsDoOffset + bitsDoIndice))
                          | ((uint64_t)indice << bitsDoOffset);
        return (uint32_t)endereco;
    }

    ResultadoDoAcesso acessar(uint32_t endereco, bool ehEscrita) {
        ResultadoDoAcesso resultado;
        uint32_t indice = pegarIndice(endereco);
        uint32_t rotulo = pegarRotulo(endereco);
        vector<LinhaDaCache>& conjuntoDoEndereco = conjuntos[indice];

        relogio++;

        // 1) procura o bloco no conjunto
        for (LinhaDaCache& linha : conjuntoDoEndereco) {
            if (linha.estaValida && linha.rotulo == rotulo) {
                resultado.foiAcerto = true;
                linha.momentoDoUltimoUso = relogio;  // acerto também conta como uso
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

        // 3) traz o bloco (vale pra leitura E escrita: write-allocate)
        linhaEscolhida.estaValida = true;
        linhaEscolhida.rotulo = rotulo;
        linhaEscolhida.estaModificada = (ehEscrita && usaWriteBack);
        linhaEscolhida.momentoDoUltimoUso = relogio;

        return resultado;
    }

    // quantos blocos ainda estão com D = 1 no fim do traço
    uint32_t contarBlocosModificadosQueSobraram() const {
        uint32_t quantidade = 0;
        for (const vector<LinhaDaCache>& conjunto : conjuntos) {
            for (const LinhaDaCache& linha : conjunto) {
                if (linha.estaValida && linha.estaModificada) {
                    quantidade++;
                }
            }
        }
        return quantidade;
    }

private:
    vector<vector<LinhaDaCache>> conjuntos;
    uint64_t relogio = 0;  // aumenta 1 a cada acesso

    // primeiro procura uma linha vazia; se não tiver, pega a usada há mais tempo (LRU)
    LinhaDaCache& escolherLinhaParaSubstituir(vector<LinhaDaCache>& conjunto) {
        for (LinhaDaCache& linha : conjunto) {
            if (!linha.estaValida) {
                return linha;
            }
        }
        LinhaDaCache* linhaMenosUsadaRecentemente = &conjunto[0];
        for (LinhaDaCache& linha : conjunto) {
            if (linha.momentoDoUltimoUso < linhaMenosUsadaRecentemente->momentoDoUltimoUso) {
                linhaMenosUsadaRecentemente = &linha;
            }
        }
        return *linhaMenosUsadaRecentemente;
    }
};

// ---------------------------------------------------------------------
//  Leitura do arquivo de traço
// ---------------------------------------------------------------------

struct AcessoDoTraco {
    bool ehEscrita;
    uint32_t endereco;
};

vector<AcessoDoTraco> lerArquivoDeTraco(const string& caminhoDoArquivo) {
    ifstream arquivo(caminhoDoArquivo);
    if (!arquivo) {
        pararComErro("nao consegui abrir o traco '" + caminhoDoArquivo + "'");
    }

    vector<AcessoDoTraco> todosOsAcessos;
    string linhaDoArquivo;
    int numeroDaLinha = 0;

    while (getline(arquivo, linhaDoArquivo)) {
        numeroDaLinha++;

        // tudo depois de # é comentário
        size_t posicaoDoComentario = linhaDoArquivo.find('#');
        if (posicaoDoComentario != string::npos) {
            linhaDoArquivo = linhaDoArquivo.substr(0, posicaoDoComentario);
        }

        char letraDaOperacao = 0;
        char textoDoEndereco[64] = {0};
        int quantosCamposLeu = sscanf(linhaDoArquivo.c_str(), " %c %63s", &letraDaOperacao, textoDoEndereco);
        if (quantosCamposLeu <= 0) {
            continue;  // linha vazia ou só comentário
        }

        string ondeEsta = "linha " + to_string(numeroDaLinha) + " do traco";
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

        try {
            unsigned long valor = stoul(textoDoEndereco, nullptr, 16);
            if (valor > 0xFFFFFFFFul) {
                pararComErro(ondeEsta + ": endereco passa de 32 bits");
            }
            acesso.endereco = (uint32_t)valor;
        } catch (...) {
            pararComErro(ondeEsta + ": endereco hexadecimal invalido");
        }

        if (acesso.endereco % 4 != 0) {
            pararComErro(ondeEsta + ": endereco nao esta alinhado em 4 bytes");
        }

        todosOsAcessos.push_back(acesso);
    }
    return todosOsAcessos;
}

// ---------------------------------------------------------------------
//  Parâmetros da linha de comando
// ---------------------------------------------------------------------

struct ParametrosDoUsuario {
    string caminhoDoTraco;
    uint32_t capacidade = 0;
    uint32_t tamanhoDoBloco = 0;
    uint32_t vias = 0;
    bool viasEhTotalmenteAssociativa = false;
    bool usaWriteBack = true;
    bool mostrarSoOResumo = false;
};

void mostrarComoUsar() {
    cerr << "Uso: ./simulador --traco ARQUIVO --capacidade BYTES --bloco BYTES\n"
            "                 --vias N|total --escrita WB|WT [--resumo]\n"
            "\n"
            "  --vias total   deixa a cache totalmente associativa\n"
            "  --resumo       mostra so o resumo final (sem a saida por acesso)\n";
}

uint32_t lerNumeroPositivo(const string& texto, const string& nomeDoParametro) {
    try {
        unsigned long valor = stoul(texto);
        return (uint32_t)valor;
    } catch (...) {
        pararComErro(nomeDoParametro + " precisa ser um numero, veio '" + texto + "'");
    }
    return 0;
}

ParametrosDoUsuario lerParametros(int quantidadeDeArgumentos, char** argumentos) {
    ParametrosDoUsuario parametros;
    bool passouEscrita = false;

    for (int i = 1; i < quantidadeDeArgumentos; i++) {
        string nome = argumentos[i];

        if (nome == "--ajuda" || nome == "-h") {
            mostrarComoUsar();
            exit(0);
        }
        if (nome == "--resumo") {
            parametros.mostrarSoOResumo = true;
            continue;
        }

        if (i + 1 >= quantidadeDeArgumentos) {
            mostrarComoUsar();
            pararComErro("faltou o valor de " + nome);
        }
        string valor = argumentos[++i];

        if (nome == "--traco") {
            parametros.caminhoDoTraco = valor;
        } else if (nome == "--capacidade") {
            parametros.capacidade = lerNumeroPositivo(valor, nome);
        } else if (nome == "--bloco") {
            parametros.tamanhoDoBloco = lerNumeroPositivo(valor, nome);
        } else if (nome == "--vias") {
            if (valor == "total") {
                parametros.viasEhTotalmenteAssociativa = true;
            } else {
                parametros.vias = lerNumeroPositivo(valor, nome);
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

    if (parametros.caminhoDoTraco.empty() || parametros.capacidade == 0 ||
        parametros.tamanhoDoBloco == 0 ||
        (parametros.vias == 0 && !parametros.viasEhTotalmenteAssociativa) || !passouEscrita) {
        mostrarComoUsar();
        pararComErro("faltam parametros obrigatorios");
    }
    return parametros;
}

// recusa combinações impossíveis
void conferirSeAConfiguracaoFazSentido(const ParametrosDoUsuario& parametros) {
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
        pararComErro("mais vias (" + to_string(parametros.vias) + ") do que blocos na cache (" +
                     to_string(quantidadeDeBlocosNaCache) + ")");
    }
}

// ---------------------------------------------------------------------
//  Programa principal
// ---------------------------------------------------------------------

int main(int quantidadeDeArgumentos, char** argumentos) {
    ParametrosDoUsuario parametros = lerParametros(quantidadeDeArgumentos, argumentos);
    if (parametros.viasEhTotalmenteAssociativa) {
        parametros.vias = parametros.capacidade / parametros.tamanhoDoBloco;
    }
    conferirSeAConfiguracaoFazSentido(parametros);

    vector<AcessoDoTraco> acessosDoTraco = lerArquivoDeTraco(parametros.caminhoDoTraco);

    Cache cacheDeVerdade(parametros.capacidade, parametros.tamanhoDoBloco, parametros.vias,
                         parametros.usaWriteBack);

    // cache sombra: totalmente associativa, mesma capacidade e bloco.
    // não aparece na saída, só serve pra separar conflito de capacidade.
    uint32_t blocosQueCabemNaCache = parametros.capacidade / parametros.tamanhoDoBloco;
    Cache cacheSombraTotalmenteAssociativa(parametros.capacidade, parametros.tamanhoDoBloco,
                                           blocosQueCabemNaCache, parametros.usaWriteBack);

    // blocos de memória que já apareceram alguma vez (pra saber se é compulsória)
    set<uint32_t> blocosQueJaForamAcessados;

    // contadores pro resumo
    uint32_t totalDeAcertos = 0;
    uint32_t faltasCompulsorias = 0;
    uint32_t faltasDeConflito = 0;
    uint32_t faltasDeCapacidade = 0;
    uint32_t escritasWriteThrough = 0;      // cada uma = 4 bytes
    uint32_t writebacksDuranteAExecucao = 0; // cada um = bloco inteiro

    // cabeçalho
    printf("CONFIGURACAO  capacidade=%u B  bloco=%u B  vias=%u  conjuntos=%u  escrita=%s\n",
           cacheDeVerdade.capacidadeEmBytes, cacheDeVerdade.tamanhoDoBlocoEmBytes,
           cacheDeVerdade.numeroDeVias, cacheDeVerdade.numeroDeConjuntos,
           parametros.usaWriteBack ? "WB" : "WT");
    printf("CAMPOS        rotulo=%u bits  indice=%u bits  offset=%u bits\n",
           cacheDeVerdade.bitsDoRotulo, cacheDeVerdade.bitsDoIndice, cacheDeVerdade.bitsDoOffset);
    if (!parametros.mostrarSoOResumo) {
        printf("\n");
    }

    int digitosHexaDoRotulo = (int)(cacheDeVerdade.bitsDoRotulo + 3) / 4;

    for (size_t posicao = 0; posicao < acessosDoTraco.size(); posicao++) {
        const AcessoDoTraco& acesso = acessosDoTraco[posicao];
        uint32_t enderecoDoBloco = acesso.endereco & ~(parametros.tamanhoDoBloco - 1);

        ResultadoDoAcesso resultado = cacheDeVerdade.acessar(acesso.endereco, acesso.ehEscrita);
        ResultadoDoAcesso resultadoNaSombra =
            cacheSombraTotalmenteAssociativa.acessar(acesso.endereco, acesso.ehEscrita);

        bool primeiraVezQueVejoEsseBloco = (blocosQueJaForamAcessados.count(enderecoDoBloco) == 0);
        blocosQueJaForamAcessados.insert(enderecoDoBloco);

        // classificação da falta
        string tipoDaFalta;
        if (resultado.foiAcerto) {
            totalDeAcertos++;
        } else if (primeiraVezQueVejoEsseBloco) {
            tipoDaFalta = "compulsoria";
            faltasCompulsorias++;
        } else if (resultadoNaSombra.foiAcerto) {
            tipoDaFalta = "conflito";      // a totalmente associativa teria acertado
            faltasDeConflito++;
        } else {
            tipoDaFalta = "capacidade";    // nem a totalmente associativa acertaria
            faltasDeCapacidade++;
        }

        if (acesso.ehEscrita && !parametros.usaWriteBack) {
            escritasWriteThrough++;
        }
        if (resultado.expulsaoGerouWriteback) {
            writebacksDuranteAExecucao++;
        }

        if (parametros.mostrarSoOResumo) {
            continue;
        }

        printf("%4zu  %c 0x%08X  rot=0x%0*X  idx=%-4uoff=%-4u",
               posicao + 1, acesso.ehEscrita ? 'W' : 'R', acesso.endereco,
               digitosHexaDoRotulo, cacheDeVerdade.pegarRotulo(acesso.endereco),
               cacheDeVerdade.pegarIndice(acesso.endereco),
               cacheDeVerdade.pegarOffset(acesso.endereco));

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

    // resumo
    uint32_t totalDeAcessos = (uint32_t)acessosDoTraco.size();
    uint32_t totalDeFaltas = faltasCompulsorias + faltasDeConflito + faltasDeCapacidade;
    double taxaDeAcertoEmPorcentagem = totalDeAcessos == 0 ? 0.0 : 100.0 * totalDeAcertos / totalDeAcessos;
    uint32_t blocosLidosDaMemoria = totalDeFaltas;  // toda falta traz um bloco

    printf("\n");
    printf("acessos ............. %u\n", totalDeAcessos);
    printf("acertos ............. %u\n", totalDeAcertos);
    printf("faltas .............. %u  (compulsorias %u, conflito %u, capacidade %u)\n",
           totalDeFaltas, faltasCompulsorias, faltasDeConflito, faltasDeCapacidade);
    printf("taxa de acerto ...... %.2f%%\n", taxaDeAcertoEmPorcentagem);
    printf("blocos lidos da mem.  %u  (%u B)\n", blocosLidosDaMemoria,
           blocosLidosDaMemoria * parametros.tamanhoDoBloco);

    if (parametros.usaWriteBack) {
        uint32_t descargaFinal = cacheDeVerdade.contarBlocosModificadosQueSobraram();
        uint32_t totalDeEscritas = writebacksDuranteAExecucao + descargaFinal;
        printf("escritas na memoria  %u  (%u B)  [writebacks %u + descarga final %u]\n",
               totalDeEscritas, totalDeEscritas * parametros.tamanhoDoBloco,
               writebacksDuranteAExecucao, descargaFinal);
    } else {
        printf("escritas na memoria  %u  (%u B)\n", escritasWriteThrough, escritasWriteThrough * 4);
    }

    return 0;
}
