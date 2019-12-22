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
 * Build In function implementations
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
        fprintf(stderr, "sh: expected 1 argument to \"ls\" provided more than 1 arguments\n");
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

void delete_char(char *str, int i, int len) {
    for (; i < len - 1 ; i++)
    {
       str[i] = str[i+1];
    }
    str[len-1] = 0;
}

char* sh_read_line(void) {
    int size = SH_TOK_BUFSIZE;
    char* line = malloc(size * sizeof(char));
    int characters = 0, left = 0;
    char ch = getch();
    
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
            //left--;
            //cursorforward(1);
            ch = getch();
            continue;
        }
        else if(ch == '\033') {
            char cc = getch();
            switch(getch()) {
            case 'A':
                printf("Up arrow");
                // code for arrow up
                break;
            case 'B':
                // code for arrow down
                break;
            case 'C':
                if(left > 0) {
                    cursorforward(1);
                    left--;
                }
                // code for arrow right
                break;
            case 'D':
                if(left < characters) {
                    cursorbackward(1);
                    left++;
                }
                // code for arrow left
                break;
            }
            ch = getch();
            continue;
        }
        else {
            printf("%c", ch);
        }
        while(size <= characters) {
            size += SH_TOK_BUFSIZE;
            line = realloc(line, size * sizeof(char));
            if(!line) {
                exit(EXIT_FAILURE);
            }
        }
        line[characters] = ch;
        ch = getch();
        characters++;
    }
    line[characters] = '\0';
    printf("\n");
    return line;
}

char** sh_tokenize(char* line) {
    int bufsize = SH_TOK_BUFSIZE;
    char* token = strtok(line, " ");
    char** tokens = malloc(bufsize * sizeof(char*));
    int position = 1;
    tokens[0] = token;
    
    while(token) {
        while(position > bufsize) {
            bufsize += SH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize*sizeof(char*));
        }
        token = strtok(NULL, " ");
        tokens[position] = token;
        position++;
    }
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
    bool status = true;
    
    do {
        printf("> ");
        line = sh_read_line();
        args = sh_tokenize(line);
        status = sh_execute(args);
        
        free(line);
        free(args);
    }while(status);
}
