#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "func.h"

pid_t pids[NUM_PROCESS];

void inicializa_pagina(Page *entrada)
{
    entrada->referenced = 0;
    entrada->modified = 0;
    entrada->contador_ref = 0;
    entrada->contador_mod = 0;
    entrada->presenca = 0;
    entrada->pfnumber = -1;
}

int main(int argc, char *argv[])
{

    char *algoritmo = argv[1];
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <algorithm>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(algoritmo, "aleatorio") != 0 && strcmp(algoritmo, "NRU") != 0 && strcmp(algoritmo, "2nCh") != 0 && strcmp(algoritmo, "LRU") != 0 && strcmp(algoritmo, "WS") != 0) {
        printf("Not a valid argument. Please select between aleatorio, NRU, 2nCh, LRU, or WS.\n");
        return EXIT_FAILURE;
    }

    printf("Selected algorithm: %s\n", algoritmo);
    printf("Number of processes: %d\n", NUM_PROCESS);
    printf("Number of page frames in memory: %d\n", SIZE_RAM);
    printf("Number of pages in each page table: %d\n", SIZE_PROCESS);
    printf("\n");

    //criacao_arquivos();

    char vpaux[2]; // Process page requested to the GMV
    char rwaux;    // Access to this request page

    // Inicialização da memória
    PageFrame *memoria = (PageFrame *)malloc(SIZE_RAM * sizeof(PageFrame));
    if (memoria == NULL)
    {
        perror("Erro ao alocar memória para memoria");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < SIZE_RAM; i++)
    {
        memoria[i].virtualaddress = -1;
        memoria[i].processnum = -1;
    }

    PageTable *pagetables = (PageTable *)malloc(NUM_PROCESS * sizeof(PageTable));
    if (pagetables == NULL)
    {
        perror("Erro ao alocar memória para pagetables");
        exit(EXIT_FAILURE);
    }

    // Inicializing each page from each process
    for (int i = 0; i < NUM_PROCESS; i++)
    {
        for (int j = 0; j < SIZE_PROCESS; j++)
        {
            Page *entrada = &(pagetables[i].mapping[j]);
            inicializa_pagina(entrada);
        }
    }

    // Abertura de arquivos
    char nome_arquivo[20];
    FILE *fp[NUM_PROCESS];
    int fd[2];

    pipe(fd);

    for (int i = 0; i < NUM_PROCESS; i++)
    {
        snprintf(nome_arquivo, sizeof(nome_arquivo), "acesso_P%d.txt", i);

        fp[i] = fopen(nome_arquivo, "r");
        if (fp[i] == NULL)
        {
            perror("File opener");
        }
    }

    // Criaçao dos processos
    int status;
    for (int i = 0; i < NUM_PROCESS; i++)
    {
        if ((pids[i] = fork()) < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        else if (pids[i] == 0)
        {                 // Filhos
            close(fd[0]); // Fecha a leitura
            int cont = 0;
            while (cont < SIZE_PROCESS)
            { // Envio dos requests por pipes
                kill(getpid(), SIGSTOP);

                char buffer[5]; // 4 chars + 1 for null terminator for print
                int cont = 0;

                while (cont < 4)
                {
                    int c = fgetc(fp[i]);
                    if (c == EOF)
                    {
                        break; // end of file or error
                    }
                    buffer[cont++] = (char)c;
                }
                fgetc(fp[i]); // To remove the end of line

                // Printing part
                char *endereco = strtok(buffer, " ");
                char *acesso = strtok(NULL, " ");
                printf("Endereço: %s, acesso: %s\n", endereco, acesso);

                // Request to the GMV 'endereco' atraves de pipes
                write(fd[1], endereco, 2);
                write(fd[1], acesso, 1);

                cont++;
            }
            close(fd[1]);
            exit(0);
        }
    }

    // Processo pai
    sleep(1);

    int process_num = 0;
    int n = 0;

    // Round-Robin
    while (n < NUM_PROCESS * NUM_LINHAS)
    {
        kill(pids[process_num], SIGCONT);
        usleep(100000);

        // GMV
        read(fd[0], vpaux, 2);
        read(fd[0], &rwaux, 1);
        int paginavirtual = atoi(vpaux);
        char rw = rwaux;
        printf("Request from process %d to page %d in %c\n", process_num, paginavirtual, rw);

        // Buscar a página requisita na tabela de pagina correspondante ao processo atual
        Page *entrada_pagina = &(pagetables[process_num].mapping[paginavirtual]);
        if (entrada_pagina->presenca == 0)
        { // A pagina nao está presente na memória
            int j = 0;
            while ((memoria[j].virtualaddress != -1) && (j < SIZE_RAM))
            {
                j++;
            }
            if (memoria[j].virtualaddress == -1)
            { // Tem espaço livre na memória
                printf("Free space in memory\n");
                memoria[j].virtualaddress = paginavirtual;
                memoria[j].processnum = process_num;
                printf("Memory page frame %d/%d contains now logical address %d of process %d\n", j + 1, SIZE_RAM, memoria[j].virtualaddress, memoria[j].processnum);
            }
            else if (j == SIZE_RAM)
            { // Nao tem espaço livre
                printf("No more free space in memory\n");
                // *ALGORITHM*
                int alea;
                if (strcmp(algoritmo, "aleatorio") == 0){
                    alea = rand() % SIZE_RAM;
                }
                else{
                    printf("Ainda nao foi desenvolvido nenhum algoritmo. Escolhendo aleatorio....\n");
                    alea = rand() % SIZE_RAM;
                }
                printf("Page frame a ser removida: posicao %d, endereço virtual %d/%d do processo %d\n", alea, memoria[alea].virtualaddress, SIZE_PROCESS, memoria[alea].processnum);
                printf("Substituicao...\n");
                memoria[alea].virtualaddress = paginavirtual;
                memoria[alea].processnum = process_num;
                Page *entrada_removida = &(pagetables[memoria[alea].processnum].mapping[memoria[alea].virtualaddress]);
                inicializa_pagina(entrada_removida);
                printf("Nova page frame na mesma posicao %d: endereço virtual %d do processo %d\n", alea, memoria[alea].virtualaddress, memoria[alea].processnum);
            }
            else
            {
                perror("Condiçoes IF");
                exit(EXIT_FAILURE);
            }
            entrada_pagina->pfnumber = j;
            entrada_pagina->presenca = 1;
        }
        if (rw == 'R')
        //  The 'referenced' counter is reset if the line is "R"
        //  We modify the referenced field if it was 0 to 1 (depends on the DeltaT we chose)
        {
            entrada_pagina->contador_ref = 0;
            entrada_pagina->referenced = 1;
            printf("A pagina %d do processo %d acabou de ser acesso em leitura\n", paginavirtual, process_num);
        }
        else if (rw == 'W')
        // The 'modified' counter is reset if the line is "W"
        // We change the modified field if it was an access to write ("W")
        {
            entrada_pagina->contador_mod = 0;
            entrada_pagina->modified = 1;
            printf("A pagina %d do processo %d acabou de ser acesso em escrita\n", paginavirtual, process_num);
        }

        // Update refernced and modified fields if across time
        for (int k = 0; k < NUM_PROCESS; k++)
        {
            for (int l = 0; l < SIZE_PROCESS; l++)
            {
                Page *entrada_pagina = &(pagetables[k].mapping[l]);
                if ((k != process_num) || (l != paginavirtual))
                {
                    if (entrada_pagina->presenca == 1)
                    {

                        entrada_pagina->contador_mod++;
                        entrada_pagina->contador_ref++;

                        // Compare the 'ref' and 'mod' counters with delta_T
                        // If ref_counter greater than delta_T, decrement ref_counter
                        // If mod_counter greater than delta_T, decrement mod_counter
                    }
                    if (entrada_pagina->contador_mod >= DELTA_T)
                    {
                        if (entrada_pagina->modified == 1){
                            // Se a pagina nao for alterada há muito tempo
                            entrada_pagina->modified = 0;
                            printf("A pagina %d do processo %d nao foi modificada há muito tempo.\n", l, k);
                        }
                    }
                    if (entrada_pagina->contador_ref >= DELTA_T)
                    {
                        if (entrada_pagina->modified == 1){
                            // Se a pagina nao for referenciada há muito tempo
                            entrada_pagina->referenced = 0;
                        printf("A pagina %d do processo %d nao foi referenciada há muito tempo.\n", l, k);
                        }
                    }
                }
                //PRINTS PARA VER O AUMENTO DOS CONTADORES
                if (l==16){
                    printf("Processo %d linha %d, contadores: mod (%d), ref (%d)\n", k, l, entrada_pagina->contador_mod, entrada_pagina->contador_ref);
                }
            }
        }

        process_num = (process_num + 1) % NUM_PROCESS;
        n++;

        usleep(100000);
        printf("\n");
    }

    waitpid(-1, &status, 0);
    waitpid(-1, &status, 0);
    waitpid(-1, &status, 0);
    waitpid(-1, &status, 0);

    close(fd[0]);

    free(memoria);
    free(pagetables);

    return 0;
}