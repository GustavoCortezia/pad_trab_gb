
#include <iostream>
#include <vector>
#include <queue>
#include <pthread.h>
#include <fstream>
#include <cstdlib>

// Configurações da tela (resolução do fractal)
const int WIDTH = 800;
const int HEIGHT = 800;
const double X_MIN = -2.0, X_MAX = 1.0;
const double Y_MIN = -1.5, Y_MAX = 1.5;

// 1. Estrutura da Tarefa ("Quadrado")
struct Task {
    int x0, y0;     // Coordenada inicial do bloco na tela
    int w, h;       // Largura e altura do bloco
};

// 2. Estrutura do Resultado
struct Result {
    Task t;
    std::vector<int> pixels; // Vetor com as cores (iterações) calculadas
};

// --- Buffers Compartilhados e Sincronização ---
std::queue<Task> task_buffer;
std::queue<Result> result_buffer;

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t result_cv = PTHREAD_COND_INITIALIZER; // Avisa a thread principal sobre novos resultados

int completed_tasks = 0;
int total_tasks = 0;
int max_iterations = 1000; // Complexidade padrão

// --- Função da Thread Trabalhadora (Worker) ---
void* worker_routine(void* arg) {
    while (true) {
        Task t;
        
        // 1. Pegar uma tarefa do Task Buffer
        pthread_mutex_lock(&task_mutex);
        if (task_buffer.empty()) {
            pthread_mutex_unlock(&task_mutex);
            break; // Se não há mais tarefas, a thread encerra
        }
        t = task_buffer.front();
        task_buffer.pop();
        pthread_mutex_unlock(&task_mutex);

        // 2. Computar a tarefa (Calcular o Mandelbrot para o bloco)
        Result r;
        r.t = t;
        r.pixels.reserve(t.w * t.h);

        for (int py = 0; py < t.h; ++py) {
            for (int px = 0; px < t.w; ++px) {
                // Mapear pixel para o plano complexo
                double cx = X_MIN + (t.x0 + px) * (X_MAX - X_MIN) / WIDTH;
                double cy = Y_MIN + (t.y0 + py) * (Y_MAX - Y_MIN) / HEIGHT;
                
                double zx = 0.0, zy = 0.0;
                int iter = 0;
                
                // Fórmula de Mandelbrot: Z = Z^2 + C
                while (zx * zx + zy * zy <= 4.0 && iter < max_iterations) {
                    double zx_new = zx * zx - zy * zy + cx;
                    zy = 2.0 * zx * zy + cy;
                    zx = zx_new;
                    iter++;
                }
                r.pixels.push_back(iter);
            }
        }

        // 3. Gravar no Buffer Resultado
        pthread_mutex_lock(&result_mutex);
        result_buffer.push(r);
        completed_tasks++;
        pthread_cond_signal(&result_cv); // Acorda a Main Thread para desenhar
        pthread_mutex_unlock(&result_mutex);
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Parâmetros do programa exigidos no quadro
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <num_threads> <complexidade> <tamanho_tarefa>\n";
        return 1;
    }

    int num_threads = std::atoi(argv[1]);
    max_iterations = std::atoi(argv[2]);
    int task_size = std::atoi(argv[3]);

    // Matriz final da imagem
    std::vector<std::vector<int>> image(HEIGHT, std::vector<int>(WIDTH, 0));

    // --- Etapa Main: Criar as Tarefas ---
    pthread_mutex_lock(&task_mutex);
    for (int y = 0; y < HEIGHT; y += task_size) {
        for (int x = 0; x < WIDTH; x += task_size) {
            Task t;
            t.x0 = x;
            t.y0 = y;
            t.w = std::min(task_size, WIDTH - x);
            t.h = std::min(task_size, HEIGHT - y);
            task_buffer.push(t);
            total_tasks++;
        }
    }
    pthread_mutex_unlock(&task_mutex);

    // --- Criar as Threads Trabalhadoras ---
    std::vector<pthread_t> threads(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, worker_routine, nullptr);
    }

    // --- Etapa Main/Renderizador: Print na tela (consumir resultados) ---
    while (true) {
        pthread_mutex_lock(&result_mutex);
        
        // Espera até que haja um resultado ou que todas as tarefas acabem
        while (result_buffer.empty() && completed_tasks < total_tasks) {
            pthread_cond_wait(&result_cv, &result_mutex);
        }

        // Consome todos os resultados disponíveis no buffer no momento
        while (!result_buffer.empty()) {
            Result r = result_buffer.front();
            result_buffer.pop();

            // "Pintar" o bloco na matriz da imagem final
            int i = 0;
            for (int py = 0; py < r.t.h; ++py) {
                for (int px = 0; px < r.t.w; ++px) {
                    image[r.t.y0 + py][r.t.x0 + px] = r.pixels[i++];
                }
            }
        }

        bool finished = (completed_tasks == total_tasks);
        pthread_mutex_unlock(&result_mutex);

        if (finished) break;
    }

    // Aguarda todas as threads finalizarem limpas
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // --- Gerar o arquivo da imagem (.ppm) ---
    std::ofstream img("mandelbrot.ppm");
    img << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n"; // Cabeçalho PPM
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int iter = image[y][x];
            // Colorização básica baseada no número de iterações
            if (iter == max_iterations) {
                img << "0 0 0 "; // Preto (pertence ao conjunto)
            } else {
                int color = (iter * 255) / max_iterations;
                img << color << " " << (color * 2) % 255 << " " << (color * 5) % 255 << " "; 
            }
        }
        img << "\n";
    }
    img.close();

    std::cout << "Processamento concluído. Imagem salva como 'mandelbrot.ppm'." << std::endl;
    return 0;
}