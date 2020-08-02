/*
 * led line editor v2
 * 6/2020 k theis
 * compile with cc -o led led.c -Wall -ansi
 *
 * This (version 2) started out as a 30 line program for a shell I wrote.
 * Commands are simple & easy to remember for fast typers. Note: I avoided
 * certain C99 shortcuts to keep this ansi compliant. I use this on several
 * older mini-computers that have 1980's ansi-c compilers.
 *
 * There is a man page for this program (led.1) in the same github repository.
 * A quick reference is below. There are also enough comments to fill you in.
 *
 * To start, led [filename] where filename is optional. If supplied, led
 * will load filename and put you in append mode. Anything typed (except
 * control statements) will be added to the main buffer. If there is no
 * filename given, you still start in append mode.
 * Commands start with a period(.) and take an optional arguement.
 * Example: 
 * .s [filename] to save the buffer to a file. You will be prompted if no filename is given.
 * (unless started w/ a filename. Then .s will save to the initial filename).
 * .l 1 30   (List 1-30) Display the buffer contents from line 1 thru 30 (inclusive).
 * .h  displays a simple help file
 * .q quit w/o saving
 * All commands are terminated with the enter key. See the help() function at program end.
 *
 * This is (C) kurt theis 2020 and licensed under the terms of the MIT license. 
 * It is free to use, change and distribute as you see fit. No warrantee is 
 * implied or given.
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include "mpe.h"

#define LINENO_MIN 1        /* used for print/insert/delete/replace */
#define LINENO_MAX 10000
#define PAGELENGTH 24       /* used in print() */
/*
#define LINUX            
#define PRIMOS
#define RSTS
*/
#define MPE

#define SHELL "/cmds/sh"


/* global vars */
char *buffer, *tbuf, *ubuf;
char cutbuffer[MAXLINE];    /* for cut/paste */
int curpos = 0;             /* current position of buffer end */
int undocurpos = 0;         /* for the undo command */
pid_t my_pid;               /* for saving backups */
char usefilename[MAXLINE]={};  /* use this name to save if file is opened at start */

/* define functions */
int quit(void);
void saveexit(void);
int save(char *);
void print(char *);
void find(char *);
void delete(char *);
void insert(char *);
void paste(char *);
void repl(char *);
void help(void);
void backup(void);
void undo(void);
void savebuf(void);


int main(int argc, char **argv) {
    char line[MAXLINE+1];
    int n=0;
	int err = 0;

    buffer = (char *) malloc(2*(MAXLINE+2)*sizeof(char *)); /* main buffer */
    if (buffer == NULL) {
        puts("fatal memory error during initialization");
        exit(1);
    }

    ubuf = (char *) malloc(2*(MAXLINE+2)*sizeof(char *));   /* used for undo */
    if (ubuf == NULL) {
        puts("fatal memory error during initialization");
        exit(1);
    }

    if (argc > 2) {
        printf("Usage: %s [filename] filename is optional\n",basename(argv[0]));
        exit(0);
    }

    if (argc==2) {  /* load a file */
        FILE *infile = fopen(argv[1],"r");
        if (infile == NULL) {
            printf("Cannot open %s\n",argv[1]);
            exit(1);
        }
        strcpy(usefilename,argv[1]);    /* keep for later when saving */
        /* load the file */
        while (1) {
            fgets(line,MAXLINE,infile);
            if (feof(infile)) break;
            tbuf = realloc(buffer,curpos+strlen(line)+1);
            if (tbuf == NULL) {
                /* don't make a backup here - nothing created yet */
                puts("fatal memory error during file load");
                exit(1);
            }
            buffer = tbuf;
            for (n=0; n<strlen(line); n++)
                buffer[curpos++] = line[n];
        }
        fclose(infile);
    }

    /* main program loop */
    while (1) {
        printf("> ");   /* append prompt */
        memset(line,0,sizeof(line));
        if (fgets(line,sizeof(line),stdin)==NULL)
            puts("returned error on input");
        
        /* test for commands */
        
        /* save buffer to a backup file */
        if (strncmp(line,".bak",4)==0) {
            /* save buffer to 'edit_pid_.tmp' */
            backup();
            continue;
        }

        /* quit, no save */
        if ( (strncmp(line,".Q",2)==0) || (strncmp(line,".q",2)==0) ) 
            quit();

        /* exit w/save */
        if ( (strncmp(line,".E",2)==0) || (strncmp(line,".e",2)==0) )
            saveexit();

        /* save buffer to file */
        if ( (strncmp(line,".S",2)==0) || (strncmp(line,".s",2)==0) ) {
            /* if just .s\n then use filename (if defined) */
            if ((strlen(line)==3) && (strlen(usefilename)!=0)) {   /* use filename we used to load a file */
                char tmp[MAXLINE] = "save ";
                strcat(tmp,usefilename);
                save(tmp);
                continue;
            } 
            /* if just .s\n and filename not defined or not wanted */
            if (save(line))
                printf("EH?\n"); /* no filename given in save() */
            continue;
        }

        /* undo - restore buffer state to before last buffer-modifying command */
        if ((strncmp(line,".UNDO",5)==0) || (strncmp(line,".undo",5)==0)) {
            undo();
            continue;
        }

        /* list/print - display the buffer a page at a time */
        if ( (strncmp(line,".L",2)==0) || (strncmp(line,".l",2)==0) ) {
            print(line);    
            continue;
        }

        /* (help) show command summary */
        if ( (strncmp(line,".H",2)==0) || (strncmp(line,".h",2)==0) ) { 
            help();
            continue;
        }

        /* find a string */
        if ((strncmp(line,".F ",3)==0) || (strncmp(line,".f ",3)==0)) {
            find(line);
            continue;
        }

        /* delete a line */
        if ( (strncmp(line,".D ",3)==0) || (strncmp(line,".d ",3)==0) ) {
            savebuf();
            delete(line);
            continue;
        } 

        /* copy */
        if ( (strncmp(line,".C ",3)==0) || (strncmp(line,".c ",3)==0) ) {
            savebuf();
            delete(line); /* part of same routine */
            continue;
        }

        /* cut */
        if ( (strncmp(line,".X ",3)==0) || (strncmp(line,".x ",3)==0) ) {
            savebuf();
            delete(line); /* part of same routine */
            continue;
        }


        /* insert a line */
        if ( (strncmp(line,".I ",3)==0) || (strncmp(line,".i ",3)==0) ) {
            savebuf();
            insert(line);
            continue;
        }

        /* paste */
        if ((strncmp(line,".P ",3)==0) || (strncmp(line,".p ",3)==0)) {
            savebuf();
            paste(line);
            continue;
        }

        /* replace */
        if ( (strncmp(line,".R ",3)==0) || (strncmp(line,".r ",3)==0) ) {
            savebuf();
            repl(line);
            continue;
        }

        /* replace all */
        if ( (strncmp(line,".RA ",4)==0) || (strncmp(line,".ra ",4)==0) ) {
            savebuf();
            repl(line);
            continue;
        }



        /* drop to a shell prompt: exit to return */
        if ((strncmp(line,".OS",3)==0) || (strncmp(line,".os",3)==0)) {
#ifdef LINUX
			err = system(SHELL);
			if (err) puts("shell error");
#endif
			continue;
        }

        /* not a command, save line to buffer */
        tbuf = realloc(buffer,curpos+strlen(line)+1);
        if (tbuf == NULL) {
            /* save buffer to 'edit_pid_.tmp' */
            backup();
            puts("fatal memory error (buffer saved)");
            exit(1);
        }
        buffer = tbuf;
        for (n=0; n<strlen(line); n++)
            buffer[curpos++] = line[n];
        
        continue;
    }
}

int quit(void) {
    free(buffer);
    exit(0);
}

void saveexit(void) {
    save("nul\0"); /* forces prompt for filename */
    quit();
}

/* save the buffer to a file */
int save(char *line) {
    FILE *outfile;
    char cmd[MAXLINE+1]={}, filename[MAXLINE+1]={}, tmp[MAXLINE+1]={};
    int n=0;

    if (curpos == 0) {
        puts("buffer empty");
        return 0;
    }
    sscanf(line,"%s %s",cmd,filename);
    /* no name given: get it (this allows for spaces in filename) */
    if (strlen(filename)==0) {
        puts("Filename: ");
        fgets(filename,MAXLINE-1,stdin);
        if (filename[0]=='\n')
            return 1;
    }
    if (filename[strlen(filename)-1] == '\n') 
        filename[strlen(filename)-1] = '\0';
    /* test if exists */
    outfile = fopen(filename,"r");
    if (outfile != NULL) {
        fclose(outfile);
        printf("File %s exists. Overwrite (y/n): ",filename);
        fgets(tmp,MAXLINE,stdin);
        if (!((tmp[0] == 'y') || (tmp[0] == 'Y'))) 
            return 0;
    }
    /* save the file */
    outfile = fopen(filename,"w");
    if (outfile == NULL) {
        printf("error writing to %s\n",filename);
        return 0;
    }
    for (n=0; n<curpos; n++)
        fprintf(outfile,"%c",buffer[n]);
    fflush(outfile); fclose(outfile);
    puts("file saved");
    return 0;
}

/* print the buffer */
void print(char *linein) {
    if (curpos == 0) return; /* don't waste time */
    char cmd[MAXLINE]={}, sline[MAXLINE]={}, eline[MAXLINE]={};
    char ch[MAXLINE] = {}, line[MAXLINE]={};
    int lineno = 1, pos = 0, n=0, row=0, startline = 0, endline = 0;
    /* get line numbers */
    sscanf(linein,"%s %s %s",cmd,sline,eline);  
    startline = (atoi(sline) < LINENO_MIN) ? LINENO_MIN:atoi(sline);
    endline = ((atoi(eline) == 0) || (atoi(eline) > LINENO_MAX)) ? LINENO_MAX:atoi(eline);
    /* show lines */
    while (pos < curpos) {
        for (n=0; buffer[pos+n] != '\n'; n++) {
            if (n >= curpos) return;
            line[n] = buffer[pos+n];
        }
        line[n] = '\0';
        if ((lineno >= startline) && (lineno <= endline)) {
            printf("%04d: %s\n",lineno,line);
            row++;
        }
        lineno++;
        pos += n+1;
        if (row == PAGELENGTH) {
            printf("\n--- MORE ---");
            ch[0]='\0';
            fgets(ch,MAXLINE,stdin);
            if ((ch[0]=='Q') || (ch[0]=='q')) return;
            row=0;
        }
    }
    return;
} 
        
/* find a string */
void find(char *linein) {
    int lineno = 1, n = 0, pos = 0;
    char *ptr=NULL;
    char line[MAXLINE]={};
    char cmd[MAXLINE]={}, findstr[MAXLINE]={};
    sscanf(linein,"%s %s",cmd,findstr);
    if (strlen(findstr)==0) {
        printf("usage: %s [search string]\n",cmd);
        return;
    }
    if (curpos == 0) return;
    while (pos < curpos) {
        for (n=0; buffer[pos+n] != '\n'; n++) {
            if (n >= curpos) {
                printf("bad search\n");
                return;
            }
            line[n] = buffer[pos+n];
        }
        line[n] = '\0';
        ptr = strstr(line,findstr);
        if (ptr != NULL)
            printf("%04d: %s\n",lineno,line);
        lineno += 1;
        pos += n+1;
    }
    return;
}

/* delete a line */
void delete(char *linein) {
    if (curpos == 0) return;
    char cmd[MAXLINE+1]={}, linenumber[MAXLINE+1]={};
    int line = 0, linectr = 0, n = 0, index = 0;
    int startpos = 0, endpos = 0, dist = 0;

    /* split linein */
    sscanf(linein,"%s %s",cmd,linenumber);
    line = atoi(linenumber);
    if ((line < LINENO_MIN) || (line > LINENO_MAX)) {
        printf("Invalid line number %d\n",line);
        return;
    }

    /* point to the line # */
    startpos = 0;
    for (n=0; n<curpos; n++) {
        if (buffer[n] == '\n') {
            linectr++;
            endpos = n;  
            if (line == linectr)
                goto del1;
            startpos = endpos+1;    /* skip the \n */
        }
    } /* went thru buffer */
    if (line != linectr) {
        if (n>=curpos) { 
            printf("line %d not found\n",line);
            return;
        }
    }

del1:   /* line #'s match */
    if (line == linectr) {
        dist = (endpos-startpos)+1;
        
        /* save the line for later paste */
        if ((strcmp(cmd,".x")==0) || (strcmp(cmd,".c")==0) ||
            (strcmp(cmd,".X")==0) || (strcmp(cmd,".C")==0)) {
            /* don't save on delete, only on cut/copy */
            for (index=0; index < dist; index++)
                cutbuffer[index] = buffer[startpos+index];
            cutbuffer[index+1]='\0';
        }

        /* see if command is copy only */
        if ((strcmp(cmd,".C")==0) || (strcmp(cmd,".c")==0)) return;
        
        /* delete the line, shift the buffer  */
        for (n=endpos+1; n<curpos; n++)
            buffer[n-dist] = buffer[n];
        
        /* reset the position counter */
        curpos -= dist;
        return;
    }
    
    /* fallthru */
    printf("unknown error\n");
    return;
}

/* insert a line BEFORE given line# */
void insert (char * linein) {
    if (curpos == 0) return;
    char cmd[MAXLINE+1] = {}, linenumber[MAXLINE+1] = {};
    char insline[MAXLINE+1]={};
    /* get line number */
    sscanf(linein,"%s %s",cmd,linenumber);
    int line = atoi(linenumber); /* line is line# to use */
    int inslength = 0;
    if ((line < LINENO_MIN) || (line > LINENO_MAX)) {
        printf("Invalid line number %d\n",line);
        return;
    }
    /* point to start of line number */
    int pos = 0, startpos = 0;
    int newcurpos = 0, n=0;
    char *p = buffer;
    while (p-buffer != curpos) {
        if (*(++p) != '\n') continue;
        if (++pos == line) break;
        startpos = (p-buffer)+1; /* startpos is start location */
    }
    if (p-buffer >= curpos) {
        printf("line %d not found\n",line);
        return;
    }
    /* insert loop */
    while (1) {
        /* get a line from the user */
        printf("insert> ");
        fgets(insline,MAXLINE,stdin);
        if ((strncmp(insline,".Q",2)==0) || (strncmp(insline,".q",2)==0)) return;
        inslength = strlen(insline);
        /* increase the buffer */
        tbuf = (char *)realloc(buffer,(curpos+inslength)+1);
        if (tbuf == NULL) {
            /* save buffer to 'edit_pid_.tmp' */
            backup();
            printf("memory error in insert()\n");
            exit(1);
        }
        buffer = tbuf;
        /* shift buffer up */
        newcurpos = curpos+inslength;
        for (n=0; n!=(curpos-startpos+1); n++)
            buffer[newcurpos-n] = buffer[curpos-n];
        /* insert new line */
        for (n=0; n!= inslength; n++)
            buffer[startpos+n] = insline[n];
        /* set new pointers */
        curpos = newcurpos;
        startpos += inslength;
    }
}

/* replace a string (.r=once, .ra=global) */
/* format: .r(replace)/.ra(replaceall) [start line] [orig string] [replacement string] */
void repl(char *linein) {
    if (curpos == 0) return;
    char cmd[MAXLINE+1]={}, linenum[MAXLINE+1]={}, orig[MAXLINE+1]={}, replace[MAXLINE+1]={};
    sscanf(linein,"%s %s %s %s",cmd,linenum,orig,replace);
    int distance = 0, startpos = 0, endpos = 0, line = 0;
    int newcurpos = 0, lineptr = 0;
    int FLAG = 0, n=0;

    /* error checking */
    line = atoi(linenum);
    if ((line < LINENO_MIN) || (line > LINENO_MAX)) {
        printf("invalid line number %d\n",line);
        return;
    }

    if ((strlen(orig)==0) || (strlen(replace)==0)) {
        printf("Format: .r /.ra [line#] [original string] [replacement string]\n");
        return;
    }

    /* find starting position (lineptr) given a line number in line */
    for (n=0; n<curpos; n++) {
        if (n == curpos-1) {    /* line not found */
            printf("line %d not found\n",line);
            return;
        }
        if (line==1) {  /* special case */
            lineptr=0;
            break;
        }
        if (buffer[n] == '\n') {
            lineptr++;
            if (lineptr == line-1) {    /* want to start at beginning of line #, not after */
                lineptr = n+1;
                break;
            }
        }
    }

replaceBegin:       /* loop back to here for global */
    /* search for instance */
    distance = 0; startpos = 0; endpos = 0;
    char *data = &buffer[lineptr];
    char *p = strstr(data,(char *)orig);
    if (p == NULL) {
        if (!FLAG) printf("string %s not found\n",orig);
        return;
    }
    /* get start position of string */
    startpos = (p - buffer);   

    /* make sure the beginning is either a space or \n */
    /* (some test here - needs work) */

    /* found start, now find end of string */
    while (p++) {
        if (*p != ' ') break;
        if (*p != '\n') break;
    }
    /* strings end either with newline or space. */

    endpos = (p - buffer);  
    distance = (endpos - startpos) + 1;

    /* old string length = new string length */
    if (strlen(orig) == strlen(replace)) {
        for (n=0; n<strlen(replace); n++)
            buffer[startpos+n] = replace[n];
        FLAG=1;
        if ((strcmp(cmd,".R")==0) || (strcmp(cmd,".r") == 0)) return;
        if ((strcmp(cmd,".RA")==0) || (strcmp(cmd,".ra") == 0)) goto replaceBegin;
    }

   /* old string length > new string length */
    if (strlen(replace) < strlen(orig)) {
       for (n=0; n<strlen(replace); n++)
           buffer[startpos+n] = replace[n];
        /* shift down the difference */
        endpos = startpos + strlen(orig);
        distance = strlen(orig) - strlen(replace);
        for (n=endpos; n<curpos; n++)
            buffer[n-distance] = buffer[n];
        curpos = curpos - distance;
        FLAG=1;
        if ((strcmp(cmd,".R")==0) || (strcmp(cmd,".r") == 0)) return;
        if ((strcmp(cmd,".RA")==0) || (strcmp(cmd,".ra") == 0)) goto replaceBegin;
    }


    /* new string length > old string length */
    if (strlen(replace) > strlen(orig)) {
        distance = (strlen(replace) - strlen(orig));
        tbuf = (char *)realloc(buffer,(curpos+distance+1));
        if (tbuf == NULL) {
            /* save buffer to 'edit_pid_.tmp' */
            backup();
            printf("memory error in replace\n");
            exit(1);
        }
        buffer = tbuf;
        
        /* move buffer contents up by distance */
        newcurpos = curpos + distance;
        for (n=0; n != (curpos-endpos+1); n++)
            buffer[newcurpos-n] = buffer[curpos-n];
        curpos = newcurpos;
        /* now replace the string */
        for (n=0; n!= strlen(replace); n++)
            buffer[startpos+n] = replace[n];
        FLAG = 1;
        if ((strcmp(cmd,".R")==0) || (strcmp(cmd,".r") == 0)) return;
        if ((strcmp(cmd,".RA")==0) || (strcmp(cmd,".ra") == 0)) goto replaceBegin;
    }

    /* should never get here */
    printf("unknown error in replace\n");
    return;
}

/* paste the cut buffer to the buffer */
void paste(char *linein) {
char cmd[MAXLINE]={}, linenumber[MAXLINE]={};
int n = 0, line = 0, linectr = 0;
int startpos = 0, endpos = 0, dist = 0;

    /* test if the buffer is occupied */
    for (n=0; n<MAXLINE; n++) 
        if (cutbuffer[n]=='\n') break;
    if ((n==0) || (n==MAXLINE) || (cutbuffer[0]=='\0')) {
        printf("buffer empty\n");
        return;
    }

    /* get the line number */
    sscanf(linein,"%s %s",cmd,linenumber);
    if (strlen(linenumber)==0) {
        printf("Usage: %s [line#]\n",cmd);
        return;
    }
    line = atof(linenumber);
    if ((line < LINENO_MIN) || (line > LINENO_MAX)) {
        printf("Invalid line number %d\n",line);
        return;
    }
    
    /* find where in the buffer it starts */
    for (n=0; n<curpos; n++) {
        if (buffer[n] == '\n') {
            linectr++;
            endpos = n+1;
        }
        if (line == linectr) break;
        startpos = endpos;
    }

    if (n > curpos) {
        printf("Line# %d not found\n",line);
        return;
    }

    /* get cutbuffer length */
    dist = strchr(cutbuffer,'\n')-&cutbuffer[0]+1;
    if (dist < 1) {
        puts("cannot paste buffer");
        return;
    }
    
    tbuf = (char *)realloc(buffer,curpos+dist+MAXLINE);
    if (tbuf == NULL) {
        /* save buffer to 'pid.tmp' */
        backup(); 
        printf("memory error in paste\n");
        exit(1);
    }
    buffer = tbuf;

    /* shift buffer up by size dist */
    int newcurpos = curpos + dist; 
    for (n=0; n != (curpos-startpos+1); n++)
        buffer[newcurpos-n] = buffer[curpos-n];

    /* insert cutbuffer */
    for (n=0; n!=dist; n++)
        buffer[startpos+n] = cutbuffer[n];

    curpos = newcurpos;

    return;
}

/* show command summary */
void help(void) {
    printf("--- Commands ---\n\n");
    printf(".q                  Quit w/o save\n");
    printf(".e                  Exit w/save (prompts for file name)\n");
    printf(".undo               Undo the last buffer-changing command\n");
    printf(".l [start#][end#]   List buffer. line#'s are optional\n");
    printf(".s  filename        Save buffer to named file\n");
    printf(".f  string          Find/display string in the buffer\n");
    printf(".d  line#           Delete line number\n");
    printf(".x  line#           Cut the line from the buffer to the clipboard\n");
    printf(".c  line#           Copy the line to the clipboard\n");
    printf(".p  line#           Paste the clipboard before line#\n");
    printf(".i  line#           Insert before line #, .q to stop insert\n");
    printf(".r  line# old new   Replace (once) old with new starting at line#\n");
    printf(".ra line# old new   Replace All occurances of old with new starting at line#\n");
    printf(".h  .?              Shows these help lines.\n");
    printf(".bak                Save buffer to backup file (editPID.bak)\n");
#ifdef LINUX
    printf(".os                 Drop to a command prompt: type 'exit' to return to the editor\n");
#endif
    printf("The following prompts are shown during operation:\n");
    printf(">                   Append prompt. Typing here appnds to the end of the buffer.\n");
    printf("insert>             Insert prompt. Typing here inserts text before the given line #.\n");
    printf("NOTE: commands can be either UPPER or lower case.\n");
    printf("\n");
    return;
}

/* save buffer to 'pid.tmp' 
 * normally called when realloc fails (should never see) 
 * but could also be called by the user for whatever reason
 * using the .bak command.
 */
void backup(void) {
    my_pid = getpid();
    char fname[MAXLINE] = {};   /* will prompt if filename exists */
    sprintf(fname,"save edit%d.bak",my_pid);
    save(fname);
    puts("buffer saved");
    return;
}

/* save buffer to ubuf for later undo */
void savebuf(void) {
    undocurpos = curpos;
    tbuf = (char *)realloc(ubuf,curpos+1*sizeof(char *));
    if (tbuf == NULL) {
        backup();
        puts("fatal memory error");
        exit(1);
    }
    ubuf = tbuf;
    memcpy(ubuf,buffer,curpos);
    return;
}

/* undo */
void undo(void) {
    if (undocurpos == 0) {
        puts("can't undo");
        return;
    }
    free(buffer);
    buffer = (char *) malloc((undocurpos+1)*sizeof(char *));
    if (buffer == NULL) {
        puts("fatal memory error");
        exit(1);
    }
    memcpy(buffer,ubuf,undocurpos);
    curpos = undocurpos;
    return;
}


