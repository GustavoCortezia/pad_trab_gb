#include <iostream>
#include <vector>
#include <queue>
#include <pthread.h>
#include <fstream>
#include <cstdlib>
#include <chrono> // Biblioteca para medir o tempo

using namespace std; // Evita ter que digitar std:: o tempo todo

// Tamanho fixo da tela
int WIDTH = 800;
int HEIGHT = 800;

// Matriz global para guardar a imagem (forma mais simples)
int imagem_final[800][800];

// Estruturas de dados pedidas no quadro
struct Task {
    int x, y; // Posição inicial do bloco
    int w, h; // Largura e altura do bloco
};

struct Result {
    Task t;
    vector<int> pixels; // Guarda as cores calculadas
};

// Buffers globais
queue<Task> fila_tarefas;
queue<Result> fila_resultados;

// Travas do Pthreads (Mutexes)
pthread_mutex_t mutex_tarefa = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_resultado = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t aviso_resultado = PTHREAD_COND_INITIALIZER;

// Variáveis de controle
int tarefas_prontas = 0;
int total_tarefas = 0;
int max_iteracoes = 1000;

// Função que cada thread vai executar
void* trabalhador(void* arg) {
    while (true) {
        Task tarefa_atual;
        
        // 1. Tenta pegar uma tarefa na fila
        pthread_mutex_lock(&mutex_tarefa);
        if (fila_tarefas.empty()) {
            pthread_mutex_unlock(&mutex_tarefa);
            break; // Sai do loop e encerra a thread se não tiver mais trabalho
        }
        tarefa_atual = fila_tarefas.front();
        fila_tarefas.pop();
        pthread_mutex_unlock(&mutex_tarefa);

        // 2. Faz as contas do Mandelbrot
        Result resultado_atual;
        resultado_atual.t = tarefa_atual;

        for (int py = 0; py < tarefa_atual.h; py++) {
            for (int px = 0; px < tarefa_atual.w; px++) {
                
                // Converte o pixel para coordenadas do plano complexo
                double cx = -2.0 + (tarefa_atual.x + px) * 3.0 / WIDTH;
                double cy = -1.5 + (tarefa_atual.y + py) * 3.0 / HEIGHT;
                
                double zx = 0.0, zy = 0.0;
                int iter = 0;
                
                while (zx * zx + zy * zy <= 4.0 && iter < max_iteracoes) {
                    double novo_zx = zx * zx - zy * zy + cx;
                    zy = 2.0 * zx * zy + cy;
                    zx = novo_zx;
                    iter++;
                }
                // Guarda o resultado desse pixel
                resultado_atual.pixels.push_back(iter);
            }
        }

        // 3. Coloca o resultado pronto no buffer
        pthread_mutex_lock(&mutex_resultado);
        fila_resultados.push(resultado_atual);
        tarefas_prontas++;
        pthread_cond_signal(&aviso_resultado); // Avisa a main que tem resultado
        pthread_mutex_unlock(&mutex_resultado);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Verifica se a pessoa digitou os 3 parametros certos
    if (argc != 4) {
        cout << "Erro. Use: mandelbrot.exe <threads> <iteracoes> <tamanho_tarefa>" << endl;
        return 1;
    }

    int num_threads = atoi(argv[1]);
    max_iteracoes = atoi(argv[2]);
    int tamanho_bloco = atoi(argv[3]);

    // INICIA O CRONÔMETRO
    auto tempo_inicio = chrono::high_resolution_clock::now();

    // --- Etapa 1: Dividir a tela em tarefas ---
    pthread_mutex_lock(&mutex_tarefa);
    for (int y = 0; y < HEIGHT; y += tamanho_bloco) {
        for (int x = 0; x < WIDTH; x += tamanho_bloco) {
            Task nova_tarefa;
            nova_tarefa.x = x;
            nova_tarefa.y = y;
            
            // Tratamento basico caso o bloco passe da borda da tela
            if (x + tamanho_bloco > WIDTH) {
                nova_tarefa.w = WIDTH - x;
            } else {
                nova_tarefa.w = tamanho_bloco;
            }

            if (y + tamanho_bloco > HEIGHT) {
                nova_tarefa.h = HEIGHT - y;
            } else {
                nova_tarefa.h = tamanho_bloco;
            }

            fila_tarefas.push(nova_tarefa);
            total_tarefas++;
        }
    }
    pthread_mutex_unlock(&mutex_tarefa);

    // --- Etapa 2: Criar as Threads ---
    // Usando um vetor fixo de 100 posições por simplicidade
    pthread_t threads[100]; 
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, trabalhador, NULL);
    }

    // --- Etapa 3: Ler os resultados e pintar a matriz ---
    while (true) {
        pthread_mutex_lock(&mutex_resultado);
        
        // Espera de forma dorminhoca até que tenha resultado novo
        while (fila_resultados.empty() && tarefas_prontas < total_tarefas) {
            pthread_cond_wait(&aviso_resultado, &mutex_resultado);
        }

        // Lê todos os resultados que já chegaram
        while (!fila_resultados.empty()) {
            Result r = fila_resultados.front();
            fila_resultados.pop();

            int contador = 0;
            for (int py = 0; py < r.t.h; py++) {
                for (int px = 0; px < r.t.w; px++) {
                    imagem_final[r.t.y + py][r.t.x + px] = r.pixels[contador];
                    contador++;
                }
            }
        }

        // Verifica se terminou tudo antes de soltar o cadeado
        bool terminou_tudo = (tarefas_prontas == total_tarefas);
        pthread_mutex_unlock(&mutex_resultado);

        if (terminou_tudo) break; // Sai do while principal
    }

    // Junta as threads (boa prática para limpar a memória)
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // PARA O CRONÔMETRO
    auto tempo_fim = chrono::high_resolution_clock::now();
    chrono::duration<double> duracao = tempo_fim - tempo_inicio;
    
    // Print do tempo
    cout << "Tempo gasto com " << num_threads << " thread(s): " << duracao.count() << " segundos." << endl;

    // --- Etapa 4: Gerar arquivo da Imagem ---
    ofstream arquivo_img("mandelbrot.ppm");
    arquivo_img << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n"; // Cabeçalho mágico do arquivo
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int iter = imagem_final[y][x];
            
            if (iter == max_iteracoes) {
                arquivo_img << "0 0 0 "; // Pixel preto
            } else {
                // Matemática simples pra gerar umas cores azuis/verdes
                arquivo_img << (iter % 255) << " " << ((iter * 2) % 255) << " " << ((iter * 5) % 255) << " ";
            }
        }
        arquivo_img << "\n";
    }
    arquivo_img.close();

    cout << "Imagem mandelbrot.ppm criada com sucesso." << endl;
    
    return 0;
}