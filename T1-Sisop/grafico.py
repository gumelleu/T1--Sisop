"""
grafico.py - Gera os graficos de escalabilidade do T1 de Sistemas Operacionais
Dados reais coletados em Apple M4 (10 nucleos, 24 GB RAM), macOS
"""

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

N = [2, 4, 8]

# Tempos reais de execucao (segundos) — coletados com 'time' em macOS
T1 = [0.522, 0.279, 0.229]   # threads sem mutex
T2 = [6.551, 13.185, 14.728] # threads com pthread_mutex
P1 = [0.523, 0.283, 0.228]   # processos sem semaforo
P2 = [1395.57, 1250.52, 22681.46]  # processos com sem_open

# --------------------------------------------------------------------------
# Grafico 1: Escala linear — T1 e P1 (experimentos sem sincronizacao)
# --------------------------------------------------------------------------
fig, axes = plt.subplots(1, 3, figsize=(16, 5))
fig.suptitle('Escalabilidade: Processos vs. Threads\nApple M4 · 10 nucleos · macOS',
             fontsize=13, fontweight='bold', y=1.02)

# --- Subplot A: sem sincronizacao (T1 vs P1) ---
ax = axes[0]
ax.plot(N, T1, 'o-', color='steelblue',  linewidth=2, markersize=7, label='T1 — Threads (sem sync)')
ax.plot(N, P1, 's-', color='tomato',     linewidth=2, markersize=7, label='P1 — Processos (sem sync)')
ax.set_title('Sem Sincronizacao (T1 vs P1)', fontweight='bold')
ax.set_xlabel('Numero de workers (N)')
ax.set_ylabel('Tempo real (segundos)')
ax.set_xticks(N)
ax.legend()
ax.grid(True, alpha=0.3)
for xi, y1, y2 in zip(N, T1, P1):
    ax.annotate(f'{y1:.3f}s', (xi, y1), textcoords='offset points', xytext=(5, 5), fontsize=8, color='steelblue')
    ax.annotate(f'{y2:.3f}s', (xi, y2), textcoords='offset points', xytext=(5, -14), fontsize=8, color='tomato')

# --- Subplot B: com sincronizacao — T2 vs P2 (escala log) ---
ax = axes[1]
ax.plot(N, T2,  'o-', color='steelblue', linewidth=2, markersize=7, label='T2 — Threads (mutex)')
ax.plot(N, P2,  's-', color='tomato',    linewidth=2, markersize=7, label='P2 — Processos (semaforo)')
ax.set_yscale('log')
ax.set_title('Com Sincronizacao (T2 vs P2)\n[escala logaritmica]', fontweight='bold')
ax.set_xlabel('Numero de workers (N)')
ax.set_ylabel('Tempo real (segundos) — log')
ax.set_xticks(N)
ax.legend()
ax.grid(True, which='both', alpha=0.3)

def fmt_time(s):
    if s >= 3600:
        return f'{s/3600:.1f}h'
    if s >= 60:
        return f'{s/60:.1f}min'
    return f'{s:.2f}s'

for xi, y1, y2 in zip(N, T2, P2):
    ax.annotate(fmt_time(y1), (xi, y1), textcoords='offset points', xytext=(5, 5),  fontsize=8, color='steelblue')
    ax.annotate(fmt_time(y2), (xi, y2), textcoords='offset points', xytext=(5, -14), fontsize=8, color='tomato')

# --- Subplot C: todos os experimentos lado a lado por N ---
ax = axes[2]
x = np.arange(len(N))
width = 0.2
bars = [T1, T2, P1, P2]
colors = ['steelblue', 'cornflowerblue', 'tomato', 'firebrick']
labels = ['T1 (threads, sem sync)', 'T2 (threads, mutex)', 'P1 (proc., sem sync)', 'P2 (proc., semaforo)']
offsets = [-1.5, -0.5, 0.5, 1.5]

for bar_data, color, label, off in zip(bars, colors, labels, offsets):
    ax.bar(x + off * width, bar_data, width, label=label, color=color, alpha=0.85)

ax.set_yscale('log')
ax.set_title('Todos os Experimentos\n[escala logaritmica]', fontweight='bold')
ax.set_xlabel('Numero de workers (N)')
ax.set_ylabel('Tempo real (segundos) — log')
ax.set_xticks(x)
ax.set_xticklabels([f'N={n}' for n in N])
ax.legend(fontsize=7.5)
ax.grid(True, which='both', axis='y', alpha=0.3)

plt.tight_layout()
plt.savefig('grafico.png', dpi=150, bbox_inches='tight')
print("Grafico salvo em: grafico.png")
