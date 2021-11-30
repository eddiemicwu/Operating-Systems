/*
 * main.c
 *
 * A simple program to illustrate the use of the GNU Readline library
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

struct bglist
{
	int num;
	pid_t cpid;
	char state;
	char path[256];
};

struct bglist list[5];

static void sigchld_hdl(int sig)
{
	int status;
	pid_t s_pid;

	while(1)
	{
		s_pid = waitpid(-1, &status, WNOHANG);
		if(s_pid < 0)
		{
			break;
		}
		if(s_pid == 0)
		{
			sleep(1);
			break;
		}
		if(WIFEXITED(status))
                {
                        //printf("exited, status=%d\n", WEXITSTATUS(status));
                }
                else if(WIFSIGNALED(status))
                {
                        printf("%x killed by signal %d\n", s_pid, WTERMSIG(status));
                }
                else if(WIFSTOPPED(status))
                {
                        printf("stopped by signal %d\n", WSTOPSIG(status));
                }
                else if(WIFCONTINUED(status))
                {
                        printf("continued\n");
                }
	}
}

//child process
void ChildProcess(char * argv[])
{
	execvp(argv[0], argv);
	printf("execvp() is not successfual.\n");
	exit(0);
}
/*
//parent process
void ParentProcess(pid_t pid)
{
	pid_t w;
	int status;
	do
	{
		w = waitpid(pid, &status, WNOHANG);
		if(w == -1)
		{
			perror("waitpid");
			exit(-1);
		}
		if(WIFEXITED(status)) 
		{
			printf("exited, status=%d\n", WEXITSTATUS(status));
		} 
		else if(WIFSIGNALED(status))
		{
			printf("killed by signal %d\n", WTERMSIG(status));
		} 
		else if(WIFSTOPPED(status)) 
		{
			printf("stopped by signal %d\n", WSTOPSIG(status));
		} 
		else if(WIFCONTINUED(status)) 
		{
			printf("continued\n");
		}
	} 
	while(!WIFEXITED(status) && !WIFSIGNALED(status));
}
*/
// print current directory
void printDir()
{
	char cwd[256];
	printf("%s\n", getcwd(cwd, sizeof(cwd)));
}
//main
int main (int argc, char * argv[])
{
	pid_t pid;
	int jobs = 0;
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	act.sa_handler = sigchld_hdl;
	if(sigaction(SIGCHLD, &act, 0))
	{
		perror("sigaction");
		return 1;
	}
	bool background;

	for (;;)
	{
		char 	*cmd = readline ("shell>");
		background = false;
		if(strcmp(cmd, "") == 0)
		{
			continue;
		}
		else
		{
			add_history(cmd);
		}
		char *tmp;
		int index = 0;
		tmp = strtok(cmd, " ");
		while(tmp != NULL)
		{
			argv[index++] = tmp;
			tmp = strtok(NULL, " ");
		}
		argv[index] = NULL;
		free(tmp);
		/** cd functionality **/
		if(strcmp(argv[0], "cd") == 0)
		{
			printDir();	// previous directory
			if(chdir(argv[1]) < 0)
			{
				perror(argv[1]);
			}
			printDir();	//current directory
			continue;
		}
		
		/** background execution **/
		if(strcmp(argv[0], "bg") == 0)
		{
			if(index == 1)
			{
				argv[0] = "bglist";
			}
			else
			{
				background = true;
				argv += 1;
				index--;

			}
		}

		/** kill background jobs **/
		if(strcmp(argv[0], "bgkill") == 0)
		{
			if(index != 2)
			{
				printf("job undefine.\n");
				continue;
			}

			if(jobs <= 0)
			{
				printf("No background jobs.\n");
			}
			else
			{
				char *p;
				bool check = false;
				int tmp = strtol(argv[1], &p, 5);
				for(int i = 0; i < jobs; i++)
				{
					if(tmp == list[i].num)
					{
						jobs -= 1;
						kill(list[i].cpid, SIGTERM);
						printf("Job %d terminated.\n", list[i].num);
						for(int c = i; c <= jobs; c++)
						{
							list[c] = list[c+1];
						}
						check = true;
						continue;
					}
				}
				if(!check)
				{
					printf("job %d is out of range.\n", tmp);
				}
			}
			continue;
			
		}

		/** show all background jobs **/
		if(strcmp(argv[0], "bglist") == 0)
	 	{
			if(jobs == 0)
			{
				printf("no jobs are running in the background.\n");
			}
			else
			{
				for(int i = 0; i < jobs; i++)
				{	
					printf("%d[%c]: %s \t PID: %x \n", list[i].num, list[i].state, list[i].path, list[i].cpid);
				}
				printf("Total Background jobs: %d\n", jobs);
			}
			continue;
		}
		
		/** suspend or resume background jobs**/
		if(strcmp(argv[0], "stop") == 0 || strcmp(argv[0], "start") == 0)
		{
			if(jobs == 0)
			{
				printf("no jobs are running in the background.\n");
				continue;
			}
			char *p;
			int tmp = strtol(argv[1], &p, 5);
			for(int i = 0; i < jobs; i++)
			{
				if(tmp == list[i].num)
	 			{
					if(strcmp(argv[0], "stop") == 0)
					{
						if(list[i].state == 'S'){
							printf("Job %d was already stopped.\n", list[i].num);
						}
						else
						{
							list[i].state = 'S';
							kill(list[i].cpid, SIGSTOP);
							printf("Job %d Stopped.\n", list[i].num);
						}
					}
					else
					{
						if(list[i].state == 'R')
						{
							printf("Job %d are currently running.\n", list[i].num);
						}
						else
						{
							list[i].state = 'R';
							kill(list[i].cpid, SIGCONT);
							printf("Job %d Running.\n", list[i].num);
						}
					}
				}
			}
			continue;
		}

		/** fork() **/
		pid = fork();
		
		if(pid < 0)
		{
			perror("Fork failed.\n");
			return -1;
		}
		
		if(pid == 0)
		{
			if(background)
			{
				if(argc == 1)
				{
					pause();
				}
				_exit(atoi(argv[1]));
			}
			else
			{
				ChildProcess(argv);
			}
		}
		else
		{
			if(!background)
			{
				//ParentProcess(pid);
			}
			else
			{
				list[jobs].cpid = pid;
				list[jobs].state = 'R';
				char t[15];
                       		strncpy(t, argv[0], 15);
                       	       	strcat(t, " ");
                        	if(argv[1] != NULL)
				{
					strncpy(list[jobs++].path, strcat(t, argv[1]), 256);
                       	        }
                       	        else
                	        {
        	                        strncpy(list[jobs++].path, t, 256);
				}

				for(int i = 0; i < jobs; i++)
				{
					if(list[i].num == i)
					{
						//printf("%d = %d\n", list[i].num, i);
					}
					else
					{
						list[jobs-1].num = i;
						break;
					}
				}
			}
		}
	}
}
