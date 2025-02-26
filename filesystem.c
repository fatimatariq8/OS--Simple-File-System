#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>






/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */


#define FILENAME_MAXLEN 8  // including the NULL char
#define BLOCK_SIZE 1024 //1 kb
#define MAX_BLOCKS 8

/* 
 * inode 
 */

typedef struct inode {
  int  dir;  // boolean value. 1 if it's a directory.
  char name[FILENAME_MAXLEN];
  int  size;  // actual file/directory size in bytes.
  int  blockptrs [8];  // direct pointers to blocks containing file's content.
  int  used;  // boolean value. 1 if the entry is in use.
  int  rsvd;  // reserved for future use
} inode;


/* 
 * directory entry
 */

typedef struct dirent {
  char name[FILENAME_MAXLEN];
  int  namelen;  // length of entry name
  int  inode;  // this entry inode index
} dirent;

typedef struct datablock{
    char data[1024]; //data of each data block
} datablock;


typedef struct superblock {
    int total_blocks;  // number of blocks in the disk
    char *data_block[127];
    int free_block_list[127];  // free block list (0 or 1)
    inode inode_table[16];
    dirent directories[16];
    datablock datablock[127];
} superblock;


/*
 * functions
 */
// create file
// copy file
// remove/delete file
// move a file
// list file info
// create directory
// remove a directory



//saving state
void save_filesystem(superblock *sb) {
    FILE* myfs = fopen("myfs", "wb");
    if (myfs == NULL) {
        perror("Unable to open myfs file for writing");
        exit(EXIT_FAILURE);
    }
    //fwrite(super.inode_table, sizeof(inode), 16, myfs);
    fwrite(sb, sizeof(superblock), 1, myfs);
    fclose(myfs);
}


//checking if directory in path exists. this function gets tokenised directory names instead of the whole path
int directoryExists(const char *dirname, superblock *sb) {
    //printf(dirname);
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].used && sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, dirname)==0) {
            return 1;  //directory already exists
        }
    }
    return 0;  
    //does not exist
}

//printing inode table
void printInodeTable(superblock *sb) {
    printf("Inode Table:\n");
    printf("Index | Used | Type | Name     | Size | Block Pointers\n");
    for (int i = 0; i < 16; i++) {
        printf("%5d | %4d | %4s | %-8s | %4d |", i, sb->inode_table[i].used, (sb->inode_table[i].dir ? "Dir" : "File"), sb->inode_table[i].name, sb->inode_table[i].size);
        for (int j = 0; j < 8; j++) {
            printf(" %d", sb->inode_table[i].blockptrs[j]);
        }
        printf("\n");
    }
}

//CD function
void createDirectory(const char *path, superblock *sb) {
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);

    //tokenising path to extract each directory that is in the path 
    char *token = strtok(path_copy, "/");
    char *dirs[10]; //(this is random. we are assuming there are 10)
    int n_d = 0; //number of directories
    char *parent;
    char *dirname;

    while (token != NULL) {
        dirs[n_d] = token;
        n_d++;
        token = strtok(NULL, "/"); 
        if (n_d>1){
            parent = dirs[n_d-2];
        }
        else {
            parent = "/"; //in this case, parent is root directory 
        }
    }

    //printf(parent);

    //now we will check if the parent directory exists or not
    char current_path[256] = "/";  // start with the root directory
    for (int i = 0; i < n_d - 1; i++) {
        strcat(current_path, dirs[i]);
        if (!directoryExists(dirs[i], sb)) {
            printf("Error: The directory %s in the given path does not exist.\n", dirs[i]);
            return; //returning since path was wrong
        }
        strcat(current_path, "/"); //updating current path
    }

    dirname = dirs[n_d-1]; //the directory we are making
   // printf(dirname);

   //checking if it already exists 
    for (int i = 0; i < 16; i++){
        if ((strcmp(sb->inode_table[i].name, dirname)==0)){
            printf("Error: Directory already exists. \n");
            return ;
        }
    }

    //finding a free inode 
    int free_inode = -1;
    for (int i = 0; i < 16; i++) {
        if (!sb->inode_table[i].used) {
            free_inode = i;
            break;
        }
    }
    
    if (free_inode == -1) {
        printf("No inodes available.");
        return;
    }


    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, parent)==0) {
            int p_ptr=0;
            for (int j = 0; j < 8; j++) {                   
                if (sb->inode_table[i].blockptrs[j]==-1){ //finding free block ptr in parent's block ptrs
                    p_ptr=j;
                    break;
                }
            }
            sb->inode_table[i].blockptrs[p_ptr]=free_inode; //the parent directory will point to this directory 
            sb->free_block_list[free_inode] = 0; 
        
        }
    }

    //updating indoe table
    if (free_inode != -1) {
        sb->inode_table[free_inode].dir = 1;  //marking it as a directory
        strcpy(sb->inode_table[free_inode].name, dirs[n_d-1]);
        sb->inode_table[free_inode].name[FILENAME_MAXLEN-1] = '\0';
        sb->inode_table[free_inode].size = 0;  //it has no files yet so size is 0
        //sb->inode_table[free_inode].blockptrs[0] = free_block;
        sb->inode_table[free_inode].used = 1;
        sb->inode_table[free_inode].rsvd = 0;
        printf("Created directory: %s\n", path);

    } 
    save_filesystem(sb);
}

//updating directory size in case of creation, deletion, moving, and copying
void updateDirectorySizes(const char *path, int fileSize, superblock *sb) {
    //tokenising to extract names
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);

    char *token = strtok(path_copy, "/");
    int count = 0;

    //taversing path and updating sizes for each directory
    while (token != NULL) {
        count++;
        int dirInodeIndex = -1;
        for (int i = 0; i < 16; i++) {
            if (sb->inode_table[i].used && sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, token) == 0) {
                dirInodeIndex = i;
                break;
            }
        }
        if (dirInodeIndex != -1) {
            sb->inode_table[dirInodeIndex].size += fileSize; //updating size
            token = strtok(NULL, "/");
        } else {
            break;
        }
    }
    sb->inode_table[0].size += fileSize; //updating size in the root directory

}

//CR function
void createFile(const char *path, int size, superblock *sb, int moving) { //moving will be 1 if its the case of moving or copying. in this, it will not check if file with same name exists 

    //extractung file name from path 
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    
    char *filename = strrchr(path_copy, '/');
    *filename = '\0';
    filename++;
    char *parent;
    
    //tokenising to get all directories in path and then chekcing if they exist
    char *token = strtok(path_copy, "/");
    char dir_path[strlen(path) + 1];
    dir_path[0] = '\0';
    while (token != NULL) {
        strcat(dir_path, token);
        if (!directoryExists(token, sb)) {
            printf("Error: the directory %s in the given path does not exist.\n", dir_path);
            return;
        }
        // printf(dir_path);
        parent = token;
        strcat(dir_path, "/"); 
        token = strtok(NULL, "/");      
    }
    
    //printf(filename);

    //if the file already exists
    for (int i = 0; i < 16; i++){
        if ((strcmp(sb->inode_table[i].name, filename)==0) && !moving){
            printf("Error: File already exists.\n");
            return ;
        }
    }

    //finding free inode 
    int free_inode_index = -1;
    for (int i = 0; i < 16; i++) {
        if (!sb->inode_table[i].used) {
            free_inode_index = i;
            break;
        }
    }

    //finding free blocks 
    int blocks_needed = (size+BLOCK_SIZE-1)/BLOCK_SIZE;  
    //printf("%d", blocks_needed);
    int free_blocks[blocks_needed];
    int blocks_found = 0;

    
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, parent)==0) { //finding parent inode and updating its block ptrs to point to file
            int p_ptr=0;
            for (int j = 0; j < 8; j++) {                   
                if (sb->inode_table[i].blockptrs[j]==-1){
                    p_ptr=j;
                    break;
                }
            }
            
            
            sb->inode_table[i].blockptrs[p_ptr]=free_inode_index;
        }
    }

    for (int i = 0; i < 127 && blocks_found < blocks_needed; i++) {
        if (sb->free_block_list[i]==1) {
            free_blocks[blocks_found] = i;
            // sb->free_block_list[i] = 0;
            blocks_found++;
        }
    }

    //printf("xxx");
    //printf("%d",blocks_found);
    if (blocks_found<blocks_needed || blocks_needed > 8)  {
        printf("Error: not enough blocks are available.\n");
        return;
    }

    //updating inode rable
    if (free_inode_index != -1) {
        sb->inode_table[free_inode_index].dir = 0;  //0 because it is a file 
        strncpy(sb->inode_table[free_inode_index].name, filename, FILENAME_MAXLEN - 1);
        sb->inode_table[free_inode_index].name[FILENAME_MAXLEN - 1] = '\0';
        sb->inode_table[free_inode_index].size = size;


        for (int i = 0; i < blocks_needed; i++) { //traversing through the blocks available and needed for this file
           // printf("%d", free_blocks[i]);
            sb->inode_table[free_inode_index].blockptrs[i] = free_blocks[i]; 
            int x = free_blocks[i];
            sb->free_block_list[x] = 0;
            char content[BLOCK_SIZE];
            for (int w = 0; w < BLOCK_SIZE; w++) { //filling file with random characters from a-z
                    content[w] = 'a' + (w % 26);
                }
            sb->data_block[x] = (char*) malloc(sizeof(char)*1024);
            strcpy(sb->data_block[x], content);
            //strcpy(sb->datablocks->block[free_blocks[i]], content);
        }
        sb->inode_table[free_inode_index].used = 1;
        sb->inode_table[free_inode_index].rsvd = 0;


        char path_copy1[strlen(path) + 1];
        strcpy(path_copy1, path);
        updateDirectorySizes(path_copy1, size, sb);

        printf("Created file: %s of size %d\n", path, size);
    } else {
        printf("Error: Unable to create file. Inode table is full.\n");
    }
    save_filesystem(sb);
}


//DL function
void deleteFile(const char *path, superblock *sb) {
    //extracting path and filenme 
    char path_copy[strlen(path)+1];
    strcpy(path_copy, path);
    char *filename = strrchr(path_copy, '/');
    if (filename == NULL) {
        printf("incorrect path.\n");
        return;
    }
    *filename = '\0';
    filename++;
    char *parent;
    
    //tokenising to get all directories in path and then chekcing if they exist
    char *token = strtok(path_copy, "/");
    char dir_path[strlen(path) + 1];
    dir_path[0] = '\0';
    while (token != NULL) {
        strcat(dir_path, token);
        if (!directoryExists(token, sb)) {
            printf("Error: the directory %s in the given path does not exist.\n", dir_path);
            return;
        }
        // printf(dir_path);
        parent = token;
        strcat(dir_path, "/"); 
        token = strtok(NULL, "/");
    }


    int file_inode_index = -1; //file inode index

    //finding file inode index
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].used && !sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, filename) == 0) {
            file_inode_index = i;
            break;
        }
    }
    //chekcing if it exists 
    if (file_inode_index == -1) {
        printf("Error: file %s does not exist.\n", filename);
        return;
    }

    //setting parents block ptr to -1 since we are deleting the file 
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, parent)==0) {
            int p_ptr=0;
            for (int j = 0; j < 8; j++) {                   
                if (sb->inode_table[i].blockptrs[j]==file_inode_index){ //parents block ptr pointing to file
                    p_ptr=j;
                    break;
                }
            }
            //printf("%d", p_ptr);
            sb->inode_table[i].blockptrs[p_ptr]=-1;
        }
    }

    //setting blocks previously occupied by this file to -1
    for (int i = 0; i < 8; i++) {
        int block_index = sb->inode_table[file_inode_index].blockptrs[i];
        if (block_index != -1) {
            sb->free_block_list[block_index] = 1;
            sb->inode_table[file_inode_index].blockptrs[i] = -1;  // Remove block pointers
        }
    }

    int file_size = -sb->inode_table[file_inode_index].size;
    updateDirectorySizes(path, file_size, sb); //reducing dir size

    //updating inode table 
    sb->inode_table[file_inode_index].used = 0;
    sb->inode_table[file_inode_index].dir = 0;
    sb->inode_table[file_inode_index].size = 0;
    sb->inode_table[file_inode_index].name[0] = '\0';

    printf("Deleted file: %s\n", filename);
    save_filesystem(sb);
}

//MV function 
void moveFile(const char *srcpath, const char *dstpath, superblock *sb) {
    //tokenising to get source filename
    char src_copy[strlen(srcpath) + 1];
    strcpy(src_copy, srcpath);

    char *src_filename = strrchr(src_copy, '/');
    if (src_filename == NULL) {
        printf("Error: Invalid source file path.\n");
        return;
    }
    *src_filename = '\0';
    src_filename++;

    //tokenising to get destination filename
    char dst_copy[strlen(dstpath) + 1];
    strcpy(dst_copy, dstpath);
    char *dst_filename = strrchr(dst_copy, '/');
    if (dst_filename == NULL) {
        printf("Error: Invalid source file path.\n");
        return;
    }
    *dst_filename = '\0';
    dst_filename++;

    //tokenign to get source path and dest path then we will check if these directories exist
    char *token = strtok(src_copy, "/");
    char dir_path[strlen(srcpath) + 1];
    dir_path[0] = '\0';
    while (token != NULL) {
        strcat(dir_path, token);
        if (!directoryExists(token, sb)) {
            printf("Error: the source directory %s in the given path does not exist.\n", dir_path);
            return;
        }
        // printf(dir_path);
        strcat(dir_path, "/");  
        token = strtok(NULL, "/");
    }
    
    // char dst_copy[strlen(dstpath) + 1];
    // strcpy(dst_copy, dstpath);
    char *token1 = strtok(dst_copy, "/"); 
    char dir_path1[strlen(dstpath) + 1];
    dir_path1[0] = '\0';
    while (token1 != NULL) {
        strcat(dir_path1, token1);
        if (!directoryExists(token1, sb)) {
            printf("Error: the destination directory %s in the given path does not exist.\n", dir_path1);
            return;
        }
        // printf(dir_path);
        strcat(dir_path1, "/");  
        token1 = strtok(NULL, "/");
    }

    int src_inode_index = -1; //source file inode index

    //traversing inode table to get source file inode index
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].used && !sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, src_filename) == 0) {
            src_inode_index = i;
            break;
        }
    }

    if (src_inode_index==-1){
        printf("Error: Source file does not exist");
        return;
    }
    //setting size of dest file same as the size of source file
    int dst_file_size = sb->inode_table[src_inode_index].size;

    //create dest file and deleting source
    deleteFile(srcpath, sb);
    createFile(dstpath, dst_file_size, sb, 1);
    
    printf("Moved file from %s to %s.\n", srcpath, dstpath);
    save_filesystem(sb);
}

//CP function
void copyFile(const char *srcpath, const char *dstpath, superblock *sb) {
    //tokenising to get source and dest filename 
    char src_copy[strlen(srcpath) + 1];
    strcpy(src_copy, srcpath);
    char *src_filename = strrchr(src_copy, '/');
    if (src_filename == NULL) {
        printf("Error: Invalid source file path.\n");
        return;
    }
    *src_filename = '\0';
    src_filename++;

    char dst_copy[strlen(dstpath) + 1];
    strcpy(dst_copy, dstpath);
    char *dst_filename = strrchr(dst_copy, '/');
    if (dst_filename == NULL) {
        printf("Error: Invalid source file path.\n");
        return;
    }
    *dst_filename = '\0';
    dst_filename++;

    //tokening to get source path and dest path then we will check if these directories exist
    char *token = strtok(src_copy, "/");
    char dir_path[strlen(srcpath) + 1];
    dir_path[0] = '\0';
    while (token != NULL) {
        strcat(dir_path, token);
        if (!directoryExists(token, sb)) {
            printf("Error: the source directory %s in the given path does not exist.\n", dir_path);
            return;
        }
        // printf(dir_path);
        strcat(dir_path, "/");  
        token = strtok(NULL, "/");
    }
    
    
    char *token1 = strtok(dst_copy, "/"); 
    char dir_path1[strlen(dstpath) + 1];
    dir_path1[0] = '\0';
    while (token1 != NULL) {
        strcat(dir_path1, token1);
        if (!directoryExists(token1, sb)) {
            printf("Error: the destination directory %s in the given path does not exist.\n", dir_path1);
            return;
        }
        // printf(dir_path);
        strcat(dir_path1, "/");  
        token1 = strtok(NULL, "/");
    }

    int src_inode_index = -1; //source file inode index

    //traversing inode table to get source file inode index
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].used && !sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, src_filename) == 0) {
            src_inode_index = i;
            break;
        }

    }
    //setting size of dest file same as the size of source file
    int dst_file_size = sb->inode_table[src_inode_index].size;

    //create dest file 
    createFile(dstpath, dst_file_size, sb, 1);
    printf("Created a copy of %s in %s.\n", srcpath, dstpath);
    save_filesystem(sb);

}


//DD function
void removeDirectory(const char *path, superblock *sb) {
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    
    //extracting directory name 
    char *dirname = strrchr(path_copy, '/');
    if (dirname == NULL) {
        printf("Error: Incorrect path.\n");
        return;
    }
    *dirname = '\0';
    dirname++;
    char* parent;

    //tookenizing to get directories in path and checking if they exist
    char *token = strtok(path_copy, "/");
    char dir_path[strlen(path) + 1];
    dir_path[0] = '\0';

    while (token != NULL) {
        strcat(dir_path, token);
        if (!directoryExists(token, sb)) {
            printf("Error: The directory %s in the given path does not exist.\n", dir_path);
            return;
        }
        parent = token;
        strcat(dir_path, "/");
        token = strtok(NULL, "/");
    }

    //traversing to get the inode index of the directory we want to delete 
    int inode_index = -1;
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].used && sb->inode_table[i].dir && strcmp(sb->inode_table[i].name, dirname) == 0) {
            inode_index = i;
            break;
        }
    }

    //if the directory doesnt exist:
    if (inode_index == -1) {
        printf("Error: The directory %s does not exist.\n", dirname);
        return;
    }

    //here, we will be RECURSIVELY deleting all the content (subdiectories and files)
    for (int i = 0; i < 8; i++) {
        int block_index = sb->inode_table[inode_index].blockptrs[i];  //each block ptr of the dir becomes our inode index
        if (block_index != -1) {
            if (sb->inode_table[block_index].dir){ // block is a dir so we do recursion here 
                char subdir[256]; //path of the subdirectory
                sprintf(subdir, "%s/%s", path, sb->inode_table[block_index].name); //this part was taken from the internet.
                removeDirectory(subdir, sb); 
            } else{ //the block is a file, we will simply delete it 
                char file_path[256];
                sprintf(file_path, "%s/%s", path, sb->inode_table[block_index].name);
                printf(file_path);
                deleteFile(file_path, sb);
            }
        }
    }
    //now we will update the parent dir's block ptrs and size (block ptr will be -1)
    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].dir && strcmp(sb->inode_table[i].name,parent) == 0){
            for (int j = 0; j < 8; j++) {
                if (sb->inode_table[i].blockptrs[j]==inode_index){
                    sb->inode_table[i].blockptrs[j]=-1;
                    break; }
            }
            sb->inode_table[i].size -= sb->inode_table[inode_index].size;
            sb->inode_table[0].size -= sb->inode_table[inode_index].size; //updating root directory's size 
        }
    }

    //update the inode table and block ptrs and the free blocks list
    for (int i = 0; i < 8; i++) {
        int block_index = sb->inode_table[inode_index].blockptrs[i];
        if (block_index != -1) {
            //printf("ssss");
            sb->free_block_list[block_index] = 1;
            sb->inode_table[inode_index].blockptrs[i] = -1;
        }
    }
    sb->inode_table[inode_index].used = 0;
    sb->inode_table[inode_index].dir = 0;
    sb->inode_table[inode_index].size = 0;
    sb->inode_table[inode_index].name[0] = '\0';

    printf("Deleted directory: %s\n", dirname);
    save_filesystem(sb);
}



//LL function
void listFiles(superblock *sb) {
    printf("Files and Directories:\n");

    for (int i = 0; i < 16; i++) {
        if (sb->inode_table[i].used) {
            printf("- %s\t%d bytes\n", sb->inode_table[i].name, sb->inode_table[i].size);
        }
    }
}

//initialising state 
void initialise_State(superblock *sb){
    for (int i = 0; i < 128 ;i++) // Free Block Initialize
    {
        //printf("xx");
        sb->free_block_list[i] = 1;
    }
    for (int i = 0; i < 16; i++){
        sb->directories[i].inode=-1;//all inodes initializes as -1
    }

    for (int i = 0; i < 16;i++)  // Inode Table Initialize
    {
        strcpy(sb->inode_table[i].name,"");
        sb->inode_table[i].used = 0;
        sb->inode_table[i].rsvd = 0;
        sb->inode_table[i].size = 0;
        sb->inode_table[i].dir = 0;
        for (int j = 0; j < 8;j++){
            sb->inode_table[i].blockptrs[j] = -1;
        }
    }
    //root directory
    sb->directories[0].inode=0;
    strcpy(sb->directories[0].name,"/");
    sb->directories[0].namelen=1;
    sb->inode_table[0].used=1;
    sb->inode_table[0].dir=1;
    sb->inode_table[0].size=0;
    strcpy(sb->inode_table[0].name,"/") ;//null root
    sb->free_block_list[0] = 0;

    //printf("xx");
}

int main (int argc, char* argv[]) {
    superblock sb;

    FILE *myfs = fopen("myfs", "r");
    
    if (myfs == NULL) {
        //printf("xxxxx");
        initialise_State(&sb);
    }

    fread(&sb, sizeof(superblock), 1, myfs);
    fclose(myfs);

    
    //for testing
    // createDirectory("/home", &sb);
    
    // createDirectory("/home/user", &sb);
    // //printf("xxx");
    // //createDirectory("/home/user", &sb);
    // //createDirectory("/home/user/user2", &sb);
    // //createDirectory("/home/user/user3", &sb);
    // createFile("/home/file1", 128, &sb, 0);
    // createFile("/home/user/file2", 8091, &sb, 0);
    // //deleteFile("home/user/file1", &sb);
    // //moveFile("/home/user/user2/file", "/home/user/newfile", &sb);
    // copyFile("/home/file1", "/home/user/file3", &sb);
    // deleteFile("/home/file1", &sb);
    // createDirectory("/home/user2", &sb);
    // moveFile("/home/user/file2", "/home/user2/file5", &sb);
    // //createFile("/home/user2/file67", 678, &sb, 0);
    // //printInodeTable(&sb);
    // // for (int i=0;i<127;i++){
    // //     printf("%d",sb.free_block_list[i]);
    // // }
    // //removeDirectory("/home/user", &sb);
    // listFiles(&sb);
    // //deleteFile("home/user/user2/file", &sb);
    // //save_filesystem(&sb);
    // printInodeTable(&sb);

    
    FILE *inputFile = fopen(argv[1], "r");
    char line[100]; 
    while (fgets(line, sizeof(line), inputFile)) {
        //tokenising
        char *token = strtok(line, " \n");
        if (token == NULL) {
            continue;
        }

        //handling each command type
        if (strcmp(token, "CD") == 0) {
            // CD command
            char *dirname = strtok(NULL, " \n");
            createDirectory(dirname, &sb);

        } else if (strcmp(token, "CR") == 0) {
            // CR command
            char *filename = strtok(NULL, " ");
            // printf(filename);
            int size = atoi(strtok(NULL, " \n"));
            createFile(filename, size, &sb, 0);
        } else if (strcmp(token, "LL") == 0) {
            listFiles(&sb);

        } else if (strcmp(token, "DL") == 0) {
            char *filename = strtok(NULL, " \n");
            //printf(filename);
            deleteFile(filename, &sb);
        } else if (strcmp(token, "MV") == 0) {
            const char *srcpath = strtok(NULL, " ");
            const char *dstpath = strtok(NULL, " \n");
            moveFile(srcpath, dstpath, &sb);
        } else if (strcmp(token, "CP") == 0) {
            const char *srcpath = strtok(NULL, " ");
            const char *dstpath = strtok(NULL, " \n");
            copyFile(srcpath, dstpath, &sb);
        } else if (strcmp(token, "DD") == 0) {
            char *dirname = strtok(NULL, " \n");
            removeDirectory(dirname, &sb);
        }
    }
    fclose(inputFile);
    for (int i = 0; i < 127; i++){
        if (sb.free_block_list[i]==0){
            free(sb.data_block[i]);
        }
    }

    save_filesystem(&sb);


	return 0;
}
