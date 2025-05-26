#include <stdio.h>     // Biblioteca padrão para entrada e saída (printf, scanf, fopen, etc)
#include <stdlib.h>    // Biblioteca padrão para funções como malloc, free, atoi, abs
#include <stdbool.h>   // Para usar o tipo booleano em C (true, false)
#include <string.h>    // Para manipulação de strings (strtok, etc)
#include <locale.h>    // Para configurar a linguagem/região (ex: pt_BR.UTF-8)

// Estrutura que representa uma fórmula CNF (Conjunctive Normal Form)
typedef struct {
    int **clausulas;       // Matriz de cláusulas. Cada cláusula é um array de inteiros terminando com 0
    int num_clausulas;     // Quantidade total de cláusulas da fórmula
    int num_literais;      // Quantidade de variáveis (x1, x2, ..., xn)
} Formula;

// Estrutura para representar uma árvore binária de decisão
typedef struct BinaryTree {
    int valor;               // Valor atribuído à variável neste ponto da árvore: 1 (true), -1 (false)
    int variavel;            // Número da variável (literal) nesse nó da árvore
    struct BinaryTree *esquerda;  // Ramo da árvore onde essa variável foi testada como verdadeira
    struct BinaryTree *direita;   // Ramo da árvore onde essa variável foi testada como falsa
} BinaryTree;

#define MAX_ARESTAS 2500 // 1ª ALTERAÇÃO: struct Grafo
typedef struct {
    int n_vertices;                   // Número de vértices do grafo
    int m_arestas;                    // Número de arestas
    int arestas[MAX_ARESTAS][2];     // Lista de arestas, onde cada aresta é um par (v1, v2)
} Grafo;

/**
 * Libera a memória alocada para uma árvore binária
 * @param no Raiz da árvore a ser liberada
 */
void liberar_arvore(BinaryTree *no) 
{
    if (no == NULL) return;          // Se o nó é nulo, retorna
    liberar_arvore(no->esquerda);    // Libera a subárvore esquerda recursivamente
    liberar_arvore(no->direita);     // Libera a subárvore direita recursivamente
    free(no);                        // Libera o nó atual
}

/**
 * Verifica se um literal está dentro do intervalo válido
 * @param literal Valor do literal a verificar
 * @param num_literais Número total de variáveis na fórmula
 * @return true se o literal é válido, false caso contrário
 */
// Função para verificar se o literal é válido, ou seja, se ele está entre o intervalo de clausulas anunciado no arquivo
bool literal_valido(int literal, int num_literais)
{
    int var = abs(literal);
    return (var >= 1 && var <= num_literais);
}

/**
 * Lê uma fórmula CNF de um arquivo no formato DIMACS
 * @param file Nome do arquivo a ser lido
 * @return Ponteiro para a fórmula lida ou NULL em caso de erro
 */
// Função para ler uma fórmula CNF de um arquivo no formato DIMACS
Formula* ler_formula(const char *file) 
{
    FILE *arquivo = fopen(file, "r");  // Abre o arquivo no modo leitura
    if (!arquivo)                      // Se o ponteiro for NULL, deu erro
    {
        printf("Erro ao abrir o arquivo");  // Informa erro
        return NULL;                        // Retorna NULL para sinalizar falha
    }

    Formula *f = malloc(sizeof(Formula));  // Aloca espaço para a estrutura Formula
    f->clausulas = NULL;                   // Inicializa o ponteiro de cláusulas como vazio
    f->num_clausulas = 0;                 // Ainda não lemos nenhuma cláusula
    f->num_literais = 0;                  // Ainda não sabemos quantas variáveis existem

    char linha[1000];         // Buffer para armazenar cada linha lida do arquivo
    int clausula_atual = 0;   // Índice usado para inserir cláusulas no array

    // Percorre linha a linha o conteúdo do arquivo
    while (fgets(linha, sizeof(linha), arquivo)) 
    {
        if (linha[0] == 'c') continue;  // Ignora linhas de comentário (começam com 'c')

        if (linha[0] == 'p')            // Se for linha de cabeçalho ("p cnf ...")
        {
            // Lê o número de variáveis e de cláusulas da fórmula
            sscanf(linha, "p cnf %d %d", &f->num_literais, &f->num_clausulas);

            // Aloca o vetor de ponteiros onde cada posição será uma cláusula
            f->clausulas = malloc(f->num_clausulas * sizeof(int*));
            continue; // Passa para a próxima linha
        }

        // === Leitura de cláusulas ===

        // Cria um vetor para a cláusula atual, com espaço para todos os literais + terminador (0)
        int *clausula = malloc((f->num_literais + 1) * sizeof(int));

        int literal;         // Armazena temporariamente cada literal lido
        int tamanho = 0;     // Contador de quantos literais já inserimos na cláusula

        // Divide a linha em tokens (números separados por espaço/tab/enter)
        char *token = strtok(linha, " \t\n");

        // Processa todos os literais da linha
        while (token != NULL) 
        {
            literal = atoi(token);  // Converte string para inteiro

            if (literal == 0) break;  // Fim da cláusula (DIMACS termina cláusulas com 0)

            // Verifica se o literal está dentro do intervalo permitido
            if (!literal_valido(literal, f->num_literais))
            {
                // Se inválido, mostra erro e aborta
                printf("Erro: literal %d invalido (max: %d)\n", literal, f->num_literais);
                printf("UNSAT\n");  // Fórmula insatisfatível por erro
                free(clausula);    // Libera memória da cláusula mal formada
                fclose(arquivo);   // Fecha o arquivo
                return NULL;       // Sinaliza erro
            }

            clausula[tamanho++] = literal;      // Armazena literal na cláusula
            token = strtok(NULL, " \t\n");      // Vai para o próximo número
        }

        clausula[tamanho] = 0;                  // Adiciona terminador 0 no final da cláusula

        f->clausulas[clausula_atual++] = clausula;  // Guarda a cláusula no array de cláusulas da fórmula
    }

    fclose(arquivo);  // Fecha o arquivo após leitura completa

    return f;         // Retorna ponteiro para a fórmula construída
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

/**
 * Verifica se uma cláusula está satisfeita dada uma interpretação
 * @param clausula Ponteiro para a cláusula a verificar
 * @param interpretacao Vetor com valores atribuídos às variáveis
 * @return true se a cláusula está satisfeita, false caso contrário
 */
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

/**
 * Verifica se toda a fórmula está satisfeita
 * @param f Ponteiro para a fórmula
 * @param interpretacao Vetor de atribuições
 * @return true se todas as cláusulas estão satisfeitas
 */
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

/**
 * Verifica se a fórmula é insatisfatível com a interpretação atual
 * @param f Ponteiro para a fórmula
 * @param interpretacao Vetor de atribuições
 * @return true se alguma cláusula é insatisfatível
 */
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

/**
 * Encontra a próxima variável não atribuída
 * @param f Ponteiro para a fórmula
 * @param interpretacao Vetor de atribuições
 * @return Índice da próxima variável ou 0 se todas estiverem atribuídas
 */
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

void gerarCNF(Grafo *g, int k, const char *nome_arquivo) {
    FILE *f = fopen(nome_arquivo, "w");
    if (!f) {
        perror("Erro ao criar arquivo CNF");
        return;
    }

    int n = g->n_vertices;
    int m = g->m_arestas;

    int num_vars = n * k;
    int num_clauses = 0;

    // Estimativa das cláusulas (n precisa ser exata, mas ajuda)
    num_clauses += n;                       // Regra 1: pelo menos uma cor
    num_clauses += n * (k * (k - 1)) / 2;   // Regra 2: no máximo uma cor
    num_clauses += m * k;                  // Regra 3: adjacentes

    fprintf(f, "p cnf %d %d\n", num_vars, num_clauses);

    // === Regra 1: cada vértice com pelo menos uma cor ===
    for (int v = 1; v <= n; v++) {          // Para cada vértice do grafo
        for (int c = 1; c <= k; c++) {      // Para cada cor possível
            int var = (v - 1) * k + c;      // Calcula o índice da variável (v-1)*k + c → mapeia vértice + cor para literal
            fprintf(f, "%d ", var);         // Escreve o literal correspondente (positivo) → quer dizer: "v tem cor c"
        }
        fprintf(f, "0\n");                  // Termina a cláusula com 0 (requisito do formato DIMACS)
    }


    // === Regra 2: vértice não pode ter 2 cores ao mesmo tempo ===
    for (int v = 1; v <= n; v++) {                    // Para cada vértice
        for (int c1 = 1; c1 <= k; c1++) {             // Para cada cor
            for (int c2 = c1 + 1; c2 <= k; c2++) {    // Compara com todas as cores seguintes (evita repetição)
                int var1 = (v - 1) * k + c1;          // Literal para vértice v com cor c1
                int var2 = (v - 1) * k + c2;          // Literal para vértice v com cor c2
                fprintf(f, "-%d -%d 0\n", var1, var2); // Escreve cláusula de exclusão: "-var1 ou -var2"
            }
        }
    }


    // === Regra 3: vértices adjacentes não podem ter mesma cor ===
    for (int i = 0; i < m; i++) {                     // Para cada aresta do grafo
        int u = g->arestas[i][0] + 1;                 // Vértice de origem (ajustando de 0 para 1)
        int v = g->arestas[i][1] + 1;                 // Vértice de destino (ajustando de 0 para 1)

        for (int c = 1; c <= k; c++) {                // Para cada cor
            int var_u = (u - 1) * k + c;              // Literal que diz: "u tem cor c"
            int var_v = (v - 1) * k + c;              // Literal que diz: "v tem cor c"
            fprintf(f, "-%d -%d 0\n", var_u, var_v);  // Cláusula dizendo que os dois **não podem** ter a mesma cor
        }
    }


    fclose(f);
}


int main() { 

    setlocale(LC_ALL, "pt_BR.UTF-8");  // Define a localização/idioma para o sistema, garantindo acentuação correta no terminal

    Grafo g;        // Declara uma variável do tipo Grafo para armazenar os dados lidos do arquivo (vértices e arestas)
    int k = 1;      // Inicializa a quantidade de cores K com 1. Esse valor será usado na versão iterativa.
    int option;     // Variável que vai armazenar a escolha do usuário no menu (1 ou 2)

    // Exibe o menu de opções para o usuário escolher o tipo de teste que deseja realizar
    printf("Boas vindas! Deseja qual tipo de teste?\n\n1 - Teste Único\n2 - Teste Iterativo\n\nDigite uma opção: ");
    scanf("%d", &option);  // Lê do teclado a opção digitada pelo usuário e armazena na variável option

    // ==========================
    // OPÇÃO 1: TESTE ÚNICO
    // ==========================
    if( option == 1 )  // Se o usuário escolheu o Teste Único
    {
        int k_manual;  // Variável para guardar o valor de K que será digitado manualmente pelo usuário

        // Tenta abrir e ler o grafo a partir do arquivo grafo.txt
        if (!lerGrafo("grafo.txt", &g)) {
            // Se a leitura falhar, mostra mensagem de erro e encerra o programa
            printf("Erro ao ler grafo!\n");
            return 1;
        }

        // Mostra a quantidade de vértices e arestas lidas do arquivo
        printf("Grafo com %d vertices e %d arestas.\n", g.n_vertices, g.m_arestas);

        // Pede ao usuário que digite a quantidade de cores desejada
        printf("Digite a quantidade de cores (K): ");
        scanf("%d", &k_manual); // Lê o valor de K digitado

        // Gera o arquivo CNF com base no grafo e no valor K digitado
        gerarCNF(&g, k_manual, "sat.cnf");

        // Lê a fórmula CNF do arquivo que foi gerado
        Formula *f = ler_formula("sat.cnf");
        if (!f) { // Se ocorrer erro ao ler a fórmula
            printf("Erro ao processar CNF.\n");
            return 1;
        }

        // Aloca um vetor para armazenar a interpretação (valores atribuídos às variáveis)
        int *interpretacao = calloc( f->num_literais + 1 , sizeof(int) );  // +1 pois não usamos o índice 0
        if (!interpretacao) { // Se falhar a alocação
            liberar_formula(f); // Libera a memória da fórmula antes de sair
            printf("Erro ao alocar vetor de interpretação.\n");
            return 1;
        }

        // Aloca a raiz da árvore binária usada na busca pelo SAT
        BinaryTree *raiz = malloc(sizeof(BinaryTree));
        if (!raiz) { // Se falhar a alocação da raiz
            free(interpretacao);         // Libera a memória da interpretação
            liberar_formula(f);          // Libera a fórmula CNF
            printf("Erro ao alocar árvore.\n");
            return 1;
        }

        // Inicializa os campos da raiz
        raiz->variavel = 0;               // Variável 0 (ainda não atribuída)
        raiz->valor = 0;                  // Valor neutro, nem negativo e nem positivo
        raiz->esquerda = raiz->direita = NULL; // Ponteiros nulos (sem filhos ainda)

        // Executa o SAT Solver com a fórmula e a raiz da árvore
        if (SAT(f, interpretacao, raiz)) {
            // Se for satisfatível, imprime o resultado
            printf("✔ SAT: fórmula satisfatível!\n");
            printf("Coloração possível com %d cor%s!\n", k_manual, k_manual > 1 ? "es" : "");
            printf("Coloração encontrada:\n");

            // Varre todos os vértices e imprime a cor atribuída a cada um
            for (int v = 1; v <= g.n_vertices; v++) {
                for (int cor = 1; cor <= k_manual; cor++) {
                    int var_index = (v - 1) * k_manual + cor; // Cálculo da posição do literal na interpretação
                    if (interpretacao[var_index] == 1) { // Se a variável está como verdadeira
                        printf("Vértice %d --> cor %d\n", v, cor); // Imprime a cor daquele vértice
                        break; // Sai do loop das cores (já encontrou a cor do vértice)
                    }
                }
            }

        } else {
            // Se não for satisfatível, imprime que não é possível
            printf("✘ UNSAT: não é possível colorir com %d cor%s.\n", k_manual, k_manual > 1 ? "es" : "");
        }

        // Libera a memória alocada
        liberar_formula(f);
        free(interpretacao);
        liberar_arvore(raiz);
    }

    // ==========================
    // OPÇÃO 2: TESTE ITERATIVO
    // ==========================
    else if( option == 2 )  // Se o usuário escolheu Teste Iterativo
    {
        // Tenta ler o grafo a partir do arquivo
        if (!lerGrafo("grafo.txt", &g)) {
            printf("Erro ao ler grafo!\n");
            return 1;
        }

        // Mostra as informações do grafo lido
        printf("Grafo com %d vertices e %d arestas.\n", g.n_vertices, g.m_arestas); 

        // Loop para tentar coloração com 1 até N ( quantidade de vértices) cores
        while (k <= g.n_vertices) {
            // Informa o valor atual de K que será testado
            printf("Testando coloração com %d cor%s...\n", k, k > 1 ? "es" : "");

            // Gera o arquivo CNF com o valor atual de K
            gerarCNF(&g, k, "sat.cnf");

            // Lê a fórmula CNF gerada
            Formula *f = ler_formula("sat.cnf");
            if (!f) {
                printf("Erro ao processar CNF.\n");
                return 1;
            }

            // Aloca o vetor de interpretação
            int *interpretacao = calloc( f->num_literais + 1 , sizeof(int) );
            if (!interpretacao) {   // Se houver algum problema, finaliza
                liberar_formula(f);
                printf("Erro ao alocar vetor de interpretação.\n");
                return 1;
            }

            // Aloca e inicializa a raiz da árvore de decisão
            BinaryTree *raiz = malloc(sizeof(BinaryTree));
            if (!raiz) {    // Se houver algum problema, finaliza
                free(interpretacao);
                liberar_formula(f);
                printf("Erro ao alocar árvore.\n");
                return 1;
            }
            raiz->variavel = 0;
            raiz->valor = 0;
            raiz->esquerda = raiz->direita = NULL;

            // Executa o SAT Solver
            if (SAT(f, interpretacao, raiz)) {
                // Se for satisfatível, imprime a coloração encontrada
                printf("✔ SAT: fórmula satisfatível!\n");
                printf("Coloração possível com %d cor%s!\n", k, k > 1 ? "es" : "");
                printf("Coloração encontrada:\n");

                // Para cada vértice, imprime sua cor
                for (int v = 1; v <= g.n_vertices; v++) {
                    for (int cor = 1; cor <= k; cor++) {
                        int var_index = (v - 1) * k + cor;
                        if (interpretacao[var_index] == 1) {
                            printf("Vértice %d --> cor %d\n", v, cor);
                            break;
                        }
                    }
                }

                // Libera memória e encerra o programa (já encontrou solução)
                liberar_formula(f);
                free(interpretacao);
                liberar_arvore(raiz);
                return 0;
            }

            // Se não for satisfatível, tenta com mais uma cor
            liberar_formula(f);
            free(interpretacao);
            liberar_arvore(raiz);
            printf("✘ Nao eh possivel com %d cor%s.\n\n", k, k > 1 ? "es" : "");
            
            k++; // Incrementa o número de cores e continua o loop
        }

        // Se o loop terminar sem encontrar coloração, imprime mensagem final
        printf("Nenhuma coloracao possivel com K ≤ %d. Verifique o grafo.\n", g.n_vertices);
    }

    // ==========================
    // OPÇÃO INVÁLIDA
    // ==========================
    else {
        // Caso o usuário digite uma opção diferente de 1 ou 2
        printf("Opção inexistente!\nFechando o programa.\n");
    }

    return 0; // Retorna 0, indicando que o programa terminou com sucesso
}

/** REGISTRO DE ALTERAÇÕES
 * 
 * 1ª ALTERAÇÃO: criação da struct Grafo
 * 2ª ALTERAÇÃO: nova main
 * 3ª ALTERAÇÃO: função lerGrafo
 * 4ª ALTERAÇÃO: função gerarCNF
 * 5ª ALTERAÇÃO: criação de menu
 * 
 */
