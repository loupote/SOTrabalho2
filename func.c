#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "func.h"

// Implementação dos algoritmos de substituição

/**
 * Algoritmo Not Recently Used (NRU) - Global
 * Encontra a melhor página para substituir baseando-se nos bits R e M.
 * Prioridade de remoção (menor para maior):
 * Classe 0: R=0, M=0
 * Classe 1: R=0, M=1
 * Classe 2: R=1, M=0
 * Classe 3: R=1, M=1
 */
int select_NRU(PageTable *pagetables, PageFrame *memoria) {
    int class0_victim = -1, class1_victim = -1, class2_victim = -1, class3_victim = -1;

    for (int i = 0; i < SIZE_RAM; i++) {
        int p_num = memoria[i].processnum;
        int v_addr = memoria[i].virtualaddress;
        Page *page = &pagetables[p_num].mapping[v_addr];

        if (page->referenced == 0 && page->modified == 0) {
            class0_victim = i;
            return class0_victim; // Melhor vítima
        }
        if (page->referenced == 0 && page->modified == 1) {
            class1_victim = i;
        }
        if (page->referenced == 1 && page->modified == 0) {
            if (class2_victim == -1) class2_victim = i;
        }
        if (page->referenced == 1 && page->modified == 1) {
            if (class3_victim == -1) class3_victim = i;
        }
    }

    if (class1_victim != -1) return class1_victim;
    if (class2_victim != -1) return class2_victim;
    if (class3_victim != -1) return class3_victim;

    return 0; // Fallback
}


/**
 * Algoritmo da Segunda Chance (Clock) - Global
 * Usa um ponteiro de relógio para varrer os quadros de página.
 * Se R=1, dá uma segunda chance (R=0) e avança.
 * Se R=0, seleciona como vítima.
 */
int select_2a_chance(PageTable *pagetables, PageFrame *memoria, int *clock_hand) {
    while (1) {
        int p_num = memoria[*clock_hand].processnum;
        int v_addr = memoria[*clock_hand].virtualaddress;
        Page *page = &pagetables[p_num].mapping[v_addr];

        if (page->referenced == 0) {
            int victim_frame = *clock_hand;
            *clock_hand = (*clock_hand + 1) % SIZE_RAM;
            return victim_frame;
        } else {
            page->referenced = 0;
            *clock_hand = (*clock_hand + 1) % SIZE_RAM;
        }
    }
}

/**
 * Algoritmo LRU/Aging - Local
 * Encontra a página menos recentemente usada no processo que causou o page fault,
 * selecionando a que tiver o menor valor no contador de "envelhecimento".
 */
int select_LRU(int process_num, PageTable *pagetables, PageFrame *memoria) {
    unsigned int min_counter = UINT_MAX;
    int victim_pf = -1;

    for (int i = 0; i < SIZE_PROCESS; i++) {
        Page *page = &pagetables[process_num].mapping[i];
        if (page->presenca == 1) {
            if (page->counter < min_counter) {
                min_counter = page->counter;
                victim_pf = page->pfnumber;
            }
        }
    }
    return victim_pf;
}

/**
 * Algoritmo Working Set (WS) - Local
 * Remove uma página do processo que está fora do seu working set (contador > k).
 * Prefere páginas não modificadas.
 */
int select_WS(int process_num, PageTable *pagetables, PageFrame *memoria, int k) {
    int best_victim = -1;
    unsigned int max_counter = 0;

    // Procura por uma página fora do WS (contador > k)
    for (int i = 0; i < SIZE_PROCESS; i++) {
        Page *page = &pagetables[process_num].mapping[i];
        if (page->presenca == 1 && page->counter > k) {
            // Se encontrarmos uma página não suja
            if (page->modified == 0) {
                return page->pfnumber;
            }
            // Senão, guarda a mais antiga (maior contador)
            if (page->counter > max_counter) {
                max_counter = page->counter;
                best_victim = page->pfnumber;
            }
        }
    }

    // Se uma vítima fora do WS foi encontrada (mesmo que suja)
    if (best_victim != -1) {
        return best_victim;
    }

    // Se todas as páginas estão no WS aplica LRU
    return select_LRU(process_num, pagetables, memoria);
}


void print_page_tables(PageTable *pagetables) {
    printf("\n--- Estado Final das Page Tables dos Processos ---\n");
    for (int i = 0; i < NUM_PROCESS; i++) {
        printf("Process %d:\n", i);
        printf("  Pagina Virtual | Pagina Fisica | Presente | Modificada | Referenciada | Contador\n");
        printf("----------------------------------------------------------------------------------\n");
        for (int j = 0; j < SIZE_PROCESS; j++) {
            Page *p = &pagetables[i].mapping[j];
            if (p->presenca == 1) {
                printf("        %02d       |      %02d       |     %d    |    %d       |        %d     |  %u\n",
                       j, p->pfnumber, p->presenca, p->modified, p->referenced, p->counter);
            }
        }
        printf("\n");
    }
}


char binomial(int n, double p) {
    char count = 0;
    for (int i = 0; i < n; i++) {
        double r = (double)rand() / RAND_MAX;
        if (r < p) count++;
    }
    return count;
}

int criacao_arquivos()
{
    char nome_arquivo[20];
    int fdoutput, novofdoutput;
    int pid[NUM_PROCESS];

    for (int i = 0; i < NUM_PROCESS; i++)
    {
        snprintf(nome_arquivo, sizeof(nome_arquivo), "acesso_P%d.txt", i);

        if ((pid[i] = fork()) < 0)
        {
            perror("Erro fork");
        }
        else if (pid[i] == 0)
        {
            srand(getpid());
            if ((fdoutput = open(nome_arquivo, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1)
            {
                perror("open output file");
                exit(EXIT_FAILURE);
            }

            if ((novofdoutput = dup2(fdoutput, 1)) == -1)
            {
                perror("dup2");
                close(fdoutput);
                exit(EXIT_FAILURE);
            }

            int j = 0;
            while (j < NUM_LINHAS)
            {
                char pagina = binomial(SIZE_PROCESS, PROBA);
                char acesso = rand() % 2 ? 'W' : 'R';

                printf("%02d %c\n", pagina, acesso);

                j++;
            }
            close(fdoutput);
            exit(0);
        }
    }
    return 0;
}