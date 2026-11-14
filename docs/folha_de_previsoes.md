# Folha de previsões — M3 Organização de Computadores

**Integrantes:** José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert

> Feita **antes** de rodar qualquer experimento: só olhando os traços e fazendo a conta na mão.
> O commit deste arquivo vem antes do commit com os resultados (dá pra conferir no `git log`).

Cache dos experimentos: capacidade 256 B. Com bloco de 16 B são 16 blocos na cache.

---

## E1 — varredura (128 palavras lidas uma vez, 1 via)

| Bloco (B)     | 4  | 8   | 16  | 32    | 64     |
|---------------|----|-----|-----|-------|--------|
| Taxa prevista | 0% | 50% | 75% | 87,5% | 93,75% |

**Por quê:** cada bloco é lido uma vez só, do começo ao fim. A primeira palavra do bloco falta (compulsória) e as outras acertam. Com *p* palavras por bloco: taxa = (p − 1) / p. Os bytes lidos devem ficar sempre em 512 B, porque o vetor inteiro é trazido uma vez.

## E2 — registros (16 registros de 72 B, primeiro campo, 4 passadas, 1 via)

| Bloco (B)     | 4   | 8   | 16    | 32 | 64 |
|---------------|-----|-----|-------|----|----|
| Taxa prevista | 75% | 75% | 37,5% | 0% | 0% |

**Por quê:** o passo de 72 B é maior que qualquer bloco, então cada acesso cai num bloco diferente e não existe localidade espacial. Os acertos só podem vir das passadas 2, 3 e 4, e só se os 16 blocos não brigarem pelo mesmo índice.
- Com 4 B e 8 B, as contas dão 16 índices diferentes, então tudo acerta depois da 1ª passada (48/64).
- Com 16 B, 4 pares de registros caem no mesmo índice (1 e 8, 3 e 10, 5 e 12, 7 e 14) e se expulsam. Sobram 8 registros acertando em 3 passadas (24/64).
- Com 32 B (8 linhas) e 64 B (4 linhas), todo índice tem 2 ou mais registros, então tudo falta.

## E3 — dois vetores (bloco 16 B)

| Vias          | 1  | 2   | 4   | 16  |
|---------------|----|-----|-----|-----|
| Taxa prevista | 0% | 75% | 75% | 75% |

**Por quê:** B começa exatamente 256 B (= capacidade) depois de A, então A[i] e B[i] caem no mesmo índice com 1 via. Como o laço alterna A[i], B[i], um expulsa o outro a cada acesso. Com 2 vias ou mais os dois blocos cabem juntos no conjunto, e só sobram as faltas compulsórias (1 a cada 4 palavras).

## E4 — varredura (mesmo traço do E1, bloco 16 B)

| Vias          | 1   | 2   | 4   | 16  |
|---------------|-----|-----|-----|-----|
| Taxa prevista | 75% | 75% | 75% | 75% |

**Por quê:** nenhum bloco é reusado depois de sair, então todas as faltas são compulsórias. Associatividade não ajuda nisso.

## E5 — laços de 80 e 60 palavras (3 passadas, bloco 16 B)

| Vias        | 1      | 2      | 4      | 16     |
|-------------|--------|--------|--------|--------|
| 80 palavras | 85%    | 81,67% | 75%    | 75%    |
| 60 palavras | 91,67% | 91,67% | 91,67% | 91,67% |

**Por quê (80):** são 20 blocos para 16 linhas.
- **16 vias:** o LRU sempre expulsa justamente o bloco que vai ser usado logo depois, então tudo falta e sobram só os acertos dentro do bloco (75%).
- **1 via:** só os blocos 16–19 brigam com os blocos 0–3. Os blocos 4–15 sobrevivem de uma passada para a outra (8 faltas por passada a partir da 2ª).
- **2 vias:** 4 conjuntos ficam com 3 blocos (ciclo, tudo falta) e 4 conjuntos com 2 blocos (cabem).
- **4 vias:** todo conjunto tem 5 blocos para 4 vias, então tudo falta.

**Por quê (60):** são 15 blocos, então cabem em qualquer configuração. Só faltam as 15 compulsórias: 165/180.

## E6 — políticas de escrita: qual escreve menos bytes na memória?

| Traço            | Acumulador | Cópia  | Escritas esparsas |
|------------------|------------|--------|-------------------|
| WT, WB ou empate | WB         | empate | WT                |

**Por quê:** WT manda 4 B por escrita, e WB manda 16 B por bloco sujo (quando ele sai ou na descarga final). Quem ganha depende de quantas escritas um bloco recebe enquanto está na cache.
- **Acumulador:** 50 escritas no mesmo bloco. WT = 200 B, WB = 16 B.
- **Cópia:** 4 escritas por bloco de destino. 64 × 4 = 16 × 16 = 256 B, empate.
- **Esparsa:** 1 escrita por bloco. WT = 128 B, WB = 512 B.

---

## Desafio — ideia antes de testar

Usar 4 blocos que caem no **mesmo conjunto** com 1, 2 e 4 vias (endereços separados por 256 B, só o offset 0) e acessar em rodízio A, B, C, D, A, B, C, D...
- **1 via e 2 vias:** 4 blocos disputam 1 ou 2 lugares, e o LRU sempre tira o próximo a ser usado. Resultado: 0% nas duas.
- **4 vias:** os 4 cabem. Depois das 4 compulsórias, tudo acerta. Com 10 rodadas: 36/40 = 90%.
