
# Trabalho Grau B - Fractal de Mandelbrot (Pthreads)

Este projeto é uma implementação em **C++** para calcular e renderizar o Fractal de Mandelbrot utilizando programação paralela.

O sistema foi desenvolvido utilizando a biblioteca nativa **Pthreads** (memória compartilhada) e implementa a arquitetura **Master-Worker** (Produtor-Consumidor). A tela é dividida em vários blocos de tarefas que são processados simultaneamente por múltiplas threads trabalhadoras, garantindo ganho de desempenho.

## Alunos: Gustavo Cortezia, Augusto Hoff e Arthur Palma


## ▶️ Como Executar

### Compilar: 

```bash
g++ -O3 mandelbrot.cpp -o mandelbrot.exe -lpthread

```

O programa exige **três parâmetros** obrigatórios na linha de comando:

1. Número de threads.
2. Complexidade (Número máximo de iterações do fractal).
3. Tamanho da tarefa (Tamanho do bloco de pixels, ex: 32x32).

**Exemplo de uso (Executando com 4 threads, 1000 iterações e blocos de 32):**

*No Windows (PowerShell/CMD):*

```powershell
.\mandelbrot.exe 4 1000 32

```

*No Linux/macOS/WSL:*

```bash
./mandelbrot 4 1000 32

```

## 📊 Resultados e Saída

Ao finalizar a execução, o programa fará duas coisas:

1. **Medição de Tempo:** Imprimirá no terminal o tempo exato (em segundos) que as threads levaram para processar os cálculos. Isso é útil para comparar o ganho de desempenho (*Speedup*).
2. **Geração da Imagem:** Criará um arquivo chamado `mandelbrot.ppm` na mesma pasta. Este arquivo contém o desenho do fractal gerado e pode ser aberto nativamente no Linux/macOS, pelo GIMP, ou através de extensões de visualização no VS Code.