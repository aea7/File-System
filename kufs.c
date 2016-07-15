#include "kufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int * displayHelper(char *name);
void display(char *name);
void create(char *dname,char array1[1024],char array2[1024],char array3[1024],int arraySize);
void rm(char *dname,int inode_entry);
int showFile(char *filename ,char *f1, char *f2, char *f3,int fileSize);
int getCurrentDirectory();

#define RED		"\x1B[31m"
#define GRN		"\x1B[32m"
#define YEL		"\x1B[33m"
#define CYN		"\x1B[36m"
#define WHT		"\x1B[37m"
#define RESET	"\x1B[0m"

void rm(char *dname,int inode_entry){

	mountKUFS();
	char itype,ftype;
	int blocks[3];
	_directory_entry _directory_entries[4];

	int i,j;

	int file_exists=0,local_entry_count=0;
	int empty_ientry,file_inode,file_dentry,file_bentry,blockEmpty,global_entry_count,empty_dblock;

	// non-empty name
	if (strlen(dname)==0) {
		printf("Usage: remove <name>\n");
		return;
	}

	// do we have free inodes
	if (free_inode_entries == 0) {
		printf("Error: Inode table is full.\n");
		return;
	}

	// read inode entry for current directory
	// in KUFS, an inode can point to three blocks at the most
	itype = _inode_table[inode_entry].TT[0];
	blocks[0] = stoi(_inode_table[inode_entry].XX,2);
	blocks[1] = stoi(_inode_table[inode_entry].YY,2);
	blocks[2] = stoi(_inode_table[inode_entry].ZZ,2);

	// its a directory; so the following should never happen
	if (itype=='F') {
		printf("Fatal Error! Aborting.\n");
		exit(1);
	}

	// now lets try to see if the file or directory exists
	for (i=0; i<3; i++) {
		local_entry_count = 0;
		if (blocks[i]==0)  	// 0 means pointing at nothing
			continue;

		if(file_exists == 1) break;

		readKUFS(blocks[i],(char *)_directory_entries); // lets read a directory entry; notice the cast

		// so, we got four possible directory entries now
		for (j=0; j<4; j++) {
			if (_directory_entries[j].F=='0'){  // means unused entry
				continue;
			}
			local_entry_count++;
			if (strncmp(dname,_directory_entries[j].fname, 252) == 0) { // compare with user given name
				file_inode = stoi(_directory_entries[j].MMM,3);
				file_exists = 1;
				file_dentry = j;
				empty_dblock =i;
				file_bentry = blocks[i];
			}
			global_entry_count = local_entry_count;

		}
	}

		if(file_exists == 1 ){
			ftype = _inode_table[file_inode].TT[0];
			readKUFS(file_bentry,(char *)_directory_entries);

			if(ftype == 'F'){

			blockEmpty = stoi(_inode_table[file_inode].XX,2);
			if(blockEmpty != 0)
				returnBlock(blockEmpty);

			blockEmpty = stoi(_inode_table[file_inode].YY,2);
			if(blockEmpty != 0)
				returnBlock(blockEmpty);

			blockEmpty = stoi(_inode_table[file_inode].ZZ,2);
			if(blockEmpty != 0)
				returnBlock(blockEmpty);

			returnInode(file_inode);

			_directory_entries[file_dentry].F = '0';
			writeKUFS(file_bentry,(char *)_directory_entries);

			if(global_entry_count == 1){
				switch(empty_dblock) {	// update the inode entry of current dir to reflect that we are using a new block
					case 0: itos(_inode_table[inode_entry].XX,0,2); break;
					case 1: itos(_inode_table[inode_entry].YY,0,2); break;
					case 2: itos(_inode_table[inode_entry].ZZ,0,2); break;
				}
				writeKUFS(file_bentry,NULL);
				returnBlock(file_bentry);
				writeKUFS(BLOCK_INODE_TABLE, (char *)_inode_table);
			}

			}

			if(ftype == 'D'){
				
			blockEmpty = stoi(_inode_table[file_inode].XX,2);
			if(blockEmpty != 0)
				returnBlock(blockEmpty);

			blockEmpty = stoi(_inode_table[file_inode].YY,2);
			if(blockEmpty != 0)
				returnBlock(blockEmpty);

			blockEmpty = stoi(_inode_table[file_inode].ZZ,2);
			if(blockEmpty != 0)
				returnBlock(blockEmpty);

			returnInode(file_inode);

			_directory_entries[file_dentry].F = '0';
			writeKUFS(file_bentry,(char *)_directory_entries);


			if(global_entry_count == 1){
				switch(empty_dblock) {	// update the inode entry of current dir to reflect that we are using a new block
					case 0: itos(_inode_table[inode_entry].XX,0,2); break;
					case 1: itos(_inode_table[inode_entry].YY,0,2); break;
					case 2: itos(_inode_table[inode_entry].ZZ,0,2); break;
				}
				writeKUFS(file_bentry,NULL);
				returnBlock(file_bentry);
				writeKUFS(BLOCK_INODE_TABLE, (char *)_inode_table);
			}

			blocks[0] = stoi(_inode_table[file_inode].XX,2);
			blocks[1] = stoi(_inode_table[file_inode].YY,2);
			blocks[2] = stoi(_inode_table[file_inode].ZZ,2);

			// going over the directory entries and recursively calling rm function
			for (i=0; i<3; i++) {
				local_entry_count = 0;
				if (blocks[i]==0)  	// 0 means pointing at nothing
					continue;

				readKUFS(blocks[i],(char *)_directory_entries); // lets read a directory entry; notice the cast

				// so, we got four possible directory entries now
				for (j=0; j<4; j++) {
					if (_directory_entries[j].F=='0'){  // means unused entry
						continue;
					}
						rm(_directory_entries[j].fname,file_inode);
				}
			}
		}
		} else{
			printf("Error: There is no such file/directory\n");
			return;
		}
}


void create(char *dname,char array1[1024],char array2[1024],char array3[1024],int arraySize){
	mountKUFS();

	char itype;
	int blocks[3];
	_directory_entry _directory_entries[4];

	int i,j;

	int empty_dblock=-1,empty_dentry=-1;
	int empty_ientry;

	// non-empty name
	if (strlen(dname)==0) {
		printf("Usage: create <fname>\n");
		return;
	}

	// do we have free inodes
	if (free_inode_entries == 0) {
		printf("Error: Inode table is full.\n");
		return;
	}

	// read inode entry for current directory
	// in KUFS, an inode can point to three blocks at the most
	itype = _inode_table[CD_INODE_ENTRY].TT[0];
	blocks[0] = stoi(_inode_table[CD_INODE_ENTRY].XX,2);
	blocks[1] = stoi(_inode_table[CD_INODE_ENTRY].YY,2);
	blocks[2] = stoi(_inode_table[CD_INODE_ENTRY].ZZ,2);


	// its a directory; so the following should never happen
	if (itype=='F') {
		printf("Fatal Error! Aborting.\n");
		exit(1);
	}

	// now lets try to see if the name already exists
	for (i=0; i<3; i++) {
		if (blocks[i]==0) { 	// 0 means pointing at nothing
			if (empty_dblock==-1) empty_dblock=i; // we can later add a block if needed
			continue;
		}

		readKUFS(blocks[i],(char *)_directory_entries); // lets read a directory entry; notice the cast

		// so, we got four possible directory entries now
		for (j=0; j<4; j++) {
			if (_directory_entries[j].F=='0') { // means unused entry
				if (empty_dentry==-1) { empty_dentry=j; empty_dblock=i; } // AAHA! lets keep a note of it, just in case we have to create the new directory
				continue;
			}

			if (strncmp(dname,_directory_entries[j].fname, 252) == 0) { // compare with user given name
					printf("%.252s: Already exists.\n",dname);
					return;
			}
		}

	}
	// so file name is new

	// if we did not find an empty directory entry and all three blocks are in use; then no new file can be made
	if (empty_dentry==-1 && empty_dblock==-1) {
		printf("Error: Maximum directory entries reached.\n");
		return;
	}
	else { // otherwise
		if (empty_dentry == -1) { // Great! didn't find an empty entry but not all three blocks have been used
			empty_dentry=0;

			if ((blocks[empty_dblock] = getBlock())==-1) {  // first get a new block using the block bitmap
				printf("Error: Disk is full.\n");
				return;
			}
			writeKUFS(blocks[empty_dblock],NULL);	// write all zeros to the block (there may be junk from the past!)

			switch(empty_dblock) {	// update the inode entry of current dir to reflect that we are using a new block
				case 0: itos(_inode_table[CD_INODE_ENTRY].XX,blocks[empty_dblock],2); break;
				case 1: itos(_inode_table[CD_INODE_ENTRY].YY,blocks[empty_dblock],2); break;
				case 2: itos(_inode_table[CD_INODE_ENTRY].ZZ,blocks[empty_dblock],2); break;
			}

		}

		int firstBlock,secondBlock,thirdBlock;
		//get the input :

		// kac tane block input alındıysa onları writeKUFS yapıp firstblock secondblock third block yap

		firstBlock = 0;
		secondBlock = 0;
		thirdBlock = 0;


		if ((firstBlock = getBlock())==-1) {  // first get a new block using the block bitmap
			printf("Error: Disk is full.\n");
			return;
		}
		if(arraySize ==2){
		if ((secondBlock = getBlock())==-1) {  // first get a new block using the block bitmap
			printf("Error: Disk is full.\n");
			return;
		}
	}

		if(arraySize == 3){
		if ((thirdBlock = getBlock())==-1) {  // first get a new block using the block bitmap
			printf("Error: Disk is full.\n");
			return;
		}
	}

		writeKUFS(firstBlock,array1);
		if(arraySize == 2)
		writeKUFS(secondBlock,array2);
		if(arraySize == 3)
		writeKUFS(thirdBlock,array3);

		// NOTE: all error checkings have already been done at this point!!
		// time to put everything together

		empty_ientry = getInode();	// get an empty place in the inode table which will store info about blocks for this new directory

		readKUFS(blocks[empty_dblock],(char *)_directory_entries);	// read block of current directory where info on this new directory will be written
		_directory_entries[empty_dentry].F = '1';			// remember we found which directory entry is unused; well, set it to used now
		strncpy(_directory_entries[empty_dentry].fname,dname,252);	// put the name in there
		itos(_directory_entries[empty_dentry].MMM,empty_ientry,3);	// and the index of the inode that will hold info inside this directory
		writeKUFS(blocks[empty_dblock],(char *)_directory_entries);	// now write this block back to the disk


		strncpy(_inode_table[empty_ientry].TT,"FI",2);		// create the inode entry...first, its a directory, so DI
	  itos(_inode_table[empty_ientry].XX,firstBlock,2);
	  itos(_inode_table[empty_ientry].YY,secondBlock,2);
	  itos(_inode_table[empty_ientry].ZZ,thirdBlock,2);

		writeKUFS(BLOCK_INODE_TABLE, (char *)_inode_table);	// phew!! write the inode table back to the disk
	}
}

void display(char *name){
	char array1[1024],array2[1024],array3[1024];
	int fileSize = 0;
	if (strlen(name)==0) {
		printf("Usage: create <fname>\n");
		return;
	}

	mountKUFS();
	int *a;

	a = displayHelper(name);

printf("ints are : %d %d %d\n",a[0],a[1],a[2]);

	if(a[0] != 0){
		fileSize++;
		readKUFS(a[0],array1);
	}

	if(a[1] != 0){
		readKUFS(a[1],array2);
		fileSize++;
	}

	if(a[2] != 0){
		fileSize++;
		readKUFS(a[2],array3);
	}

 showFile(name,array1,array2,array3,fileSize);

}

int * displayHelper(char *name){

	char itype;
	int blocks[3];
	_directory_entry _directory_entries[4];
	_inode_entry	temp;
	static int fileBlocks[3];

	int total_files=0, total_dirs=0;

	int i,j;
	int e_inode;

	// read inode entry for current directory
	// in KUFS, an inode can point to three blocks at the most
	itype = _inode_table[CD_INODE_ENTRY].TT[0];
	blocks[0] = stoi(_inode_table[CD_INODE_ENTRY].XX,2);
	blocks[1] = stoi(_inode_table[CD_INODE_ENTRY].YY,2);
	blocks[2] = stoi(_inode_table[CD_INODE_ENTRY].ZZ,2);

	// its a directory; so the following should never happen
	if (itype=='F') {
		printf("Fatal Error! Aborting.\n");
		exit(1);
	}

	// lets traverse the directory entries in all three blocks
	for (i=0; i<3; i++) {
		if (blocks[i]==0) continue;	// 0 means pointing at nothing

		readKUFS(blocks[i],(char *)_directory_entries);	// lets read a directory entry; notice the cast

		// so, we got four possible directory entries now
		for (j=0; j<4; j++) {
			if (_directory_entries[j].F=='0')	continue;	// means unused entry

			e_inode = stoi(_directory_entries[j].MMM,3);	// this is the inode that has more info about this entry

			temp = _inode_table[e_inode];

			if (temp.TT[0]=='F'  &&  strncmp(name,_directory_entries[j].fname, 252) == 0)
			 { // entry is for a file
				fileBlocks[0] = stoi(temp.XX,2);
				fileBlocks[1] = stoi(temp.YY,2);
				fileBlocks[2] = stoi(temp.ZZ,2);
				total_files++;
			}

		}
	}

return fileBlocks;

}

int getCurrentDirectory(){
	return CD_INODE_ENTRY;
}

int showFile(char *filename ,char f1[1024], char f2[1024], char f3[1024],int fileSize){
	int filenameLen = strlen(filename);
	if(fileSize == 1){
		strcpy(f2," ");
		strcpy(f3," ");
	}
	if(fileSize == 2){
		strcpy(f3," ");
	}

	printf(WHT "\e[1;1H\e[2JDisplaying File: %s%*sFile size:" CYN "%dbytes\n" RESET,
		filename,
		getWinColSize()-(36+filenameLen),
		" ",
		(1024*fileSize));//size
	printf("%s%*s%s%s%s\n%s%*s",
		" ",
		(getWinColSize()/2)+25,
		WHT "- - - -   BEGINNING OF FILE   - - - -\n" RESET,
		f1,
		f2,
		f3,
		" ",
		(getWinColSize()/2)+22,
		WHT "- - - -   END OF FILE   - - - -\n" RESET);
}
