# 📚 M3 Org Comp — Simulador de Cache: tudo pra defesa

> **Como usar:** a **Parte 1** dá uns 30 minutos de leitura e explica o trabalho do zero: o que é cache, o que o professor pediu, o que ele deu, o que a gente fez e o que dizer de cada experimento. A **Parte 2** é a explicação técnica do código, arquivo por arquivo, no formato de um code review, com os pontos fortes e as fraquezas que vale conhecer antes que o professor pergunte.

**Grupo:** José Gabriel Santos Gomes · Matheus Pompeo Dias · Nathan Gustavo Reichert
**Repositório:** https://github.com/naasdd/simulador-cache-m3
**Entrega + defesa:** provavelmente **26/11**. A anotação da aula de 9/11 dizia 30/11 (dia da prova), então **confirmar no AVA**. Ao chegar, coloca o nome no quadro pra apresentar. Defesa ou entrega na aula seguinte vale só 70%.

---

# PARTE 1 — Leitura (≈ 30 min)

## 1. O trabalho em uma página

O trabalho é construir um **simulador de cache** e usar ele como instrumento de medição. A frase central do enunciado é: *"aqui o programa é o instrumento"*. O código só precisa estar certo. **A maior parte da nota vem do relatório**, ou seja, de medir o efeito de cada decisão de projeto da cache e explicar por que os números saíram como saíram.

**Distribuição da nota (10,0 pontos):**

| Item | Pontos |
|---|---|
| Calibração C1–C4 | 2,0 (0,5 por traço reproduzido **exatamente**) |
| E1 — bloco numa varredura | 0,5 |
| E2 — bloco com dados espalhados | 1,5 |
| E3 — associatividade contra conflito | 1,0 |
| E4 — associatividade numa varredura | 0,75 |
| E5 — laço maior e menor que a cache | 1,5 |
| E6 — políticas de escrita | 1,25 |
| Desafio — projetar um traço | 1,5 |

Em cada experimento, a nota se divide em **30% medição, 50% justificativa e 20% conciliação**. Ou seja, metade da nota de cada experimento está em explicar.

**Três regras de ouro do enunciado:**
1. **Justificativa tem que citar a saída.** Algo como "os acessos 3, 5 e 7 caem no índice 0 e expulsam um ao outro". Justificativa que não aponta número de acesso não ganha nota inteira.
2. **Errar a previsão não tira ponto. Não explicar o erro tira.** Uma previsão errada seguida de uma boa explicação de onde o raciocínio falhou vale a nota inteira.
3. **A defesa é individual.** O professor pode ajustar a nota de cada um. **Os três precisam entender tudo**, não só a parte que cada um fez.

> 💡 Se a calibração de alguma regra falhar, os experimentos que dependem dela valem metade. A nossa calibração passa nos 4 traços, e o `./calibrar.sh` mostra isso na frente do professor.

---

## 2. Cache do zero (a teoria que você precisa ter na ponta da língua)

### 2.1 Por que existe cache

A memória principal (DRAM) é grande e lenta, e o processador é muito mais rápido que ela. A cache é uma memória pequena e rápida que fica no meio e guarda **cópias** de pedaços da memória que provavelmente vão ser usados de novo logo. Ela funciona porque programas reais têm **localidade**:

- **Localidade temporal:** o que foi usado agora tende a ser usado de novo em breve. Exemplos: um contador num laço, ou um vetor percorrido várias vezes.
- **Localidade espacial:** o que está *perto* do que foi usado tende a ser usado em breve. Exemplo: percorrer um vetor em sequência, `v[0], v[1], v[2]...`.

A cache aproveita a **localidade espacial** trazendo um **bloco** inteiro (várias palavras vizinhas) a cada falta, e aproveita a **localidade temporal** mantendo os blocos lá dentro enquanto der.

### 2.2 Vocabulário

- **Palavra:** 4 bytes. No trabalho, todo acesso lê ou escreve uma palavra alinhada (endereço múltiplo de 4).
- **Bloco (ou linha):** a unidade que vai e volta entre a memória e a cache. Pode ter 4, 8, 16, 32 ou 64 B, ou seja, 1, 2, 4, 8 ou 16 palavras.
- **Capacidade:** quantos bytes de *dados* cabem na cache. Rótulos e bits de controle não contam. Nos experimentos é sempre **256 B**.
- **Número de blocos da cache** = capacidade ÷ bloco. Com 256 B e bloco de 16 B, são **16 linhas**.
- **Vias (associatividade):** quantos lugares diferentes um bloco pode ocupar dentro do seu conjunto.
- **Conjuntos** = capacidade ÷ (bloco × vias).

| Organização | Vias | Conjuntos (256 B, bloco 16 B) |
|---|---|---|
| Mapeamento direto | 1 | 16 |
| Associativa de 2 vias | 2 | 8 |
| Associativa de 4 vias | 4 | 4 |
| Totalmente associativa | 16 (= nº de blocos) | 1 |

### 2.3 Divisão do endereço: rótulo | índice | offset

O endereço de 32 bits é dividido em três pedaços:

```
| rótulo (o resto dos bits) | índice (log2 conjuntos) | offset (log2 bloco) |
```

- **Offset:** qual byte dentro do bloco. Com bloco de 16 B são 4 bits.
- **Índice:** em qual conjunto o bloco tem que estar. Com 16 conjuntos são 4 bits.
- **Rótulo (tag):** identifica *qual* dos vários blocos de memória que caem naquele conjunto está ali agora.

**Exemplo do C1** (128 B, bloco 16 B, 1 via → 8 conjuntos):
- offset = 4 bits, índice = 3 bits, rótulo = 32 − 3 − 4 = **25 bits**;
- o endereço `0x10000084` em binário termina em `...1000 0100`;
- offset = os 4 últimos bits = `0100` = **4**;
- índice = os 3 bits seguintes = `000` = **0**;
- rótulo = o resto = `0x10000084 >> 7` = **0x0200001**.

A saída do professor mostra exatamente isso: `rot=0x0200001 idx=0 off=4`.

> 💡 **O truque que resolve metade dos experimentos:** dois endereços caem no **mesmo índice** quando a diferença entre eles é múltiplo de *(bloco × conjuntos)*. Com 1 via isso é o próprio tamanho da cache, 256 B. Por isso, no E3, o segundo vetor 256 B depois do primeiro dá briga em todo índice.

### 2.4 Acerto, falta, validade, expulsão

A cada acesso, a cache faz isto:
1. Calcula o índice e olha **só aquele conjunto**.
2. Se alguma via estiver **válida** (V = 1) e tiver o **mesmo rótulo**, é **ACERTO**.
3. Se nenhuma estiver, é **FALTA**: o bloco é trazido da memória e colocado numa via do conjunto.
   - Se houver uma via vazia (V = 0), o bloco vai pra ela.
   - Se estiverem todas ocupadas, alguém tem que sair: isso é a **expulsão**.

A cache **começa vazia** (todos os V = 0).

### 2.5 LRU (Least Recently Used)

Quando o conjunto está cheio, sai o bloco **usado há mais tempo**. O "usado" inclui acertos: um acerto renova o bloco.

O C2 existe justamente pra pegar quem implementou FIFO (sai quem *chegou* primeiro) no lugar do LRU:

```
1 R 0x10000000  falta  (A entra)
2 R 0x10000020  falta  (B entra)       conjunto 0 = {A, B}
3 R 0x10000004  ACERTO em A            ← A passa a ser o mais recente
4 R 0x10000040  falta, expulsa B       ← LRU tira B. FIFO tiraria A!
5 R 0x10000008  ACERTO em A            ← com FIFO isso seria falta
```

> ⚠️ **Armadilha do LRU que aparece no E5 e no Desafio:** num **laço circular** que usa mais blocos do que cabem no conjunto, o bloco usado há mais tempo é **exatamente o próximo que vai ser pedido**. O LRU expulsa sempre o bloco errado e **tudo falta**. É o pior caso do LRU.

### 2.6 Políticas de escrita

**Write-through (WT):**
- Toda escrita vai também pra memória, sempre 1 palavra = **4 B por escrita**.
- O bloco na cache nunca fica diferente da memória, então nunca precisa de writeback.

**Write-back (WB):**
- A escrita só altera a cache e liga o **bit D (dirty/modificado)**.
- Quando um bloco com D = 1 é expulso, ele é gravado **inteiro** na memória (um *writeback* de um bloco = 16 B com bloco de 16 B).
- No fim do traço, os blocos que ainda têm D = 1 são gravados. Isso é a **descarga final**, contada separado.

**Write-allocate (nas duas políticas, regra do trabalho):** uma escrita que falta **traz o bloco** pra cache, igualzinho a uma leitura que falta.

> 💡 **Consequência importante:** a política de escrita **não muda acertos nem faltas**. Ela só muda o tráfego de escrita pra memória. Isso é a pergunta 1 do E6.

### 2.7 Os 3 Cs: compulsória, capacidade, conflito

Toda falta tem que ser classificada em um destes tipos:

| Tipo | Definição do enunciado | Intuição |
|---|---|---|
| **Compulsória** | primeiro acesso, no traço inteiro, àquele bloco | "nunca vi esse bloco", inevitável |
| **Capacidade** | não é compulsória **e** também faltaria numa cache **totalmente associativa LRU** de mesma capacidade e bloco | "não cabe de jeito nenhum" |
| **Conflito** | não é compulsória **e** acertaria nessa totalmente associativa | "caberia, mas ficou no índice errado" |

**Como implementar (o enunciado manda):** simular em paralelo uma **segunda cache, totalmente associativa**, que recebe **todos** os acessos. Ela não aparece na saída. Quando a cache de verdade falta num bloco que já foi visto, a pergunta é:
- a sombra acertou? Então é **conflito**;
- a sombra também faltou? Então é **capacidade**.

> ⚠️ **Pegadinha que caiu no nosso E2 e E5:** "dois blocos no mesmo índice se expulsando" **não é automaticamente conflito**. Se a totalmente associativa também faltaria (porque o conjunto de trabalho é maior que a cache), a falta é de **capacidade**. A definição é sempre *comparar com a totalmente associativa*.

---

## 3. O que o professor deu

O zip `Pratica_Simulador_Cache_Tracos_Aluno.zip` tem:

- **`calibracao/`**: 4 traços **com resposta** (`*_esperado.txt`). O simulador tem que reproduzir essas saídas byte a byte.

  | Traço | Config | O que testa |
  |---|---|---|
  | C1 | 128 B, bloco 16, 1 via | divisão do endereço, bit de validade, expulsão no mesmo índice |
  | C2 | 64 B, bloco 16, 2 vias | LRU (FIFO erra o acesso 5) |
  | C3 | 64 B, bloco 16, 1 via, **WB e WT** | bit D, writeback na expulsão, descarga final, contagem do WT |
  | C4 | 64 B, bloco 16, 1 via | os três tipos de falta, inclusive capacidade |

- **`experimentos/`**: traços E1 a E6, **sem resposta**. O E4 usa o mesmo arquivo do E1.
- **O formato do traço** é uma linha por acesso, `R 0x10000000` ou `W 0x10000004`. Tudo depois de `#` é comentário.
- **O formato de saída** dos arquivos esperados, que a gente copiou igualzinho:

```
CONFIGURACAO  capacidade=128 B  bloco=16 B  vias=1  conjuntos=8  escrita=WB
CAMPOS        rotulo=25 bits  indice=3 bits  offset=4 bits

   1  R 0x10000000  rot=0x0200000  idx=0   off=0   FALTA  compulsoria
   5  R 0x10000080  rot=0x0200001  idx=0   off=0   FALTA  compulsoria expulsa 0x10000000

acessos ............. 10
...
escritas na memoria  0  (0 B)  [writebacks 0 + descarga final 0]
```

---

## 4. O que o enunciado pede (as 4 partes)

1. **Simulador.**
   - Parâmetros: traço, capacidade, bloco, vias e política (WT/WB).
   - Tem que recusar combinações impossíveis, como mais vias que blocos.
   - Tem que rodar várias configurações **sem editar o código** (linha de comando).
   - Saída por acesso: número, operação, endereço, rótulo, índice, offset, acerto/falta, tipo da falta, bloco expulso e se gerou writeback.
   - Resumo: acessos, acertos, faltas por tipo, taxa de acerto, blocos lidos (qtd e bytes) e escritas na memória (qtd e bytes; no WB separando writebacks e descarga final).
2. **Calibração.** Reproduzir C1–C4 exatamente. Na defesa, os 4 são executados na frente do professor.
3. **Experimentos.** E1 a E6 + Desafio, com tabela preenchida e perguntas respondidas citando acessos.
4. **Relatório.** Pra cada experimento, nesta ordem:
   - **Previsão:** o que a gente esperava antes de rodar, transcrito da folha de previsões.
   - **Medição:** a tabela.
   - **Justificativa:** as respostas, apoiadas em acessos específicos.
   - **Conciliação:** onde a previsão errou, o que estava errado no raciocínio. Se bateu, uma frase basta.

> 📝 **Folha de previsões:** é entregue ao professor no início do laboratório, **antes de rodar os experimentos**. A nossa está em `docs/folha_de_previsoes.md` e foi commitada **antes** dos resultados (dá pra provar pelo `git log`: o commit "Folha de previsões" vem antes do commit com os resultados). Imprime ela do jeito que está, sem mudar nada.

**Fora do escopo (não vale ponto extra):** outras políticas de substituição, multinível, busca de instruções, prefetch, escrita sem alocação e interface gráfica.

---

## 5. O que a gente fez

### 5.1 Estrutura do repositório

```
simulador-cache-m3/
├── src/                 ← código C++ (main, cache, leitor_de_traco, parametros, ferramentas)
├── include/             ← os .hpp
├── Makefile             ← make / make calibrar / make experimentos
├── calibrar.sh          ← roda C1–C4 e compara com diff
├── rodar_experimentos.sh← roda todas as configs e gera resultados/tabelas.md
├── tracos/
│   ├── calibracao/      ← do professor
│   ├── experimentos/    ← do professor
│   └── nossos/          ← E3 modificado + desafio
├── resultados/          ← saída por acesso de cada config (é daqui que saem os números citados)
└── docs/
    ├── enunciado.pdf
    ├── folha_de_previsoes.md
    ├── relatorio.md
    └── estudo_notion.md (este arquivo)
```

### 5.2 Como rodar (decora isso pra defesa)

```bash
make                     # compila
./calibrar.sh            # mostra "OK" nos 4 e "Calibracao: 4 de 4"
./simulador --traco tracos/calibracao/C2_associativa_LRU.txt --capacidade 64 --bloco 16 --vias 2 --escrita WB
./rodar_experimentos.sh  # regera tudo de resultados/
```

Opções úteis:
- `--vias total` deixa a cache totalmente associativa;
- `--resumo` mostra só o resumo final.

Se o professor pedir uma configuração na hora, é só trocar os números. Configuração impossível dá erro, por exemplo `--vias 32` com 16 blocos responde `ERRO: mais vias (32) do que blocos na cache (16)`.

### 5.3 Ordem em que o trabalho foi feito
1. Simulador num arquivo só → calibração 4 de 4.
2. **Folha de previsões** feita só olhando os traços e fazendo as contas na mão. Commit **antes** de rodar.
3. Simulador separado em arquivos (a calibração continuou 4 de 4).
4. Script dos experimentos → tabelas.
5. Traço do E3 modificado e do Desafio.
6. Relatório com as justificativas tiradas da saída por acesso.

---

## 6. Cada experimento: intuição + números + o que falar

> As tabelas completas estão no `docs/relatorio.md`. Aqui vai a **intuição** de cada um, que é o que você precisa saber explicar sem olhar.

### E1 — Bloco numa varredura (128 palavras, lidas uma vez)
- **Números:** 0% / 50% / 75% / 87,5% / 93,75% para blocos de 4 / 8 / 16 / 32 / 64 B.
- **Fórmula:** com *p* palavras por bloco, taxa = (p − 1)/p. A primeira palavra de cada bloco falta e as outras vêm "de carona".
- **Só faltas compulsórias:** ninguém é reusado.
- **Bytes lidos sempre 512 B:** o vetor é trazido uma vez, só muda o tamanho dos pedaços. Bloco maior aqui é de graça.
- **Frase pra defesa:** "Bloco grande explora localidade espacial. Como todo byte trazido é usado, o tráfego não muda e as faltas caem de 128 pra 8."

### E2 — Bloco com dados espalhados (16 registros de 72 B, só o 1º campo, 4 passadas)
- **Números:** 75% / 75% / 37,5% / 0% / 0%.
- **O passo de 72 B é maior que qualquer bloco**, então não tem localidade espacial nenhuma: cada acesso cai num bloco diferente.
- **Os acertos só podem vir das passadas 2–4**, se os 16 blocos sobreviverem.
- **Bloco 4/8 B:** 64/32 linhas, os 16 blocos caem em índices diferentes e tudo acerta depois da 1ª passada.
- **Bloco 16 B:** 16 linhas, mas 4 pares disputam índice. Os registros 1/8, 3/10, 5/12 e 7/14 (contando do 0) se expulsam por **conflito** nos acessos 18/25, 20/27, 22/29 e 24/31.
- **Bloco 32/64 B:** só 8/4 linhas pra 16 blocos, então tudo falta, e as faltas são de **capacidade** (nem a totalmente associativa daria conta).
- **Bytes trazidos por byte útil:** 1, 2, 10, 32 e 64. Com bloco de 64 B, 60 de cada 64 bytes trazidos são lixo.
- **Frase pra defesa:** "O que decide se bloco grande ajuda é o passo do acesso comparado ao tamanho do bloco. Sem localidade espacial, bloco maior só desperdiça banda e reduz o número de linhas."
- **Conciliação:** a gente achou que 32/64 B seria conflito, mas deu capacidade.

### E3 — Associatividade contra conflito (A[i] + B[i], B = A + 256)
- **Números:** 0% (1 via) e 75% com 2, 4 e 16 vias.
- **1 via:** 256 B de distância é exatamente o tamanho da cache, então A[i] e B[i] caem no **mesmo índice** e se expulsam a cada acesso (acessos 2, 3, 4... sempre "expulsa"). São 48 faltas de conflito, e metade da cache nunca é usada.
- **2 vias:** cabem os dois no conjunto. Só sobram as compulsórias (1 a cada 4 palavras). 4 e 16 vias não melhoram porque 75% já é o teto.
- **Traço modificado:** B em `0x10000080` (+128 B). A fica nos índices 0–7 e B nos 8–15, o que dá 75% com 1 via, o máximo possível.
- **Frase pra defesa:** "A cache tinha espaço. O problema era o mapeamento. Associatividade, ou mudar o endereço, resolve conflito."

### E4 — Associatividade numa varredura (traço do E1, bloco 16 B)
- **Números:** 75% em tudo, com 32 faltas compulsórias em todas as configurações.
- **Associatividade não muda nada** porque não há reuso pra proteger.
- **Conclusão E3 + E4 (+ E5):** associatividade elimina **conflito**; **não** elimina compulsória nem capacidade.

### E5 — Laço maior e menor que a cache (3 passadas)
- **80 palavras = 20 blocos > 16 linhas:** 85% (1 via) > 81,67% (2) > 75% (4) = 75% (16).
  - **Contraintuitivo:** mais vias = pior!
  - **Por quê:** o LRU num laço circular expulsa sempre o próximo da fila. Quanto mais blocos cabem num mesmo conjunto, mais blocos entram nesse ciclo.
  - **16 vias:** tudo no ciclo, só acertos dentro do bloco, 75%.
  - **4 vias:** 5 blocos por conjunto pra 4 vias, tudo no ciclo, 75%.
  - **2 vias:** 4 conjuntos com 3 blocos (ciclo) e 4 conjuntos com 2 blocos (cabem).
  - **1 via:** só os índices 0–3 têm 2 blocos. Os blocos 4–15 **sobrevivem entre as passadas** (acessos 97 a 144 todos ACERTO na 2ª passada).
- **60 palavras = 15 blocos:** cabe tudo, 91,67% em qualquer configuração (só as 15 compulsórias).
- **O que é melhor pro programador:** **diminuir o conjunto de trabalho**. Mais vias, com 80 palavras, piora; reduzir pra 60 palavras dá 91,67% em qualquer cache.
- **Conciliação:** as faltas do 1 via foram classificadas como capacidade, não conflito, porque a totalmente associativa falta em tudo nesse traço. Neste caso o mapeamento direto **ganha** da totalmente associativa.

### E6 — Políticas de escrita (bloco 16 B, 2 vias)
- **A taxa de acerto é igual entre WT e WB** nos 3 traços. Pelas regras isso tem que acontecer (write-allocate nas duas e o mesmo LRU).

  | Traço | WT (escritas / bytes) | WB (escritas / bytes) | Menos bytes |
  |---|---|---|---|
  | Acumulador (50 escritas no mesmo endereço) | 50 / 200 B | 1 / 16 B | **WB** |
  | Cópia (64 palavras) | 64 / 256 B | 16 / 256 B | **empate** |
  | Esparsa (1 escrita por bloco) | 32 / 128 B | 32 / 512 B | **WT** |

- **Regra numa frase:** WB ganha quando cada bloco sujo recebe mais escritas do que cabem palavras nele (bloco ÷ 4 = 4). Se recebe menos, WT ganha; se recebe exatamente isso, empata.
- **Esparsa:** o programa escreveu 128 B mas a cache **leu 512 B**, por causa do write-allocate (cada escrita traz um bloco de 16 B pra usar 4 B dele). Sem alocação, leria **0 B**.

### Desafio — 1 e 2 vias ≤ 10% (iguais), 4 vias ≥ 70%
- **Traço:** 4 blocos separados por 256 B (`0x10000000`, `0x100`, `0x200`, `0x300`), acessados em rodízio por 10 rodadas, sempre no offset 0.
- **Por que funciona:**
  - somar 256 não muda os bits de índice, então os 4 caem no **conjunto 0** com 1, 2 e 4 vias;
  - offset sempre 0 garante que não tem acerto de "palavra vizinha".
- **1 e 2 vias:** 4 blocos pra 1 ou 2 lugares, e o LRU expulsa sempre o próximo. **0%**.
- **4 vias:** os 4 cabem, só as 4 compulsórias faltam. **36/40 = 90%**.
- **Generalizando:** com R rodadas, a taxa com 4 vias é (4R − 4)/4R. R ≥ 4 já passa de 70%.

---

## 7. Perguntas prováveis na defesa (treina em voz alta)

1. **"Mostra o LRU funcionando."** Roda o C2. O acesso 3 acerta em A e renova A; o acesso 4 expulsa B, não A. Com FIFO o acesso 5 seria falta.
2. **"Como vocês classificam conflito × capacidade?"** Com uma cache sombra totalmente associativa, de mesma capacidade e mesmo bloco, que recebe todos os acessos. Se a sombra acertou, é conflito; se faltou, é capacidade. Antes disso, um `set` diz se o bloco já foi visto (senão é compulsória).
3. **"Por que no E5 mais vias piorou?"** É o pior caso do LRU num laço circular maior que o conjunto: ele sempre expulsa o próximo a ser usado. O mapeamento direto, por não ter escolha, deixa os blocos 4–15 em paz.
4. **"Calcula o índice desse endereço."** offset = endereço mod bloco; índice = (endereço ÷ bloco) mod conjuntos; rótulo = endereço ÷ (bloco × conjuntos).
5. **"Por que no WT os bytes escritos são 4 × escritas e no WB são bloco × writebacks?"** O WT manda só a palavra escrita. O WB manda o bloco inteiro quando ele sai sujo, ou na descarga final.
6. **"A taxa de acerto deveria mudar entre WT e WB?"** Não. Com write-allocate nas duas, as duas trazem os mesmos blocos e expulsam os mesmos. Muda só o tráfego.
7. **"Blocos lidos é igual a quê?"** Ao número de faltas: toda falta, de leitura ou de escrita (write-allocate), traz um bloco.
8. **"E se a escrita não alocasse?"** No E6 esparsa, os bytes lidos cairiam de 512 B pra 0. As escritas iriam direto pra memória, 4 B cada.
9. **"Onde a previsão de vocês errou?"** No *tipo* da falta. Achamos que seria conflito no E2 (bloco 32/64) e no E5 (1 via), e deu capacidade, porque a totalmente associativa também faltaria.
10. **"Esse endereço mal alinhado funciona?"** O programa recusa (`endereco nao esta alinhado em 4 bytes`), porque o enunciado diz que todo acesso é uma palavra alinhada.

---

## 8. Checklist pro dia da entrega (26/11? confirmar no AVA)

- [ ] Data de entrega e defesa confirmada no AVA (26/11 ou 30/11).
- [ ] Os três sabem rodar `make`, `./calibrar.sh` e um `./simulador` com parâmetros na hora.
- [ ] Folha de previsões impressa, do jeito que está no repositório.
- [ ] Relatório em PDF (`docs/relatorio.pdf`, gerado por `python3 scripts/gerar_pdf.py`) postado no AVA.
- [ ] Link do GitHub no relatório (o repositório é público).
- [ ] Notebook carregado, com o repositório clonado e compilado (sem depender de internet).
- [ ] Cada um explica sem ler: LRU, os 3 Cs, WT × WB, E5 (mais vias piora) e o Desafio.
- [ ] Nome no quadro ao chegar pra prova.

---

# PARTE 2 — Explicação técnica / Code review

> Leitura do código de cima pra baixo, como se fosse um review de PR: o que cada parte faz, por que foi feita assim, o que está bom e o que poderia ser melhor.

## 2.1 Visão geral da arquitetura

```
main.cpp
 ├── lerParametros()          (parametros.cpp)   → ParametrosDoUsuario
 ├── lerArquivoDeTraco()      (leitor_de_traco.cpp) → vector<AcessoDoTraco>
 ├── Cache cacheDeVerdade     (cache.cpp)        ← a configurada pelo usuário
 ├── Cache cacheSombra...     (cache.cpp)        ← totalmente associativa, só pra classificar
 ├── set blocosQueJaForamAcessados               ← pra saber se é compulsória
 └── laço: acessar nas duas → classificar → contar → imprimir
```

**Decisão de design:** a mesma classe `Cache` serve pras duas caches. A totalmente associativa é só uma `Cache` com `vias = capacidade / bloco` (um conjunto só). Isso evita duplicar a lógica do LRU e garante que as duas sigam exatamente as mesmas regras.

## 2.2 `ferramentas.cpp`: utilitários

```cpp
bool ehPotenciaDeDois(uint32_t numero) {
    return numero != 0 && (numero & (numero - 1)) == 0;
}
```
- O truque clássico: uma potência de 2 tem um único bit 1. Subtrair 1 transforma esse bit em 0 e todos os de baixo em 1, então o `&` dá zero. Exemplo: 16 = `10000`, 15 = `01111`, e `10000 & 01111 = 0`.
- `quantosBitsPrecisaPara` é um log2 por laço. Para números ≤ 2³¹, roda no máximo 31 iterações e é executado só na construção da cache, então custo zero na prática.
- `pararComErro` é `[[noreturn]]`: avisa o compilador que a função nunca retorna. Isso evita warnings de "função sem return" nos lugares que chamam ela.

**Review:** ✅ simples e correto. ⚠️ Usar `exit(1)` no meio do programa não é elegante (o ideal seria lançar exceção e tratar no `main`). Pra um programa de linha de comando desse tamanho, é aceitável.

## 2.3 `cache.hpp` / `cache.cpp`: o coração

### Estado

```cpp
struct LinhaDaCache {
    bool estaValida = false;          // V
    bool estaModificada = false;      // D
    uint32_t rotulo = 0;
    uint64_t momentoDoUltimoUso = 0;  // LRU
};
std::vector<std::vector<LinhaDaCache>> conjuntos;  // conjuntos[indice][via]
uint64_t relogio = 0;
```

- A cache é uma matriz `conjuntos × vias` de linhas. A estrutura espelha o hardware: V, D, rótulo e "idade".
- **LRU por carimbo de tempo:** cada acesso incrementa `relogio`, e a linha usada recebe o valor atual. A vítima é a de menor carimbo. Isso é equivalente a manter uma lista ordenada por uso, mas muito mais simples de escrever.
- `uint64_t` no relógio pra nunca estourar (2⁶⁴ acessos é impossível na prática).

### Divisão do endereço

```cpp
uint32_t Cache::pegarOffset(uint32_t endereco) const { return endereco & (tamanhoDoBlocoEmBytes - 1); }
uint32_t Cache::pegarIndice(uint32_t endereco) const { return (endereco >> bitsDoOffset) & (numeroDeConjuntos - 1); }
uint32_t Cache::pegarRotulo(uint32_t endereco) const { return (uint32_t)((uint64_t)endereco >> (bitsDoOffset + bitsDoIndice)); }
```

- Como tudo é potência de 2, `x mod 2ⁿ` é igual a `x & (2ⁿ − 1)`, e `x / 2ⁿ` é igual a `x >> n`. São operações de bit, como o hardware faz.
- O cast pra `uint64_t` no rótulo protege um caso de borda: em C++, deslocar um `uint32_t` por 32 bits é **comportamento indefinido**. Isso só aconteceria com cache de 4 GB, mas o cast custa nada.
- `montarEnderecoDoBloco` faz o caminho inverso (rótulo e índice → endereço com offset 0). É usado pra imprimir "expulsa 0x...".

### `acessar()`: o algoritmo

```cpp
relogio++;
// 1) procura
for (LinhaDaCache& linha : conjuntoDoEndereco)
    if (linha.estaValida && linha.rotulo == rotulo) {
        resultado.foiAcerto = true;
        linha.momentoDoUltimoUso = relogio;           // acerto conta como uso
        if (ehEscrita && usaWriteBack) linha.estaModificada = true;
        return resultado;
    }
// 2) escolhe vítima (vazia primeiro, senão LRU)
// 3) se a vítima era válida → registra expulsão e se gera writeback (D == 1)
// 4) preenche: V=1, rótulo novo, D = (escrita && WB), carimbo = relogio
```

Pontos que o professor pode cutucar:
- **"Acerto também conta como uso"**: a linha `momentoDoUltimoUso = relogio` no acerto. Sem ela, o LRU vira FIFO e o C2 quebra no acesso 5.
- **Write-allocate**: o passo 4 roda pra leitura e escrita igualmente.
- **Bit D no preenchimento**: se o bloco foi trazido por uma escrita no WB, já nasce sujo. No C3, o acesso 1 (W que falta) faz o bloco 0x10000000 ser expulso com "(writeback)" no acesso 4.
- **No WT o D nunca liga** (a condição `usaWriteBack` impede), então nunca há writeback. É o "blocos nunca ficam modificados" do enunciado.
- **Preferir via inválida antes do LRU**: sem isso, uma linha vazia com carimbo 0 seria escolhida de qualquer jeito (0 é o menor carimbo), então funcionaria igual. O laço separado deixa a intenção explícita.

**Complexidade:** cada acesso é O(vias). Com 16 vias e traços de centenas de acessos, é instantâneo.

**Review:** ✅ fiel às regras, legível, sem estado escondido. ⚠️ Os campos de configuração (`capacidadeEmBytes`, `bitsDoRotulo`...) são `public` e não `const`. Em teoria alguém poderia alterar e quebrar a cache. Dava pra fazer `const` + lista de inicialização no construtor. ⚠️ O construtor não valida nada sozinho: confia que `parametros.cpp` já validou. Pra uso interno, ok.

## 2.4 `leitor_de_traco.cpp`

- Lê linha por linha, corta tudo depois de `#` e usa `sscanf(" %c %63s")` pra pegar a letra e o endereço.
- `quantosCamposLeu <= 0` indica linha vazia ou só comentário, e é pulada. Se veio só 1 campo, o programa dá erro com o número da linha.
- `std::stoul(texto, nullptr, 16)` aceita `0x10000000` e também `10000000`. Erro de conversão vira mensagem amigável.
- Recusa endereço > 32 bits e endereço não alinhado em 4.

**Review:** ✅ mensagens de erro com número da linha ajudam muito quando a gente edita traço na mão (o Desafio, o E3 modificado). ⚠️ Lixo depois do endereço na mesma linha, tipo `R 0x10 abc`, é ignorado silenciosamente. ⚠️ `stoul` aceita sinal negativo e espaços em alguns casos. Não é problema pros traços do trabalho.

## 2.5 `parametros.cpp`

- Laço simples sobre `argv`: cada `--nome` consome o próximo argumento como valor (exceto `--resumo` e `--ajuda`).
- `--vias total` é convertido em `capacidade / bloco`.
- `conferirSeAConfiguracaoFazSentido` recusa: não-potência de 2, bloco < 4 B (menor que uma palavra), bloco > capacidade e vias > blocos. Isso cobre o "recusar combinações impossíveis" do enunciado.
- Funções auxiliares são `static` (visíveis só nesse arquivo). Só `lerParametros` é exportada no `.hpp`.

**Review:** ✅ cobre os casos do enunciado. ⚠️ `stoul` aceita valores acima de 2³² e o cast pra `uint32_t` trunca. Exemplo: `--capacidade 4294967552` viraria 256. É improvável, mas uma verificação de limite resolveria. ⚠️ Não existe valor padrão: todos os parâmetros são obrigatórios. Foi escolha consciente, pra ninguém rodar um experimento com a config errada sem perceber.

## 2.6 `main.cpp`: classificação e saída

### A classificação (o trecho mais importante pra defesa)

```cpp
ResultadoDoAcesso resultado        = cacheDeVerdade.acessar(endereco, ehEscrita);
ResultadoDoAcesso resultadoNaSombra = cacheSombraTotalmenteAssociativa.acessar(endereco, ehEscrita);

bool primeiraVezQueVejoEsseBloco = (blocosQueJaForamAcessados.count(enderecoDoBloco) == 0);
blocosQueJaForamAcessados.insert(enderecoDoBloco);

if (resultado.foiAcerto)                 acerto
else if (primeiraVezQueVejoEsseBloco)    compulsória
else if (resultadoNaSombra.foiAcerto)    conflito
else                                     capacidade
```

- **A sombra recebe todos os acessos**, inclusive os que acertam na cache de verdade. Isso é essencial: o estado LRU da sombra tem que refletir o traço inteiro, não só as faltas.
- **A ordem dos testes importa:** compulsória vem antes de olhar a sombra. A sombra também falta no primeiro acesso, e sem essa ordem essa falta seria classificada como capacidade.
- **`enderecoDoBloco = endereco & ~(bloco − 1)`** zera o offset. Dois endereços do mesmo bloco viram a mesma chave no `set`.
- **Por que "compulsória" é um `set` e não a sombra?** A definição é "primeiro acesso no traço inteiro a esse bloco", independente de cache. Um bloco que foi expulso da sombra e volta não é compulsório. O `set` guarda o histórico completo.

### Contadores e resumo
- `blocosLidosDaMemoria = totalDeFaltas()`: toda falta traz um bloco (write-allocate).
- WT: `escritasWriteThrough++` em todo `W`, acerto ou falta; bytes = × 4.
- WB: `writebacksDuranteAExecucao` soma as expulsões com D = 1. No fim, `contarBlocosModificadosQueSobraram()` dá a descarga final. Bytes = (writebacks + descarga) × bloco.

### Formato de saída
- `printf` com larguras fixas pra bater **byte a byte** com o professor:
  - `%4u` pro número do acesso;
  - `0x%08X` pro endereço;
  - `0x%0*X` pro rótulo, com nº de dígitos hexadecimais = ⌈bits do rótulo / 4⌉ (25 bits → 7 dígitos, 24 bits → 6);
  - `idx=%-4u` e `off=%-4u` alinhados à esquerda em 4 colunas;
  - `%-11s` no tipo da falta (largura de "compulsoria") antes de "expulsa".
- Detalhe que custou uma iteração: a primeira versão tinha um espaço a mais depois de `idx`/`off`, e o `diff` acusou. Por isso a calibração por `diff` vale ouro.

**Review:** ✅ a lógica de classificação é a do enunciado, literalmente. ✅ Separar a impressão (`imprimirLinhaDoAcesso`, `imprimirResumo`) do laço deixa o `main` legível. ⚠️ A saída fica acoplada ao formato do professor, então mudar uma largura quebra a calibração. É intencional. ⚠️ `idx=%-4u` sem espaço depois: se o índice tiver 4 dígitos (≥ 1000 conjuntos), ele gruda no `off=`. Isso só acontece com caches bem maiores que as do trabalho (o máximo nos experimentos é 64 conjuntos). ⚠️ O `std::set` dá O(log n) por acesso. Um `unordered_set` daria O(1), mas com centenas de acessos não faz diferença.

## 2.7 Scripts

**`calibrar.sh`**
- Compila (`make -s`), roda os 4 traços com as configs do cabeçalho de cada um e compara com `diff -u`.
- O C3 roda duas vezes (WB e WT), juntando as saídas com a mesma linha de 70 `=` do arquivo de resposta.
- Termina com código 0 só se os 4 passarem.

**`rodar_experimentos.sh`**
- Uma função `rodar` executa o simulador, salva a saída completa em `resultados/<exp>/<config>.txt` e usa `awk` pra extrair os números do resumo.
- Três funções montam as tabelas variando bloco, vias ou política, já no formato markdown do relatório.
- **Bug que pegamos:** a 1ª versão lia o campo errado do `awk` e mostrava o número de *blocos* na coluna de *bytes* lidos. A tabela do E1 saiu com 128 / 64 / 32 em vez de 512 / 512 / 512, o que contradizia a teoria (o vetor tem 512 B). Foi corrigido. É bom exemplo de "conferir número com a teoria antes de confiar".

## 2.8 O que dá pra melhorar (se o professor perguntar "e se fosse pra produção?")

1. **Testes unitários** da classe `Cache` (hoje a única verificação é a calibração end-to-end).
2. **Configuração `const`** na `Cache`, com construtor usando lista de inicialização.
3. **Exceções no lugar de `exit()`**, tratadas no `main`.
4. **Validação de limite** nos números da linha de comando (> 2³²).
5. **Saída em CSV** (`--csv`) pra não depender de `awk` no script.
6. **LRU com lista ligada + hash** se a associatividade fosse muito grande (hoje é O(vias) por acesso, o que é ótimo pra ≤ 16 vias).

## 2.9 Resumo do review

| Aspecto | Nota | Comentário |
|---|---|---|
| Corretude | ✅ | C1–C4 idênticos byte a byte; regras do enunciado seguidas literalmente |
| Legibilidade | ✅ | nomes em português bem explícitos, um arquivo por responsabilidade |
| Reuso | ✅ | a mesma `Cache` serve pra cache de verdade e pra sombra |
| Robustez | 🟡 | valida o que o enunciado pede; casos absurdos (> 2³², índice ≥ 1000) não tratados |
| Testabilidade | 🟡 | só teste end-to-end (calibração); sem teste unitário |
| Desempenho | ✅ | O(vias) por acesso, instantâneo pros traços do trabalho |
