
# Trabalho Grau B - Fractal de Mandelbrot (Pthreads)

Este projeto é uma implementação em **C++** para calcular e renderizar o Fractal de Mandelbrot utilizando programação paralela.

O sistema foi desenvolvido utilizando a biblioteca nativa **Pthreads** (memória compartilhada) e implementa a arquitetura **Master-Worker** (Produtor-Consumidor). A tela é dividida em vários blocos de tarefas que são processados simultaneamente por múltiplas threads trabalhadoras, garantindo ganho de desempenho.

## Alunos: Gustavo Cortezia, Augusto Hoff e Arthur Palma


## ▶️ Como Executar

### Compilar: 

```bash
g++ -O3 main.cpp -o main.exe -lpthread

```

O programa exige **três parâmetros** obrigatórios na linha de comando:

1. Número de threads.
2. Complexidade (Número máximo de iterações do fractal).
3. Tamanho da tarefa (Tamanho do bloco de pixels, ex: 32x32).

**Exemplo de uso (Executando com 4 threads, 1000 iterações e blocos de 32):**

*No Windows (PowerShell/CMD):*

```powershell
.\main.exe 4 1000 32

```

*No Linux/macOS/WSL:*

```bash
./main 4 1000 32

```

## 📊 Resultados e Saída

Ao finalizar a execução, o programa fará duas coisas:

1. **Medição de Tempo:** Imprimirá no terminal o tempo exato (em segundos) que as threads levaram para processar os cálculos. Isso é útil para comparar o ganho de desempenho (*Speedup*).
2. **Geração da Imagem:** Criará um arquivo chamado `mandelbrot.ppm` na mesma pasta. Este arquivo contém o desenho do fractal gerado e pode ser aberto nativamente no Linux/macOS, pelo GIMP, ou através de extensões de visualização no VS Code.



## Relatório de Desempenho


### 1. Teste de Escalabilidade e Speedup
Este teste mostra o ganho de velocidade ao aumentar o número de trabalhadores.
* **Parâmetros fixos:** Complexidade = 10.000 | Tamanho do Bloco = 32x32

| Threads | Tempo de Execução (s) | Aceleração |
| :--- | :--- | :--- |
| 1 | 3.151 | - |
| 2 | 1.574 | ~ 2x mais rápido |
| 4 | 0.805 | ~ 4x mais rápido |
| 8 | 0.417 | ~ 8x mais rápido |
| 16 | 0.234 | ~ 13x mais rápido |

**Explicação dos Resultados:**
O ganho de desempenho foi praticamente perfeito (linear). Ao dobrar o número de threads (de 1 para 2, 4 e 8), o tempo caiu exatamente pela metade a cada passo. Isso comprova que a arquitetura Master-Worker funcionou, dividindo o esforço matematicamente sem gerar gargalos, aproveitando o máximo de núcleos físicos do computador.

---

### 2. Teste de Saturação (Limite do Hardware)
Este teste mostra o que acontece quando criamos mais threads do que o computador suporta.
* **Parâmetros fixos:** Complexidade = 10.000 | Tamanho do Bloco = 32x32

| Threads | Tempo de Execução (s) |
| :--- | :--- |
| 16 | 0.234 |
| 64 | 0.245 |
| 256 | 0.223 |

**Explicação dos Resultados:**
O tempo de execução parou de cair ao atingir a marca de 16 threads, criando um "platô". Isso acontece porque o processador atingiu o seu limite físico de núcleos. A partir desse ponto, adicionar 64 ou 256 threads não acelera o processo. Pelo contrário: obriga o Sistema Operacional a gastar recursos preciosos da CPU apenas pausando e trocando as threads de lugar na fila (*Context Switching*), em vez de focar nos cálculos.

---

### 3. Teste de Granularidade (Tamanho das Tarefas)
Este teste compara o impacto de dividir o trabalho em pedaços grandes ou pequenos demais.
* **Parâmetros fixos:** Threads = 4 | Complexidade = 10.000

| Tamanho do Bloco | Tarefas Geradas | Tempo (s) | 
| :--- | :--- | :--- |
| 800x800 | 1 tarefa gigante | 3.139 | 
| 400x400 | 4 tarefas grandes | 1.202 | 
| 32x32 | 625 tarefas | 0.803 | 
| 1x1 | 640.000 tarefas | 1.107 | 

**Explicação dos Resultados:**
* **Bloco 800:** Foi o pior cenário. Como só existe 1 tarefa gigante, uma única thread pegou todo o trabalho e as outras 3 ficaram paradas (desbalanceamento de carga). O tempo foi igual a rodar de forma sequencial.
* **Bloco 1:** Foi o cenário de *overhead*. Com centenas de milhares de tarefas, as threads passaram mais tempo trancando e destrancando o cadeado da fila do que fazendo contas.
* **Bloco 32:** Foi a "zona ideal". Forneceu blocos suficientes para manter as 4 threads trabalhando 100% do tempo, sem congestionar o acesso à memória compartilhada.

---

### 4. Teste de Estresse Matemático
Este teste verifica se o sistema quebra ou trava quando a carga de trabalho se torna extrema.
* **Parâmetros fixos:** Tamanho do Bloco = 32x32

| Iterações por Pixel | Tempo 1 Thread (s) | Tempo 4 Threads (s) | Ganho Real |
| :--- | :--- | :--- | :--- |
| 1.000 (Leve) | 0.324 | 0.083 | ~ 3.9x mais rápido |
| 50.000 (Pesado)| 15.591 | 4.028 | ~ 3.8x mais rápido |

**Explicação dos Resultados:**
Sob extremo estresse de processamento, o código sequencial demorou dolorosos 15 segundos. Porém, a versão paralela com 4 threads não sofreu instabilidades ou vazamentos de memória. Ela manteve a sua eficiência e resolveu o mesmo problema em apenas 4 segundos. Isso ratifica que a implementação é robusta e a proporção de aceleração se mantém mesmo sob alta demanda computacional.