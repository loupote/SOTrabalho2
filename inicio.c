#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define NUM_PROCESSOS 4
#define NUM_LINHAS 101

int main() {
    int fdoutput, novofdoutput;

    if ((fdoutput = open("acesso_P1.txt", O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("open output file");
        exit(EXIT_FAILURE);
    }

    if ((novofdoutput = dup2(fdoutput, 1)) == -1) { // Redireciona stdout
        perror("dup2");
        close(fdoutput);
        exit(EXIT_FAILURE);
    }

    // Agora, tudo que for printf vai para o arquivo

    // Lê linha por linha da entrada padrão e escreve no arquivo
    int j = 0;
    while (j < NUM_LINHAS) {
    char pagina = rand() % 32;     // valores entre 0 e 31
    char acesso = rand() % 2 ? 'W' : 'R';  // 'W' ou 'R'

    printf("%02d %c\n", pagina, acesso);  // Exemplo: 04 R

    j++;
}

    close(fdoutput);
    return 0;
}