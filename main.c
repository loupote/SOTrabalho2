#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#include "func.h"

pid_t pids[NUM_PROCESS];

void inicializa_pagina(Page *entrada)
{
    entrada->referenced = 0;
    entrada->modified = 0;
    entrada->counter = 0;
    entrada->contador_mod = 0;
    entrada->presenca = 0;
    entrada->pfnumber = -1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <algoritmo> [k_ws]\n", argv[0]);
        fprintf(stderr, "Algoritmos: NRU, 2a_chance, LRU, WS\n");
        return EXIT_FAILURE;
    }
    char *algoritmo = argv[1];
    int k_ws = 5; // Valor padrão para k do Working Set

    if (strcmp(algoritmo, "NRU") != 0 && strcmp(algoritmo, "2a_chance") != 0 && strcmp(algoritmo, "LRU") != 0 && strcmp(algoritmo, "WS") != 0) {
        printf("Argumento invalido. Selecione entre: NRU, 2a_chance, LRU, WS.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(algoritmo, "WS") == 0) {
        if (argc > 2) {
            k_ws = atoi(argv[2]);
        }
        printf("Algoritmo de Substituicao WorkingSet(k=%d)\n", k_ws);
    } else {
        printf("Algoritmo de Substituicao %s\n", algoritmo);
    }
    
    int total_rounds = NUM_PROCESS * NUM_LINHAS;
    printf("Rodadas a executar: %d\n", total_rounds);
    printf("----------------------------------------\n");

    srand(time(NULL));

    char vpaux[3];
    char rwaux;

    PageFrame *memoria = (PageFrame *)malloc(SIZE_RAM * sizeof(PageFrame));
    for (int i = 0; i < SIZE_RAM; i++) {
        memoria[i].virtualaddress = -1;
        memoria[i].processnum = -1;
    }

    PageTable *pagetables = (PageTable *)malloc(NUM_PROCESS * sizeof(PageTable));
    for (int i = 0; i < NUM_PROCESS; i++) {
        for (int j = 0; j < SIZE_PROCESS; j++) {
            inicializa_pagina(&(pagetables[i].mapping[j]));
        }
    }

    char nome_arquivo[20];
    FILE *fp[NUM_PROCESS];
    int fd[2];
    pipe(fd);

    for (int i = 0; i < NUM_PROCESS; i++) {
        snprintf(nome_arquivo, sizeof(nome_arquivo), "acesso_P%d.txt", i);
        fp[i] = fopen(nome_arquivo, "r");
        if (fp[i] == NULL) {
            perror("File opener");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_PROCESS; i++) {
        if ((pids[i] = fork()) < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pids[i] == 0) { // Filhos
            close(fd[0]);
            char buffer[6];
            while(fgets(buffer, sizeof(buffer), fp[i]) != NULL) {
                kill(getpid(), SIGSTOP);
                write(fd[1], buffer, strlen(buffer));
            }
            close(fd[1]);
            fclose(fp[i]);
            exit(0);
        }
    }

    sleep(1);

    int process_num = 0;
    int n = 0;
    int clock_hand = 0; // Ponteiro para o algoritmo da Segunda Chance

    while (n < total_rounds) {
        // Atualização periódica dos contadores para os algoritmos
        if (strcmp(algoritmo, "LRU") == 0) { // Lógica do Aging para LRU
             for (int i = 0; i < NUM_PROCESS; i++) {
                for (int j = 0; j < SIZE_PROCESS; j++) {
                    if (pagetables[i].mapping[j].presenca == 1) {
                        pagetables[i].mapping[j].counter >>= 1;
                    }
                }
            }
        }

        kill(pids[process_num], SIGCONT);

        char read_buffer[6];
        read(fd[0], read_buffer, 5);
        read_buffer[5] = '\0';

        sscanf(read_buffer, "%s %c", vpaux, &rwaux);
        
        int paginavirtual = atoi(vpaux);
        char rw = rwaux;

        Page *entrada_pagina = &(pagetables[process_num].mapping[paginavirtual]);
        if (entrada_pagina->presenca == 0) {
            printf("Page Fault: Processo P%d requisitou pagina %d\n", process_num, paginavirtual);
            
            int quadro_livre = -1;
            for(int i = 0; i < SIZE_RAM; i++) {
                if(memoria[i].virtualaddress == -1) {
                    quadro_livre = i;
                    break;
                }
            }

            if (quadro_livre != -1) { // Existe quadro de página livre
                memoria[quadro_livre].virtualaddress = paginavirtual;
                memoria[quadro_livre].processnum = process_num;
                entrada_pagina->pfnumber = quadro_livre;
                entrada_pagina->presenca = 1;
                printf("  -> Alocado no quadro de pagina livre %d.\n", quadro_livre);
            } else { // Memória cheia, precisa substituir
                int victim_frame = -1;
                if (strcmp(algoritmo, "NRU") == 0) {
                    victim_frame = select_NRU(pagetables, memoria);
                } else if (strcmp(algoritmo, "2a_chance") == 0) {
                    victim_frame = select_2a_chance(pagetables, memoria, &clock_hand);
                } else if (strcmp(algoritmo, "LRU") == 0) {
                    victim_frame = select_LRU(process_num, pagetables, memoria);
                } else if (strcmp(algoritmo, "WS") == 0) {
                    victim_frame = select_WS(process_num, pagetables, memoria, k_ws);
                } else {
                    victim_frame = rand() % SIZE_RAM; // Aleatório (fallback)
                }

                int old_p_num = memoria[victim_frame].processnum;
                int old_v_addr = memoria[victim_frame].virtualaddress;
                Page *pagina_removida = &(pagetables[old_p_num].mapping[old_v_addr]);

                if (pagina_removida->modified == 1) {
                    printf("  -> Pagina suja (P%d, pag %d) sera escrita no swap.\n", old_p_num, old_v_addr);
                }
                
                printf("  -> Substituindo pagina %d do processo P%d no quadro %d.\n", old_v_addr, old_p_num, victim_frame);
                
                inicializa_pagina(pagina_removida);

                memoria[victim_frame].virtualaddress = paginavirtual;
                memoria[victim_frame].processnum = process_num;
                entrada_pagina->pfnumber = victim_frame;
                entrada_pagina->presenca = 1;
            }
        }
        
        // Atualiza bits de referência e modificação
        entrada_pagina->referenced = 1;
        if (strcmp(algoritmo, "LRU") == 0) {
            entrada_pagina->counter |= (1 << 31);
        } else { // Para WS e NRU, o contador é o tempo desde o último acesso
             entrada_pagina->counter = 0;
        }

        if (rw == 'W') {
            entrada_pagina->modified = 1;
        }

        // Incrementa contadores para páginas não acessadas (usado por WS e NRU)
        for (int k = 0; k < NUM_PROCESS; k++) {
            for (int l = 0; l < SIZE_PROCESS; l++) {
                 Page *p = &(pagetables[k].mapping[l]);
                if (p->presenca == 1 && (k != process_num || l != paginavirtual)) {
                    p->counter++;
                }
                // Lógica para resetar o bit R para o NRU
                 if (p->counter >= DELTA_T) {
                    p->referenced = 0;
                }
            }
        }

        process_num = (process_num + 1) % NUM_PROCESS;
        n++;
    }

    for (int i = 0; i < NUM_PROCESS; i++) {
        kill(pids[i], SIGKILL);
        wait(NULL);
    }
    
    close(fd[0]);
    close(fd[1]);

    print_page_tables(pagetables);

    free(memoria);
    free(pagetables);

    return 0;
}