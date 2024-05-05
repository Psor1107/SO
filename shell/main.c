#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void executeCommand(char *command, char **arguments, int background);
void sigHandler(int signum);
void executePipe(char **command1, char **command2, int background);

int main()
{
    char command[50];
    char *arguments[10];
    int background = 0;
    char *token;
    while (1)
    {
        fflush(stdout);
        printf("$ ");
        fgets(command, sizeof(command), stdin);
        //checa input vazio
        if (strlen(command) != 1)
        {
            command[strcspn(command, "\n")] = '\0';
        }
        if (strstr(command, "exit") != NULL)
        {
            return 0;
        }
        token = strtok(command, " ");
        int i = 0;
        //itera pelos tokens e guarda os endereços em arguments[]
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
    int i = 0;
    int status;
    int oldSTDOUT = 0;
    //checa se o fork falhou
    if (pid < 0)
    {
        perror("fork error");
        return;
    }
    else if (pid == 0) //filho
    {
        //checa se existe o operador ">"
        int fd = -1;
        for (i = 0; arguments[i] != NULL; i++)
        {
            if (strcmp(arguments[i], ">") == 0)
            {
                fd = open(arguments[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
                if (fd == -1)
                {
                    perror("open failed");
                    exit(1);
                }
                oldSTDOUT = dup(STDOUT_FILENO);
                dup2(fd, STDOUT_FILENO);

                //tira ">" e o nome do arquivo
                arguments[i] = NULL;
                arguments[i + 1] = NULL;
                break;
            }
        }

        // checa se existe o operador "|"
        int pipeIndex = -1;
        for (i = 0; arguments[i] != NULL; i++)
        {
            if (strcmp(arguments[i], "|") == 0)
            {
            pipeIndex = i;
            break;
            }
        }

        int j = 0;
        int k = 0;

        if (pipeIndex != -1)
        {
            char *command1[10];
            char *command2[10];

            //printf("command index 1 vale: %d\n command index 2 vale: %d\n", pipeIndex + 1, i - pipeIndex);

            for (j = 0; j < pipeIndex; j++)
            {
            command1[j] = arguments[j];
            }
            command1[pipeIndex] = NULL;

            for(j = pipeIndex + 1, k = 0; arguments[j] != NULL; j++, k++){
                command2[k] = arguments[j];
            }
            command2[k] = NULL;
            executePipe(command1, command2, background);
        }
        else
        {
            if (execvp(command, arguments) < 0)
            {
                perror("exec pipe S");
                exit(1);
            }
        }
        if (oldSTDOUT != 0){
            dup2(oldSTDOUT, STDOUT_FILENO);
            close(fd);
        }
    }
    else //pai
    {
        //wait() em caso de execucao em fg
        if (!background)
        {
            wait(NULL);
            return;
        }
        signal(SIGCHLD, sigHandler);
    }
}

void sigHandler(int signum)
{
    pid_t pid;
    int status;
    pid = waitpid(0, &status, 0);
    //printf("Child %d terminated: %d %d\n",pid,signum,status);
}

void executePipe(char **command1, char **command2, int background)
{
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1)
    {
        perror("pipe error");
        exit(1);
    }

    pid1 = fork();
    if (pid1 < 0)
    {
        perror("fork error");
        exit(1);
    }
    else if (pid1 == 0)
    {
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);

        if (execvp(command1[0], command1) < 0)
        {
            perror("exec pipe C1 error");
            exit(1);
        }
    }
    else
    {
        pid2 = fork();
        if (pid2 < 0)
        {
            perror("fork error");
            exit(1);
        }
        else if (pid2 == 0)
        {
            close(pipefd[1]);

            dup2(pipefd[0], STDIN_FILENO);

            if (execvp(command2[0], command2) < 0)
            {
                perror("exec pipe C2 error");
                exit(1);
            }
        }
        else
        {
            close(pipefd[0]);
            close(pipefd[1]);

            if (!background)
            {
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
            }
            else
            {
                signal(SIGCHLD, sigHandler);
            }
        }
    }
}