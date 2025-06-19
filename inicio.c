
#include <stdio.h>

#include <sys/shm.h>
#include <sys/stat.h>

#define NUM_PROCESSOS 4

typedef struct{
    char page_index;
    char acesso; // 0:leitura, 1:escrita
} linha;

int main(){
    int *pid = (int *)malloc(NUM_PROCESSOS * sizeof(int));
    int *shmid = (int *)malloc(NUM_PROCESSOS * sizeof(int));

    //Alocaçao da memória compartilhada e criaçao dos 4 processos filhos
    for (int i = 0; i < NUM_PROCESSOS; i++){
        shmid[i] = shmget(IPC_PRIVATE, sizeof(linha), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

        if ((pid[i] = fork()) == -1){
            perrorf("Erro ao criar o processo %d", i);
        }
    }

    // Geração aleatória de linhas
    

}