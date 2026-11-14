ARQUIVOS_FONTE = src/main.cpp src/cache.cpp src/leitor_de_traco.cpp src/parametros.cpp src/ferramentas.cpp
CABECALHOS     = $(wildcard include/*.hpp)

simulador: $(ARQUIVOS_FONTE) $(CABECALHOS)
	g++ -std=c++17 -O2 -Wall -Wextra -Iinclude -o simulador $(ARQUIVOS_FONTE)

calibrar: simulador
	./calibrar.sh

experimentos: simulador
	./rodar_experimentos.sh

limpar:
	rm -f simulador

.PHONY: calibrar experimentos limpar
