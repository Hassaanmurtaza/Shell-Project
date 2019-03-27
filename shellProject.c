#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <fcntl.h>
#define BUFSIZE 1000
#define INPBUF 100
#define ARGMAX 10
#define GREEN "\x1b[92m"
#define BLUE "\x1b[94m"
#define DEF "\x1B[0m"
#define CYAN "\x1b[96m"

/* Instructions to support :
*  cd <dir>                                 
*  pwd                                      
*  mkdir <dir>                           
*  rmdir <dir>                              
*  ls (support ls -l)                       
*  cp <file1> <file2>                                  
*  list
*  run
*  kill            
*  exit   
*/
struct myarray{
int pid;
char pname[20];
} myarray[100];
int abcd=0;

struct _instr
{
    char * argval[INPBUF];
    int argcount;
};
typedef struct _instr instruction;

char *input,*input1;
int exitflag = 0;
int filepid,fd[2];
char cwd[BUFSIZE];
char* argval[ARGMAX]; // our local argc, argv
int argcount = 0,inBackground = 0;
int externalIn=0,externalOut=0;
char inputfile[INPBUF],outputfile[INPBUF];
void getInput();
int function_exit();
void function_pwd(char*, int);
void function_cd(char*);
void function_mkdir(char*);
void function_rmdir(char*);
void function_clear();
void nameFile(struct dirent*, char*);
void function_ls();
void function_lsl();
void function_cp(char*, char*);
void run(char*);
void list();
void killprog(char*);
/*Stop processes if running in terminal(a.out), close terminal if only Ctrl+C*/
void stopSignal()
{

    if(filepid!=0)
    {
        int temp = filepid;
        kill(filepid, SIGINT);
        filepid = 0;

    }
}

int main(int argc, char* argv[])
{
    signal(SIGINT,stopSignal);
    int i;
    int pipe1 = pipe(fd);
    function_clear();
    function_pwd(cwd,0);

    while(exitflag==0)
    {
        externalIn = 0; externalOut = 0;inBackground = 0;
        printf("%s%s ~> ",DEF,cwd ); 
        getInput();

        if(strcmp(argval[0],"exit")==0 || strcmp(argval[0],"z")==0)
        {
            function_exit();
        }
        else if (strcmp(argval[0],"run")==0 && !inBackground){
		char* prog=argval[1];
		run(prog);
	}	
	else if(strcmp(argval[0],"list")==0 && !inBackground){
		list();
	}
	else if(strcmp(argval[0],"kill")==0 && !inBackground){
		char* prog=argval[1];
		killprog(prog);		
	}
	else if(strcmp(argval[0],"pwd")==0 && !inBackground)
        {
            function_pwd(cwd,1);
        }
        else if(strcmp(argval[0],"cd")==0 && !inBackground)
        {
            char* path = argval[1];
            function_cd(path);
        }
        else if(strcmp(argval[0],"mkdir")==0 && !inBackground)
        {
            char* foldername = argval[1];
            function_mkdir(foldername);
        }
        else if(strcmp(argval[0],"rmdir")==0 && !inBackground)
        {
            char* foldername = argval[1];
            function_rmdir(foldername);
        }
        else if(strcmp(argval[0],"clear")==0 && !inBackground)
        {
            function_clear();
        }
        else if(strcmp(argval[0],"ls")==0 && !inBackground)
        {
            char* optional = argval[1];
            if(strcmp(optional,"-l")==0 && !inBackground)
            {
                function_lsl();
            }
            else function_ls();
        }
        else if(strcmp(argval[0],"cp")==0 && !inBackground)
        {
            char* file1 = argval[1];
            char* file2 = argval[2];
            if(argcount > 2 && strlen(file1) > 0 && strlen(file2) > 0)
            {
                function_cp(file1,file2);
            }
            else
            {
                printf("+--- Error in cp : insufficient parameters\n");
            }
        }
        else
        {
            printf("Invalid Command\n");
        }

    }

}


/*get input containing spaces and tabs and store it in argval*/
void getInput()
{
    fflush(stdout); 
    input = NULL;
    ssize_t buf = 0;
    getline(&input,&buf,stdin);

    input1 = (char *)malloc(strlen(input) * sizeof(char));
    strncpy(input1,input,strlen(input));
    argcount = 0;inBackground = 0;
    while((argval[argcount] = strsep(&input, " \t\n")) != NULL && argcount < ARGMAX-1)
    {

        if(sizeof(argval[argcount])==0)
        {
            free(argval[argcount]);
        }
        else argcount++;
        if(strcmp(argval[argcount-1],"&")==0)
        {
            inBackground = 1; 
            return;
        }
    }
    free(input);
}

void run(char* prog){
int pid=fork();		
		if(pid==0){
			execlp(prog,prog,NULL);}
		if(pid>0){	
			strcpy(myarray[abcd].pname,argval[1]);
			myarray[abcd].pid=pid;
			abcd++;
		}
}
/* cp */
void function_cp(char* file1, char* file2)
{
    FILE *f1,*f2;
    struct stat t1,t2;
    f1 = fopen(file1,"r");
    if(f1 == NULL)
    {
        perror("+--- Error in cp file1 ");
        return;
    }
    f2 = fopen(file2,"r");
    if(f2)
    {
        stat(file1, &t1);
        stat(file2, &t2);
        if(difftime(t1.st_mtime,t2.st_mtime) < 0)
        {
            printf("+--- Error in cp : %s is more recently updated than %s\n",file2,file1);
            fclose(f1);
            fclose(f2);
            return;
        }
    }
    f2 = fopen(file2,"ab+"); 
    fclose(f2);

    f2 = fopen(file2,"w+");
    if(f2 == NULL)
    {
        perror("Error in cp file2 ");
        fclose(f1);
        return;
    }
    
    if(open(file2,O_WRONLY)<0 || open(file1,O_RDONLY)<0)
    {
        perror("Error in cp access ");
        return;
    }
    char cp;
    while((cp=getc(f1))!=EOF)
    {
        putc(cp,f2);
    }

    fclose(f1);
    fclose(f2);
}

void list(){

		int ga=0;
		char ag[20];
		while (ga<abcd){
			if(myarray[ga].pid!=0){
			printf("The Processs id is %d. The process name is %s. \n",myarray[ga].pid,myarray[ga].pname);
			}ga++;
		}}
/* Pretty print*/
void nameFile(struct dirent* name,char* followup)
{
    if(name->d_type == DT_REG)         
    {
        printf("%s%s%s",BLUE, name->d_name, followup);
    }
    else if(name->d_type == DT_DIR)    
    {
        printf("%s%s/%s",GREEN, name->d_name, followup);
    }
    else                              
    {
        printf("%s%s%s",CYAN, name->d_name, followup);
    }
}


/*ls -l*/
void function_lsl()
{
    int i=0,total=0;
    char timer[14];
    struct dirent **listr;
    struct stat details;
    int listn = scandir(".",&listr,0,alphasort);
    if(listn > 0)
    {
        printf("%s+--- Total %d objects in this directory\n",CYAN,listn-2);
        for ( i = 0; i < listn; i++)
        {
            if(strcmp(listr[i]->d_name,".")==0 || strcmp(listr[i]->d_name,"..")==0)
            {
                continue;
            }
            else if(stat(listr[i]->d_name,&details)==0)
            {
                
                total += details.st_blocks;

                printf("%s%1s",DEF,(S_ISDIR(details.st_mode)) ? "d" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IRUSR) ? "r" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IWUSR) ? "w" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IXUSR) ? "x" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IRGRP) ? "r" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IWGRP) ? "w" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IXGRP) ? "x" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IROTH) ? "r" : "-");
                printf("%s%1s",DEF,(details.st_mode & S_IWOTH) ? "w" : "-");
                printf("%s%1s ",DEF,(details.st_mode & S_IXOTH) ? "x" : "-");

                printf("%2ld ",(unsigned long)(details.st_nlink));
                printf("%s ",(getpwuid(details.st_uid))->pw_name);
                printf("%s ",(getgrgid(details.st_gid))->gr_name);

                printf("%5lld ",(unsigned long long)details.st_size);
                strftime (timer,14,"%h %d %H:%M",localtime(&details.st_mtime));
                printf("%s ",timer);
                nameFile(listr[i],"\n");
            }
        }
        printf("%s+--- Total %d object contents\n",CYAN,total/2);
    }
    else
    {
            printf("+--- Empty directory\n" );
    }
}
void killprog(char* prog){
		int n=0;
		n=atoi(prog);
		kill(n,SIGKILL);
		for(int i=0;i<abcd;i++){
			if (myarray[i].pid==n){
			myarray[i].pid=0;
}}}
/* list cwd contents*/
void function_ls()
{
    int i=0;
    struct dirent **listr;
    int listn = scandir(".", &listr, 0, alphasort);
    if (listn >= 0)
    {
        printf("%s+--- Total %d objects in this directory\n",CYAN,listn-2);
        for(i = 0; i < listn; i++ )
        {
            if(strcmp(listr[i]->d_name,".")==0 || strcmp(listr[i]->d_name,"..")==0)
            {
                continue;
            }
            else nameFile(listr[i],"    ");
            if(i%8==0) printf("\n");
        }
        printf("\n");
    }
    else
    {
        perror ("+--- Error in ls ");
    }

}


/* clear the screen*/
void function_clear()
{
    const char* blank = "\e[1;1H\e[2J";
    write(STDOUT_FILENO,blank,12);
}

/* remove folder */
void function_rmdir(char* name)
{
    int statrm = rmdir(name);
    if(statrm==-1)
    {
        perror("+--- Error in rmdir ");
    }
}


/* Make folder */
void function_mkdir(char* name)
{
    int stat = mkdir(name, 0777);// all appropriate permissions
    if(stat==-1)
    {
        perror("+--- Error in mkdir ");
    }
}


/*change directory functionality*/
void function_cd(char* path)
{
    int ret = chdir(path);
    if(ret==0) // path could be changed if cd successful
    {
        function_pwd(cwd,0);
    }
    else perror("+--- Error in cd ");
}


/*Implement basic exit*/
int function_exit()
{
    exitflag = 1;
    return 0; // return 0 to parent process in run.c
}

/* Implement pwd function in shell - 1 prints, 0 stores*/
void function_pwd(char* cwdstr,int command)
{
    char temp[BUFSIZE];
    char* path=getcwd(temp, sizeof(temp));
    if(path != NULL)
    {
        strcpy(cwdstr,temp);
        if(command==1)  
        {
            printf("%s\n",cwdstr);
        }
    }
    else perror("+--- Error in getcwd() : ");

}



