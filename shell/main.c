#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void executeCommand(char *command, char **arguments, int background);

int main()
{

    char command[50];
    char *arguments[10];
    int background = 0;
    char *token;

    while (1)
    {

        printf("$ ");
        fflush(stdout);

        fgets(command, sizeof(command), stdin);

        //checa input vazio
        if (strlen(command) != 1)
        {
            command[strcspn(command, "\n")] = '\0';
        }

        token = strtok(command, " ");

        if (strcmp(token, "exit") == 0)
        {
            break;
        }

        //itera pelos tokens e guarda em arguments[]
        int i = 0;
        while (token != NULL)
        {
            arguments[i] = token;
            //printf("debug arg: %s\n", arguments[i]);
            token = strtok(NULL, " ");
            i++;
        }
        arguments[i] = NULL;

        //checa se o programa deve rodar em background
        if (i > 0 && strcmp(arguments[i - 1], "&") == 0)
        {
            background = 1;
            arguments[i - 1] = NULL;
        }
        else
        {
            background = 0;
        }
        executeCommand(arguments[0], arguments, background);
    }
    return 0;
}

void executeCommand(char *command, char **arguments, int background)
{

    pid_t pid = fork();
    int result;
    int i = 0;
    int status;

    //checa se o fork falhou
    if (pid < 0)
    {
        perror("fork failed");
        exit(1);
    }
    else if (pid == 0) //filho
    {
        result = execvp(command, arguments);
        if (result < 0)
        {
            perror("error");
            exit(1);
        }
    }
    else    //pai
    {
        //waitpid() faz com que o processo pai aguarde ate o filho concluir sua execucao, evitando zumbis
        if (!background)
        {
            waitpid(pid, &status, 0); 
        }
    }
}