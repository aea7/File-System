#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>

#define RED		"\x1B[31m"
#define GRN		"\x1B[32m"
#define YEL		"\x1B[33m"
#define CYN		"\x1B[36m"
#define WHT		"\x1B[37m"
#define RESET	"\x1B[0m"

int parseCommand(char inputBuffer[], char *args[],char *leb[]);
int textIput(char inputBuffer[], char *args[],char *leb[]);


int kbhit(void);
void changemode(int);
int getWinColSize();


int readInput(){
	int ch;
	changemode(1);
	while ( !kbhit() );

	ch = getchar();

	//printf("%c", ch);

	changemode(0);

	// Exit on ESC
	if ( ch == 27)
		return 0;

	// Backspace
	if ( ch == 127)
		return 1000;

	return ch;
}


int main(){
	char inputBuffer[100];
	char *args[51];
	char *leb[51];
	char k;
	int shouldrun = 1;
	int size = 0;
	int textsize = 0;
	char key;
	int a;
	char text[4000];
	char textsplit1[1024];
	char textsplit2[1024];
	char textsplit3[1024];
	int textcnt=0;
	int i;

	mountKUFS();
	while (shouldrun){
		parseCommand(inputBuffer,args,leb); // Reading next command
		// Exiting from kufs
		if (strcmp(args[0], "exit") == 0)
			shouldrun = 0;

		if (strcmp(args[0], "create") == 0){
			// Empty argument protection
			if (args[1] == '\0'){
				printf("'Create' needs at least one argument\n" );
				continue;
			}

			int filenameLen = strlen(args[1]);
			char esctect[100] = "\x1B[31mPress ECS to exit\n\x1B[0m";

			a=1;
			printf(WHT "\e[1;1H\e[2JFile: %s%*sCharacter remaining:%d\n%s",
				args[1],
				getWinColSize()-(30+filenameLen),
				" ",
				3072-textcnt,
				esctect);
			while(a!=0){
				a = readInput();
				if (textcnt<4000){
					if (a == 1000){
						if (textcnt > 0){
							textcnt--;
							text[textcnt] = 0;
							printf(WHT "\e[1;1H\e[2JFile: %s%*sCharacter remaining:" GRN "%d" RESET"\n%s",
								args[1],
								getWinColSize()-(30+filenameLen),
								" ",
								3072-textcnt,
								esctect);
							printf("%s\n", text);
						}else{
							printf(WHT "\e[1;1H\e[2JFile: %s%*sCharacter remaining:" CYN "%d" RESET"\n%s",
								args[1],
								getWinColSize()-(30+filenameLen),
								" ",
								3072-textcnt,
								esctect);
						}

					}else{
						text[textcnt] = a;
						textcnt++;
						printf(WHT "\e[1;1H\e[2JFile: %s%*sCharacter remaining:\x1B[33m%d\x1B[0m\n%s",
								args[1],
								getWinColSize()-(30+filenameLen),
								" ",
								3072-textcnt,
								esctect);
						printf("%s\n", text);
					}
				}
			}

			// Replace SPACE's with 0's
			/*
			for (i=0;i<textcnt;i++)
				if (text[i] == 32)
					text[i] = 48;
			*/

			// Input parser depending on size
			if (strlen(text) <= 1024){
				textsize = 1;
				for (i=0;i<1024;i++)
					textsplit1[i]=text[i];
			}else if (strlen(text) <= 2048){
				textsize = 2;
				for (i=0;i<1024;i++)
					textsplit1[i]=text[i];
				for (i=1024;i<2048;i++)
					textsplit2[i]=text[i];
			}else if (strlen(text) <= 3072){
				textsize = 3;
				for (i=0;i<102;i++)
					textsplit1[i]=text[i];
				for (i=1024;i<2048;i++)
					textsplit2[i]=text[i];
				for (i=2048;i<3072;i++)
					textsplit3[i]=text[i];
			}else {
				printf("Cannot create a file this big!\n");
			}

			create(args[1],textsplit1,textsplit2,textsplit3,textsize);

			// Reset text/counter to default
			memset(&text[0], 0, sizeof(text));
			textcnt=0;
		}

		if ( strcmp(args[0], "display")  == 0) {
			// Empty argument protection
			if (args[1] == '\0'){
				printf("'display' needs at least one argument\n" );
				continue;
			}

			display(args[1]);

		}

		if ( strcmp(args[0], "rm")  == 0 ) {
			int current_directory;
			if (args[1] == '\0'){
				printf("'rm' needs at least one argument\n" );
				continue;
			} else{
				current_directory = getCurrentDirectory();
				rm(args[1],current_directory);
			}
		}

		if ( strcmp(args[0], "ls")  == 0 ) {
			ls();
		}

		if ( strcmp(args[0], "stats")  == 0 ) {
			stats();
		}

		if ( strcmp(args[0], "rd")  == 0 ) {
			rd();
		}

		if ( strcmp(args[0], "md")  == 0 ) {
			if (args[1] == '\0'){
				printf("'md' needs at least one argument\n" );
				continue;
			} else{
				md(args[1]);
			}
		}

		if ( strcmp(args[0], "cd")  == 0 ) {
			if (args[1] == '\0'){
				printf("'cd' needs at least one argument\n" );
				continue;
			} else{
				cd(args[1]);
			}
		}

	}
}


// helper
int parseCommand(char inputBuffer[], char *args[],char *leb[])
{
	int length,
		i,
		start,
		ct,
		command_number;
	ct = 0;

	do{
		printPrompt();
		fflush(stdout);
		length = read(STDIN_FILENO,inputBuffer,100);
	}
	while (inputBuffer[0] == '\n');

	start = -1;
	if (length == 0)
		exit(0);

	if ( (length < 0) && (errno != EINTR) ) {
		perror("error reading the command");
		exit(-1);
	}

	/**
	* Parse the contents of inputBuffer
	*/

	for (i=0;i<length;i++) {
		switch (inputBuffer[i]){
		case ' ':
		case '\t':
			if(start != -1){
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			start = -1;
			break;

		case '\n':
			if (start != -1){
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL;
			break;

	default :
		if (start == -1)
		start = i;

		}
	}
	args[ct] = NULL;
	return 1;
}

// helper
void changemode(int dir)
{
	static struct termios oldt, newt;

	if ( dir == 1 )
	{
		tcgetattr( STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~( ICANON | ECHO );
		tcsetattr( STDIN_FILENO, TCSANOW, &newt);
	}
	else
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

// helper
int kbhit (void)
	{
	struct timeval tv;
	fd_set rdfs;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&rdfs);
	FD_SET (STDIN_FILENO, &rdfs);

	select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &rdfs);
}

// helper
/**
* Returns terminal column size
*/
int getWinColSize(){
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	return w.ws_col;
}
