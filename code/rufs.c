/*
 *  Copyright (C) 2022 CS416/518 Rutgers CS
 *	RU File System
 *	File:	rufs.c
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "rufs.h"

#define DIRECTORY_TYPE 0
#define FILE_TYPE 1

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here

struct superblock *memSuperBlock;
struct inode * rootInode;

int dataBlockCount = 0;
int inodeAssigCount = 0;

//void *buff;
//struct inode * memInode;
//struct dirent * memDirent;

int sizeOfInode; //calculates the size of the Inode
int sizeOfDirent;//claculates the size of the Directory entry
int inode_per_block;
int dirent_per_block;
int noOfInodes;
int noOfDataBlocks;
int noOfInodeBlocks;

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	
	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk 
	void *buff = malloc(BLOCK_SIZE);
		bio_read(1,buff);
		bitmap_t b = (bitmap_t) buff;
		for(int i = 0;i<noOfInodes;i++){
			if(get_bitmap(b,i)==0){
				set_bitmap(b,i);
				bio_write(1,buff);
				free(buff);
				inodeAssigCount++;
				return i;
			}
		}
		free(buff);
	return -1;
}
void clear_ino(int ino){
	void *buff = malloc(BLOCK_SIZE);
		bio_read(1,buff);
		bitmap_t b = (bitmap_t) buff;
		
				unset_bitmap(b,ino);
				bio_write(1,buff);
				free(buff);
				inodeAssigCount--;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	
	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk 
	void *buff = malloc(BLOCK_SIZE);
		bio_read(2,buff);
		bitmap_t b = (bitmap_t) buff;
		for(int i = 0;i<noOfDataBlocks;i++){
			if(get_bitmap(b,i)==0){
				set_bitmap(b,i);
				bio_write(2,buff);
				free(buff);
				dataBlockCount++;
				return i;
			}
		}
		free(buff);
	return -1;

	return 0;
}


void clear_block(int ino){
	void *buff = malloc(BLOCK_SIZE);
		bio_read(2,buff);
		bitmap_t b = (bitmap_t) buff;
				unset_bitmap(b,ino);
				bio_write(2,buff);
				free(buff);
				dataBlockCount--;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

  // Step 1: Get the inode's on-disk block number

  // Step 2: Get offset of the inode in the inode on-disk block

  // Step 3: Read the block from disk and then copy into inode structure

	int inodeBlock = ino/inode_per_block;
	int inodeOffset = ino%inode_per_block;


		void *buff = malloc(BLOCK_SIZE);
		bio_read(memSuperBlock->i_start_blk+inodeBlock,buff);
		*inode = *((struct inode *)((struct inode *)buff + inodeOffset));
		//*inode = *((struct inode *)(buff + (sizeOfInode *inodeOffset)));
		
		free(buff);

	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	
	// Step 2: Get the offset in the block where this inode resides on disk

	// Step 3: Write inode to disk 

	int inodeBlock = ino/inode_per_block;
	int inodeOffset = ino%inode_per_block;


		void *buff = malloc(BLOCK_SIZE);
		bio_read(memSuperBlock->i_start_blk+inodeBlock,buff);
	 *((struct inode *)((struct inode *)buff + inodeOffset))= *inode;
	// *((struct inode *)(buff + (sizeOfInode *inodeOffset)))	= *inode;
		bio_write(memSuperBlock->i_start_blk+inodeBlock,buff);

		free(buff);


	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

  // Step 2: Get data block of current directory from inode

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure
	void *buff;
struct inode *	memInode = malloc(sizeOfInode);
	readi(ino,memInode);
	struct dirent * temp_dirent;
	int no_of_Dir_D_Blocks = memInode->size/BLOCK_SIZE;
	if(memInode->size%BLOCK_SIZE!=0){
		no_of_Dir_D_Blocks +=1;
	}
	for(int i = 0;i<16;i++){
		if(memInode->direct_ptr[i]!=-1){
		buff = malloc(BLOCK_SIZE);
		bio_read(memInode->direct_ptr[i],buff);
		for(int j = 0;j<dirent_per_block;j++){
		temp_dirent = malloc(sizeOfDirent);
			*temp_dirent = *((struct dirent *)((struct dirent *)buff+j));
			if(temp_dirent->valid ==1){
			char *name = malloc(temp_dirent->len);
			strcpy(name, temp_dirent->name);
			if(strcmp(name,fname)==0){
				*dirent = *temp_dirent;
				free(name);
				free(buff);
				free(temp_dirent);
				free(memInode);
				return 0;
			}
				free(name);
			}
				free(temp_dirent);
		}
				free(buff);
		}
	}
	free(memInode);
	return -1;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	
	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry
	struct dirent * temp_dirent;
	void *buff;
	temp_dirent =(struct dirent *) malloc(sizeOfDirent);
	if(dir_find(dir_inode.ino,fname,name_len,temp_dirent)==-1){
		free(temp_dirent);
		int no_of_Dir_D_Blocks = ((dir_inode.size)+sizeOfDirent)/BLOCK_SIZE;
	if(((dir_inode.size)+sizeOfDirent)%BLOCK_SIZE!=0){
		no_of_Dir_D_Blocks +=1;
	}
	for(int i = 0;i<16;i++){
		if(dir_inode.direct_ptr[i]!=-1){
		buff = malloc(BLOCK_SIZE);
		bio_read(dir_inode.direct_ptr[i],buff);
		for(int j = 0;j<dirent_per_block;j++){
		temp_dirent = malloc(sizeOfDirent);
			*temp_dirent = *((struct dirent *)((struct dirent *)buff+j));
			if(temp_dirent->valid ==0){
				temp_dirent->ino = f_ino;
			temp_dirent->valid = 1;
			strcpy(temp_dirent->name,fname);
			temp_dirent->len = name_len;
			*((struct dirent *)((struct dirent *)buff+j))= *temp_dirent;
			bio_write(dir_inode.direct_ptr[i],buff);
			

			free(temp_dirent);
			free(buff);
			dir_inode.size +=sizeOfDirent;
			time(&(dir_inode.vstat.st_atime));       
			time(&(dir_inode.vstat.st_mtime));         
			time(&(dir_inode.vstat.st_ctime));    
      dir_inode.vstat.st_size = dir_inode.size; 
			writei(dir_inode.ino,&dir_inode);
			return 0;
			}
		}
		}else{
			int blockNum = memSuperBlock->d_start_blk+get_avail_blkno();
			buff = malloc(BLOCK_SIZE);
		bio_read(blockNum,buff);
		temp_dirent =(struct dirent *) malloc(sizeOfDirent);
		temp_dirent->ino = -1;
			temp_dirent->valid = 0;
			strcpy(temp_dirent->name , "");
			temp_dirent->len = -1;
		for(int j = 0; j<dirent_per_block;j++){
			*((struct dirent *)((struct dirent *)buff+j))= *temp_dirent;
		}
		free(temp_dirent);
		temp_dirent =(struct dirent *) malloc(sizeOfDirent);
			temp_dirent->ino = f_ino;
			temp_dirent->valid = 1;
			strcpy(temp_dirent->name,fname);
			temp_dirent->len = name_len;
			*((struct dirent *)((struct dirent *)buff+0))= *temp_dirent;
			bio_write(blockNum,buff);


			free(temp_dirent);
			free(buff);
			dir_inode.direct_ptr[i] = blockNum;
			dir_inode.size +=sizeOfDirent;
			time(&(dir_inode.vstat.st_atime));       
			time(&(dir_inode.vstat.st_mtime));         
			time(&(dir_inode.vstat.st_ctime));    
      dir_inode.vstat.st_size = dir_inode.size; 
			writei(dir_inode.ino,&dir_inode);
			return 0;
		}
	}
	}


	printf("\nThe file with same name exists");
	return -1;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	
	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
	void *buff;
	struct dirent * temp_dirent;
	int no_of_Dir_D_Blocks = dir_inode.size/BLOCK_SIZE;
	if(dir_inode.size%BLOCK_SIZE!=0){
		no_of_Dir_D_Blocks +=1;
	}
	for(int i = 0;i<16;i++){
		if(dir_inode.direct_ptr[i]!=-1){
		buff = malloc(BLOCK_SIZE);
		bio_read(dir_inode.direct_ptr[i],buff);
		for(int j = 0;j<dirent_per_block;j++){
		temp_dirent = malloc(sizeOfDirent);
			*temp_dirent = *((struct dirent *)((struct dirent *)buff+j));
			if(temp_dirent->valid ==1){
			char *name = malloc(temp_dirent->len);
			strcpy(name, temp_dirent->name);
			if(strcmp(name,fname)==0){
				temp_dirent->ino = -1;
			temp_dirent->valid = 0;
			strcpy(temp_dirent->name , "");
			temp_dirent->len = -1;
				*((struct dirent *)((struct dirent *)buff+j))= *temp_dirent;
			bio_write(dir_inode.direct_ptr[i],buff);
			free(temp_dirent);
			free(buff);
			dir_inode.size -=sizeOfDirent;
			time(&(dir_inode.vstat.st_atime));       
			time(&(dir_inode.vstat.st_mtime));         
			time(&(dir_inode.vstat.st_ctime));    
      dir_inode.vstat.st_size = dir_inode.size; 
			writei(dir_inode.ino,&dir_inode);
				return 0;
			}
				free(name);
			}
				free(temp_dirent);
		}
				free(buff);
		}
	}
	printf("\nThe file does not exists");
	return -1;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
	int pathValid =0;


	char *newPath = strdup(path);
	
	char * baseName = strdup(basename(newPath));
	char * direName = strdup(dirname(newPath));
	

	
	if(strcmp(direName, baseName)!=0){
	
		pathValid = get_node_by_path(direName,ino,inode);
	}
	if(strcmp(direName, baseName)==0){
		if(strcmp("/", baseName)==0){
			readi(ino,inode);
			return 0;
		}else{
			return -1;
		}
	}else{
		if(pathValid != -1){
			struct dirent * temp_dirent = malloc(sizeOfDirent);
			if(dir_find(inode->ino,baseName,strlen(baseName),temp_dirent)!= -1){
				readi(temp_dirent->ino,inode);
				free(temp_dirent);
				return 0;
			}else{
			// 	struct inode * temp = malloc(sizeOfInode);
			// 	memset(temp,0,sizeOfInode);
			// 	*inode = *((struct inode *)temp);
			// //	free(temp);
				printf("\n pathInvalid");
				free(temp_dirent);
				return -1;
			}
		}else{
			// struct inode * temp = malloc(sizeOfInode);
			// 	*inode = *((struct inode *)temp);
			// 	//free(temp);
			printf("\n pathInvalid");
			return -1;
		}
	}

}

/* 
 * Make file system
 */
int rufs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile

	// write superblock information

	// initialize inode bitmap

	// initialize data block bitmap

	// update bitmap information for root directory

	// update inode for root directory
	
				//Code here

		void * buff;

	  dev_init(diskfile_path);
		dev_open(diskfile_path);
		memSuperBlock = (struct superblock *)malloc(sizeof(struct superblock));
		memSuperBlock->magic_num = MAGIC_NUM;			/* magic number */
		memSuperBlock->max_inum = MAX_INUM;			/* maximum inode number */
		memSuperBlock->max_dnum = MAX_DNUM;			/* maximum data block number */
		memSuperBlock->i_bitmap_blk = 1;		/* start block of inode bitmap */
		memSuperBlock->d_bitmap_blk = 2;		/* start block of data block bitmap */
		memSuperBlock->i_start_blk = 3;		/* start block of inode region */
		memSuperBlock->d_start_blk = memSuperBlock->i_start_blk+noOfInodeBlocks;


		//Initializing super block
		buff = malloc(BLOCK_SIZE);
		bio_read(0,buff);
		*((struct superblock*)buff) = *memSuperBlock;
		bio_write(0,buff);
		free(buff);


		//Initializing inode bitmap
		buff = malloc(BLOCK_SIZE);
		bio_read(1,buff);
		memset(buff, 0, BLOCK_SIZE);
		bio_write(1,buff);
		free(buff);

		

		//Initializing Datablock bitmap
		buff = malloc(BLOCK_SIZE);
		bio_read(2,buff);
		memset(buff, 0, BLOCK_SIZE);
		bio_write(2,buff);
		free(buff);

		
		
		


		//Root directory initialization 
			rootInode = (struct inode *)malloc(sizeof(struct inode));
			int inodeNumberToSetup = get_avail_ino();
			if(inodeNumberToSetup==-1){
				printf("\nNo space available to create the file");
				return -1;
			}
		 	rootInode->ino = inodeNumberToSetup;
			rootInode->valid = 1;
			rootInode->size = 0;
			rootInode->type = DIRECTORY_TYPE;
			rootInode->link = 2;
			for(int i=0;i<16;i++){
				rootInode->direct_ptr[i] = -1;
			}
			for(int i=0;i<8;i++){
				rootInode->indirect_ptr[i] = -1;
			}
			rootInode->vstat.st_ino = inodeNumberToSetup;
			rootInode->vstat.st_mode = rootInode->type == DIRECTORY_TYPE?S_IFDIR | 0755:S_IFREG | 0644;        
			rootInode->vstat.st_nlink = rootInode->type == DIRECTORY_TYPE?2:1;       
			rootInode->vstat.st_uid = getuid();         
			rootInode->vstat.st_gid = getgid();         
			time(&(rootInode->vstat.st_atime));       
			time(&(rootInode->vstat.st_mtime));         
			time(&(rootInode->vstat.st_ctime));       
      rootInode->vstat.st_size = rootInode->size;        

			writei(rootInode->ino,rootInode);

			 dir_add(*rootInode,rootInode->ino,".",1);

			readi(rootInode->ino,rootInode);

		 dir_add(*rootInode,rootInode->ino,"..",2);

			readi(rootInode->ino,rootInode);
			
			

	return 0;
}


/* 
 * FUSE file operations
 */
static void *rufs_init(struct fuse_conn_info *conn) {

	// Step 1a: If disk file is not found, call mkfs

  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk

			//Code here

		sizeOfInode = sizeof(struct inode); //calculates the size of the Inode
   	sizeOfDirent = sizeof(struct dirent);//claculates the size of the Directory entry
    inode_per_block = BLOCK_SIZE/sizeOfInode;
    dirent_per_block = BLOCK_SIZE/sizeOfDirent;

		noOfInodes = MAX_INUM;
		noOfDataBlocks = MAX_DNUM;
		noOfInodeBlocks = (noOfInodes*sizeOfInode)/BLOCK_SIZE;


	if(dev_open(diskfile_path)==-1){
		rufs_mkfs();
	}else{
	//Initializing super block
	void *buff;
		buff = malloc(BLOCK_SIZE);
		memSuperBlock = (struct superblock *)malloc(sizeof(struct superblock));
		bio_read(0,buff);
		*memSuperBlock = *((struct superblock*)buff);
		free(buff);

		//Initializing root Inode
		rootInode = malloc(sizeOfInode);
		readi(0,rootInode);

			//This is for test check 



	}
  

	return NULL;
}

static void rufs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
free(rootInode);
free(memSuperBlock);

printf("\nTotal number of DataBlocks %d \n Total number of InodeBlocks %f",dataBlockCount,(inodeAssigCount*1.0)/(inode_per_block*1.0));

	// Step 2: Close diskfile
	dev_close();
}

static int rufs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode


stbuf->st_uid = getuid();
stbuf->st_gid = getgid();
stbuf->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
stbuf->st_mtime = time( NULL );
struct inode *	memInode = malloc(sizeOfInode);
int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
if(fileFound!=-1){
		*stbuf = memInode->vstat;
		// stbuf->st_mode   = S_IFDIR | 0755;
		// stbuf->st_nlink  = 2;
		// time(&stbuf->st_mtime);
		//Code here

		free(memInode);
	return fileFound;
	}else{
		free(memInode);
		return -ENOENT;
	}
}

static int rufs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
		
	// Step 2: If not find, return -1
	struct inode *	memInode = malloc(sizeOfInode);
		int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
		if(fileFound!=-1){
			if(memInode->type!=DIRECTORY_TYPE){
				free(memInode);
				return -1;
			}
		}
		free(memInode);
    return fileFound;
}

static int rufs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode *	memInode = malloc(sizeOfInode);
		struct inode *phInode;
		struct dirent *temp_dirent;
		int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
		if(fileFound!=-1){
					if(memInode->type!=DIRECTORY_TYPE){
						free(memInode);
						return -1;
					}
						for(int i = 0;i<16;i++){
										if(memInode->direct_ptr[i]!=-1){
										void *temp_buff = malloc(BLOCK_SIZE);
										bio_read(memInode->direct_ptr[i],temp_buff);
										for(int j = 0;j<dirent_per_block;j++){
										temp_dirent = malloc(sizeOfDirent);
											*temp_dirent = *((struct dirent *)((struct dirent *)temp_buff+j));
											if(temp_dirent->valid ==1){
													phInode = malloc(sizeOfInode);
													readi(temp_dirent->ino,phInode);
													filler(buffer,temp_dirent->name,&(phInode->vstat),0);
													free(phInode);
											}
												free(temp_dirent);
										}
												free(temp_buff);
										}
					}
			
		}
	// Step 2: Read directory entries from its data blocks, and copy them to filler

	free(memInode);
	return fileFound;
}


static int rufs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name



		char *newPath = strdup(path);
	char * baseName = strdup(basename(newPath));
	char * direName = strdup(dirname(newPath));
	// Step 2: Call get_node_by_path() to get inode of parent directory



		struct inode *parentInode;
		parentInode = malloc(sizeOfInode);
		int fileFound = 	get_node_by_path(direName,rootInode->ino,parentInode);
		if(fileFound!=-1){
					if(parentInode->type!=DIRECTORY_TYPE){
						free(parentInode);
						return -1;
					}
					struct dirent * temp_dirent = malloc(sizeOfDirent);
					if(dir_find(parentInode->ino,baseName,strlen(baseName),temp_dirent)!=-1){
						printf("\nfile exists");
						free(parentInode);
						free(temp_dirent);
						return -1;
					}
				free(temp_dirent);

					struct inode *	memInode = (struct inode *)malloc(sizeOfInode);
									int inodeNumberToSetup = get_avail_ino();
									if(inodeNumberToSetup==-1){
										printf("\nNo space available to create the file");
										return -1;
									}
								 	memInode->ino = inodeNumberToSetup;
									memInode->valid = 1;
									memInode->size = 0;
									memInode->type = DIRECTORY_TYPE;
									memInode->link = 2;
									for(int i=0;i<16;i++){
										memInode->direct_ptr[i] = -1;
									}
									for(int i=0;i<8;i++){
										memInode->indirect_ptr[i] = -1;
									}
									memInode->vstat.st_ino = inodeNumberToSetup;
									memInode->vstat.st_mode = S_IFDIR | 0755;        
									memInode->vstat.st_nlink = memInode->type == DIRECTORY_TYPE?2:1;       
									memInode->vstat.st_uid = getuid();         
									memInode->vstat.st_gid = getgid();         
									time(&(memInode->vstat.st_atime));       
									time(&(memInode->vstat.st_mtime));         
									time(&(memInode->vstat.st_ctime));       
						      memInode->vstat.st_size = memInode->size;        
									
									writei(memInode->ino,memInode);

									dir_add(*parentInode,memInode->ino,baseName,strlen(baseName));

									dir_add(*memInode,memInode->ino,".",1);

									readi(memInode->ino,memInode);

									dir_add(*memInode,parentInode->ino,"..",2);

									readi(memInode->ino,memInode);

									

									free(memInode);
			
		}
	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk
		//Code here

	free(parentInode);
	return fileFound;
}

static int rufs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
		char *newPath = strdup(path);
		char * baseName = strdup(basename(newPath));
		char * direName = strdup(dirname(newPath));
	// Step 2: Call get_node_by_path() to get inode of target directory
		struct inode *	memInode = malloc(sizeOfInode);
		struct inode *  parentInode;
		int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
		if(fileFound!=-1){
					if(memInode->type!=DIRECTORY_TYPE||memInode->size>(2*sizeOfDirent)){
						free(memInode);
						return -1;
					}
					for(int i = 0;i<16;i++){
										if(memInode->direct_ptr[i]!=-1){
										void *temp_buff = malloc(BLOCK_SIZE);
										bio_read(memInode->direct_ptr[i],temp_buff);
										memset(temp_buff,0,BLOCK_SIZE);
										bio_write(memInode->direct_ptr[i],temp_buff);
										free(temp_buff);
										clear_block(memInode->direct_ptr[i] - memSuperBlock->d_start_blk);
										memInode->direct_ptr[i] = -1;
										}
					}
					memInode->valid = 0;
					clear_ino(memInode->ino);
					parentInode = malloc(sizeOfInode);
					get_node_by_path(direName,rootInode->ino,parentInode);
					dir_remove(*parentInode,baseName,strlen(baseName));
					free(parentInode);
					free(memInode);
					return 0;
		}
	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory
	free(memInode);
	return fileFound;
}


static int rufs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

char *newPath = strdup(path);
	char * baseName = strdup(basename(newPath));
	char * direName = strdup(dirname(newPath));
	// Step 2: Call get_node_by_path() to get inode of parent directory


		struct inode *parentInode;
		parentInode = malloc(sizeOfInode);
		int fileFound = 	get_node_by_path(direName,rootInode->ino,parentInode);
		if(fileFound!=-1){
					if(parentInode->type!=DIRECTORY_TYPE){
						free(parentInode);
						return -1;
					}
				struct dirent * temp_dirent = malloc(sizeOfDirent);
					if(dir_find(parentInode->ino,baseName,strlen(baseName),temp_dirent)!=-1){
						printf("\nfile exists");
						free(parentInode);
						free(temp_dirent);
						return -1;
					}
				free(temp_dirent);

					struct inode *	memInode = (struct inode *)malloc(sizeOfInode);
									int inodeNumberToSetup = get_avail_ino();
									if(inodeNumberToSetup==-1){
										printf("\nNo space available to create the file");
										return -1;
									}
								 	memInode->ino = inodeNumberToSetup;
									memInode->valid = 1;
									memInode->size = 0;
									memInode->type = FILE_TYPE;
									memInode->link = 1;
									for(int i=0;i<16;i++){
										memInode->direct_ptr[i] = -1;
									}
									for(int i=0;i<8;i++){
										memInode->indirect_ptr[i] = -1;
									}
									memInode->vstat.st_ino = inodeNumberToSetup;
									memInode->vstat.st_mode =DIRECTORY_TYPE?S_IFDIR | 0755:S_IFREG | 0644;        
									memInode->vstat.st_nlink = memInode->type == DIRECTORY_TYPE?2:1;       
									memInode->vstat.st_uid = getuid();         
									memInode->vstat.st_gid = getgid();         
									time(&(memInode->vstat.st_atime));       
									time(&(memInode->vstat.st_mtime));         
									time(&(memInode->vstat.st_ctime));       
						      memInode->vstat.st_size = memInode->size;        

									writei(memInode->ino,memInode);

									
									dir_add(*parentInode,memInode->ino,baseName,strlen(baseName));

									free(memInode);
			
		}
	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk
	free(parentInode);
	return 0;
}

static int rufs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1
struct inode *	memInode = malloc(sizeOfInode);
		int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
		if(fileFound!=-1){
			if(memInode->type!=FILE_TYPE){
				free(memInode);
				return -1;
			}
		}
		free(memInode);
    return fileFound;

	return 0;
}

static int rufs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path
struct inode *	memInode = malloc(sizeOfInode);
int maxSize = (16 * BLOCK_SIZE) + (8 * (BLOCK_SIZE/sizeof(int)) * BLOCK_SIZE);
off_t offsetC = offset;
int offsetStart = -1;
int offsetSize = -1;
int writtenSize = 0;
int toBeWritten = size;
int currentBlock = -1;
void *temp_buff;
//indirect block variables
int indCurrentBlock;
int indIndex;
int indBlockArrayIndex;
int indPointedBlockId;

		int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
		if(fileFound!=-1){
			if(memInode->type!=FILE_TYPE){
				free(memInode);
				return writtenSize;
			}
				if(offset>memInode->size){
				free(memInode);
				return writtenSize;
			}
				while(toBeWritten!=0){
				currentBlock = offsetC/BLOCK_SIZE;
				offsetStart = offsetC%BLOCK_SIZE;
				offsetSize = BLOCK_SIZE-offsetStart;
				if(offsetSize>toBeWritten){
					offsetSize= toBeWritten;
				}
				if(offsetC<maxSize){
			if(currentBlock<16){
				if(memInode->direct_ptr[currentBlock]==-1){
					free(memInode);
				return writtenSize;
				}
				temp_buff = malloc(BLOCK_SIZE);
				bio_read(memInode->direct_ptr[currentBlock],temp_buff);
				int j = writtenSize;
				for(int i = 0;i<offsetSize;i++){
				*((char *)buffer + j) =	*((char *)temp_buff + offsetStart +i);
					j++;
				}
				free(temp_buff);

			}else{
					//Indirect blocks addition

				indCurrentBlock = currentBlock-16;
				indIndex = indCurrentBlock/(BLOCK_SIZE/sizeof(int));
				indBlockArrayIndex = indCurrentBlock%(BLOCK_SIZE/sizeof(int));

					if(memInode->indirect_ptr[indIndex]==-1){
							free(memInode);
							return writtenSize;
					}
					temp_buff = malloc(BLOCK_SIZE);
						bio_read(memInode->indirect_ptr[indIndex],temp_buff);
							 indPointedBlockId = *((int *)temp_buff+indBlockArrayIndex);
						if(indPointedBlockId == -1){
								free(memInode);
								return writtenSize;
						}
						free(temp_buff);
						temp_buff = malloc(BLOCK_SIZE);
				bio_read(indPointedBlockId,temp_buff);
				int j = writtenSize;
				for(int i = 0;i<offsetSize;i++){
					*((char *)buffer + j) =	*((char *)temp_buff + offsetStart +i);
					j++;
				}
			//	bio_write(indPointedBlockId,temp_buff);
				free(temp_buff);
					}
					writtenSize += offsetSize;
					toBeWritten -= offsetSize;
					offsetC += offsetSize;
			
			}else{
			
				free(memInode);
				return writtenSize;
			}				
		}
		}
		free(memInode);
    //return fileFound;
	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return writtenSize;
}

static int rufs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
struct inode *	memInode = malloc(sizeOfInode);
int maxSize = (16 * BLOCK_SIZE) + (8 * (BLOCK_SIZE/sizeof(int)) * BLOCK_SIZE);
off_t offsetC = offset;
int offsetStart = -1;
int offsetSize = -1;
int writtenSize = 0;
int toBeWritten = size;
int currentBlock = -1;
void *temp_buff;
//indirect block variables
int indCurrentBlock;
int indIndex;
int indBlockArrayIndex;
int indPointedBlockId;

		int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
		if(fileFound!=-1){
			if(memInode->type!=FILE_TYPE){
				free(memInode);
				return writtenSize;
			}
			if(offset>memInode->size){
				free(memInode);
				return writtenSize;
			}
			
			while(toBeWritten!=0){
				currentBlock = offsetC/BLOCK_SIZE;
				offsetStart = offsetC%BLOCK_SIZE;
				offsetSize = BLOCK_SIZE-offsetStart;
				if(offsetSize>toBeWritten){
					offsetSize= toBeWritten;
				}
				if(offsetC<maxSize){
			if(currentBlock<16){
				if(memInode->direct_ptr[currentBlock]==-1){
					int blockNum = memSuperBlock->d_start_blk+get_avail_blkno();
					memInode->direct_ptr[currentBlock]=blockNum;
				}
				temp_buff = malloc(BLOCK_SIZE);
				bio_read(memInode->direct_ptr[currentBlock],temp_buff);
				int j = writtenSize;
				for(int i = 0;i<offsetSize;i++){
					*((char *)temp_buff + offsetStart +i) = *((char *)buffer + j);
					j++;
				}
				bio_write(memInode->direct_ptr[currentBlock],temp_buff);
				free(temp_buff);

			}else{
					//Indirect blocks addition
					indCurrentBlock = currentBlock-16;
					indIndex = indCurrentBlock/(BLOCK_SIZE/sizeof(int));
					indBlockArrayIndex = indCurrentBlock%(BLOCK_SIZE/sizeof(int));

					if(memInode->indirect_ptr[indIndex]==-1){
						int blockNum = memSuperBlock->d_start_blk+get_avail_blkno();
						memInode->indirect_ptr[indIndex]=blockNum;
							temp_buff = malloc(BLOCK_SIZE);
						bio_read(memInode->indirect_ptr[indIndex],temp_buff);
						for(int i =0;i<(BLOCK_SIZE/sizeof(int));i++){
							*((int *)temp_buff+i) = -1;
						}
						bio_write(memInode->indirect_ptr[indIndex],temp_buff);
						free(temp_buff);
					}
					temp_buff = malloc(BLOCK_SIZE);
						bio_read(memInode->indirect_ptr[indIndex],temp_buff);
							 indPointedBlockId = *((int *)temp_buff+indBlockArrayIndex);
						if(indPointedBlockId == -1){
							indPointedBlockId = memSuperBlock->d_start_blk+get_avail_blkno();
							*((int *)temp_buff+indBlockArrayIndex) = indPointedBlockId;
								bio_write(memInode->indirect_ptr[indIndex],temp_buff);
						}
						free(temp_buff);
						temp_buff = malloc(BLOCK_SIZE);
				bio_read(indPointedBlockId,temp_buff);
				int j = writtenSize;
				for(int i = 0;i<offsetSize;i++){
					*((char *)temp_buff + offsetStart +i) = *((char *)buffer + j);
					j++;
				}
				bio_write(indPointedBlockId,temp_buff);
				free(temp_buff);

			}
			writtenSize += offsetSize;
			toBeWritten -= offsetSize;
			offsetC += offsetSize;
			
			}else{
			memInode->size +=writtenSize;
			time(&(memInode->vstat.st_atime));       
			time(&(memInode->vstat.st_mtime));         
			time(&(memInode->vstat.st_ctime));    
      memInode->vstat.st_size = memInode->size; 
			writei(memInode->ino,memInode);
				free(memInode);
				return writtenSize;
			}				
		}


			memInode->size +=writtenSize;
			time(&(memInode->vstat.st_atime));       
			time(&(memInode->vstat.st_mtime));         
			time(&(memInode->vstat.st_ctime));    
      memInode->vstat.st_size = memInode->size; 
			writei(memInode->ino,memInode);
			free(memInode);
			return writtenSize;
		}
		free(memInode);
    //return fileFound;
	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return writtenSize;
}

static int rufs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
		char *newPath = strdup(path);
		char * baseName = strdup(basename(newPath));
		char * direName = strdup(dirname(newPath));
	// Step 2: Call get_node_by_path() to get inode of target file
	struct inode *	memInode = malloc(sizeOfInode);
		struct inode *  parentInode;
		int fileFound = 	get_node_by_path(path,rootInode->ino,memInode);
		if(fileFound!=-1){
					if(memInode->type!=FILE_TYPE){
						free(memInode);
						return -1;
					}
					for(int i=0;i<16;i++){
								if(memInode->direct_ptr[i]!=-1){
										void *temp_buff = malloc(BLOCK_SIZE);
										bio_read(memInode->direct_ptr[i],temp_buff);
										memset(temp_buff,0,BLOCK_SIZE);
										bio_write(memInode->direct_ptr[i],temp_buff);
										free(temp_buff);
										clear_block(memInode->direct_ptr[i] - memSuperBlock->d_start_blk);
										memInode->direct_ptr[i] = -1;
								}
						}
						for(int i=0;i<8;i++){
							if(memInode->indirect_ptr[i]!=-1){
								void *ind_buff = malloc(BLOCK_SIZE);
									bio_read(memInode->indirect_ptr[i],ind_buff);
									for(int j =0;j<(BLOCK_SIZE/sizeof(int));j++){
										int indArrayBlock = *((int *)ind_buff + j);
									if(indArrayBlock!=-1){
										void *temp_buff = malloc(BLOCK_SIZE);
										bio_read(indArrayBlock,temp_buff);
										memset(temp_buff,0,BLOCK_SIZE);
										bio_write(indArrayBlock,temp_buff);
										free(temp_buff);
										clear_block(indArrayBlock - memSuperBlock->d_start_blk);
										*((int *)ind_buff + j) = -1;
										}
										}
										memset(ind_buff,0,BLOCK_SIZE);
										bio_write(memInode->indirect_ptr[i],ind_buff);
										free(ind_buff);
										clear_block(memInode->indirect_ptr[i] - memSuperBlock->d_start_blk);
										memInode->indirect_ptr[i] = -1;
						}
						}
						memInode->valid = 0;
						clear_ino(memInode->ino);
						parentInode = malloc(sizeOfInode);
						get_node_by_path(direName,rootInode->ino,parentInode);
						dir_remove(*parentInode,baseName,strlen(baseName));
						free(parentInode);
						free(memInode);
						return 0;

		}
	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory
	free(memInode);
	return fileFound;
}

static int rufs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int rufs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations rufs_ope = {
	.init		= rufs_init,
	.destroy	= rufs_destroy,

	.getattr	= rufs_getattr,
	.readdir	= rufs_readdir,
	.opendir	= rufs_opendir,
	.releasedir	= rufs_releasedir,
	.mkdir		= rufs_mkdir,
	.rmdir		= rufs_rmdir,

	.create		= rufs_create,
	.open		= rufs_open,
	.read 		= rufs_read,
	.write		= rufs_write,
	.unlink		= rufs_unlink,

	.truncate   = rufs_truncate,
	.flush      = rufs_flush,
	.utimens    = rufs_utimens,
	.release	= rufs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &rufs_ope, NULL);

	return fuse_stat;
}

