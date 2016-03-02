/*
 * Jay Patel
 * file:// shell.c
 * homework 3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define MAXARGS 128
#define MAXLINE 1000

int errno;


// the next few functions are taken straight from the book
// function prototype

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int builtin_command(char **argv)
{
	if(!strcmp(argv[0], "exit"))
		exit(0);
	if(!strcmp(argv[0], "&"))
		return 1;
	return 0; // not a built in command
}



/*
 * this function gets called right after parsing the line
 * checks if there is any redirection exists inside the command
 * return 1 if < was found
 * returns 2 if > was found
 * returns 3 if >> was found
 * else return 0 if none of the above conditions were met
 */
int contains_redirection(char **argv)
{
	int i = 0;
	while(argv[i] != NULL)
	{
		if(strcmp(argv[i], "<") == 0)
			return 1;
		else if(strcmp(argv[i], ">") == 0)
			return 2;
		else if(strcmp(argv[i], ">>") == 0)
			return 3;

		i++;
	}

	return 0;
}

int parseline(char *buf, char **argv)
{
	char *delim;
	int argc, bg;

	buf[strlen(buf)-1] = ' ';
	while(*buf && (*buf == ' '))
		buf++;
	
	argc = 0;
	while((delim = strchr(buf, ' ')))
	{
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while(*buf && (*buf == ' '))
			buf++;
	}

	argv[argc] = NULL;

	if(argc == 0)
		return 1;

	if((bg = (*argv[argc-1]== '&')) != 0)
		argv[--argc] = NULL;
	
	return bg;
}

void perform_redirection(char **argv, int input)
{
	char *redirect;

	if(input == 1)
		redirect = "<";
	else if(input == 2)
		redirect = ">";
	else if(input == 3)
		redirect = ">>";

	char *filename;

	int i = 0;
	while(argv[i] != NULL)
	{
		if(strcmp(argv[i], redirect) == 0)
		{
			i++;
			filename = malloc(sizeof(char) * strlen(argv[i])-1);
			strcpy(filename, argv[i]);
			break;
		}

		i++;
	}

	/*
	 * creating a new argument vector to store all the commands leading up to redirection
	 * this reduces us from creating a new vector inside the child process
	 */
	char *cmd[i]; // getting it up to i  since everything before it has been before the operator
	int j = 0;
	while(strcmp(argv[j], redirect) != 0)
	{
		cmd[j] = malloc(sizeof(char) * strlen(argv[j]) - 1);
		strcpy(cmd[j], argv[j]);
		j++;
	}

	cmd[j] = NULL;

	// creating a new process to fork

	pid_t pid;

	pid = fork();

	if(pid == 0) // in the child process
	{
		if(strcmp(redirect, ">") == 0)
		{
			int ft_out = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if(ft_out > 0)
			{
				dup2(ft_out, 1);

				// the next line goes inside the file
				execv(cmd[0], cmd);

				exit(0);
			}
		}
		else if(strcmp(redirect, ">>") == 0)
		{
			int ft_out = open(filename, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if(ft_out > 0)
			{
				dup2(ft_out, 1);

				// the next line goes inside the file
				execv(cmd[0], cmd);
				exit(0);
			}
		}
		else if(strcmp(redirect, "<") == 0)
		{
			int ft_out = open(filename, O_RDONLY);
			if(ft_out > 0)
			{
				dup2(ft_out, 0);
				FILE *fin;
				fin = fopen(filename, "r");
				char *tempv[MAXARGS];

				tempv[0] = cmd[0];

				int k = 1;
				char line[MAXARGS];
				while(fgets(line, MAXLINE, fin) != NULL)
				{
					sscanf(line, "%s", tempv[k]);
					printf("%s\n", line);
					k++;
				}

				tempv[k] = NULL;

				fclose(fin);

				execv(tempv[0], tempv);

				exit(0);
			}
		}
	}
	else // in the parent process
	{
		// waiting for the child process to end
		wait(NULL);
	}
}

void eval(char *cmdline)
{
	char *argv[MAXARGS];
	char *buf = malloc((strlen(cmdline) + 1) * sizeof(char));
	int bg;
	pid_t pid;

	strcpy(buf, cmdline);

	bg = parseline(buf, argv);
	if(argv[0] == NULL)
		return;
	
	int chk_rdrection = contains_redirection(argv);
	if(chk_rdrection)
	{
		perform_redirection(argv, chk_rdrection);
		return;
	}
	
	if(!builtin_command(argv))
	{
		pid = fork();

		if(pid == 0)
		{
			printf("pid %d status %d\n", getpid(), 0);

			execv(argv[0], argv);
			exit(0);
		}

		if(!bg)
		{
			//waiting for the child process to end
			wait(NULL);
		}
		else
			printf("%d %s", pid, cmdline);
	}

	return;
}

void sigint_handler(int sig)
{
	write(1, "catch sigint\n CS361 >", 50);
}

void sigtstp_handler(int sig)
{
	write(1, "catch sigtstp\n CS361 >", 50);
}

int main()
{
	char cmdline[MAXARGS];

	while(1)
	{
		printf("JAY >");
		signal(SIGINT, sigint_handler);
		signal(SIGTSTP, sigtstp_handler);
	//	printf("CS361 >");
		fgets(cmdline, MAXLINE, stdin);
	//	scanf("%s", cmdline);
		if(feof(stdin))
			exit(0);
		
		eval(cmdline);
	}
}


