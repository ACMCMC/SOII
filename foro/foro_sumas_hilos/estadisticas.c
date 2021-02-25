// Aldán Creo Mariño, SOII 2020/21

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void generar_estadisticas(int tam, int num_hilos, int diferencia, FILE *arch_estadisticas)
{
    fprintf(arch_estadisticas, "%d;%d;%d\n", tam, num_hilos, diferencia==0?0:1);
}

void run_test(int tam, int num_inicial_hil, int incremento_hil, int num_final_hil, FILE *arch_estadisticas)
{
    pid_t pid;
    int stat_loc, num_actual_hil, i, valor_correcto;
    char num_actual_hil_string[13], tam_string[13], valor_correcto_string[13];
    sprintf(tam_string, "%d", tam);
    valor_correcto = 0;
    for (i = 0; i <= tam; i++)
    {
        valor_correcto += i;
    }
    sprintf(valor_correcto_string, "%d", valor_correcto);
    printf("Haciendo test del sumatorio de 0 a %d, num inicial de hilos %d, incremento %d, num final %d\n", tam, num_inicial_hil, incremento_hil, num_final_hil);

    for (num_actual_hil = num_inicial_hil; num_actual_hil <= num_final_hil; num_actual_hil += incremento_hil)
    {
        if (num_actual_hil > 0)
        {
            sprintf(num_actual_hil_string, "%d", num_actual_hil);
            pid = fork();
            if (pid < 0)
            {
                perror("Error ejecutando fork(). Abortando.");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                if (execl("sumas_hilos.out", "sumas_hilos.out", tam_string, num_actual_hil_string, valor_correcto_string, (char *)NULL) < 0)
                {
                    perror("Error ejecutando execlp");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (wait(&stat_loc) == -1)
                {
                    perror("Error esperando por el proceso");
                    exit(EXIT_FAILURE);
                }
                else if (!WIFEXITED(stat_loc))
                {
                    fprintf(stderr, "Error en el proceso, no ha salido normalmente");
                    exit(EXIT_FAILURE);
                }
                else if (WEXITSTATUS(stat_loc) < 0)
                {
                    fprintf(stderr, "Error en el proceso");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    generar_estadisticas(tam, num_actual_hil, WEXITSTATUS(stat_loc), arch_estadisticas);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    int i, M_inicial, M_final, incremento_M, stat_loc, num_inicial_hil, incremento_hil, num_final_hil;
    pid_t pid = -1;

    FILE *arch_estadisticas;

    if (argc != 7)
    {
        fprintf(stderr, "Deben especificarse: [M inicial] [incremento] [M final] [numero inicial de hilos] [incremento] [numero final]\n");
        exit(EXIT_SUCCESS);
    }

    printf("PID del padre: %d\n", getpid());

    M_inicial = atoi(argv[1]);
    incremento_M = atoi(argv[2]);
    M_final = atoi(argv[3]);
    num_inicial_hil = atoi(argv[4]);
    incremento_hil = atoi(argv[5]);
    num_final_hil = atoi(argv[6]);

    arch_estadisticas = fopen("estadisticas.txt", "w");
    if (arch_estadisticas == NULL)
    {
        perror("Error abriendo el archivo de salida de estadísticas");
        return (EXIT_FAILURE);
    }

    for (i = M_inicial; (i <= M_final) && (pid != 0); i += incremento_M)
    {
        if (i > 0)
        {
            pid = fork();
            if (pid < 0)
            {
                perror("Error ejecutando fork(). Abortando.");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                run_test(i, num_inicial_hil, incremento_hil, num_final_hil, arch_estadisticas);
            }
            else
            {
                if ((pid = waitpid(pid, &stat_loc, 0)) == -1)
                {
                    perror("Error esperando por el proceso");
                    exit(EXIT_FAILURE);
                }
                else if (!WIFEXITED(stat_loc))
                {
                    fprintf(stderr, "Error en el proceso, no ha salido normalmente");
                    exit(EXIT_FAILURE);
                }
                else if (WEXITSTATUS(stat_loc) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "Error en el proceso, no ha devuelto EXIT_SUCCESS");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    if (fclose(arch_estadisticas))
    {
        perror("Error cerrando el archivo de salida de estadísticas");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}
