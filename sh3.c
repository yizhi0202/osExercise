#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#define MAX_DIR_NAME 100
#define MAX_CMD 200
int totalArgc = 0;
int parseline(char *buf, char **argv)
{
  while (*buf == ' ')
  {
    buf++;
  }
  int delim = 0;

  int argc = 0;
  while (*buf != '\n')
  {

    while (buf[delim] != '\n' && buf[delim] != ' ')
    {
      delim++;
    }

    if (buf[delim] == '\n')
    {
      buf[delim] = '\0';
      argv[argc++] = buf;
      break;
    }
    buf[delim] = '\0';
    argv[argc++] = buf;

    buf += delim + 1;
    /*指示器indicator=0*/
    delim = 0;
    while (*buf == ' ')
    {
      buf++;
    }
  }
  /*the last element is NULL*/
  argv[argc] = NULL;
  totalArgc = argc;
  return 0;
}

/*void mysys(char* cmd)
{	
	pid_t pid;
	pid = fork();
	if(pid == 0)
	{
		execl("/bin/sh","sh","-c",cmd,NULL);
	}
	else wait(NULL);
}*/

int buildin_command(char **argv)
{
  if (strcmp(argv[0], "exit") == 0)
  {
    exit(0);
  }
  if (strcmp(argv[0], "cd") == 0)
  {
    if (argv[1] == NULL)
      argv[1] = "..";
    char *cwd;
    char *tipcwd = NULL; //提示现在的所处工作目录
    char temp[1024];
    cwd = getcwd(temp, sizeof(temp));
    //去掉前面的/home/shiyanlou/
    strtok_r(cwd, "/", &tipcwd);
    for (int i = 0; i < 1; ++i)
    {
      strtok_r(NULL, "/", &tipcwd);
    }
    if ((strcmp(tipcwd, "Code") == 0) && (strcmp(argv[1], "..") == 0))
      ;
    else
    {
      if (chdir(argv[1]))
      {
        printf("Error:cd:%s:no such directory\n", argv[1]);
      }
    }
    return 1;
  }

  if (strcmp(argv[0], "pwd") == 0)
  {
    char buf[MAX_DIR_NAME];
    printf("%s\n", getcwd(buf, sizeof(buf)));
    return 1;
  }
  if (strcmp(argv[0], "echo") == 0)						//echo 为内置命令 
  {
    int lenofArgv = totalArgc;
	int stdinFd = dup(1); 								//方便复原重定向 
	
    if (strcmp(argv[lenofArgv - 2], ">") == 0)			// 如果输入命令为 echo abc > log 
    {
      int fd = open(argv[lenofArgv - 1], O_CREAT | O_RDWR | O_APPEND, 0666);
      if (fd < 0)
      {
        printf("Fail to open the file of %s", argv[lenofArgv - 1]);
        return 1;
      }
      dup2(fd, 1);										//重定向，将标准输入文件指针指向文件 
      close(fd);

      for (int i = 1; i < lenofArgv - 2; ++i)
      {
        write(1, argv[i], strlen(argv[i]));

        write(1, " ", 1);								//添加参数间的空格 
      }
      write(1,"\n",1);									//末尾添加换行符 
      dup2(stdinFd,1);									//复原重定向 
      return 1;
    }
    else if (argv[lenofArgv - 1][0] == '>')				//如果输入的命令为 echo abc >log 
    {
      char *temp ;
      char *filename; 
      filename = strtok_r(argv[lenofArgv - 1], ">", &temp);
      int fd = open(filename, O_CREAT | O_RDWR | O_APPEND, 0666);
      if (fd < 0)
      {
        printf("Fail to open the file of %s", temp);
        return 1;
      }
      else
      {
        dup2(fd, 1);
        close(fd);
        for (int i = 1; i < lenofArgv - 1; ++i)
        {
          write(1, argv[i], strlen(argv[i]));
          write(1, " ", 1);
        }
        write(1,"\n",1);
        dup2(stdinFd,1);
        return 1;
      }
    }
    else
    {
     
      for (int i = 1; i < lenofArgv; ++i)
      {
        write(1, argv[i], strlen(argv[i]));

        write(1, " ", 1);
      }
      write(1,"\n",1);
    }
    return 1;
  }
  return 0; //not a buildin_command
}
void eval(char *cmdstring)
{
  /*parse the cmdstring to argv*/
  char *argv[MAX_CMD];
  /*Holds modified command line*/
  char buf[MAX_CMD];
  
  strcpy(buf, cmdstring);
  char* segcmd1; //store the arguments(>=2)
  char* segcmd2;
  char* savestr; //store the left cmd after segmentation
  segcmd1 = strtok_r(buf,"|",&savestr);
  if(strcmp(segcmd1, cmdstring) == 0)		//if there is  only one instruction.
  {
  	/*parse the cmdstring*/
	  parseline(buf, argv);
	  if (argv[0] == NULL)
	  {
	    return; /*ignore empty lines*/
	  }
	  /*is a buildin command*/
	  /*direct return*/
	  if (buildin_command(argv))
	    return;
	  pid_t pid = fork();
	  if (pid == 0)
	  {
	  	if(strcmp(argv[0],"cat")==0)
	  	{
	  	    if(totalArgc == 1) 
	  		{
			  printf("ERROR:The instruction of cat must follow an argument!\n");
			  exit(0);
			}	
		}
	    if (execvp(argv[0], argv) < 0)
	    {
	      printf("%s:command not found.\n", argv[0]);
	      exit(0);
	    }
	  }
	  wait(NULL);
   } 
   else				//there are two instructions.
   {
   	 segcmd2 = strtok_r(NULL,"|",&savestr);
   	 char cmd2[MAX_CMD];			//store the second instruction.
   	 strcpy(cmd2,segcmd2);			//avoid the strange problem , need to dump. 
   	 char* argv1[MAX_CMD];
   	 strcat(segcmd1,"\n");			// to parseline correctly 
	 parseline(segcmd1,argv);
	 parseline(cmd2,argv1);
	 int stdin, stdout;
	 stdin = dup(0);				//for recover the standard input and output. 
	 stdout = dup(1);
	 int pipefd[2]={-1};
	  if(pipe(pipefd) < 0){
	    perror("pipe error");
	    return ;
	  }
	  pid_t first_pid=fork();
	  if(first_pid==0){
	  	
	    dup2(pipefd[1],1);
	    execvp(argv[0],argv);
	    dup2(stdout,1);
	    exit(0);
	  }
	  pid_t second_pid=fork();
	  if(second_pid==0){      
	   close(pipefd[1]);
	    
	    dup2(pipefd[0],0);
	    dup2(stdout,1);
	    execvp(argv1[0],argv1);
	    dup2(stdin,0);
	    
	    exit(0);
	  }
	  //dup2(stdin, 0);
	  //dup2(stdout,1);
	  close(pipefd[0]);
	  close(pipefd[1]);
	  waitpid(first_pid,NULL,0);
	  waitpid(second_pid,NULL,0);  
   }
	
  
  
}

char *getTipCwd()
{
  char *cwd;
  char *tipcwd = NULL; //提示现在的所处工作目录
  char temp[1024];
  cwd = getcwd(temp, sizeof(temp));
  //去掉前面的/home/shiyanlou/
  strtok_r(cwd, "/", &tipcwd);
  for (int i = 0; i < 1; ++i)
  {
    strtok_r(NULL, "/", &tipcwd);
  }
  if (strcmp(tipcwd, "Code") == 0)
  {
    tipcwd = "~/ $";
  }
  else
  {
    strtok_r(NULL, "/", &tipcwd);
    strcat(tipcwd, "/ $");
  }
  return tipcwd;
}
int main(int argc, char *argv[])
{

  char cmdstring[MAX_CMD] = {0};
  char *tipcwd;
  int n;
  while (1)
  {
    tipcwd = getTipCwd();
    printf("*myshell*:%s", tipcwd);
    fflush(stdout);
	
    /*read*/
    if ((n = read(0, cmdstring, MAX_CMD)) < 0)
    {
      printf("read error");
    }

    eval(cmdstring);
    memset(cmdstring,0,sizeof(cmdstring));		//清空cmdstring 
  }

  return 0;
}

