# Simulador de Cache — M3 Organização de Computadores (UNIVALI)

Prática: simulador de cache + experimentos com estratégias de organização.
Prof. Thiago Felski Pereira.

**Integrantes:** José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert

## Estrutura

- `docs/enunciado.pdf` — enunciado da prática
- `tracos/calibracao/` — traços C1–C4 com a saída esperada (`*_esperado.txt`)
- `tracos/experimentos/` — traços E1–E6 (sem resposta)
- `tracos/nossos/` — traço do E3 modificado e traço do Desafio
- `src/`, `include/` — código do simulador (C++17)
- `resultados/` — saída por acesso de cada configuração + `tabelas.md`
- `docs/folha_de_previsoes.md` — previsões (feitas antes de rodar)
- `docs/relatorio.md` — relatório final
- `docs/estudo_notion.md` — material de estudo pra defesa

## Roteiro

1. **Simulador** — parâmetros por linha de comando (traço, capacidade, bloco, vias, WT/WB);
   LRU, write-allocate, classificação de faltas com uma cache totalmente associativa "sombra".
2. **Calibração** — reproduzir exatamente C1, C2, C3 (WB e WT) e C4 (diff automático).
3. **Folha de previsões** — preencher ANTES de rodar os experimentos (entregue no início do lab).
4. **Experimentos** E1–E6 + Desafio — script que roda todas as configurações e gera as tabelas.
5. **Relatório** — por experimento: Previsão → Medição → Justificativa (citando acessos) → Conciliação.

## Como compilar e rodar

```bash
make                 # gera ./simulador
./calibrar.sh        # roda C1–C4 e compara com as respostas (deve dar 4 de 4)
./rodar_experimentos.sh   # roda E1–E6 + Desafio e gera resultados/tabelas.md
./simulador --traco tracos/experimentos/E1_varredura.txt --capacidade 256 --bloco 16 --vias 1 --escrita WB
```

Opções: `--vias total` (totalmente associativa), `--resumo` (só o resumo final).
