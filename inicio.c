#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define NUM_PROCESSOS 4
#define NUM_LINHAS 101
#define SIZE_RAM 16
#define DELTA_T 7 //Number of cycles before a process gets dereferenced and demodified

typedef struct{ //Representa a página
    char referenced; //0 ou 1 se for referenciado recentemente
    char modified; //0 ou 1 se uma linha com
    char contador_ref; // Para saber há quanto tempo a página nao foi referenciada
    char contador_mod; // Para saber há quanto tempo a pagina nao foi modificada
    char presenca; //1 se a página presente na RAM, 0 se estiver fora
    long pfnumber; //Em qual endereco da RAM fica essa página

} Page;


typedef struct{
    Page mapping[32]; //Contém numeros entre 0 et 31 que representam o endereço da RAM
    // Ex: mapping[0] := [1, 0, 1, 14] -> o endereço 0 da memoria virtual representa o endereço 14 na RAM
} PageTable;



int criacao_arquivos(){
    char nome_arquivo[20];
    int fdoutput, novofdoutput;
    int pid[NUM_PROCESSOS];

    for (int i = 0; i < NUM_PROCESSOS; i++){
        snprintf(nome_arquivo, sizeof(nome_arquivo), "acesso_P%d.txt", i);

        if ((pid[i] = fork()) < 0){
            perror("Erro fork");
        }

        else if (pid[i] == 0){
            srand(getpid()); // Inicializa rand() com uma semente
            if ((fdoutput = open(nome_arquivo, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1){
                perror("open output file");
                exit(EXIT_FAILURE);
            }

            if ((novofdoutput = dup2(fdoutput, 1)) == -1){ // Redireciona stdout
                perror("dup2");
                close(fdoutput);
                exit(EXIT_FAILURE);
            }

            // Agora, tudo que for printf vai para o arquivo

            // Lê linha por linha da entrada padrão e escreve no arquivo
            int j = 0;
            while (j < NUM_LINHAS){
                char pagina = rand() % 32;            // valores entre 0 e 31
                char acesso = rand() % 2 ? 'W' : 'R'; // 'W' ou 'R'

                printf("%02d %c\n", pagina, acesso); // Exemplo: 04 R

                j++;
            }
            close(fdoutput);
            exit(0);
        }
    }
    return 0;
}



int main(){
    if (criacao_arquivos()){
        perror("Erro ao executar a funcao criacao_arquivos");
    }

    int *ram = (int*)malloc(SIZE_RAM*sizeof(int));
    PageTable *pt = (PageTable*)malloc(NUM_PROCESSOS*sizeof(PageTable));

    // Round-Robin on each file
    // Example: the first line of the first file is 04 R
    // We look for the address pt[0].pfnumber
    // If it is < 0, there is a page fault, the page is outside the RAM.
        //We have to swap out one page frame and swap in the one we need.
        // --> use of algorithm NRU and others
    // Else, it is a hit:
        // The 'referenced' counter is reset if the line is "R"
        // The 'modified' counter is reset if the line is "W"
        // We modify the referenced field if it was 0 to 1 (depends on the DeltaT we chose)
        // We change the modified field if it was an access to write ("W")
    // For all the other pages (from all the processes)
        // Compare the 'ref' and 'mod' counters with delta_T
        // If ref_counter greater than delta_T, decrement ref_counter
        // If mod_counter greater than delta_T, decrement mod_counter
    // Increment the number of cycles
    // Come back until there is no more access lines in the processes

    return 0;
}