#include <iostream>
#include <vector>
#include <queue>
#include <pthread.h>
#include <fstream>
#include <cstdlib>
#include <chrono>

using namespace std;

int largura_tela = 800;
int altura_tela = 800;
int imagem_final[800][800];

// Estruturas do padrão Master-Worker
struct TarefaBloco {
    int posicao_x, posicao_y;
    int largura, altura;
};

struct ResultadoBloco {
    TarefaBloco tarefa_origem;
    vector<int> cores_pixels;
};

queue<TarefaBloco> fila_tarefas;
queue<ResultadoBloco> fila_resultados;

// Sincronização das threads
pthread_mutex_t mutex_tarefa = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_resultado = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t aviso_resultado = PTHREAD_COND_INITIALIZER;

int tarefas_concluidas = 0;
int total_tarefas = 0;
int limite_iteracoes = 1000;

// Função executada pelas threads operárias
void* rotina_trabalhador(void* arg) {
    while (true) {
        TarefaBloco tarefa_atual;
        
        pthread_mutex_lock(&mutex_tarefa);
        if (fila_tarefas.empty()) {
            pthread_mutex_unlock(&mutex_tarefa);
            break;
        }
        tarefa_atual = fila_tarefas.front();
        fila_tarefas.pop();
        pthread_mutex_unlock(&mutex_tarefa);

        ResultadoBloco resultado_atual;
        resultado_atual.tarefa_origem = tarefa_atual;

        for (int pixel_y = 0; pixel_y < tarefa_atual.altura; pixel_y++) {
            for (int pixel_x = 0; pixel_x < tarefa_atual.largura; pixel_x++) {
                
                double plano_x = -2.0 + (tarefa_atual.posicao_x + pixel_x) * 3.0 / largura_tela;
                double plano_y = -1.5 + (tarefa_atual.posicao_y + pixel_y) * 3.0 / altura_tela;
                
                double z_real = 0.0, z_imaginario = 0.0;
                int iteracao_atual = 0;
                
                while (z_real * z_real + z_imaginario * z_imaginario <= 4.0 && iteracao_atual < limite_iteracoes) {
                    double proximo_z_real = z_real * z_real - z_imaginario * z_imaginario + plano_x;
                    z_imaginario = 2.0 * z_real * z_imaginario + plano_y;
                    z_real = proximo_z_real;
                    iteracao_atual++;
                }
                resultado_atual.cores_pixels.push_back(iteracao_atual);
            }
        }

        pthread_mutex_lock(&mutex_resultado);
        fila_resultados.push(resultado_atual);
        tarefas_concluidas++;
        pthread_cond_signal(&aviso_resultado);
        pthread_mutex_unlock(&mutex_resultado);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Uso correto: mandelbrot.exe <threads> <iteracoes> <tamanho_tarefa>" << endl;
        return 1;
    }

    int quantidade_threads = atoi(argv[1]);
    limite_iteracoes = atoi(argv[2]);
    int tamanho_bloco = atoi(argv[3]);

    auto tempo_inicio = chrono::high_resolution_clock::now();

    // Gerar tarefas baseadas no tamanho da tela
    pthread_mutex_lock(&mutex_tarefa);
    for (int y = 0; y < altura_tela; y += tamanho_bloco) {
        for (int x = 0; x < largura_tela; x += tamanho_bloco) {
            TarefaBloco nova_tarefa;
            nova_tarefa.posicao_x = x;
            nova_tarefa.posicao_y = y;
            
            if (x + tamanho_bloco > largura_tela) nova_tarefa.largura = largura_tela - x;
            else nova_tarefa.largura = tamanho_bloco;

            if (y + tamanho_bloco > altura_tela) nova_tarefa.altura = altura_tela - y;
            else nova_tarefa.altura = tamanho_bloco;

            fila_tarefas.push(nova_tarefa);
            total_tarefas++;
        }
    }
    pthread_mutex_unlock(&mutex_tarefa);

    // Iniciar processamento paralelo
    pthread_t threads[100]; 
    for (int i = 0; i < quantidade_threads; i++) {
        pthread_create(&threads[i], NULL, rotina_trabalhador, NULL);
    }

    // Coletar resultados e montar a matriz final
    while (true) {
        pthread_mutex_lock(&mutex_resultado);
        
        while (fila_resultados.empty() && tarefas_concluidas < total_tarefas) {
            pthread_cond_wait(&aviso_resultado, &mutex_resultado);
        }

        while (!fila_resultados.empty()) {
            ResultadoBloco resultado_pronto = fila_resultados.front();
            fila_resultados.pop();

            int indice_cor = 0;
            for (int pixel_y = 0; pixel_y < resultado_pronto.tarefa_origem.altura; pixel_y++) {
                for (int pixel_x = 0; pixel_x < resultado_pronto.tarefa_origem.largura; pixel_x++) {
                    imagem_final[resultado_pronto.tarefa_origem.posicao_y + pixel_y][resultado_pronto.tarefa_origem.posicao_x + pixel_x] = resultado_pronto.cores_pixels[indice_cor];
                    indice_cor++;
                }
            }
        }

        bool processamento_finalizado = (tarefas_concluidas == total_tarefas);
        pthread_mutex_unlock(&mutex_resultado);

        if (processamento_finalizado) break;
    }

    for (int i = 0; i < quantidade_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    auto tempo_fim = chrono::high_resolution_clock::now();
    chrono::duration<double> duracao_total = tempo_fim - tempo_inicio;
    cout << "Tempo de execucao: " << duracao_total.count() << " segundos." << endl;

    // Exportar matriz calculada para arquivo de imagem
    ofstream arquivo_imagem("mandelbrot.ppm");
    arquivo_imagem << "P3\n" << largura_tela << " " << altura_tela << "\n255\n";
    
    for (int y = 0; y < altura_tela; y++) {
        for (int x = 0; x < largura_tela; x++) {
            int valor_iteracao = imagem_final[y][x];
            
            if (valor_iteracao == limite_iteracoes) {
                arquivo_imagem << "0 0 0 ";
            } else {
                arquivo_imagem << (valor_iteracao % 255) << " " << ((valor_iteracao * 2) % 255) << " " << ((valor_iteracao * 5) % 255) << " ";
            }
        }
        arquivo_imagem << "\n";
    }
    arquivo_imagem.close();

    return 0;
}