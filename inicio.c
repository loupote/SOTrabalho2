#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define NUM_PROCESSOS 4
#define NUM_LINHAS 101

int main(){
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