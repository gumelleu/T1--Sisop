# T1 — O Duelo de Contextos: Processos vs. Threads

**Sistemas Operacionais – PUCRS**
Prof. Filipo Mór

| Gustavo Melleu - 22200589 | Felipe Ferreira - 21100492 | Caetano Marasca - 23108514 |
---

## 1. Assinatura do Hardware

```
$ sysctl -a | grep hw.ncpu
hw.ncpu: 10
```

Modelo: Apple M4 | Núcleos físicos: 10 | RAM: 24 GB | macOS

---

## 2. Como Compilar e Executar

```bash
# Compilar tudo
make all

# Rodar um experimento específico com medição de tempo
time ./threads  <N> 0   # T1: threads sem sincronização
time ./threads  <N> 1   # T2: threads com pthread_mutex
time ./processes <N> 0  # P1: processos sem sincronização
time ./processes <N> 1  # P2: processos com sem_open muito lento

# Rodar todos automaticamente
make run-all
```

>  O experimento P2 (processos com semáforo) é extremamente lento
> porque cada um dos 1.000.000.000 de incrementos exige duas syscalls de kernel
> (`sem_wait` + `sem_post`). No Apple M4, N=8 levou **6 horas e 18 minutos**.

---

## 3. Tabela de Tempos

Tempo **real** de execução medido com o comando `time` do shell (coluna `real`).

| Experimento | Sincronização | N=2 | N=4 | N=8 |
|---|---|---|---|---|
| **T1** | Threads — sem mutex | 0,522 s | 0,279 s | 0,229 s |
| **T2** | Threads — pthread_mutex | 6,551 s | 13,185 s | 14,728 s |
| **P1** | Processos — sem semáforo | 0,523 s | 0,283 s | 0,228 s |
| **P2** | Processos — sem_open | 23 min 15 s | 20 min 50 s | **6 h 18 min** |

<details>
<summary>Saída completa do comando <code>time</code></summary>

```
--- T1 | N=2 ---
./threads 2 0  1,04s user  0,00s system 199% cpu  0,522 total

--- T1 | N=4 ---
./threads 4 0  1,10s user  0,00s system 394% cpu  0,279 total

--- T1 | N=8 ---
./threads 8 0  1,72s user  0,02s system 761% cpu  0,229 total

--- T2 | N=2 ---
./threads 2 1  5,19s user  4,70s system 151% cpu  6,551 total

--- T2 | N=4 ---
./threads 4 1  7,57s user 29,25s system 279% cpu 13,185 total

--- T2 | N=8 ---
./threads 8 1 11,15s user 85,92s system 659% cpu 14,728 total

--- P1 | N=2 ---
./processes 2 0  1,04s user  0,00s system 199% cpu  0,523 total

--- P1 | N=4 ---
./processes 4 0  1,11s user  0,00s system 394% cpu  0,283 total

--- P1 | N=8 ---
./processes 8 0  1,71s user  0,02s system 757% cpu  0,228 total

--- P2 | N=2 ---
./processes 2 1  163,68s user 1259,10s system 101% cpu 23:15,57 total

--- P2 | N=4 ---
./processes 4 1  167,16s user 1104,03s system 101% cpu 20:50,52 total

--- P2 | N=8 ---
./processes 8 1  206,02s user 1392,80s system   7% cpu 6:18:01,46 total
```

</details>

---

## 4. Análise de Corrupção (T1 e P1)

### Valores finais do contador

| Experimento | N=2 | N=4 | N=8 |
|---|---|---|---|
| **T1** — Threads sem sync | 500.362.773 | 252.627.367 | 175.712.075 |
| **P1** — Processos sem sync | 501.273.997 | 256.029.203 | 172.528.941 |
| **Esperado** | 1.000.000.000 | 1.000.000.000 | 1.000.000.000 |

### Por que o contador não chega em 1.000.000.000 :

A operação `counter++` em C **não é atômica**. Ela se decompõe em três instruções
de máquina:

```
1. LOAD  — lê o valor atual da memória para um registrador
2. ADD   — incrementa o valor no registrador
3. STORE — escreve o resultado de volta na memória
```

Quando duas ou mais threads (ou processos) executam essas instruções de forma
intercalada pelo escalonador, ocorre a **race condition**: dois workers leem o
mesmo valor antes que qualquer um escreva, incrementam independentemente e
sobrescrevem um ao outro — perdendo incrementos.

**Exemplo com 2 threads, valor inicial = 100:**

```
Thread A: LOAD  → 100
Thread B: LOAD  → 100        ← lê o mesmo valor!
Thread A: ADD   → 101
Thread A: STORE → 101
Thread B: ADD   → 101        ← baseado no valor lido antes do STORE de A
Thread B: STORE → 101        ← sobrescreve 101 com 101 — perdeu 1 incremento
```

### Como o hardware influenciou o erro

O Apple M4 possui **10 núcleos físicos**, o que significa que as threads/processos
executam em **paralelo real** (não apenas time-slicing). Isso maximiza a frequência
de colisões na seção crítica:

- **N=2:** Cada worker processa 500 milhões de incrementos. Com 2 núcleos ativos
  em paralelo real, a janela de conflito é constante — resultado final em torno
  de **50% do esperado** (~500 milhões), pois metade dos incrementos é perdida
  por sobreescrita.

- **N=4 e N=8:** Com mais workers em paralelo, a proporção de colisões aumenta.
  O resultado final cai para ~25% e ~17% do esperado, respectivamente, pois
  mais threads leem o mesmo valor simultaneamente antes que qualquer uma escreva.

Em hardware com **um único núcleo**, a race condition raramente se manifestaria,
pois as threads alternariam por time-slicing e a janela de colisão seria muito menor.
O M4 com 10 núcleos torna a corrupção de dados praticamente garantida e proporcional
ao número de workers.

---

## 5. Gráfico de Escalabilidade

<img width="2384" height="770" alt="image" src="https://github.com/user-attachments/assets/631a0ba0-bfe6-4806-b57c-031160da4ebb" />


O gráfico exibe três painéis:

- **Esquerda:** T1 vs. P1 (sem sincronização) em escala linear. Ambos ficam abaixo
  de 0,6 s e melhoram com mais workers (paralelismo real no M4).
- **Centro:** T2 vs. P2 (com sincronização) em escala logarítmica. T2 cresce
  moderadamente de ~6 s a ~15 s; P2 vai de 23 minutos a **6 horas e 18 minutos**.
- **Direita:** Todos os experimentos em barras com escala logarítmica, evidenciando
  a diferença de mais de 4 ordens de magnitude entre T1/P1 e P2.

---

## 6. Análise e Conclusão

### Overhead de Criação: Processos vs. Threads

| Modelo | Mecanismo | Overhead |
|---|---|---|
| **Thread** (pthreads) | Compartilha memória, descritores e código com o pai | **Baixo** |
| **Processo** (fork) | Cria novo PCB, duplica mapa de memória (Copy-on-Write), copia descritores | **Alto** |

O `fork()` exige que o kernel crie um novo Process Control Block, duplique a
tabela de descritores de arquivo e configure um novo mapa de endereçamento virtual
(mesmo que Copy-on-Write adie a cópia física das páginas). Threads são mais leves
pois compartilham tudo isso com o processo pai.

### Custo de Comunicação

| Modelo | Mecanismo de comunicação | Custo |
|---|---|---|
| **Threads** | Variável global — acesso direto à memória compartilhada | **Mínimo** (sem syscall) |
| **Processos** | `shmget/shmat` + `sem_open/sem_wait/sem_post` | **Alto** (syscalls do kernel) |

Threads acessam a variável compartilhada com uma simples instrução de leitura/escrita
de memória. Processos precisam de IPC explícito: a memória compartilhada (shmget/shmat)
adiciona overhead de mapeamento, e cada operação de semáforo (`sem_wait`/`sem_post`)
é uma chamada de sistema completa com transição para o modo kernel.

Esse custo explica o resultado mais dramático do trabalho: P2 com N=8 levou
**6 horas e 18 minutos** — contra **14,7 segundos** do T2 com N=8 —
fazendo as mesmas 1 bilhão de operações protegidas.

### Eficiência com Sincronização: Mutex vs. Semáforo Nomeado

No T2 (mutex), o pthreads pode usar **futex** (Linux) ou primitivas otimizadas (macOS)
que evitam syscalls quando não há contenção. No P2 (sem_open), o semáforo é
**sempre** gerenciado pelo kernel — cada wait e post é uma syscall obrigatória,
independentemente de haver ou não contenção.

### Por que T2 ficou mais lento com N maior:

Com mutex, toda a seção crítica é serializada — apenas uma thread incrementa por
vez. Adicionar mais threads não aumenta a taxa de incrementos; só aumenta a
contenção pelo lock. Com N=8, as 8 threads disputam o mesmo mutex, gerando mais
overhead de context-switch e mais tempo de CPU perdido em espera (kernel time),
por isso o tempo cresceu de 6,5 s (N=2) para 14,7 s (N=8).

### Por que P2 com N=8 foi mais lento que N=4:

Com N=4, os 4 processos alternavam eficientemente no semáforo (~20 min). Com N=8,
a contenção de 8 processos pelo mesmo semáforo nomeado gerou um nível de contenção
no kernel que causou degradação massiva: de 20 minutos (N=4) para **6 horas e 18
minutos** (N=8). O overhead de escalonamento de processos — muito maior que o de
threads — multiplicou o custo a cada disputa pelo semáforo.

### Resumo Final

| Critério | Vencedor | Motivo |
|---|---|---|
| Menor overhead de criação | **Thread** | Compartilha recursos, sem novo PCB |
| Comunicação mais eficiente | **Thread** | Acesso direto à memória, sem IPC |
| Melhor escalabilidade sem sync | **Empate** | T1 ≈ P1 em tempo real |
| Sincronização mais eficiente | **Thread** (mutex) | Mutex ~1540× mais rápido que semáforo nomeado (N=8: 14,7 s vs. 6 h 18 min) |

---



