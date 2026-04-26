# Makefile - T1 Sistemas Operacionais
# Compila os experimentos de Threads e Processos

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lpthread

# Detecta o sistema operacional para linkar semaforos corretamente
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    LDFLAGS += -lrt
endif

TARGETS = threads processes

.PHONY: all clean run-all

all: $(TARGETS)

threads: threads.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

processes: processes.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Executa todos os experimentos (T1, T2, P1, P2) para N=2,4,8
# Mede o tempo real com 'time' e exibe o resultado
run-all: all
	@echo ""
	@echo "=============================================="
	@echo "  EXPERIMENTOS - Threads"
	@echo "=============================================="
	@for n in 2 4 8; do \
		echo ""; \
		echo "--- T1 (sem sync) | N=$$n ---"; \
		time ./threads $$n 0; \
		echo ""; \
		echo "--- T2 (mutex)    | N=$$n ---"; \
		time ./threads $$n 1; \
	done
	@echo ""
	@echo "=============================================="
	@echo "  EXPERIMENTOS - Processos"
	@echo "=============================================="
	@for n in 2 4 8; do \
		echo ""; \
		echo "--- P1 (sem sync) | N=$$n ---"; \
		time ./processes $$n 0; \
		echo ""; \
		echo "--- P2 (semaforo) | N=$$n ---"; \
		time ./processes $$n 1; \
	done

# Hardware signature (macOS)
hw-info:
	sysctl -a | grep hw.ncpu
	sysctl -a | grep hw.physicalcpu
	sysctl -a | grep hw.logicalcpu
	sysctl -a | grep machdep.cpu.brand_string

clean:
	rm -f $(TARGETS)
