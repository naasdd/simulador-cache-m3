# Simulador de Cache e Experimentos com Estratégias de Organização

## 1 Introdução

Este relatório apresenta o simulador de cache desenvolvido para a avaliação prática da disciplina de Organização de Computadores e os experimentos realizados com ele. O simulador é o instrumento: com ele medimos o efeito de cada decisão de projeto de uma **memória cache** (tamanho do bloco, associatividade e política de escrita) e explicamos por que os números saíram como saíram.

A cache funciona porque os programas apresentam **localidade de referência**: o que foi usado agora tende a ser usado de novo em breve (localidade temporal) e o que está perto do que foi usado tende a ser usado em seguida (localidade espacial) (PATTERSON; HENNESSY, 2017). Toda falta é classificada pelo modelo dos três Cs, proposto por Hill e Smith (1989): **compulsória**, **capacidade** e **conflito**. A forma de separar conflito de capacidade, comparando com uma cache totalmente associativa de mesma capacidade, segue a definição usada por Hennessy e Patterson (2019) e pelo enunciado do trabalho. As políticas de escrita comparadas, **write-through** e **write-back**, diferem no momento em que os dados modificados chegam à memória principal (STALLINGS, 2017).

Para cada experimento são apresentadas, nesta ordem, a previsão feita antes de rodar o simulador, a medição, a justificativa apoiada na saída por acesso e a conciliação entre previsão e medição. O código-fonte, os traços e a saída completa de todas as execuções estão no repositório https://github.com/naasdd/simulador-cache-m3.

## 2 O Simulador

### 2.1 Organização do código

O simulador foi escrito em C++17 e está dividido em cinco arquivos, cada um com uma responsabilidade:

| Arquivo | Responsabilidade |
|---|---|
| src/main.cpp | laço principal, classificação das faltas, saída por acesso e resumo |
| src/cache.cpp | classe Cache: divisão do endereço, procura no conjunto, LRU, bit de modificação e expulsão |
| src/leitor_de_traco.cpp | leitura do arquivo de traço |
| src/parametros.cpp | leitura da linha de comando e recusa de configurações impossíveis |
| src/ferramentas.cpp | funções auxiliares (potência de 2, logaritmo na base 2, mensagem de erro) |

### 2.2 Como executar

Os parâmetros são passados pela linha de comando, o que permite rodar o mesmo traço com várias configurações sem editar o código:

```
make
./simulador --traco tracos/experimentos/E1_varredura.txt --capacidade 256 --bloco 16 --vias 1 --escrita WB
./calibrar.sh
./rodar_experimentos.sh
```

O script calibrar.sh roda os quatro traços de calibração e compara a saída com as respostas. O script rodar_experimentos.sh executa todas as configurações dos experimentos, guarda a saída por acesso de cada uma na pasta resultados e monta as tabelas deste relatório.

### 2.3 Decisões de implementação

- **Substituição LRU:** a cache tem um relógio que aumenta a cada acesso. Cada linha guarda o momento do seu último uso, atualizado tanto no acerto quanto no preenchimento. Na falta, a cache usa primeiro uma linha inválida; se não houver, sai a linha usada há mais tempo.
- **Escrita com alocação:** na falta, o bloco é sempre trazido, seja leitura ou escrita. No write-back, a linha já nasce com o bit D ligado quando o acesso que a trouxe foi uma escrita.
- **Tráfego de memória:** os blocos lidos são iguais ao número de faltas. No write-through, cada escrita soma 4 bytes. No write-back, cada expulsão de bloco com D = 1 soma um bloco inteiro e, ao final, as linhas que ainda têm D = 1 são contadas como descarga final.
- **Classificação das faltas:** uma segunda instância da classe Cache, com uma via para cada bloco (totalmente associativa, mesma capacidade e mesmo bloco), recebe todos os acessos do traço. Um conjunto ordenado guarda os blocos que já apareceram. A falta é compulsória quando o bloco nunca apareceu, de conflito quando a cache totalmente associativa acertou e de capacidade quando ela também faltou.
- **Validação:** o programa recusa valores que não são potência de 2, bloco menor que uma palavra, bloco maior que a capacidade e mais vias do que blocos.

## 3 Calibração

Os quatro traços de calibração reproduzem exatamente os arquivos de resposta, conferidos com o comando diff, byte a byte. O traço C3 é rodado em write-back e em write-through.

```
  OK     C1_mapeamento_direto
  OK     C2_associativa_LRU
  OK     C3_escrita
  OK     C4_classificacao
Calibracao: 4 de 4
```

## 4 Experimentos

Em todos os experimentos a capacidade da cache é 256 B. Os números de acesso citados nas justificativas vêm da saída por acesso de cada configuração, guardada em resultados/<experimento>/ no repositório.

### 4.1 E1: Tamanho do bloco numa varredura

**Previsão.** Taxas de 0%, 50%, 75%, 87,5% e 93,75% para blocos de 4 a 64 B. Cada bloco é lido uma única vez: a primeira palavra falta e as demais acertam, então a taxa é (p − 1)/p, com p palavras por bloco. Os bytes lidos ficariam sempre em 512 B.

**Medição** (1 via, write-back):

| Bloco (B) | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Bytes lidos |
|---:|---:|---:|---:|---:|---:|---:|
| 4 | 0 | 0,00% | 128 | 0 | 0 | 512 |
| 8 | 64 | 50,00% | 64 | 0 | 0 | 512 |
| 16 | 96 | 75,00% | 32 | 0 | 0 | 512 |
| 32 | 112 | 87,50% | 16 | 0 | 0 | 512 |
| 64 | 120 | 93,75% | 8 | 0 | 0 | 512 |

**Justificativa.**

1. Com p = B/4 palavras por bloco, cada bloco recebe p acessos seguidos: uma falta e p − 1 acertos. A taxa é, portanto, (p − 1)/p = 1 − 4/B, o que dá 0% para p = 1, 50% para p = 2, 75% para p = 4, 87,5% para p = 8 e 93,75% para p = 16, exatamente os valores da tabela. Com bloco de 16 B, o acesso 1 (0x10000000) falta e os acessos 2, 3 e 4 acertam no mesmo índice 0; o acesso 5 (0x10000010) falta no índice 1 e o padrão se repete.
2. Só apareceram faltas compulsórias. Cada bloco do vetor é usado uma única vez e nunca é procurado de novo, então nenhuma falta pode ser de conflito ou de capacidade. O vetor tem 512 B, o dobro da cache. Com bloco de 16 B, a partir do acesso 65 (0x10000100) os blocos novos expulsam os antigos ("expulsa 0x10000000"); com os outros tamanhos de bloco acontece o mesmo assim que o vetor passa dos primeiros 256 B. Mesmo assim, isso não gera falta extra porque os blocos expulsos não voltam a ser lidos.
3. Os bytes lidos ficaram em 512 B em todas as configurações: o vetor inteiro é trazido exatamente uma vez, só que em pedaços maiores ou menores. Neste traço, aumentar o bloco não tem custo em tráfego e reduz as faltas de 128 para 8, porque todo byte trazido é usado.

**Conciliação.** A medição coincidiu com a previsão.

### 4.2 E2: Tamanho do bloco com dados espalhados

**Previsão.** Taxas de 75%, 75%, 37,5%, 0% e 0%. O passo de 72 B elimina a localidade espacial, e os acertos dependem de os 16 blocos caberem sem disputar índice. Com bloco de 16 B, quatro pares de registros disputariam índice; com 32 e 64 B, todos os índices teriam dois ou mais registros.

**Medição** (1 via, write-back):

| Bloco (B) | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Bytes lidos |
|---:|---:|---:|---:|---:|---:|---:|
| 4 | 48 | 75,00% | 16 | 0 | 0 | 64 |
| 8 | 48 | 75,00% | 16 | 0 | 0 | 128 |
| 16 | 24 | 37,50% | 16 | 24 | 0 | 640 |
| 32 | 0 | 0,00% | 16 | 0 | 48 | 2048 |
| 64 | 0 | 0,00% | 16 | 0 | 48 | 4096 |

**Justificativa.**

1. O que determina o efeito do bloco é a **localidade espacial**, isto é, se as próximas palavras usadas estão no mesmo bloco. No E1 o passo é de 4 B e um bloco maior traz palavras que serão usadas logo depois. No E2 o passo é de 72 B, maior que qualquer bloco testado, e cada acesso cai num bloco diferente (os acessos 1 a 16 com bloco de 64 B são todos faltas compulsórias). Um bloco maior não traz nada útil e ainda reduz o número de linhas da cache, de 64 para 4, o que destrói o reuso entre as passadas.
2. Com bloco de 16 B, numerando os registros de 0 a 15, quatro pares disputam o mesmo índice, conforme a tabela abaixo.

| Índice | Registros | Acessos na 1ª passada |
|---:|---|---|
| 4 | 1 e 8 | 2 (0x10000048) e 9 (0x10000240) |
| 13 | 3 e 10 | 4 (0x100000D8) e 11 (0x100002D0) |
| 6 | 5 e 12 | 6 (0x10000168) e 13 (0x10000360) |
| 15 | 7 e 14 | 8 (0x100001F8) e 15 (0x100003F0) |

   Na saída, o acesso 9 cai no índice 4 e expulsa 0x10000040, o bloco do registro 1. Na segunda passada, o acesso 18 (registro 1) falta por conflito e expulsa 0x10000240 (registro 8); o acesso 25 (registro 8) faz o mesmo e expulsa 0x10000040. O padrão se repete nos pares de acessos 20 e 27, 22 e 29, 24 e 31. Os outros oito registros acertam sempre (acessos 17, 19, 21, 23, 26, 28, 30 e 32), o que dá 8 registros vezes 3 passadas, ou 24 acertos.
3. O programa faz 64 leituras de 4 B, ou seja, lê 256 B, dos quais 64 B são distintos (o primeiro campo dos 16 registros). A tabela abaixo mostra quantos bytes foram trazidos da memória para cada byte lido. Com bloco de 4 B cada byte útil é trazido uma única vez; com bloco de 64 B a cache traz 64 bytes para cada byte útil, e 60 de cada 64 bytes trazidos nunca são usados.

| Bloco (B) | Bytes trazidos | Por byte lido (÷ 256) | Por byte distinto (÷ 64) |
|---:|---:|---:|---:|
| 4 | 64 | 0,25 | 1 |
| 8 | 128 | 0,5 | 2 |
| 16 | 640 | 2,5 | 10 |
| 32 | 2048 | 8 | 32 |
| 64 | 4096 | 16 | 64 |

4. Os tipos de falta mudam da seguinte forma. Com 4 e 8 B, aparecem apenas as 16 compulsórias: os 16 blocos caem em 16 índices diferentes (nenhum dos acessos 1 a 16 expulsa outro bloco) e tudo acerta a partir da segunda passada. Com 16 B há 16 linhas para 16 blocos, mas mal distribuídos; uma cache totalmente associativa de 16 linhas guardaria todos, por isso as 24 faltas extras são de conflito. Com 32 e 64 B a cache tem só 8 ou 4 linhas para os 16 blocos usados em rodízio, e nem uma cache totalmente associativa daria conta, por isso as 48 faltas extras são de capacidade (o acesso 17 com bloco de 32 B falta por capacidade e expulsa 0x10000300).

**Conciliação.** As taxas coincidiram nas cinco configurações, e os pares de registros previstos (1 e 8, 3 e 10, 5 e 12, 7 e 14) foram confirmados pela saída. O raciocínio para 32 e 64 B, porém, errou o tipo da falta. Pensamos em registros brigando pelo mesmo índice, o que levaria a esperar faltas de conflito, mas a medição mostrou que são de capacidade: o problema não é a distribuição nos índices, e sim que 16 blocos não cabem em 8 ou 4 linhas de nenhuma forma. Faltou comparar o número de blocos usados com o número de linhas antes de olhar os índices.

### 4.3 E3: Associatividade contra conflito

**Previsão.** Taxas de 0%, 75%, 75% e 75%. Com 1 via, A[i] e B[i] caem no mesmo índice e se expulsam a cada acesso; com 2 vias ou mais, os dois blocos cabem juntos no conjunto.

**Medição** (bloco de 16 B, write-back):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 0,00% | 16 | 48 | 0 |
| 2 | 48 | 75,00% | 16 | 0 | 0 |
| 4 | 48 | 75,00% | 16 | 0 | 0 |
| 16 | 48 | 75,00% | 16 | 0 | 0 |

**Justificativa.**

1. Os dois vetores juntos ocupam 256 B e caberiam na cache, mas o segundo começa 256 B depois do primeiro, e 256 B é exatamente o período do índice com 1 via. Assim A[i] e B[i] têm o mesmo índice e só o rótulo muda. Os acessos 1 (0x10000000) e 2 (0x10000100) caem no índice 0 e o acesso 2 expulsa 0x10000000. O acesso 3 (A[1], mesmo bloco de A[0]) falta por conflito e expulsa 0x10000100, o acesso 4 expulsa A de novo, e assim até o fim: os 64 acessos faltam (16 compulsórias e 48 de conflito), e os índices 8 a 15 nunca são usados.
2. Com 2 vias há 8 conjuntos. A[i] e B[i] continuam no mesmo conjunto, mas agora existem dois lugares. Os acessos 1 e 2 faltam (compulsórias), os acessos 3 a 8 acertam e o acesso 9 (0x10000010) falta porque é um bloco novo de A. São 2 faltas e 6 acertos a cada 8 acessos, ou seja, 75%, todas as faltas compulsórias: a associatividade eliminou as 48 faltas de conflito. Com 4 e 16 vias o resultado é idêntico, porque 2 vias já bastavam para o par A[i] e B[i] e os 16 blocos cabem nas 16 linhas. Como 75% é o máximo possível com bloco de 16 B, não há o que melhorar.
3. No traço modificado (tracos/nossos/E3_dois_vetores_modificado.txt), o segundo vetor passa a começar em 0x10000080, 128 B depois do primeiro, em vez de 0x10000100. O traço mantém os mesmos 64 acessos alternados A[i], B[i]; só muda o endereço de B. O início e o fim do arquivo são mostrados abaixo.

```
# E3 modificado: o segundo vetor agora começa 128 bytes depois do primeiro (0x10000080)
R 0x10000000    # A[0]
R 0x10000080    # B[0]
R 0x10000004    # A[1]
R 0x10000084    # B[1]
R 0x10000008    # A[2]
R 0x10000088    # B[2]
...
R 0x1000007C    # A[31]
R 0x100000FC    # B[31]
```

   Com 1 via, A ocupa os índices 0 a 7 e B ocupa os índices 8 a 15, sem sobreposição. O acesso 1 (0x10000000) cai no índice 0, o acesso 2 (0x10000080) cai no índice 8 e os acessos 3 a 8 acertam; nos acessos 33 e 34, A[16] e B[16] caem nos índices 4 e 12. O resultado é 75%, o máximo possível, já que as 16 faltas restantes são compulsórias. Qualquer posição que não coloque blocos dos dois vetores no mesmo índice funciona, como 0x10000080 ou 0x10000180.

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 48 | 75,00% | 16 | 0 | 0 |

**Conciliação.** A medição coincidiu com a previsão.

### 4.4 E4: Associatividade numa varredura

**Previsão.** Taxa de 75% em todas as configurações: todas as faltas são compulsórias e a associatividade não ajuda.

**Medição** (bloco de 16 B, write-back):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 96 | 75,00% | 32 | 0 | 0 |
| 2 | 96 | 75,00% | 32 | 0 | 0 |
| 4 | 96 | 75,00% | 32 | 0 | 0 |
| 16 | 96 | 75,00% | 32 | 0 | 0 |

**Justificativa.**

1. As 32 faltas são compulsórias em todas as configurações: são os 32 blocos do vetor (512 B divididos por 16 B), cada um trazido uma única vez, nos acessos 1, 5, 9 e assim por diante até o 125. Com 1 via, o acesso 65 (0x10000100) expulsa 0x10000000; com 16 vias, o acesso 65 também expulsa 0x10000000, por ser o bloco usado há mais tempo. Em nenhum caso o bloco expulso volta a ser pedido. Como não existe reuso a preservar, a forma como a cache organiza os blocos não altera nada.
2. A associatividade elimina faltas de **conflito**: no E3, as 48 faltas de conflito desapareceram com 2 vias. Ela não elimina faltas **compulsórias**, que continuam iguais no E3 e no E4 com qualquer número de vias, porque o primeiro acesso a um bloco sempre falta. O E5 mostra ainda que ela também não elimina faltas de **capacidade**: com 16 vias, a cache totalmente associativa, as 40 faltas de capacidade continuam presentes.

**Conciliação.** A medição coincidiu com a previsão.

### 4.5 E5: Laço maior e menor que a cache

**Previsão.** Para 80 palavras, 85% com 1 via, 81,67% com 2 vias, 75% com 4 vias e 75% com 16 vias. Para 60 palavras, 91,67% em todas as configurações.

**Medição** (bloco de 16 B, write-back). Laço de 80 palavras (320 B, 20 blocos):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 204 | 85,00% | 20 | 0 | 16 |
| 2 | 196 | 81,67% | 20 | 0 | 24 |
| 4 | 180 | 75,00% | 20 | 0 | 40 |
| 16 | 180 | 75,00% | 20 | 0 | 40 |

Laço de 60 palavras (240 B, 15 blocos):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 165 | 91,67% | 15 | 0 | 0 |
| 2 | 165 | 91,67% | 15 | 0 | 0 |
| 4 | 165 | 91,67% | 15 | 0 | 0 |
| 16 | 165 | 91,67% | 15 | 0 | 0 |

**Justificativa.**

1. A ordem para 80 palavras é 1 via (85%), 2 vias (81,67%) e, empatadas, 4 e 16 vias (75%). O laço usa 20 blocos em sequência circular numa cache de 16 linhas. O LRU expulsa o bloco usado há mais tempo, e num laço circular esse é justamente o próximo que será pedido. Sempre que um conjunto recebe mais blocos do que tem vias, o LRU expulsa todos antes do reuso e tudo falta. Quanto mais vias, maior o conjunto e mais blocos entram nesse ciclo. Com 16 vias, todos os 20 blocos entram no ciclo: o acesso 81 (início da segunda passada, 0x10000000) falta e expulsa 0x10000040, o próximo da fila, e o acesso 97 (0x10000040) falta e expulsa 0x10000080; sobram apenas os acertos dentro do bloco, 3 em cada 4. Com 4 vias, cada um dos 4 conjuntos recebe 5 blocos para 4 vias e o resultado é o mesmo. Com 2 vias, os conjuntos 0 a 3 recebem 3 blocos cada (por exemplo 0x10000000, 0x10000080 e 0x10000100) e faltam sempre (acessos 81, 85, 89, 93, 113 e seguintes até o 157, todos falta de capacidade), enquanto os conjuntos 4 a 7 recebem só 2 blocos cada (por exemplo 0x10000040 e 0x100000C0), que cabem: o acesso 97 (0x10000040) acerta. Com 1 via, só os índices 0 a 3 recebem 2 blocos cada; os índices 4 a 15 têm um bloco só e sobrevivem.
2. Com 1 via, na primeira passada os blocos 0 a 15 entram sem expulsar ninguém e, a partir do acesso 65, os blocos 16 a 19 expulsam os blocos 0 a 3 (acesso 65 expulsa 0x10000000, acesso 69 expulsa 0x10000010, acesso 73 expulsa 0x10000020 e acesso 77 expulsa 0x10000030). Na segunda passada, os acessos 81, 85, 89 e 93 faltam e expulsam os blocos 16 a 19, mas do acesso 97 (0x10000040, bloco 4) até o 144 (0x100000FC, bloco 15) tudo acerta, porque os blocos 4 a 15 sobreviveram à primeira passada. Depois, os acessos 145 e 149 faltam de novo nos blocos 16 e 17. São 8 faltas de bloco por passada a partir da segunda, num total de 20 + 8 + 8 = 36 faltas e 204 acertos. O mapeamento direto não tem escolha de vítima dentro do conjunto, então não cai no ciclo do LRU nos índices 4 a 15. Essas 16 faltas aparecem como capacidade, e não conflito, porque a cache totalmente associativa de referência também falta nelas; neste traço, o mapeamento direto supera a totalmente associativa.
3. Com 60 palavras, os 15 blocos cabem nas 16 linhas em qualquer configuração: 15 índices distintos com 1 via, no máximo 2 blocos por conjunto com 2 vias e no máximo 4 com 4 vias. Só faltam as 15 compulsórias da primeira passada (as 15 faltas da saída com 1 via estão todas entre os acessos 1 e 57), e a partir do acesso 61 (0x10000000, início da segunda passada) tudo acerta: 165 de 180 acessos, ou 91,67%.
4. Para o programador, reduzir o conjunto de trabalho é mais eficaz. Com 80 palavras, aumentar as vias piora o resultado, de 85% para 81,67% e depois para 75%. Reduzir o laço para 60 palavras leva qualquer configuração a 91,67%, o máximo possível com bloco de 16 B. A medida que funciona é fazer os dados do laço caberem na cache, por exemplo dividindo o laço em blocos menores (técnica de *blocking*), pois mais vias não resolvem falta de capacidade.

**Conciliação.** As oito taxas coincidiram com a previsão. O que a previsão não antecipou foi a classificação das faltas com 1 via. No raciocínio, os blocos 16 a 19 estariam brigando com os blocos 0 a 3, o que sugeria conflito, mas o simulador marcou as 16 faltas como capacidade, porque a referência totalmente associativa também falta nelas. O erro foi tratar dois blocos no mesmo índice como sinônimo de conflito, quando a definição compara com a cache totalmente associativa, que num laço maior que a cache falta sempre.

### 4.6 E6: Políticas de escrita

**Previsão.** O write-back escreveria menos bytes no acumulador, haveria empate na cópia e o write-through escreveria menos nas escritas esparsas.

**Medição** (bloco de 16 B, 2 vias). Acumulador:

| Política | Acertos | Taxa | Compuls. | Conflito | Capac. | Escritas | Bytes escritos | Bytes lidos |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| WT | 99 | 99,00% | 1 | 0 | 0 | 50 | 200 | 16 |
| WB | 99 | 99,00% | 1 | 0 | 0 | 1 | 16 | 16 |

Cópia:

| Política | Acertos | Taxa | Compuls. | Conflito | Capac. | Escritas | Bytes escritos | Bytes lidos |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| WT | 96 | 75,00% | 32 | 0 | 0 | 64 | 256 | 512 |
| WB | 96 | 75,00% | 32 | 0 | 0 | 16 | 256 | 512 |

Escritas esparsas:

| Política | Acertos | Taxa | Compuls. | Conflito | Capac. | Escritas | Bytes escritos | Bytes lidos |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| WT | 0 | 0,00% | 32 | 0 | 0 | 32 | 128 | 512 |
| WB | 0 | 0,00% | 32 | 0 | 0 | 32 | 512 | 512 |

**Justificativa.**

1. A taxa de acerto não muda, e pelas regras do simulador não deveria mudar. As duas políticas alocam o bloco na escrita e usam o mesmo LRU; a política só decide quando os dados vão para a memória, não o que fica na cache. Acertos, faltas e expulsões são idênticos nos três pares. No acesso 66 da cópia, por exemplo, as duas políticas expulsam 0x10000400, e só no write-back aparece a marca de writeback.
2. No acumulador, o write-through faz 50 escritas de 4 B (200 B), uma por escrita do traço. No write-back, as 50 escritas acertam no mesmo bloco (acessos 2, 4 e assim por diante até o 100), que vai para a memória uma única vez, na descarga final: 1 escrita, 16 B. O write-back escreveu menos. Na cópia, o write-through faz 64 escritas de 4 B (256 B). No write-back, cada um dos 16 blocos de destino recebe 4 escritas e é gravado inteiro uma vez: 8 writebacks durante a execução (o acesso 66 expulsa 0x10000400 com writeback, o acesso 74 expulsa 0x10000410 com writeback, e assim por diante) e 8 na descarga final, total de 16 escritas e 256 B. Há empate em bytes, com 4 vezes menos transações no write-back. Nas escritas esparsas, o write-through faz 32 escritas de 4 B (128 B). No write-back, cada escrita suja um bloco inteiro por causa de uma palavra: 16 writebacks (o acesso 17 expulsa 0x10000000 com writeback, o acesso 18 expulsa 0x10000010, e assim por diante) e 16 na descarga final, total de 32 escritas e 512 B. O write-through escreveu menos.
3. Vence o write-back quando cada bloco recebe, enquanto está na cache, mais escritas do que palavras cabem nele (bloco dividido por 4), vence o write-through quando recebe menos, e há empate quando recebe exatamente esse número. No acumulador são 50 escritas por bloco, na cópia 4 e nas escritas esparsas 1, para blocos de 4 palavras.
4. Nas escritas esparsas, o programa escreveu 128 B, mas a cache leu 512 B da memória, 4 vezes mais, que é a razão entre o bloco e a palavra (16 por 4). Por causa da escrita com alocação, cada escrita que falta traz o bloco inteiro de 16 B para a cache, e os 12 bytes restantes de cada bloco nunca são usados. Sem alocação na escrita, as escritas que faltam iriam direto para a memória sem trazer o bloco: os bytes lidos cairiam para 0, não haveria expulsões nem writebacks, e o tráfego de escrita seria de 32 vezes 4 B, ou 128 B, nas duas políticas. Para esse padrão de escrever uma vez e não ler de volta, não alocar é muito melhor.

**Conciliação.** As três previsões coincidiram. A previsão de empate na cópia foi feita contando bytes (64 vezes 4 é igual a 16 vezes 16); a medição confirmou o empate em bytes e mostrou também a diferença no número de escritas (64 contra 16), que a folha não pedia.

### 4.7 Desafio: projeto de um traço

O traço (tracos/nossos/desafio.txt) acessa 4 blocos separados por 256 B, em rodízio, por 10 rodadas, sempre no offset 0, num total de 40 acessos:

```
R 0x10000000
R 0x10000100
R 0x10000200
R 0x10000300
(repetido 10 vezes)
```

**Medição** (capacidade de 256 B, bloco de 16 B, write-back):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 0,00% | 4 | 36 | 0 |
| 2 | 0 | 0,00% | 4 | 36 | 0 |
| 4 | 36 | 90,00% | 4 | 0 | 0 |

As duas condições são cumpridas: 1 e 2 vias têm a mesma taxa, 0%, que é no máximo 10%, e 4 vias têm 90%, que é pelo menos 70%.

**Como o traço foi construído.**

- **Mesmo conjunto em todas as configurações.** Os endereços dos 4 blocos diferem apenas por múltiplos de 256 B. Com bloco de 16 B, o índice usa os bits 4 a 7 com 1 via (16 conjuntos), os bits 4 a 6 com 2 vias (8 conjuntos) e os bits 4 e 5 com 4 vias (4 conjuntos). Somar 256, que é 2 elevado a 8, não altera nenhum desses bits, então os 4 blocos caem sempre no conjunto 0, como mostra a saída (índice 0 em todos os acessos).
- **Sem localidade espacial.** Cada acesso usa só o offset 0 de um bloco diferente, então nenhum acerto vem de palavra vizinha; todo acerto precisa vir de reuso temporal.
- **Por que 2 vias não bastam.** São 4 blocos em rodízio para 2 lugares, e o LRU cai no mesmo ciclo do E5. O acesso 3 expulsa 0x10000000 e o acesso 4 expulsa 0x10000100; o acesso 5 pede 0x10000000, que acabou de sair, e expulsa 0x10000200. O bloco expulso é sempre o próximo da fila, e os 40 acessos faltam. Com 1 via acontece o mesmo, pois cada bloco expulsa o anterior.
- **Por que 4 vias resolvem.** O conjunto 0 passa a ter 4 lugares para os 4 blocos. Os acessos 1 a 4 faltam (compulsórias) sem expulsar ninguém, e do acesso 5 em diante tudo acerta: 36 de 40, ou 90%.
- **Ajuste da taxa.** Com R rodadas, a taxa com 4 vias é (4R − 4)/4R. Com R = 4 já se chega a 75%; usamos R = 10 para ter folga.
- **Tipo das faltas.** As faltas de 1 e 2 vias são de conflito porque os 4 blocos (64 B) cabem com folga numa cache totalmente associativa de 256 B.

**Conciliação.** A ideia registrada na folha de previsões funcionou sem ajustes.

## 5 Conclusão

O tamanho do bloco ajuda quando há localidade espacial (E1) e prejudica quando não há (E2): sem localidade espacial, um bloco maior traz bytes inúteis e reduz o número de linhas da cache. A associatividade remove faltas de conflito (E3 e Desafio), mas não remove faltas compulsórias (E4) nem de capacidade (E5), e num laço maior que a cache pode até piorar o resultado por causa do comportamento do LRU (E5).

Reduzir o conjunto de trabalho para caber na cache mostrou-se mais eficaz do que mudar a organização da cache (E5). Por fim, a política de escrita não altera a taxa de acerto, apenas o tráfego para a memória: o write-back é melhor quando um bloco é reescrito várias vezes e o write-through quando as escritas são esparsas (E6).

Os experimentos também mostraram que a intuição de "blocos brigando pelo mesmo índice" não basta para classificar uma falta como conflito: a classificação depende sempre da comparação com uma cache totalmente associativa de mesma capacidade, como definido por Hill e Smith (1989).

## Referências

HENNESSY, John L.; PATTERSON, David A. *Arquitetura de computadores: uma abordagem quantitativa*. 6. ed. Rio de Janeiro: GEN LTC, 2019.

HILL, Mark D.; SMITH, Alan J. Evaluating associativity in CPU caches. *IEEE Transactions on Computers*, v. 38, n. 12, p. 1612-1630, dez. 1989.

PATTERSON, David A.; HENNESSY, John L. *Organização e projeto de computadores: a interface hardware/software*. 5. ed. Rio de Janeiro: GEN LTC, 2017.

STALLINGS, William. *Arquitetura e organização de computadores*. 10. ed. São Paulo: Pearson, 2017.
