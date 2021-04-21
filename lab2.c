#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>

/*6. Написать программу подсчета частоты встречающихся символов в файлах заданного каталога
и его подкаталогов. Пользователь задаёт имя каталога. Главный процесс открывает каталоги и
запускает для каждого файла каталога отдельный процесс подсчета количества символов. Каждый 
процесс выводит на экран свой pid, полный путь к файлу, общее число просмотренных байт и 
частоты встречающихся символов (все в одной строке). Число одновременно работающих процессов
не должно превышать N (вводится пользователем). Проверить работу программы для каталога /etc. 
*/


char* AppName;
long long  MAX_SIZE_PROCESSES, COUNTER=0;
int SYMBOLS[256] = {0};
char BUFFER[16777216];

void startOfFinding(char* curpath){
  DIR* curdir;

  if((curdir=opendir(curpath))==0){
    fprintf(stderr,"%s: %s: %s\n", AppName, curpath, strerror(errno));
    return;
  }

  struct dirent *dent;
  struct stat buf;
  long long size = 0;
  char newPath[256];
  errno=0;
  
        while (dent = readdir(curdir)){

          if (strcmp(dent->d_name, ".")==0  ||  strcmp(dent->d_name, "..")==0) continue;
          snprintf(newPath, sizeof(newPath), "%s/%s", curpath, dent->d_name);
                
          if(lstat(newPath,&buf)){
            fprintf(stderr,"%s: %s: %s\n", AppName, newPath, strerror(errno));
          }

          size = buf.st_size;
          
          if (S_ISDIR(buf.st_mode)){
             if (size > 0) { startOfFinding(newPath); }
          }

          if (S_ISREG(buf.st_mode)){
             if (COUNTER < MAX_SIZE_PROCESSES - 1){
             pid_t childPid = fork();
             switch (childPid){
               case -1:{
                fprintf(stderr, "%d: %s %s\n", getpid(), AppName, strerror(errno));
                break;
               }
               
               case 0:/*потомок*/{
                int fd;
                if ((fd=open(newPath, O_RDONLY)) == -1){
                  fprintf(stderr, "%d: %s %s %s\n", getpid(), AppName, newPath, strerror(errno));
                  exit(EXIT_FAILURE);
                }
                
                int isRead = 0;
                int totalSize = 0; 
                while ((isRead = read(fd, (char*)BUFFER, sizeof(BUFFER))) > 0){                                                                                    
                    for (int i = 0; i < isRead; ++i){
                      SYMBOLS[BUFFER[i]]++;
                    }
                    totalSize += isRead;
                    if (totalSize == size) break;
                }
                
                if (isRead == -1){
                  fprintf(stderr, "%d: %s %s %s\n", getpid(), AppName, newPath, strerror(errno));
                  exit(EXIT_FAILURE);
                }

                if (close(fd) == -1){
                  fprintf(stderr, "%d: %s %s %s\n", getpid(), AppName, newPath, strerror(errno));
                  exit(EXIT_FAILURE);
                }
                
                printf("%d: %s %lld\n", getpid(), newPath, size);
                
                for (int i = 0; i <= 255; i++){
                  if (SYMBOLS[i] != 0){
                    printf("0x%02X - %d ", i, SYMBOLS[i]);
                  }
                }    
                printf("\n");
                exit(EXIT_SUCCESS);                           
               }
               
               default:/*родитель*/{
                COUNTER++;
                if (COUNTER == MAX_SIZE_PROCESSES){
                  wait(NULL);
                  COUNTER--;
                }
               }
               
           }
          } else {
            wait(NULL);
            COUNTER--;
          }
                
        }                         
    }  

  if(errno!=0){
    fprintf(stderr,"%s: %s: %s\n", AppName, curpath, strerror(errno));
  }

  if(closedir(curdir)==-1){
    fprintf(stderr,"%s: %s: %s\n", AppName, curpath, strerror(errno));
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
rlim_t getLimitProcess() {
   struct rlimit rlp;
   getrlimit(RLIMIT_NPROC, &rlp);
   return rlp.rlim_max;
}

int main(int argc, char** argv){
      
  AppName=alloca(strlen(basename(argv[0])));
  strcpy(AppName,basename(argv[0]));
  
  if (argc < 3){
    fprintf(stderr, "%d: %s: To few arguments: path and to dirent size of process\n", getpid(), AppName);
    exit(EXIT_FAILURE);
  }
    
  if (realpath(argv[1],NULL)==NULL){
    fprintf(stderr,"%d: %s: %s: %s\n", getpid(),AppName, argv[1], strerror(errno));
    exit(EXIT_FAILURE);
   }    
    

  if((MAX_SIZE_PROCESSES = atoi(argv[2]))<=0){
    fprintf(stderr,"%s: %s: Invalid number of process\n", AppName, argv[2]);
    exit(EXIT_FAILURE);
  }
  
  if (MAX_SIZE_PROCESSES > getLimitProcess()){
    fprintf(stderr, "%s: the number of processes exceeds the maximum\n", AppName);  
    exit(EXIT_FAILURE);
  }
    
  startOfFinding(realpath(argv[1],NULL));
    
  while (wait(NULL) != -1){
    continue; 
  }
    
  printf("%d: %s: the Parent process will shut down soon...\n", getpid(),AppName);
    
  return 0;
}