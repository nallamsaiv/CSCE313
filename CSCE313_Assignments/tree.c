#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>


//recursive function to print directory contents in the tree
void print_tree(const char *dir_name, int depth, int is_last[]);

int main() {
    //array to track if a parent directory is the last in its level
    int is_last[1024] = {0};

    //start by calling the print_tree function at the current directory with depth 0
    print_tree(".", 0, is_last);
    return 0;
}

//function that is used recursively to print the tree like structure
void print_tree(const char *dir_name, int depth, int is_last[]){
    DIR *dir;
    struct dirent *entry;

    //opening the directory
    if((dir = opendir(dir_name)) == NULL){
        perror("opendir");
        return;
    }

    //read entries and count the number of directories and files separately
    struct dirent **entries;
    int n = scandir(dir_name, &entries, NULL, alphasort);
    if(n < 0){
        perror("scandir");
        closedir(dir);
        return;
    }

    int dir_count = 0;
    int file_count = 0;

    //count directories and files to manage the is_last flag correctly
    for(int i = 0; i < n; i++){
        if(strcmp(entries[i]->d_name, ".") != 0 && strcmp(entries[i]->d_name, "..") != 0){
            if(entries[i]->d_type == DT_DIR){
                dir_count++;
            }else{
                file_count++;
            }
        }
    }

    int dir_index = 0;
    int file_index = 0;

    //first for loop the prints all the directories
    for(int i = 0; i < n; i++){
        entry = entries[i];

        //ignoring "." and ".." entries to avoid infinite recursion
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        //check to see if entry is a directory
        if(entry->d_type == DT_DIR){
            //identing based on the current depth of the tree
            for(int j = 0; j < depth; j++){
                if(is_last[j]){
                    printf("    ");
                }else{
                    printf("│   ");
                }
            }

            //checking to see if this is the last directory entry to print the correct symbol
            int current_last = (dir_index == dir_count - 1 && file_count == 0);
            if(current_last){
                printf("└── ");
            }else{
                printf("├── ");
            }
            printf("%s\n", entry->d_name);

            //construct full path for recursive calls
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

            //update the is_last array for recursion
            is_last[depth] = current_last;
            dir_index++;

            //recursive call for subdirectory
            print_tree(path, depth + 1, is_last);
        }
    }

    //second for loop the prints all the files
    for(int i = 0; i < n; i++){
        entry = entries[i];

        //check if entry is a file
        if(entry->d_type != DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
            //identing based on the current depth of the tree
            for(int j = 0; j < depth; j++){
                if(is_last[j]){
                    printf("    ");
                }else{
                    printf("│   ");
                }
            }

            //checking to see if the file is the last file to print the correct symbol
            int is_file_last = (file_index == file_count - 1);
            if(is_file_last){
                printf("└── ");
            }else{
                printf("├── ");
            }
            printf("%s\n", entry->d_name);
            file_index++;
        }
    }

    //free memory for each entry after both passes
    for(int i = 0; i < n; i++){
        free(entries[i]);
    }
    free(entries);
    closedir(dir);//closing the directory
}
