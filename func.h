
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
    unsigned int contador_ref; // Para saber há quanto tempo a página nao foi referenciada
    unsigned int contador_mod; // Para saber há quanto tempo a pagina nao foi modificada
    unsigned int presenca;     // 1 se a página presente na RAM, 0 se estiver fora

    int pfnumber;     // Em qual endereco da RAM fica essa página

} Page;

typedef struct
{
    Page mapping[SIZE_PROCESS]; // Contém numeros entre 0 et 31 que representam o endereço da RAM
    // Ex: mapping[0] := [1, 0, 1, 14] -> o endereço 0 da memoria virtual representa o endereço 14 na RAM
} PageTable;

// Each memory page frame is made of a virtual address and the process it comes from
typedef struct{
    int processnum;
    int virtualaddress;
} PageFrame;

int criacao_arquivos();

#endif