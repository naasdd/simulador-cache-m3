simulador: src/simulador.cpp
	g++ -std=c++17 -O2 -Wall -Wextra -o simulador src/simulador.cpp

calibrar: simulador
	./calibrar.sh

limpar:
	rm -f simulador
	rm -rf saidas

.PHONY: calibrar limpar
