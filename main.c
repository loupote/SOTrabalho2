#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "func.h"

pid_t pids[NUM_PROCESS];

int main()
{
    printf("Number of processes: %d\n", NUM_PROCESS);
    printf("Number of page frames in memory: %d\n", SIZE_RAM);
    printf("Number of pages in each page table: %d\n", SIZE_PROCESS);
    printf("\n");

    // criacao_arquivos();

    char *paginavirtual = (char *)malloc(2 * sizeof(char));
    char *rw = (char *)malloc(sizeof(char));

    // Inicialização da memória
    PageFrame *memoria = (PageFrame *)malloc(SIZE_RAM * sizeof(PageFrame));
    for (int i = 0; i < SIZE_RAM; i++)
    {
        memoria[i].virtualaddress = -1;
    }

    PageTable *pagetables = (PageTable *)malloc(NUM_PROCESS * sizeof(PageTable));

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

            while (1)
            {
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

                // Request to the GMV 'endereco' via pipes
                write(fd[1], endereco, 2);
                write(fd[1], acesso, 1);
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
        sleep(1);

        // GMV
        read(fd[0], paginavirtual, 2);
        read(fd[0], rw, 1);
        printf("Request from process %d to page %s in %s\n", process_num, paginavirtual, rw);

        // Buscar na tabela de pagina correspondante ao processo atual
        Page entrada_pagina = pagetables[process_num].mapping[*paginavirtual];
        if (entrada_pagina.presenca == 0)
        { // A pagina nao está presente na memória
            int j = 0;
            while ((memoria[j].virtualaddress != -1) && (j < SIZE_RAM))
            {
                j++;
            }
            if (memoria[j].virtualaddress == -1)
            {
                printf("Free space in memory\n");
                entrada_pagina.pfnumber = j;
                memoria[j].virtualaddress = *paginavirtual;
                memoria[j].processnum = process_num;
                printf("Memory page frame %d/%d contains now logical address %d of process %d\n", j, SIZE_RAM, memoria[j].virtualaddress, memoria[j].processnum);
            }
            else if (j == SIZE_RAM)
            {
                printf("No more free space in memory\n");
                // *ALGORITHM*
            }
            else
            {
                perror("Condiçoes IF");
                exit(EXIT_FAILURE);
            }
        }
        else
        { // A pagina esta presente na memória: HIT
            // The 'referenced' counter is reset if the line is "R"
            // We modify the referenced field if it was 0 to 1 (depends on the DeltaT we chose)
            if (*rw == 'R')
            {
                entrada_pagina.contador_ref = 0;
                entrada_pagina.referenced = 1;
                printf("A pagina %d do processo %d foi acesso em leitura\n", *paginavirtual, process_num);
            }
            // The 'modified' counter is reset if the line is "W"
            // We change the modified field if it was an access to write ("W")
            else if (*rw == 'W')
            {
                entrada_pagina.contador_mod = 0;
                entrada_pagina.modified = 1;
                printf("A pagina %d do processo %d foi acesso em escrita \n", *paginavirtual, process_num);
            }

            // For all the other pages (from all the processes)
            for (int k = 0; k < NUM_PROCESS; k++)
            {
                for (int l = 0; l < SIZE_PROCESS; l++)
                {
                    if ((k != process_num) && (l != *paginavirtual))
                    {
                        entrada_pagina.contador_mod++;
                        entrada_pagina.contador_ref++;

                        // Compare the 'ref' and 'mod' counters with delta_T
                        // If ref_counter greater than delta_T, decrement ref_counter
                        // If mod_counter greater than delta_T, decrement mod_counter
                        if (entrada_pagina.contador_mod >= DELTA_T)
                        {
                            // Se a pagina nao for alterada há muito tempo
                            entrada_pagina.modified = 0;
                            printf("A pagina %d do processo %d nao foi modificada há muito tempo.\n", l, k);
                        }
                        if (entrada_pagina.contador_ref >= DELTA_T)
                        {
                            // Se a pagina nao for referenciada há muito tempo
                            entrada_pagina.referenced = 0;
                            printf("A pagina %d do processo %d nao foi referenciada há muito tempo.\n", l, k);
                        }
                    }
                }
            }
        }

        process_num = (process_num + 1) % NUM_PROCESS;
        n++;

        sleep(1);
        printf("\n");
    }
    close(fd[0]);

    free(paginavirtual);
    free(rw);
    free(memoria);
    free(pagetables);

    return 0;
}