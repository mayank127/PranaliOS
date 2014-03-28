#include <m2skernel.h>
#include <string.h>

// update time stamps for file or directory
void update_time_stamps(struct FCB * file, int mask) {
	switch(mask) {
		case 1: // update all times
			file->creation_time = time(NULL);
			file->last_modified_time = file->creation_time;
			file->last_seen_time = file->creation_time;
			break;
		case 2: // update modification time and seen time
			file->last_modified_time = file->creation_time;
			file->last_seen_time = file->creation_time;
			break;
		case 3: // update seen time
			file->last_seen_time = file->creation_time;
			break;
	}
}

// search for file name in directory
struct FCB * search_in_directory(struct FCB * directory, char * name) {
	int number_of_entries = directory->size / sizeof(struct FCB);
	int i = 0;
	for (i=0;i<number_of_entries;i++) {
		struct FCB * temp = (struct FCB *) malloc (sizeof(struct FCB));
		seek_file(directory,i*sizeof(struct FCB));
		*temp = read_file(directory, sizeof(struct FCB))
		if (strcmp(name,temp->name)==0) {
			return temp;
		}
	}
	return NULL;
}

// search file
struct FCB * search_file_or_directory(char * path) {
	if (strcmp(path,"root")==0)
		return get_root();
	else {
		char * name;
		name = strtok(path,"/");
		struct FCB * temp = search_file_or_directory(name);
		if (temp==NULL) {
			// error root not found
		}
		strtok(NULL,"/");
		while (name!=NULL) {
			if (temp->type==1) {
				// error temp is not a directory
			}
			temp = search_in_directory(temp,name);
			if (temp==NULL) {
				// error couldn't find file
			}
			name = strtok(NULL,"/");
		}
		return temp;
	}
}

struct FCB * get_parent_directory(char * path) {
	if(strcmp(path,"root")==0) {
		// error no parent exists for root directory
	}
	char * name;
	name = strtok(path,"/");
	struct FCB * parent;
	struct FCB * temp = search_file_or_directory(name);
	if (temp==NULL) {
		// error root not found
	}
	name = strtok(NULL,"/");
	while (name!=NULL) {
		if (temp->type==1) {
			// error temp is not a directory
		}
		struct FCB * cur_dir = search_in_directory(temp,name);
		if (temp==NULL) {
			// error couldn't find file
		}
		parent = temp;
		temp = cur_dir;
		name = strtok(NULL,"/");
	}
	return parent;
}

// creates the root directory FCB
struct FCB * create_root_FCB() {
	/*
		index = 0 for root directory
		uid = 0 for root directory [created by PranaliOS]
	*/
	struct FCB * root = (struct FCB *) malloc(sizeof(struct FCB));
	strcmp(root->name,"root");
	root->index = 0;
	root->file_size = 0;
	update_time_stamps(root,1);
	root->uid = 0;
	root->type = 0;
	root->seek_block = 0;
	root->seek_block_addr = root->address_space[0];
	root->seek_offset = 0;
}

// create a new file/directory FCB and add to its parent directory
void create_file(char * path, char * name, int type) {
	/*
		type = 0 :- directory creation
		type = 1 :- file creation
	*/
	struct FCB * cur_directory = search_file_or_directory(path);
	struct FCB * new_file = (struct FCB *) (malloc(sizeof(struct FCB)));
	// new_file->index = get_free_FCB_index();
	new_file->file_size = 0;
	strcmp(new_file->name,name);
	update_time_stamps(new_file,1);
	// new_file->uid = get_uid();
	new_file->type = type;
	new_file->seek_block = 0;
	new_file->seek_block_addr = new_file->address_space[0];
	new_file->seek_offset = 0;
	seek_file(cur_directory, cur_directory->file_size);
	write_file(cur_directory,sizeof(struct FCB),(char *)(new_file));
	cur_directory->file_size += sizeof(struct FCB);
	update_time_stamps(cur_directory,2);
}

void delete_file(char * path) {
	struct FCB * file = search_file_or_directory(path);
	struct FCB * directory = get_parent_directory(path);
	if (directory == NULL) {
		// error file not found
	}
	if (directory->type != 1){
		// error given path points to a file
	}
	int number_of_entries = directory->size()/sizeof(struct FCB);
	int i;
	struct FCB_list cur_list;
	cur_list.cur = read(directory, 0, sizeof(struct FCB));
	FCB_list * prev = &cur_list;
	for (i=1; i<number_of_entries; i++){
		struct FCB_list * next = malloc(sizeof(struct * FCB_list));
		next->cur = read(directory, i*sizeof(struct FCB), sizeof(struct FCB));
		prev->next = next;
		prev = next;
	}
	prev.next = NULL;
	truncate_file(directory);
	int j=0;
	struct FCB_list * itr = cur_list;
	while(itr->next != NULL){
		if (strcmp(itr->cur->name, file->name) != 0){
			seek_file(directory, directory->size);
			write_file(directory,sizeof(struct FCB),(char *)(itr->cur));
			directory->size += sizeof(struct FCB);
		}
	}
}

// remove a directory (recursively deleting all its contents)
void remove_directory(char * path) {
	struct FCB * cur_directory = search_file_or_directory(path);
	if (cur_directory->type==0) {
		// now first recursively delete all the contents
		int i = 0;
		for (i=0;i<number_of_entries;i++) {
			struct FCB * temp = (struct FCB *) malloc (sizeof(struct FCB));
			seek_file(directory, i*sizeof(struct FCB));
			temp = (struct FCB *)read_file(directory, sizeof(struct FCB))
			remove_directory(temp);
		}
	}
	// now delete this FCB (can be a directory or file) [base case]
	delete_file(path);
}
