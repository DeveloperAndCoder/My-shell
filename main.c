#include<stdio.h> 
#include<string.h>
#include<stdbool.h>
#include<stdlib.h>
#include<termios.h>
#include<unistd.h>
#include<sys/wait.h>
#include<dirent.h>

#define SH_TOK_BUFSIZE 64
#define cursorforward(x) printf("\033[%dC", (x))
#define cursorbackward(x) printf("\033[%dD", (x))
#define CLEARLINE printf("%c[2K", 27)

bool sh_cd(char **args);
bool sh_help(char **args);
bool sh_exit(char **args);
bool sh_ls(char **args);

char** sh_tokenize(char* line, char* delim, int* ptr);
char** get_history(int*);

int getch(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

char* builtin_str[] = {
    "cd",
    "help",
    "exit",
    "ls"
};

bool (*builtin_fun[])(char**) = {
    &sh_cd,
    &sh_help,
    &sh_exit,
    &sh_ls
};

int sh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*  
 * BEGIN Build In function implementations
 * */

bool sh_cd(char **args) {
    if(args[1] == NULL) {
        fprintf(stderr, "sh: expected argument to \"cd\"\n");
    }
    else {
        if(chdir(args[1]) != 0) {
            perror("sh");
        }
    }
    return true;
}

bool sh_help(char **args) {
    printf("Anurag Shah's Shell\n");
    printf("The following built in functions are implemented by me and all other functions are of linux's.\n");
    for(int i = 0; i < sh_num_builtins(); i++) {
        printf("%d. %s\n", i+1, builtin_str[i]);
    }
    printf("Use man command for information for other functions.\n");
    return true;
}

bool sh_exit(char **args) {
    return false;
}

bool sh_ls(char **args) {
    if(args[2] != NULL) {
        fprintf(stderr, "sh: expected 0 argument to \"ls\" provided more than 1 arguments\n");
    }
    else {
        struct dirent **namelist;
        int n;
        if(args[1] != NULL) {
            n = scandir(args[1], &namelist, NULL, alphasort);
        }
        else {
            n = scandir(".", &namelist, NULL, alphasort);
        }
        if (n < 0)
            perror("scandir error");
        else {
            while (n--) {
                printf("%s\n", namelist[n]->d_name);
                free(namelist[n]);
            }
            free(namelist);
        }
    }
    return true;
}

/* 
 * END OF IMPLEMENTATION OF BUILTIN FUNCTIONS 
 * */

void delete_char(char *str, int i, int len) {
    for (; i < len - 1 ; i++)
    {
       str[i] = str[i+1];
    }
    str[len-1] = 0;
}

void insert_char(char *str, char ch, int i, int len) {
    for (int j = len+1; j > i ; j--)
    {
        str[j] = str[j-1];
    }
    str[i] = ch;
}

char** get_history(int* ptr)
{
    FILE * pFile;
    long lSize;
    char * buffer;
    size_t result;

    pFile = fopen ( "/home/anurag/.bash_history" , "rb" );
    if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    // allocate memory to contain the whole file:
    buffer = (char*) malloc (sizeof(char)*lSize);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    // copy the file into the buffer:
    result = fread (buffer,1,lSize,pFile);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

    /* the whole file is now loaded in the memory buffer. */

    // terminate
    fclose (pFile);
    char** cmds = sh_tokenize(buffer, "\n", ptr);
    *ptr--;
    /*
    for(int i = 0; i < sz; i++) {
        printf("%s %d/%d\n", cmds[sz-2], i, sz);
    }
    */
    return cmds;
}

char* sh_read_line(char** cmds, int position) {
    int size = SH_TOK_BUFSIZE;
    char* line = malloc(size * sizeof(char));
    int characters = 0, left = 0;
    char ch = getch();
    int p = position;
    
    while(ch != '\n' || ch == EOF) {
        if((int)ch == 127 || (int)ch == 8) {
            CLEARLINE;
            cursorbackward(characters+2);
            if(characters - left > 0) {
                delete_char(line, characters - left - 1, characters);
                characters--;
            }
            printf("> %.*s", characters, line);
            if(left > 0) {
                cursorbackward(left);
            }
            ch = getch();
            p=position;
            continue;
        }
        else if(ch == '\033') {
            char cc = getch();
            switch(getch()) {
            case 'A':
                // code for arrow up
                CLEARLINE;
                cursorbackward(characters+2);
                char* prev_cmd = cmds[p];
                characters = strlen(prev_cmd)+1;
                line = (char*)realloc(line, characters * sizeof(char));
                strcmp(line, prev_cmd);
                p--;
                for(int i = 0; i < characters; i++)
                    line[i] = prev_cmd[i];
                line[characters]=0;
                printf("> %.*s", characters, line);
                break;
            case 'B':
                // code for arrow down
                if(p == position) break;
                else {
                    CLEARLINE;
                    cursorbackward(characters+2);
                    ++p;
                    char* nxt_cmd = cmds[p];
                    characters = strlen(nxt_cmd)+1;
                    line = (char*)realloc(line, characters * sizeof(char));
                    for(int i = 0; i < characters; i++)
                        line[i] = nxt_cmd[i];
                    line[characters]=0;
                    printf("> %.*s", characters, line);
                }
                break;
            case 'C':
                if(left > 0) {
                    cursorforward(1);
                    left--;
                    p=position;
                }
                // code for arrow right
                break;
            case 'D':
                if(left < characters) {
                    cursorbackward(1);
                    left++;
                }
                p=position;
                // code for arrow left
                break;
            }
            ch = getch();
            continue;
        }
        else {
            p=position;
            printf("%c", ch);
        }
        while(size <= characters) {
            size += SH_TOK_BUFSIZE;
            line = (char*)realloc(line, size * sizeof(char));
            if(!line) {
                exit(EXIT_FAILURE);
            }
        }
        insert_char(line, ch, characters - left, characters);
        characters++;
        if(left > 0) {
            CLEARLINE;
            cursorbackward(characters - left + 2);
            printf("> %.*s", characters, line);
            cursorbackward(left);
        }
        ch = getch();
    }
    line[characters] = '\0';
    printf("\n");
    return line;
}

char** sh_tokenize(char* line, char* delim, int* ptr) {
    int bufsize = SH_TOK_BUFSIZE;
    char* token = strtok(line, delim);
    char** tokens = malloc(bufsize * sizeof(char*));
    for(int i = 0; i < bufsize; i++) {
        tokens[i] = NULL;
    }
    int position = 1;
    tokens[0] = token;
    
    while(token) {
        while(position > bufsize) {
            bufsize += SH_TOK_BUFSIZE;
            tokens = (char**)realloc(tokens, bufsize*sizeof(char*));
        }
        token = strtok(NULL, delim);
        tokens[position] = token;
        position++;
    }
    if(ptr) {
        *ptr = position-1;
    }
    //tokens = realloc(tokens, position*sizeof(char*));
    return tokens;
}

bool launch(char **args) {
    pid_t pid, wpid;
    int status;
    
    pid = fork();
    if(pid == 0) {
        if(execvp(args[0], args) == -1) {
            perror("sh: cannot execute");
        }
        exit(EXIT_FAILURE);
    }
    else if(pid < 0) {
        perror("sh: Error forking\n");
    }
    else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return true;
}

bool sh_execute(char** args) {
    if(args[0] == NULL) {
        return true;
    }
    for(int i = 0; i < sh_num_builtins(); i++) {
        if(strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_fun[i])(args);
        }
    }
    return launch(args);
}

void main() {
    char *line;
    char **args;
    int sz =  0;
    bool status = true;
    int position = 0;
    char** cmds = get_history(&position);
    --position;
    
    do {
        printf("> ");
        line = sh_read_line(cmds, position);
        args = sh_tokenize(line, " ", &sz);
        status = sh_execute(args);
        
        free(line);
        free(args);
    }while(status);
}
