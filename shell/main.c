#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void executeCommand(char *command, char **arguments, int background);
void sigHandler (int signum);

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

    //checa se o fork falhou
    if (pid < 0)
    {
        perror("fork failed");
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
                fd = open(arguments[i+1], O_WRONLY| O_CREAT| O_TRUNC, S_IWUSR | S_IRUSR);
                if (fd == -1)
                {
                    perror("open failed");
                    exit(1);
                }
                
                dup2(fd, STDOUT_FILENO);
                
                //tira ">" e o nome do arquivo
                arguments[i] = NULL;
                arguments[i + 1] = NULL;
                close(fd);
                break;
            }
        }
        
        if (execvp(command, arguments) < 0)
        {
            perror("error");
            exit(1);
        }
    }
    else    //pai
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

void sigHandler (int signum)
{
    pid_t pid;
    int status;
    pid = waitpid(0, &status, 0);
    //printf("Child %d terminated: %d %d\n",pid,signum,status);
}