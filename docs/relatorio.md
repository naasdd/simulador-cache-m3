# Relatório — Simulador de Cache e Experimentos com Estratégias de Organização

**UNIVALI — Organização de Computadores — Prof. Thiago Felski Pereira, M.Sc.**
**Integrantes:** José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
**Código-fonte:** https://github.com/naasdd/simulador-cache-m3

---

## 1. O simulador

O simulador foi escrito em C++17 e está dividido em cinco arquivos:

| Arquivo | O que faz |
|---|---|
| `src/main.cpp` | laço principal: manda cada acesso para a cache e para a cache sombra, classifica a falta, imprime a saída por acesso e o resumo |
| `src/cache.cpp` / `include/cache.hpp` | a classe `Cache`: divisão do endereço, procura no conjunto, LRU, bit de modificação, expulsão |
| `src/leitor_de_traco.cpp` | lê o arquivo de traço (`R`/`W` + endereço hexadecimal, `#` é comentário) |
| `src/parametros.cpp` | lê a linha de comando e recusa configurações impossíveis |
| `src/ferramentas.cpp` | potência de 2, log2 e mensagem de erro |

**Como executar:**

```bash
make
./simulador --traco tracos/experimentos/E1_varredura.txt --capacidade 256 --bloco 16 --vias 1 --escrita WB
./calibrar.sh            # roda C1–C4 e compara com as respostas
./rodar_experimentos.sh  # roda todas as configurações dos experimentos e gera resultados/tabelas.md
```

**Decisões de implementação:**

- **LRU:** a cache tem um relógio que aumenta 1 a cada acesso. Cada linha guarda `momentoDoUltimoUso`, que é atualizado tanto no acerto quanto no preenchimento. Na falta, a cache usa primeiro uma linha inválida. Se não houver nenhuma, sai a de menor `momentoDoUltimoUso`.
- **Write-allocate:** na falta, o bloco é sempre trazido, seja em leitura ou em escrita. No write-back, a linha nasce com D = 1 se o acesso que a trouxe foi uma escrita.
- **Tráfego:** "blocos lidos" é igual ao número de faltas. No WT, cada `W` soma 4 B. No WB, cada expulsão com D = 1 soma um bloco, e no fim são contadas as linhas que ainda têm D = 1 (descarga final).
- **Classificação das faltas:** uma segunda `Cache`, com uma via por bloco (totalmente associativa, mesma capacidade e mesmo bloco), recebe todos os acessos. Um `std::set` guarda os blocos que já apareceram no traço. Cada falta é classificada assim:
  - bloco que nunca apareceu → **compulsória**;
  - a sombra acertou → **conflito**;
  - a sombra também faltou → **capacidade**.
- **Validação:** o programa recusa valores que não são potência de 2, bloco menor que 4 B, bloco maior que a capacidade e mais vias do que blocos.

## 2. Calibração

Os quatro traços reproduzem exatamente os arquivos de resposta. O `calibrar.sh` usa `diff` byte a byte; o C3 é rodado em WB e em WT.

```
  OK     C1_mapeamento_direto
  OK     C2_associativa_LRU
  OK     C3_escrita
  OK     C4_classificacao
Calibracao: 4 de 4
```

---

## 3. Experimentos

Em todos os experimentos a capacidade é 256 B. A saída completa por acesso de cada configuração está em `resultados/<experimento>/`, e os números de acesso citados abaixo vêm desses arquivos.

### E1 — Tamanho do bloco numa varredura

**Previsão.** 0%, 50%, 75%, 87,5% e 93,75% para blocos de 4 a 64 B. Cada bloco é lido uma vez: a primeira palavra falta e as outras acertam, então taxa = (p − 1)/p. Bytes lidos sempre 512 B.

**Medição** (1 via, WB):

| Bloco (B) | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Bytes lidos |
|---:|---:|---:|---:|---:|---:|---:|
| 4 | 0 | 0,00% | 128 | 0 | 0 | 512 |
| 8 | 64 | 50,00% | 64 | 0 | 0 | 512 |
| 16 | 96 | 75,00% | 32 | 0 | 0 | 512 |
| 32 | 112 | 87,50% | 16 | 0 | 0 | 512 |
| 64 | 120 | 93,75% | 8 | 0 | 0 | 512 |

**Justificativa.**

1. Com *p* = B/4 palavras por bloco, cada bloco recebe *p* acessos seguidos: 1 falta e *p* − 1 acertos. Então **taxa = (p − 1)/p = 1 − 4/B**. Conferindo:
   - p = 1 → 0%;
   - p = 2 → 50%;
   - p = 4 → 75%;
   - p = 8 → 87,5%;
   - p = 16 → 93,75%.

   Na saída com bloco 16 B (`resultados/E1/bloco_16.txt`), o acesso 1 (0x10000000) falta e os acessos 2, 3 e 4 acertam no mesmo índice 0. O acesso 5 (0x10000010) falta no índice 1, e o padrão se repete.
2. **Só apareceram faltas compulsórias.** Cada bloco do vetor é usado uma única vez e nunca é procurado de novo, então nenhuma falta pode ser de conflito ou de capacidade. O vetor tem 512 B, o dobro da cache, e a partir do acesso 65 (0x10000100, com bloco 16 B) os blocos novos expulsam os antigos ("expulsa 0x10000000"). Mesmo assim isso não gera falta extra, porque os blocos expulsos não voltam a ser lidos.
3. Os bytes lidos ficaram em **512 B em todas as configurações**: o vetor inteiro é trazido exatamente uma vez, só que em pedaços maiores ou menores. Neste traço, aumentar o bloco **não tem custo em tráfego**. Ele só reduz o número de faltas (de 128 para 8), porque todo byte trazido é usado.

**Conciliação.** A medição bateu com a previsão.

---

### E2 — Tamanho do bloco com dados espalhados

**Previsão.** 75%, 75%, 37,5%, 0% e 0%. O raciocínio foi este: o passo de 72 B não deixa localidade espacial, e os acertos dependem de os 16 blocos caberem sem disputar índice. Com 16 B, 4 pares disputam; com 32 e 64 B, todos os índices teriam 2 ou mais registros.

**Medição** (1 via, WB):

| Bloco (B) | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Bytes lidos |
|---:|---:|---:|---:|---:|---:|---:|
| 4 | 48 | 75,00% | 16 | 0 | 0 | 64 |
| 8 | 48 | 75,00% | 16 | 0 | 0 | 128 |
| 16 | 24 | 37,50% | 16 | 24 | 0 | 640 |
| 32 | 0 | 0,00% | 16 | 0 | 48 | 2048 |
| 64 | 0 | 0,00% | 16 | 0 | 48 | 4096 |

**Justificativa.**

1. **Comparação com o E1.** O que decide o efeito do bloco é a **localidade espacial**: as próximas palavras usadas estão ou não no mesmo bloco.
   - No E1 o passo é de 4 B, então um bloco maior traz palavras que vão ser usadas logo depois. Aumentar o bloco só ajuda.
   - No E2 o passo é de 72 B, maior que qualquer bloco testado, então cada acesso cai num bloco diferente (acessos 1 a 16 de `bloco_64.txt` são todos faltas compulsórias). Um bloco maior não traz nada útil e ainda **diminui o número de linhas** da cache (64 → 4), o que destrói o reuso temporal entre as passadas.

2. **Registros que disputam índice com bloco 16 B** (registros numerados de 0 a 15; o registro *r* é o acesso *r* + 1 da passada):

   | Índice | Registros | Acessos (1ª passada) |
   |---:|---|---|
   | 4  | 1 e 8  | 2 (0x10000048) e 9 (0x10000240) |
   | 13 | 3 e 10 | 4 (0x100000D8) e 11 (0x100002D0) |
   | 6  | 5 e 12 | 6 (0x10000168) e 13 (0x10000360) |
   | 15 | 7 e 14 | 8 (0x100001F8) e 15 (0x100003F0) |

   Na saída (`resultados/E2/bloco_16.txt`):
   - O acesso 9 cai no idx 4 e "expulsa 0x10000040", que é o bloco do registro 1.
   - Na 2ª passada, o acesso 18 (registro 1) falta por **conflito** e "expulsa 0x10000240" (registro 8). O acesso 25 (registro 8) devolve o favor e "expulsa 0x10000040".
   - O mesmo acontece em 20/27 (idx 13), 22/29 (idx 6) e 24/31 (idx 15).
   - Os outros 8 registros acertam sempre: acessos 17, 19, 21, 23, 26, 28, 30 e 32.
   - Resultado: 8 registros × 3 passadas = 24 acertos.

3. **Bytes trazidos por byte lido.** O programa faz 64 leituras de 4 B, ou seja, 256 B lidos (64 B distintos: o primeiro campo dos 16 registros).

   | Bloco | Bytes trazidos | por byte lido (÷ 256) | por byte distinto útil (÷ 64) |
   |---:|---:|---:|---:|
   | 4  | 64   | 0,25 | 1  |
   | 8  | 128  | 0,5  | 2  |
   | 16 | 640  | 2,5  | 10 |
   | 32 | 2048 | 8    | 32 |
   | 64 | 4096 | 16   | 64 |

   - Com bloco 4 B, cada byte útil é trazido uma única vez.
   - Com bloco 64 B, a cache traz 64 bytes para cada byte útil: 60 de cada 64 bytes trazidos são lixo, e o mesmo bloco é trazido de novo a cada passada.

4. **Tipos de falta em cada configuração:**
   - **4 B e 8 B:** só as 16 compulsórias. Os 16 blocos caem em 16 índices diferentes (acessos 1 a 16 sem nenhum "expulsa"), então da 2ª passada em diante tudo acerta.
   - **16 B:** são 16 linhas e 16 blocos, mas mal distribuídos, com 4 pares no mesmo índice. Uma cache totalmente associativa de 16 linhas guardaria os 16 blocos, então as 24 faltas extras são de **conflito**.
   - **32 B e 64 B:** a cache tem só 8 ou 4 linhas, e o traço usa 16 blocos em rodízio. Nem uma cache totalmente associativa com 8 ou 4 linhas guardaria os 16 blocos (o LRU expulsa sempre o que vai ser usado depois), então as 48 faltas extras são de **capacidade**. Exemplo: o acesso 17 de `bloco_32.txt` falta por capacidade e "expulsa 0x10000300".

**Conciliação.**
- As taxas bateram nas cinco configurações, e os pares de registros previstos (1/8, 3/10, 5/12, 7/14) foram confirmados pela saída.
- O raciocínio para 32 B e 64 B estava errado no **tipo** da falta. Pensamos em "registros brigando pelo mesmo índice", o que levaria a esperar faltas de conflito. A medição mostrou que são de **capacidade**: o problema não é a distribuição nos índices, e sim que 16 blocos não cabem em 8 ou 4 linhas de jeito nenhum. Faltou comparar o número de blocos usados com o número de linhas antes de olhar os índices.

---

### E3 — Associatividade contra conflito

**Previsão.** 0%, 75%, 75% e 75%. Com 1 via, A[i] e B[i] caem no mesmo índice e se expulsam a cada acesso. Com 2 vias ou mais, cabem juntos.

**Medição** (bloco 16 B, WB):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 0,00% | 16 | 48 | 0 |
| 2 | 48 | 75,00% | 16 | 0 | 0 |
| 4 | 48 | 75,00% | 16 | 0 | 0 |
| 16 | 48 | 75,00% | 16 | 0 | 0 |

**Justificativa.**

1. **Mapeamento direto.** Os dois vetores juntos têm 256 B, o tamanho exato da cache, então em tese caberiam. O problema é a posição: B começa 256 B depois de A, e 256 B é exatamente o "período" do índice com 1 via. Assim A[i] e B[i] têm o mesmo índice e só o rótulo muda.
   - Em `resultados/E3/vias_1.txt`, o acesso 1 (0x10000000) e o acesso 2 (0x10000100) caem no idx 0, e o 2 "expulsa 0x10000000".
   - O acesso 3 (A[1], mesmo bloco de A[0]) falta por **conflito** e "expulsa 0x10000100". O acesso 4 expulsa A de novo, e assim por diante.
   - Os 64 acessos faltam (16 compulsórias + 48 conflito). Metade da cache (índices 8 a 15) **nunca é usada**.
2. **2, 4 e 16 vias.** Com 2 vias há 8 conjuntos, e A[i] e B[i] continuam caindo no mesmo conjunto, mas agora existem duas vias.
   - Em `vias_2.txt`, os acessos 1 e 2 faltam (compulsórias) e os acessos 3 a 8 acertam todos.
   - O acesso 9 (0x10000010) falta porque é um bloco novo de A.
   - Por bloco são 8 acessos, 2 faltas e 6 acertos, ou seja, 75%. Todas as faltas são compulsórias: a associatividade eliminou as 48 de conflito.
   - Com 4 e 16 vias o resultado é idêntico, porque 2 vias já bastavam para o par A[i]/B[i] e todos os blocos cabem (16 blocos = 16 linhas). Mais vias não têm o que melhorar, já que 75% é o máximo possível com bloco de 16 B (o limite das faltas compulsórias).
3. **Traço modificado** (`tracos/nossos/E3_dois_vetores_modificado.txt`). O segundo vetor passa a começar em **0x10000080**, 128 B depois do primeiro. Com 1 via e bloco 16 B, A ocupa os índices 0 a 7 e B os índices 8 a 15, sem nenhuma sobreposição.

   | Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
   |---:|---:|---:|---:|---:|---:|
   | 1 | 48 | 75,00% | 16 | 0 | 0 |

   - Em `resultados/E3_modificado/vias_1.txt`, o acesso 1 (0x10000000) cai no idx 0 e o acesso 2 (0x10000080) no idx 8. Os acessos 3 a 8 acertam.
   - Nos acessos 33 e 34, A[16] e B[16] caem nos idx 4 e 12, de novo sem disputa.
   - 75% é o máximo possível: as 16 faltas restantes são compulsórias e nenhuma organização elimina essas.
   - Qualquer posição de B que não deixe os blocos dos dois vetores no mesmo índice funciona, por exemplo 0x10000080 ou 0x10000180.

**Conciliação.** A medição bateu com a previsão.

---

### E4 — Associatividade numa varredura

**Previsão.** 75% em todas as configurações: todas as faltas são compulsórias e a associatividade não ajuda.

**Medição** (bloco 16 B, WB):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 96 | 75,00% | 32 | 0 | 0 |
| 2 | 96 | 75,00% | 32 | 0 | 0 |
| 4 | 96 | 75,00% | 32 | 0 | 0 |
| 16 | 96 | 75,00% | 32 | 0 | 0 |

**Justificativa.**

1. As 32 faltas são compulsórias em todas as configurações: são os 32 blocos do vetor (512 B ÷ 16 B), cada um trazido uma única vez (acessos 1, 5, 9, ..., 125).
   - Com 1 via, o acesso 65 (0x10000100) "expulsa 0x10000000". Com 16 vias, o acesso 65 também "expulsa 0x10000000", porque é o LRU.
   - Em nenhum caso o bloco expulso volta a ser pedido. Não existe reuso a ser preservado, então a forma como a cache organiza os blocos não muda nada.
2. **O que a associatividade consegue fazer:**
   - Ela **elimina faltas de conflito**: no E3, as 48 de conflito sumiram com 2 vias.
   - Ela **não elimina faltas compulsórias**: no E4 e no E3, as compulsórias continuam iguais com qualquer número de vias. O primeiro acesso a um bloco sempre falta.
   - Pelo E5, também **não elimina faltas de capacidade**. Com 16 vias, a cache totalmente associativa, as 40 faltas de capacidade continuam lá.

**Conciliação.** A medição bateu com a previsão.

---

### E5 — Laço maior e menor que a cache

**Previsão.**
- 80 palavras: 85% (1 via), 81,67% (2 vias), 75% (4 vias) e 75% (16 vias).
- 60 palavras: 91,67% em todas.

**Medição** (bloco 16 B, WB):

Laço de 80 palavras (320 B = 20 blocos):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 204 | 85,00% | 20 | 0 | 16 |
| 2 | 196 | 81,67% | 20 | 0 | 24 |
| 4 | 180 | 75,00% | 20 | 0 | 40 |
| 16 | 180 | 75,00% | 20 | 0 | 40 |

Laço de 60 palavras (240 B = 15 blocos):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 165 | 91,67% | 15 | 0 | 0 |
| 2 | 165 | 91,67% | 15 | 0 | 0 |
| 4 | 165 | 91,67% | 15 | 0 | 0 |
| 16 | 165 | 91,67% | 15 | 0 | 0 |

**Justificativa.**

1. **Ordem para 80 palavras:** 1 via (85%) > 2 vias (81,67%) > 4 vias = 16 vias (75%).

   O laço usa 20 blocos em sequência circular e a cache tem 16 linhas. O LRU expulsa o bloco usado há mais tempo, mas num laço circular **o bloco usado há mais tempo é justamente o próximo que vai ser pedido**. Por isso, sempre que um conjunto recebe mais blocos do que tem vias, o LRU expulsa todos eles antes do reuso e tudo falta. Quanto mais vias, mais blocos compartilham o mesmo conjunto (o conjunto fica "grande"), e mais blocos entram nesse ciclo ruim:
   - **16 vias (1 conjunto com os 20 blocos):** todos entram no ciclo.
     - Em `E5_80/vias_16.txt`, o acesso 81 (início da 2ª passada, bloco 0x10000000) falta e "expulsa 0x10000040", o próximo da fila.
     - O acesso 97 (0x10000040) falta e "expulsa 0x10000080".
     - Só sobram os acertos dentro do bloco: 3 de cada 4, ou seja, 75%.
   - **4 vias (4 conjuntos com 5 blocos cada):** 5 > 4 em todos os conjuntos, então tudo falta do mesmo jeito (acesso 81 "expulsa 0x10000040", que é do mesmo conjunto). Resultado: 75%.
   - **2 vias (8 conjuntos):**
     - Os conjuntos 0 a 3 recebem 3 blocos cada, por exemplo blocos 0, 8 e 16 = 0x10000000, 0x10000080 e 0x10000100. É o ciclo, e esses conjuntos faltam sempre: acessos 81, 85, 89, 93, 113, ..., 157 de `vias_2.txt`, todos "FALTA capacidade".
     - Os conjuntos 4 a 7 recebem só 2 blocos cada, por exemplo blocos 4 e 12 = 0x10000040 e 0x100000C0. Eles cabem e acertam da 2ª passada em diante: o acesso 97 (0x10000040) acerta.
     - São 12 blocos faltando por passada e 8 sobrevivendo: 81,67%.
   - **1 via (16 conjuntos):** só os índices 0 a 3 recebem 2 blocos cada (blocos 0/16, 1/17, 2/18, 3/19). Os índices 4 a 15 têm um bloco só e sobrevivem.
2. **Por que 1 via se saiu melhor** (`resultados/E5_80/vias_1.txt`):
   - **1ª passada:** os blocos 0 a 15 entram sem expulsar ninguém. Do acesso 65 em diante, os blocos 16 a 19 expulsam os blocos 0 a 3:
     - acesso 65 (0x10000100) "expulsa 0x10000000";
     - acesso 69 "expulsa 0x10000010";
     - acesso 73 "expulsa 0x10000020";
     - acesso 77 "expulsa 0x10000030".
   - **2ª passada:** os acessos 81, 85, 89 e 93 (blocos 0 a 3) faltam e expulsam os blocos 16 a 19 ("expulsa 0x10000100", "... 0x10000110", etc.). Já do acesso 97 (0x10000040, bloco 4) até o 144 (0x100000FC, bloco 15), **tudo acerta**: os blocos 4 a 15 sobreviveram da 1ª passada. Depois, os acessos 145 e 149 faltam de novo nos blocos 16 e 17.
   - **Conta:** são 8 faltas de bloco por passada a partir da 2ª. Total de faltas = 20 + 8 + 8 = 36, então acertos = 204 (85%).
   - O mapeamento direto não tem LRU dentro do conjunto (cada índice tem um lugar só), então ele **não consegue cair no ciclo nos índices 4 a 15**: cada um desses blocos tem seu lugar fixo e ninguém o expulsa.
   - Essas 16 faltas aparecem como **capacidade**, não conflito, porque a cache totalmente associativa de referência também falta nelas (ela falta em tudo). Neste traço, a 1 via é **melhor** que a totalmente associativa.
3. **60 palavras (15 blocos ≤ 16 linhas):** o laço inteiro cabe na cache em qualquer configuração.
   - 1 via: 15 índices diferentes.
   - 2 vias: no máximo 2 blocos por conjunto.
   - 4 vias: no máximo 4 blocos por conjunto.
   - Só faltam as 15 compulsórias da 1ª passada (`E5_60/vias_1.txt` tem 15 linhas com FALTA, todas nos acessos 1 a 60). A partir do acesso 61 (0x10000000, início da 2ª passada) tudo acerta: 165/180 = 91,67%.
4. **Mais vias ou conjunto de trabalho menor?** Conjunto de trabalho menor.
   - Com 80 palavras, aumentar as vias **piora** o resultado: de 85% para 81,67% e depois para 75%.
   - Reduzir o laço para 60 palavras leva **qualquer** configuração a 91,67%, o máximo possível com bloco de 16 B (só as compulsórias).
   - Para o programador, a medida que funciona é fazer os dados do laço caberem na cache, por exemplo dividindo o laço em pedaços (*blocking/tiling*). Mais vias não resolvem falta de capacidade.

**Conciliação.**
- As oito taxas bateram com a previsão.
- O que a previsão não antecipou foi a **classificação**. No raciocínio do 1 via, descrevemos os blocos 16 a 19 "brigando" com os blocos 0 a 3, o que sugeria faltas de conflito. O simulador marcou as 16 faltas como capacidade, porque a referência totalmente associativa também falta nelas.
- O erro foi tratar "dois blocos no mesmo índice" como sinônimo de conflito. A definição compara com a cache totalmente associativa, e num laço maior que a cache essa cache falta sempre.

---

### E6 — Políticas de escrita

**Previsão.** WB escreve menos no acumulador, empate na cópia, WT escreve menos nas escritas esparsas.

**Medição** (bloco 16 B, 2 vias):

Acumulador:

| Política | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Escritas | Bytes escritos | Bytes lidos |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| WT | 99 | 99,00% | 1 | 0 | 0 | 50 | 200 | 16 |
| WB | 99 | 99,00% | 1 | 0 | 0 | 1 | 16 | 16 |

Cópia:

| Política | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Escritas | Bytes escritos | Bytes lidos |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| WT | 96 | 75,00% | 32 | 0 | 0 | 64 | 256 | 512 |
| WB | 96 | 75,00% | 32 | 0 | 0 | 16 | 256 | 512 |

Escritas esparsas:

| Política | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. | Escritas | Bytes escritos | Bytes lidos |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| WT | 0 | 0,00% | 32 | 0 | 0 | 32 | 128 | 512 |
| WB | 0 | 0,00% | 32 | 0 | 0 | 32 | 512 | 512 |

**Justificativa.**

1. **A taxa de acerto não muda, e pelas regras não deveria mudar.** Nas duas políticas a escrita aloca o bloco (write-allocate) e o LRU é o mesmo. A política só decide **quando** os dados vão para a memória, não **o que** fica na cache. Os acertos, as faltas e as expulsões são idênticos nos três pares, só muda o tráfego de escrita. Por exemplo, no acesso 66 da cópia, as duas políticas expulsam 0x10000400; só no WB aparece "(writeback)".
2. **Comparação por traço:**
   - **Acumulador:** o WT faz 50 escritas de 4 B (200 B), uma por `W`. No WB, as 50 escritas acertam no mesmo bloco (acessos 2, 4, ..., 100 em `E6_acumulador/WB.txt`), que só vai para a memória uma vez, na descarga final: 1 escrita, 16 B. **WB escreveu menos.**
   - **Cópia:** o WT faz 64 escritas de 4 B (256 B). No WB, cada um dos 16 blocos de destino recebe 4 escritas e é gravado inteiro uma vez: 8 writebacks durante a execução (acesso 66 "expulsa 0x10000400 (writeback)", acesso 74 "... 0x10000410 (writeback)", ...) mais 8 na descarga final. São 16 escritas, 256 B. **Empate em bytes**, mas o WB fez 4× menos transações.
   - **Esparsas:** o WT faz 32 escritas de 4 B (128 B). No WB, cada escrita suja um bloco inteiro de 16 B só por causa de uma palavra: 16 writebacks (acesso 17 "expulsa 0x10000000 (writeback)", acesso 18, ...) mais 16 na descarga final. São 32 escritas, 512 B. **WT escreveu menos.**
3. **Em uma frase:** vence a política cujo custo por escrita é menor, e isso depende de **quantas escritas cada bloco recebe enquanto está na cache**. Se cada bloco sujo recebe mais escritas do que cabem palavras nele (bloco ÷ 4), vence o write-back; se recebe menos, vence o write-through; se recebe exatamente isso, empata.
   - Acumulador: 50 escritas por bloco, mais que 4.
   - Cópia: 4 escritas por bloco, igual a 4.
   - Esparsas: 1 escrita por bloco, menos que 4.
4. **Bytes lidos × bytes escritos nas esparsas.** O programa escreveu 128 B (32 palavras), mas a cache **leu 512 B** da memória, 4× mais. É a razão bloco/palavra (16 ÷ 4). Por causa do write-allocate, cada escrita que falta traz o bloco inteiro de 16 B para a cache (os 32 "FALTA compulsoria"), e nenhum desses 12 bytes extras por bloco é usado.

   Sem alocação na escrita (*write-no-allocate*), as escritas que faltam iriam direto para a memória sem trazer o bloco:
   - os **bytes lidos cairiam para 0**;
   - nada seria colocado na cache, então não haveria expulsões nem writebacks;
   - o tráfego de escrita seria 32 × 4 = 128 B nas duas políticas.

   Para esse padrão (escrever uma vez e não ler de volta), não alocar é muito melhor.

**Conciliação.** As três previsões bateram. A previsão de empate na cópia foi feita contando bytes (64 × 4 = 16 × 16). A medição confirmou o empate em bytes e mostrou uma diferença que a folha não pedia: o número de escritas (64 × 16).

---

### Desafio — Projetem o traço

**Traço** (`tracos/nossos/desafio.txt`): 4 blocos separados por 256 B, acessados em rodízio por 10 rodadas, sempre no offset 0 (40 acessos):

```
R 0x10000000   # A
R 0x10000100   # B = A + 256
R 0x10000200   # C = A + 512
R 0x10000300   # D = A + 768
... (repete 10 vezes)
```

**Medição** (capacidade 256 B, bloco 16 B, WB):

| Vias | Acertos | Taxa de acerto | Compuls. | Conflito | Capac. |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 0,00% | 4 | 36 | 0 |
| 2 | 0 | 0,00% | 4 | 36 | 0 |
| 4 | 36 | 90,00% | 4 | 0 | 0 |

As duas condições são cumpridas: 1 e 2 vias com a mesma taxa (0% ≤ 10%), e 4 vias com 90% (≥ 70%).

**Como foi construído.**
- **Mesmo conjunto em todas as configurações.** O endereço dos 4 blocos difere só em múltiplos de 256 B. Com bloco de 16 B, o índice usa os bits 4 a 7 (1 via, 16 conjuntos), 4 a 6 (2 vias, 8 conjuntos) ou 4 a 5 (4 vias, 4 conjuntos). Somar 256 = 2⁸ não mexe em nenhum desses bits, então os 4 blocos caem sempre no **conjunto 0** (idx=0 em todas as linhas da saída).
- **Sem localidade espacial.** Cada acesso usa só o offset 0 de um bloco diferente, então nenhum acerto vem de "palavra vizinha". Todo acerto tem que vir de reuso temporal.
- **Por que 2 vias não bastam:** são 4 blocos em rodízio disputando 2 lugares, e o LRU cai no mesmo ciclo do E5.
  - Em `resultados/Desafio/vias_2.txt`, o acesso 3 (C) "expulsa 0x10000000" (A) e o acesso 4 (D) "expulsa 0x10000100" (B).
  - O acesso 5 pede A, que acabou de sair, e "expulsa 0x10000200" (C).
  - O bloco expulso é sempre o próximo da fila, então os 40 acessos faltam. Com 1 via acontece o mesmo, porque cada bloco expulsa o anterior.
- **Por que 4 vias resolvem:** o conjunto 0 passa a ter 4 lugares para 4 blocos. Em `vias_4.txt`, os acessos 1 a 4 faltam (compulsórias) e **nenhum expulsa ninguém**. Do acesso 5 em diante tudo acerta: 36/40 = 90%.
- **Ajustando a taxa:** com *R* rodadas, a taxa com 4 vias é (4R − 4)/4R. Com R = 4 já dá 75%. Usamos R = 10 para ficar com folga.
- **Por que as faltas de 1 e 2 vias são de conflito:** os 4 blocos (64 B) cabem com folga numa cache totalmente associativa de 256 B.

**Conciliação.** A ideia da folha de previsões funcionou sem ajuste.

---

## 4. Conclusões

- **Tamanho do bloco:** ajuda quando há localidade espacial (E1) e atrapalha quando não há (E2). Sem localidade espacial, um bloco maior traz bytes inúteis e diminui o número de linhas.
- **Associatividade:** remove faltas de conflito (E3, Desafio). Não remove faltas compulsórias (E4) nem de capacidade (E5), e num laço maior que a cache pode até piorar o resultado por causa do LRU (E5).
- **Conjunto de trabalho:** reduzir o conjunto de trabalho para caber na cache é mais eficaz do que mudar a organização da cache (E5).
- **Política de escrita:** não altera a taxa de acerto. Ela define o tráfego: WB é melhor quando o bloco é reescrito várias vezes, WT quando as escritas são esparsas (E6).
