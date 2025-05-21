#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Estrutura que representa uma fórmula CNF (Conjunctive Normal Form)
typedef struct 
{
    int **clausulas;       // Matriz de cláusulas (cada linha é uma cláusula terminada com 0)
    int num_clausulas;     // Número total de cláusulas na fórmula
    int num_literais;      // Número total de variáveis (literais) na fórmula
} Formula;

// Estrutura para representar uma árvore binária de decisão
typedef struct BinaryTree
{
    int valor;              // Valor atribuído à variável (1 para verdadeiro, -1 para falso)
    int variavel;           // Número da variável (literal)
    struct BinaryTree *esquerda;  // Subárvore para quando a variável é verdadeira
    struct BinaryTree *direita;   // Subárvore para quando a variável é falsa
} BinaryTree;

#define MAX_ARESTAS 1000 // 1ª ALTERAÇÃO: struct Grafo
typedef struct {
    int n_vertices;
    int m_arestas;
    int arestas[MAX_ARESTAS][2];
} Grafo;

// Função para liberar a memória alocada para a árvore binária
void liberar_arvore(BinaryTree *no) 
{
    if (no == NULL) return;          // Se o nó é nulo, retorna
    liberar_arvore(no->esquerda);    // Libera a subárvore esquerda recursivamente
    liberar_arvore(no->direita);     // Libera a subárvore direita recursivamente
    free(no);                        // Libera o nó atual
}

// Função para verificar se o literal é válido, ou seja, se ele está entre o intervalo de clausulas anunciado no arquivo
bool literal_valido(int literal, int num_literais)
{
    int var = abs(literal);
    return (var >= 1 && var <= num_literais);
}

// Função para ler uma fórmula CNF de um arquivo no formato DIMACS
Formula* ler_formula(const char *file) 
{
    FILE *arquivo = fopen(file, "r");  // Abre o arquivo para leitura
    if (!arquivo) 
    {
        printf("Erro ao abrir o arquivo");
        return NULL;
    }

    // Aloca memória para a estrutura da fórmula
    Formula *f = malloc(sizeof(Formula));
    f->clausulas = NULL;     // Inicializa o ponteiro para cláusulas como NULL
    f->num_clausulas = 0;    // Inicializa contador de cláusulas
    f->num_literais = 0;     // Inicializa contador de literais

    char linha[1000];         // Buffer para ler linhas do arquivo
    int clausula_atual = 0;  // Índice da cláusula sendo processada

    // Lê o arquivo linha por linha
    while (fgets(linha, sizeof(linha), arquivo)) 
    {
        if (linha[0] == 'c') continue;  // Ignora linhas de comentário (começam com 'c')
        
        // Linha de parâmetro (começa com 'p')
        if (linha[0] == 'p') 
        {
            // Extrai o número de literais e cláusulas da linha
            sscanf(linha, "p cnf %d %d", &f->num_literais, &f->num_clausulas);
            // Aloca memória para o vetor de ponteiros para cláusulas
            f->clausulas = malloc(f->num_clausulas * sizeof(int*));
            continue;
        }

        // Processa uma cláusula
        int *clausula = malloc((f->num_literais + 1) * sizeof(int));  // Aloca espaço para os literais + terminador 0
        int literal, tamanho = 0;
        
        // Divide a linha em tokens usando espaços, tabs e quebras de linha como delimitadores
        char *token = strtok(linha, " \t\n");
        
        // Processa cada token (literal) da cláusula
        while (token != NULL) 
        {
            literal = atoi(token);  // Converte o token para inteiro
            if (literal == 0) break;  // Se for 0, indica fim da cláusula
            if (!literal_valido(literal, f->num_literais))
            {
                printf("Erro: literal %d invalido (max: %d)\n", literal, f->num_literais);
                printf("UNSAT\n");
                free(clausula);
                fclose(arquivo);
                return NULL;

            }
            clausula[tamanho++] = literal;  // Armazena o literal no vetor
            token = strtok(NULL, " \t\n");  // Pega o próximo token
        }
        
        clausula[tamanho] = 0;  // Adiciona o terminador 0 no final da cláusula
        
        // Armazena a cláusula na fórmula
        f->clausulas[clausula_atual++] = clausula;
    }

    fclose(arquivo);  // Fecha o arquivo
    return f;         // Retorna a fórmula lida
}

// Função para liberar a memória alocada para a fórmula
void liberar_formula(Formula *f) 
{
    // Libera cada cláusula individualmente
    for (int i = 0; i < f->num_clausulas; i++) 
    {
        free(f->clausulas[i]);
    }
    free(f->clausulas);  // Libera o vetor de cláusulas
    free(f);             // Libera a estrutura da fórmula
}


// Verifica se uma cláusula está satisfeita dada uma interpretação
bool clausula_satisfeita(int *clausula, int *interpretacao) 
{
    for (int i = 0; clausula[i] != 0; i++) 
    {
        int var = abs(clausula[i]);  // Pega o número da variável (ignorando sinal)
        int valor = clausula[i] > 0 ? 1 : -1;  // 1 se literal positivo, -1 se negativo
        
        // Se a interpretação da variável satisfaz o literal
        if (interpretacao[var] == valor) 
        {
            return true;  // Cláusula satisfeita
        }
    }
    return false;  // Nenhum literal satisfez a cláusula
}

// Verifica se toda a fórmula está satisfeita
bool formula_satisfativel(Formula *f, int *interpretacao) 
{
    // Verifica cada cláusula
    for (int i = 0; i < f->num_clausulas; i++) 
    {
        if (!clausula_satisfeita(f->clausulas[i], interpretacao)) 
        {
            return false;  // Se alguma cláusula não está satisfeita
        }
    }
    return true;  // Todas as cláusulas satisfeitas
}

// Verifica se a fórmula é insatisfatível (todas as cláusulas têm conflito)
bool formula_insatisfativel(Formula *f, int *interpretacao) 
{
    for (int i = 0; i < f->num_clausulas; i++) 
    {
        bool todos_falsos = true; //refere-se aos literais
        
        // Verifica cada literal na cláusula
        for (int j = 0; f->clausulas[i][j] != 0; j++) 
        {
            int var = abs(f->clausulas[i][j]);
            int valor = f->clausulas[i][j] > 0 ? 1 : -1;
            
            // Se o literal ainda pode ser satisfeito
            //interpretacao[var] == 0 significa q o literal ainda n foi atribuido, por isso ele ainda pode ser satisfeito
            if (interpretacao[var] == 0 || interpretacao[var] == valor) 
            {
                todos_falsos = false;
                break;
            }
        }
        
        // Se todos os literais da cláusula estão em conflito
        if (todos_falsos) return true;
    }
    return false;
}

// Encontra a próxima variável não atribuída na interpretação
int proxima_variavel_nao_atribuida(Formula *f, int *interpretacao) 
{
    // Varre todas as variáveis (começando de 1)
    for (int i = 1; i <= f->num_literais; i++) 
    {    
        if (interpretacao[i] == 0)  // Se ainda não foi atribuído valor
        {
            return i;  // Retorna o número da variável
        }
    }
    return 0;  // Todas as variáveis já foram atribuídas
}

// Função que implementa um solver SAT (problema de satisfabilidade booleana) usando backtracking
// e construindo uma árvore binária de decisão durante o processo.
//
// Parâmetros:
//   - f: Ponteiro para a fórmula booleana a ser verificada
//   - interpretacao: Array que armazena a atribuição atual de valores às variáveis
//   - no: Ponteiro para o nó atual da árvore de decisão binária
//
// Retorno:
//   - true se a fórmula for satisfatível com a atribuição atual
//   - false caso contrário
bool SAT(Formula *f, int *interpretacao, BinaryTree *no) 
{
    // Casos base:
    
    // Verifica se a fórmula já está satisfeita com a interpretação atual
    if (formula_satisfativel(f, interpretacao)) return true;
    
    // Verifica se a fórmula é insatisfatível com a interpretação atual
    if (formula_insatisfativel(f, interpretacao)) return false;

    // Passo recursivo:
    
    // Escolhe a próxima variável não atribuída para tentar uma atribuição
    int var = proxima_variavel_nao_atribuida(f, interpretacao);
    if (var == 0) return false;  // Não há mais variáveis não atribuídas

    // Se é a raiz (nó vazio), inicializa o nó com a variável atual
    if (no->variavel == 0) 
    {
        no->variavel = var;
    }

    // Tenta atribuir verdadeiro (1) à variável atual
    no->esquerda = malloc(sizeof(BinaryTree));
    no->esquerda->variavel = 0;  // Inicializa como nó vazio
    no->esquerda->valor = 0;      // Valor inicial
    no->esquerda->esquerda = no->esquerda->direita = NULL;  // Inicializa filhos

    // Atualiza a interpretação com verdadeiro para a variável
    interpretacao[var] = 1;
    
    // Chama recursivamente para continuar a busca
    if (SAT(f, interpretacao, no->esquerda)) 
    {
        no->valor = 1;  // Marca que este ramo levou a uma solução satisfatível
        return true;
    }

    // Se a atribuição verdadeira falhou, libera o nó esquerdo
    free(no->esquerda);
    no->esquerda = NULL;

    // Tenta atribuir falso (-1) à variável atual
    no->direita = malloc(sizeof(BinaryTree));
    no->direita->variavel = 0;    // Inicializa como nó vazio
    no->direita->valor = 0;       // Valor inicial
    no->direita->esquerda = no->direita->direita = NULL;  // Inicializa filhos

    // Atualiza a interpretação com falso para a variável
    interpretacao[var] = -1;
    
    // Chama recursivamente para continuar a busca
    if (SAT(f, interpretacao, no->direita)) 
    {
        no->valor = -1;  // Marca que este ramo levou a uma solução satisfatível
        return true;
    }

    // Se ambas atribuições (verdadeiro e falso) falharem, faz backtracking
    free(no->direita);      // Libera o nó direito
    no->direita = NULL;     // Reseta o ponteiro
    interpretacao[var] = 0; // Reseta a interpretação para a variável
    
    return false;  // Retorna false indicando que não encontrou solução neste ramo
}

bool lerGrafo(const char *nome_arquivo, Grafo *g) { // 3ª ALTERAÇÃO: função lerGrafo
    FILE *fp = fopen(nome_arquivo, "r");
    if (!fp) {
        perror("Erro ao abrir grafo.txt");
        return false;
    }

    // Lê número de vértices
    if (fscanf(fp, "%d", &g->n_vertices) != 1) {
        fclose(fp);
        printf("Erro ao ler número de vértices.\n");
        return false;
    }

    // Lê número de arestas
    if (fscanf(fp, "%d", &g->m_arestas) != 1) {
        fclose(fp);
        printf("Erro ao ler número de arestas.\n");
        return false;
    }

    // Pular a linha de separador (*)
    char buffer[100];
    fgets(buffer, sizeof(buffer), fp);  // Consome o restante da linha anterior
    fgets(buffer, sizeof(buffer), fp);  // Pula a linha com "*"

    // Lê as arestas
    for (int i = 0; i < g->m_arestas; i++) {
        int a, b;
        if (fscanf(fp, "%d %d", &a, &b) != 2) {
            fclose(fp);
            printf("Erro ao ler aresta %d.\n", i + 1);
            return false;
        }
        g->arestas[i][0] = a;
        g->arestas[i][1] = b;
    }

    fclose(fp);
    return true;
}
 
// Função principal
int main() { // 2º ALTERAÇÃO: nova main
    Grafo g; // cria uma variável do tipo grafo
    int k = 1; // K = cor(es) --> sempre começa com 1 cor

    // === Ler o grafo do arquivo ===
    if (!lerGrafo("grafo.txt", &g)) { // hora de ler o arquivo contendo o grafo com a função lerGrafo
        // se FALSE, arquivo corrompido/problemático. Logo, o programa acaba
        printf("Erro ao ler grafo!\n");
        return 1;
    }

    // informações obtidas do grafo.txt
    printf("Grafo com %d vértices e %d arestas.\n", g.n_vertices, g.m_arestas); 

    // === Loop de tentativa de coloração com K cores ===
    while (k <= g.n_vertices) { // enquanto K for <= a quantidade de vértices, o while roda
        printf("Testando coloração com %d cor%s...\n", k, k > 1 ? "es" : "");

        // === Gerar CNF para o valor atual de K ===
        gerarCNF(&g, k, "sat.cnf"); 

        // === Ler fórmula CNF ===
        Formula *f = ler_formula("sat.cnf");
        if (!f) {
            printf("Erro ao processar CNF.\n");
            return 1;
        }

        // Aloca vetor de interpretação
        int *interpretacao = calloc(f->num_literais + 1, sizeof(int));
        if (!interpretacao) {
            liberar_formula(f);
            printf("Erro ao alocar vetor de interpretação.\n");
            return 1;
        }

        // Cria árvore
        BinaryTree *raiz = malloc(sizeof(BinaryTree));
        if (!raiz) {
            free(interpretacao);
            liberar_formula(f);
            printf("Erro ao alocar árvore.\n");
            return 1;
        }

        raiz->variavel = 0;
        raiz->valor = 0;
        raiz->esquerda = raiz->direita = NULL;

        // Executa SAT
        if (SAT(f, interpretacao, raiz)) {
            printf("Coloração possível com %d cor%s!\n", k, k > 1 ? "es" : "");
            printf("Coloração encontrada:\n");

            for (int v = 1; v <= g.n_vertices; v++) {
                for (int cor = 1; cor <= k; cor++) {
                    int var_index = (v - 1) * k + cor;
                    if (interpretacao[var_index] == 1) {
                        printf("Vértice %d → cor %d\n", v, cor);
                        break;
                    }
                }
            }

            // Libera memória
            liberar_formula(f);
            free(interpretacao);
            liberar_arvore(raiz);
            return 0;
        }

        // Libera e tenta com mais uma cor
        liberar_formula(f);
        free(interpretacao);
        liberar_arvore(raiz);
        printf("✘ Não é possível com %d cor%s.\n\n", k, k > 1 ? "es" : "");
        k++;
    }

    printf("Nenhuma coloração possível com K ≤ %d. Verifique o grafo.\n", g.n_vertices);
    return 0;
}
