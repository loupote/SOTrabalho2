#ifndef FUNC_H
#define FUNC_H

#define NUM_PROCESS 4
#define NUM_LINHAS 101
#define SIZE_RAM 16
#define SIZE_PROCESS 32
#define DELTA_T 7 // Number of cycles before a process gets dereferenced and demodified
#define PROBA 0.5

typedef struct
{                      // Representa a página
    unsigned int referenced;   // 0 ou 1 se for referenciado recentemente
    unsigned int modified;     // 0 ou 1 se uma linha com
    unsigned int counter;      // Contador para os algoritmos LRU/Aging e WS
    unsigned int contador_mod; // Para saber há quanto tempo a pagina nao foi modificada
    unsigned int presenca;     // 1 se a página presente na RAM, 0 se estiver fora

    int pfnumber;     // Em qual endereco da RAM fica essa página

} Page;

typedef struct
{
    Page mapping[SIZE_PROCESS]; // Contém numeros entre 0 et 31 que representam o endereço da RAM
} PageTable;

// Each memory page frame is made of a virtual address and the process it comes from
typedef struct{
    int processnum;
    int virtualaddress;
} PageFrame;

int criacao_arquivos();

// Funções dos algoritmos de substituição
int select_NRU(PageTable *pagetables, PageFrame *memoria);
int select_2a_chance(PageTable *pagetables, PageFrame *memoria, int *clock_hand);
int select_LRU(int process_num, PageTable *pagetables, PageFrame *memoria);
int select_WS(int process_num, PageTable *pagetables, PageFrame *memoria, int k);

// Função de utilidade
void print_page_tables(PageTable *pagetables);


#endif