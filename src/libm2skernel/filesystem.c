#include <m2skernel.h>
#include <string.h>

SuperBlock super_block;


void * read_block(int file_block_number){
	void * read_buffer ;
	size_t number_of_byte_read;
	read_buffer = calloc(1, super_block.block_size) ;
	if(file_block_number >= total_blocks_virtual_mem){
		fatal("read_block : block number out of range");
	}
	fseek(disk_file_pointer, file_block_number*4096 , SEEK_SET) ;
	number_of_byte_read = fread(read_buffer, 1, super_block.block_size, disk_file_pointer);

	if (number_of_byte_read != super_block.block_size) {
		fatal("read error");
	}
	return read_buffer ;
}

void write_block(int file_block_number, void * buffer){
	if(file_block_number >= total_blocks_virtual_mem){
		fatal("read_block : block number out of range");
	}
	fseek(disk_file_pointer, file_block_number*4096, SEEK_SET);
	fwrite(buffer, 1, super_block.block_size, disk_file_pointer);
}

void init_super_block(){
	super_block.number_of_blocks = total_blocks_virtual_mem;
	super_block.block_size = 4096;
	super_block.blocks_in_track = blocks_in_track;
	super_block.FCB_root = * (create_root_FCB());

	super_block.highest_FCB_index_used = 0;

	super_block.free_block_pointer = NULL;
	super_block.free_FCB_list_head = NULL;

	int size = sizeof(super_block);
	size += 8;
	size += total_blocks_virtual_mem * 4;
	size += 1024 * 4;			// 1024 is number of FCBs
	size += total_blocks_virtual_mem * sizeof(int);

	super_block.size = size/4096 + 1;
	super_block.free_block_count = super_block.size;
	write_super_block();
}

void read_super_block(){
	super_block = * ((SuperBlock*) (malloc(sizeof(SuperBlock))));
	fseek(disk_file_pointer, 0, SEEK_SET);
	fread(&super_block, sizeof(super_block), 1, disk_file_pointer);


	//Free block list read
	int size = 0;
	fread(&size, sizeof(int), 1, disk_file_pointer);
	int i = 0;
	int_list* prev = NULL;
	for(i=0;i<size;i++){
		int_list * temp = (int_list*) (malloc(sizeof(int_list)));
		fread(&(temp->num), sizeof(int), 1, disk_file_pointer);
		if(prev == NULL){
			super_block.free_block_pointer = temp;
		}
		else{
			prev->next = temp;
		}
	}

	//free FCB list read
	fread(&size, sizeof(int), 1, disk_file_pointer);
	prev = NULL;
	for(i=0;i<size;i++){
		int_list * temp = (int_list*) (malloc(sizeof(int_list)));
		fread(&(temp->num), sizeof(int), 1, disk_file_pointer);
		if(prev == NULL){
			super_block.free_FCB_list_head = temp;
		}
		else{
			prev->next = temp;
		}
	}

	//disk_block_data read
	total_blocks_virtual_mem = super_block.number_of_blocks;
	blocks_in_track = super_block.blocks_in_track;
	prev_track = 0;
	disk_block_data = (int *) malloc(total_blocks_virtual_mem * sizeof(int));
	fread(disk_block_data, sizeof(total_blocks_virtual_mem * sizeof(int)), 1, disk_file_pointer);
}

void write_super_block(){
	fseek(disk_file_pointer, 0, SEEK_SET);
	fwrite(&super_block, sizeof(super_block), 1, disk_file_pointer);


	//free_block_pointer write
	int size = list_size(super_block.free_block_pointer);
	fwrite(&size, sizeof(int), 1, disk_file_pointer);
	int_list* cur = super_block.free_block_pointer;
	int i = 0;
	for(i=0;i<size;i++){
		fwrite(&(cur->num), sizeof(int), 1, disk_file_pointer);
		cur = cur->next;
	}


	//free_FCB_list_write
	size = list_size(super_block.free_FCB_list_head);
	fwrite(&size, sizeof(int), 1, disk_file_pointer);
	cur = super_block.free_block_pointer;
	for(i=0;i<size;i++){
		fwrite(&(cur->num), sizeof(int), 1, disk_file_pointer);
		cur = cur->next;
	}

	//disk_block_data write
	fwrite(disk_block_data, sizeof(total_blocks_virtual_mem * sizeof(int)), 1, disk_file_pointer);
}

int list_size(int_list* l){
	int size = 0;
	while(l != NULL){
		l = l->next;
		size++;
	}
	return size;
}

int get_free_FCB_index(){
	if(super_block.free_FCB_list_head != NULL){
		int num = super_block.free_FCB_list_head->num;
		super_block.free_FCB_list_head = super_block.free_FCB_list_head->next;
		return num;
	}
	else{
		int num = super_block.highest_FCB_index_used;
		super_block.highest_FCB_index_used+=1;
		return num;
	}
}

int get_free_block(){
	if(super_block.free_block_pointer != NULL){
		int num = super_block.free_block_pointer->num;
		super_block.free_block_pointer = super_block.free_block_pointer->next;
		return num;
	}
	else{
		int num = super_block.free_block_count;
		super_block.free_block_count+=1;
		return num;
	}
}

void add_free_block(int block_num){
	int_list * temp = (int_list*) (malloc(sizeof(int_list)));
	temp->num = block_num;
	temp->next = super_block.free_block_pointer;
	super_block.free_block_pointer = temp;
}

void add_free_FCB(int fcb){
	int_list * temp = (int_list*) (malloc(sizeof(int_list)));
	temp->num = fcb;
	temp->next = super_block.free_FCB_list_head;
	super_block.free_FCB_list_head = temp;
}

// update time stamps for file or directory
void update_time_stamps(struct FCB * file, int mask) {
	switch(mask) {
		case 1: // update all times
			file->creation_time = time(NULL);
			file->last_modified_time = file->creation_time;
			file->last_seen_time = file->creation_time;
			break;
		case 2: // update modification time and seen time
			file->last_modified_time = time(NULL);
			file->last_seen_time = file->last_modified_time;
			break;
		case 3: // update seen time
			file->last_seen_time = time(NULL);
			break;
	}
}

// search for file name in directory
struct FCB * search_in_directory(struct FCB * directory, char * name, int uid) {
	if (directory->uid!=uid && directory->permission==0) {
		printf("Search in directory : Not enough permission\n");
		return NULL;
	}
	update_time_stamps(directory, 3);
	int number_of_entries = directory->file_size / sizeof(struct FCB);
	printf("%s %s, %d %d\n", name, directory->path, directory->file_size, number_of_entries);
	int i = 0;
	for (i=0;i<number_of_entries;i++) {
		struct FCB * temp = (struct FCB *) malloc (sizeof(struct FCB));
		seek_file(directory,i*sizeof(struct FCB));
		temp = (FCB *) read_file(directory, sizeof(struct FCB));
		printf("%s == %s, %s, %s\n", name, temp->name, temp->path, directory->path);
		if (strcmp(name,temp->name)==0) {
			return temp;
		}
	}
	return NULL;
}

// search file
struct FCB * search_file_or_directory(char * p, int uid) {
	char path[500];
	strcpy(path, p);
	if (strcmp(path,"root")==0){
		printf("\nsearching\n");
		update_time_stamps(super_block.FCB_root, 3);
		return &(super_block.FCB_root);
	}
	else {
		char * name;
		name = strtok(path,"/");
		struct FCB * temp = search_file_or_directory(name, uid);
		if (temp==NULL) {
			printf("Search file or directory : File not found\n");
			return NULL;
		}
		else if (temp->uid!=uid && temp->permission==0) {
			printf("Search file or directory : Not enough permission\n");
			return NULL;
		}
		name = strtok(NULL,"/");
		while (name!=NULL) {
			printf("%s - Searching for\n", name);
			if (temp == NULL || temp->type==1) {
				// error temp is not a directory
			printf("bb\n");
				return NULL;
			}
			if (temp)
			temp = search_in_directory(temp, name, uid);
			if (temp==NULL) {
				// error couldn't find file
				printf("cc\n");
				return NULL;
			}
			name = strtok(NULL,"/");
		}
		return temp;
	}
}

struct FCB * get_parent_directory(char * p, int uid) {
	char path[500];
	strcpy(path, p);
	if(strcmp(path,"root")==0) {
		printf("Get parent directory : No parent for root\n");
		return NULL;
	}
	char * name;
	name = strtok(path,"/");
	struct FCB * parent = NULL;
	struct FCB * temp = search_file_or_directory(name, uid);
	if (temp==NULL) {
		printf("Search file or directory : File not found\n");
		return NULL;
	}
	name = strtok(NULL,"/");
	while (name!=NULL) {
		if (temp->type==1) {
			printf("Search file or directory : File not found\n");
			return NULL;
		}
		struct FCB * cur_dir = search_in_directory(temp, name, uid);
		if (cur_dir==NULL) {
			printf("Search file or directory : File not found\n");
			return NULL;
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
	strcpy(root->name,"root");
	root->index = 0;
	root->file_size = 0;
	update_time_stamps(root,1);
	root->uid = 0;
	root->permission = 2;
	root->type = 0;
	root->seek_block = 0;
	root->seek_block_addr = root->block_address[0];
	root->seek_offset = 0;
	root->location = 0;
	strcpy(root->path, "root");
	return root;
}

// create a new file/directory FCB and add to its parent directory
FCB * create_file(char * p, int type, int uid) {
	/*
		type = 0 :- directory creation
		type = 1 :- file creation
	*/
	int i = 0;
	int last = 0;
	for(i = 0; i < strlen(p); i++){
		if(p[i] == '/')
			last = i;
	}
	char name[50]; // name of new file
	char path[300]; // current directory
	strcpy(name, p+last+1);
	for(i=last+1; i<strlen(p); i++){
		name[i-last-1] = p[i];
	}
	name[i] = 0;
	for(i = 0; i < last; i++){
		path[i] = p[i];
	}
	path[i] = 0;
	printf("%s %s %s %d\n", p, path, name, last);
	struct FCB * cur_directory = search_file_or_directory(path, uid);
	if (file->uid!=uid && file->permission!=2) {
		printf("Create file : Not enough permission\n");
		return NULL;
	}
	else {
		struct FCB * new_file = (struct FCB *) (malloc(sizeof(struct FCB)));
		new_file->index = get_free_FCB_index();
		new_file->file_size = 0;
		strcpy(new_file->name,name);
		update_time_stamps(new_file,1);
		new_file->uid = uid;
		new_file->type = type;
		new_file->seek_block = 0;
		new_file->seek_block_addr = new_file->block_address[0];
		new_file->seek_offset = 0;
		new_file->location = cur_directory->file_size;
		strcpy(new_file->path, path);
		strcat(new_file->path, "/");
		strcat(new_file->path, name);

		update_time_stamps(cur_directory,2);
		seek_file(cur_directory, cur_directory->file_size);
		write_file(cur_directory,sizeof(struct FCB),(char *)(new_file));
		//cur_directory->file_size += sizeof(struct FCB);
		printf("%s %d AFTER CREATION\n", cur_directory->path, cur_directory->file_size);
		update_directory_trace(cur_directory);
		return new_file;
	}
}

void delete_file(char * p, int uid) {
	char path[500];
	strcpy(path, p);
	struct FCB * file = search_file_or_directory(path, uid);
	struct FCB * directory = get_parent_directory(path, uid);
	if (directory == NULL) {
		printf("Search Directory : Search failed\n");
		return;
	}
	else if (directory->type != 1){
		printf("Search Directory : Parent directory is not a directory\n");
		return;
	}
	else if (file->uid!=uid && file->permission!=2) {
		printf("File Deletion : Not enough permission\n");
		return;
	}
	else {
		update_time_stamps(directory, 2);
		int number_of_entries = directory->file_size/sizeof(struct FCB);
		int i;
		struct FCB_list cur_list;
		cur_list.cur = (FCB *) read_file(directory, sizeof(struct FCB));
		FCB_list * prev = &cur_list;
		for (i=1; i<number_of_entries; i++){
			FCB_list * next = malloc(sizeof(struct FCB_list));
			next->cur = (FCB *) read_file(directory, sizeof(struct FCB));
			prev->next = next;
			prev = next;
		}
		prev->next = NULL;
		truncate_file(directory);
		struct FCB_list * itr = &cur_list;
		while(itr->next != NULL){
			if (strcmp(itr->cur->name, file->name) != 0){
				itr->cur->location = directory->file_size;
				seek_file(directory, directory->file_size);
				write_file(directory,sizeof(struct FCB),(char *)(itr->cur));
			}
		}
		truncate_file(file);
		add_free_FCB(file->index);
		printf("File deleted successfully : %s\n",file->path);
	}
}

// remove a directory (recursively deleting all its contents)
void remove_directory(char * p, int uid) {
	char path[500];
	strcpy(path, p);
	struct FCB * cur_directory = search_file_or_directory(path, uid);
	if (cur_directory!=NULL && cur_directory->type==0) {
		// now first recursively delete all the contents
		int number_of_entries = cur_directory->file_size/sizeof(struct FCB);
		int i = 0;
		int flag = 1;
		for (i=0;i<number_of_entries;i++) {
			struct FCB * temp = (struct FCB *) malloc (sizeof(struct FCB));
			seek_file(cur_directory, i*sizeof(struct FCB));
			temp = (struct FCB *)read_file(cur_directory, sizeof(struct FCB));
			if (temp->uid==uid || temp->permission==2) {
				remove_directory(temp->path, uid);
			}
			else {
				flag = 0;
			}
		}
		update_time_stamps(cur_directory, 2);
	}
	// now delete this FCB if allowed (can be a directory or file) [base case]
	if (flag) delete_file(path, uid);
}

char * read_file(FCB * file_fcb, int size){
	update_time_stamps(file_fcb, 3);
	int start_block = file_fcb->seek_block;
	int byte_offset = file_fcb->seek_offset;
	uint32_t block_address = file_fcb->seek_block_addr;

	char * read_buff;
	char * return_buff;
	int j = 0;

	return_buff = malloc(size);

	if(start_block*super_block.block_size + byte_offset + size <= file_fcb->file_size){
		int num_blocks = (byte_offset + size)/super_block.block_size;

		if(num_blocks == 0){
			read_buff = read_block_from_cache(block_address);
			int i = 0;
			for(i = 0; i < size; i++){
				return_buff[j] = read_buff[byte_offset + i];
				j++;
			}
			byte_offset = byte_offset + size;
			file_fcb->seek_offset = byte_offset;
		}
		else{
			read_buff = read_block_from_cache(block_address);
			int i = 0;
			for(i = byte_offset; i < super_block.block_size; i++){
				return_buff[j] = read_buff[i];
				j++;
			}
			start_block++;

			for(i = 1; i <= num_blocks - 1; i++){
				block_address = get_block_address(file_fcb, start_block);
				start_block++;
				read_buff = read_block_from_cache(block_address);
				int k = 0;
				for(k = 0; k < super_block.block_size; k++){
					return_buff[j] = read_buff[k];
					j++;
				}
			}

			block_address = get_block_address(file_fcb, start_block);
			read_buff = read_block_from_cache(block_address);
			int k = 0;
			for(k = 0; j < size; k++){
				return_buff[j] = read_buff[k];
				j++;
			}
			file_fcb->seek_block = start_block;
			file_fcb->seek_offset = k;
			file_fcb->seek_block_addr = block_address;
		}
	}
	else{
		fatal("read_file: size out of range");
	}
	return return_buff;
}

uint32_t get_block_address(FCB * file_fcb, int block_number){
	if(block_number <= 11){
		return file_fcb->block_address[block_number];
	}
	else if(block_number <= 11 + 1024){
		uint32_t * address_buff = (uint32_t *) read_block_from_cache(file_fcb->block_address[12]);
		return address_buff[block_number - 12];
	}
	else if(block_number <= 11 + 1024 + 1024 * 1024){
		uint32_t * address_buff = (uint32_t *) read_block_from_cache(file_fcb->block_address[13]);
		uint32_t * address_buff_level2 = (uint32_t *) read_block_from_cache(address_buff[(block_number - 11 - 1024) / 1024]);
		return address_buff_level2[(block_number - 11 - 1024) % 1024];
	}
	else{
		fatal("get_block_address: size out of range");
		return -1;
	}
}

void seek_file(FCB * file_fcb, int size){
	// int start_block = file_fcb->seek_block;
	// int byte_offset = file_fcb->seek_offset;
	// uint32_t block_address = file_fcb->seek_block_addr;
	update_time_stamps(file_fcb, 3);
	if(size <= file_fcb->file_size){
		int num_blocks = size/super_block.block_size;
		int offset = size%super_block.block_size;
		file_fcb->seek_block = num_blocks;
		file_fcb->seek_offset = offset;
		file_fcb->seek_block_addr = get_block_address(file_fcb, file_fcb->seek_block);
	}
	else{
		fatal("seek_file: size out of range");
	}
}

void write_file(FCB * file_fcb, int size, char * data){
	int start_block = file_fcb->seek_block;
	int byte_offset = file_fcb->seek_offset;
	uint32_t block_address = file_fcb->seek_block_addr;
	update_time_stamps(file_fcb, 2);
	int j = 0;

    int num_blocks = ceil((float)(byte_offset + size)/(float)super_block.block_size);
	int total_num_blocks = ceil((float)file_fcb->file_size / (float)super_block.block_size);

	char * write_buff;
	write_buff = calloc(1, super_block.block_size);

	if(start_block >= total_num_blocks){
		block_address = allocate_block(file_fcb, start_block);
	}
	if(num_blocks == 1){
		write_buff = read_block_from_cache(block_address);
		int i = 0;
		for(i = byte_offset; j < size; i++){
			write_buff[i] = data[j];
			j++;
		}
		file_fcb->seek_offset = byte_offset + size;
		write_block_to_cache(block_address, write_buff);
		if(file_fcb->seek_offset == super_block.block_size){
			file_fcb->seek_offset = 0;
			file_fcb->seek_block += 1;
		}
	}
	else{
		int i = 0;
		for(i = 0; i < num_blocks; i++){
			if(start_block < total_num_blocks){
				if(0 == i){
					write_buff = read_block_from_cache(block_address);
					int k = 0;
					for(k = byte_offset; k < super_block.block_size; k++){
						write_buff[k] = data[j];
						j++;
					}
				}
				else if(i == num_blocks - 1){
					int k = 0;
					for(k = 0; j < size; k++){
						write_buff[k] = data[j];
						j++;
					}
					file_fcb->seek_offset = k;
					file_fcb->seek_block = start_block;
					if(file_fcb->seek_offset == super_block.block_size){
						file_fcb->seek_offset = 0;
						file_fcb->seek_block += 1;
					}
				}
				else{
					int k = 0;
					for(k = 0; k < super_block.block_size; k++){
						write_buff[k] = data[j];
						j++;
					}
				}
				write_block_to_cache(block_address, write_buff);
				start_block += 1;
				block_address = get_block_address(file_fcb, start_block);
			}
			else{
				block_address = allocate_block(file_fcb, start_block);
				total_num_blocks += 1;
				i -= 1;
			}
		}
	}
	file_fcb->seek_block_addr = block_address;
	file_fcb->file_size += size;
}

uint32_t allocate_block(FCB * file_fcb, int block_number){
 	uint32_t new_block_addr = (uint32_t)get_free_block();

 	if(block_number <= 11){
		file_fcb->block_address[block_number] = new_block_addr;
	}
	else if(block_number <= 11 + 1024){
		if(block_number == 12){
			file_fcb->block_address[12] = new_block_addr;
			new_block_addr = (uint32_t) get_free_block();
		}
		uint32_t * address_buff = (uint32_t *) read_block(file_fcb->block_address[12]);
		address_buff[block_number - 12] = new_block_addr;
		write_block(file_fcb->block_address[12], address_buff);
	}
	else if(block_number <= 11 + 1024 + 1024 * 1024){
		if(block_number == 12 + 1024){
			file_fcb->block_address[13] = new_block_addr;
			new_block_addr = (uint32_t) get_free_block();
		}
		uint32_t * address_buff = read_block(file_fcb->block_address[13]);
		if((block_number - 12 - 1024) % 1024 == 0){
			address_buff[(block_number - 12 - 1024) / 1024] = new_block_addr;
			new_block_addr = (uint32_t) get_free_block();
			write_block(file_fcb->block_address[13], address_buff);
		}
		uint32_t * address_buff_level2 = (uint32_t *) read_block(address_buff[(block_number - 11 - 1024) / 1024]);
		address_buff_level2[(block_number - 11 - 1024) % 1024] = new_block_addr;
		write_block(address_buff[(block_number - 11 - 1024) / 1024], address_buff_level2);
	}
	return new_block_addr;
}

void truncate_file(FCB * file_fcb){
   if(file_fcb != NULL){
   		update_time_stamps(file_fcb, 2);
		int number_of_blocks = ceil(file_fcb->file_size/super_block.block_size) ;
		file_fcb->file_size = 0 ;
		file_fcb->seek_offset = 0 ;
		file_fcb->seek_block = 0 ;
		int i ;
		for(i = 0 ; i < number_of_blocks ; i++){
			uint32_t block_num =  get_block_address(file_fcb , i) ;
			add_free_block(block_num) ;
		}
   }
}

///Library Routines
int open_call(char * p, int mode, int pid, int uid){
	char path[500];
	strcpy(path, p);
	int i = 0;
	for(i=0; i<1024;i++){
		if(oft_table[i].file_fcb == NULL){
			oft_table[i].pid = pid;
			oft_table[i].offset = 0;
			oft_table[i].mode = mode;
			if (isa_ctx->path==NULL) {
				isa_ctx->path = malloc(10);
				strcpy(isa_ctx->path, "root");
			}
			char name[10];
			strncpy(name, path, 5);
			if (strcmp(name, "root/")!=0) {
				strcpy(name, isa_ctx->path);
				strcat(name, "/");
				strcat(name, path);
				strcpy(path, name);
			}
			switch(mode){
				case 'r':
					FCB * temp = search_file_or_directory(path, uid);
					if (temp==NULL) {
						printf("Search file : File doesn't exists\n");
						return -1;
					}
					else if (temp->uid!=uid && temp->permission==0) {
						printf("Open file for read : Not enough permission\n");
						return -1;
					}
					else if (temp->type==0) {
						printf("Open file for read : Not a file\n");
						return -1;
					}
					oft_table[i].file_fcb == temp;
					return i;
				case 'a':
					FCB * temp = search_file_or_directory(path, uid);
					if (temp==NULL) {
						temp = create_file(path, 1, uid);
						if (temp==NULL) {
							return -1;
						}
						oft_table[i].file_fcb = temp;
						return i;
					}
					else if (temp->uid!=uid && temp->permission!=2) {
						printf("Open file for append : Not enough permission\n");
						return -1;
					}
					else {
						oft_table[i].file_fcb = temp;
						oft_table[i].offset = oft_table[i].file_fcb->file_size;
						return i;
					}
				case 'w':
					FCB * temp = search_file_or_directory(path, uid);
					if (temp==NULL) {
						temp = create_file(path, 1, uid);
						if (temp==NULL) {
							return -1;
						}
						oft_table[i].file_fcb = temp;
						return i;
					}
					else if (temp->uid!=uid && temp->permission!=2) {
						printf("Open file for write : Not enough permission\n");
						return -1;
					}
					else {
						oft_table[i].file_fcb = temp;
						truncate_file(oft_table[i].file_fcb);
						return i;
					}
				default:
					return -1;
			}
		}
	}
	return -1;
}

void update_directory_trace(FCB * file){
	if(strcmp(file->name, "root") == 0){
		write_super_block();
	}
	else{
		FCB * parent = get_parent_directory(file->path);
		update_time_stamps(parent, 2);
		seek_file(parent, file->location);
		write_file(parent, sizeof(FCB), (char*)file);
		printf("%d FILE SIZE AT CLOSE %d\n", file->file_size, file->location);
		seek_file(parent, file->location);

		FCB* temp = (FCB*)malloc(sizeof(FCB));
		temp = (FCB*) read_file(parent, sizeof(FCB));
		printf("%s %d HERE IT IS\n", temp->path, temp->file_size);
		write_super_block();
	}
}

int close_call(int num, int pid){
	if(oft_table[num].pid == pid){
		update_directory_trace(oft_table[num].file_fcb);
		oft_table[num].file_fcb = NULL;
		return 1;
	}
	printf("Close call : File not opened\n");
	return -1;
}

int read_call(int num, char * buf, int size, int pid){
	if(oft_table[num].pid == pid && oft_table[num].file_fcb != NULL){
		seek_file(oft_table[num].file_fcb, oft_table[num].offset);
		int i = 0;
		char * temp = read_file(oft_table[num].file_fcb, size);
		for(i = 0; i< size; i++){
			buf[i] = temp[i];
		}
		oft_table[num].offset += size;
		return 1;
	}
	printf("Read call : File not opened\n");
	return -1;
}

int write_call(int num, char * buf, int size, int pid){
	if(oft_table[num].pid == pid && oft_table[num].file_fcb != NULL){
		seek_file(oft_table[num].file_fcb, oft_table[num].offset);
		write_file(oft_table[num].file_fcb, size, buf);
		oft_table[num].offset += size;
		return 1;
	}
	printf("Write call : File not opened\n");
	return -1;
}

int seek_call(int num, int size, int pid){
	if(oft_table[num].pid == pid && oft_table[num].file_fcb != NULL){
		oft_table[num].offset = size;
		return 1;
	}
	printf("Seek call : File not opened\n");
	return -1;
}

int tell_call(int num, int pid){
	if(oft_table[num].pid == pid && oft_table[num].file_fcb != NULL){
		return oft_table[num].offset;
	}
	printf("Tell call : File not opened\n");
	return -1;
}

int create_directory(char * p, int uid){
	char path[500];
	strcpy(path, p);
	if (isa_ctx->path==NULL) {
		isa_ctx->path = malloc(10);
		strcpy(isa_ctx->path, "root");
	}
	char name[10];
	strncpy(name, path, 5);
	if (strcmp(name, "root/")!=0) {
		strcpy(name, isa_ctx->path);
		strcat(name, "/");
		strcat(name, path);
		strcpy(path, name);
	}
	if(search_file_or_directory(path, uid) == NULL){
		if(create_file(path, 0, uid) != NULL)
			return 1;
		else {
			printf("Create Directory : File creation failed\n");
			return -1;
		}
	}
	else {
		printf("Create Directory : Already exists\n");
		return -1;
	}
	return -1;
}

int remove_call(char * p, int uid){
	char path[500];
	strcpy(path, p);
	if (strcmp(path, "root")==0) {
		printf("Trying to delete root : I dare ya\n");
		return -1;
	}
	if (isa_ctx->path==NULL) {
		isa_ctx->path = malloc(10);
		strcpy(isa_ctx->path, "root");
	}
	char name[10];
	strncpy(name, path, 5);
	if (strcmp(name, "root/")!=0) {
		strcpy(name, isa_ctx->path);
		strcat(name, "/");
		strcat(name, path);
		strcpy(path, name);
	}
	FCB * file = search_file_or_directory(path, uid);
	if(file!=NULL){
		if (uid==file->uid || file->permission==2) {
			remove_directory(path, uid);
			return 1;
		}
		else {
			printf("Remove Directory : Not enough permission\n");
			return -1;
		}
	}
	return -1;
}

// disk_cache functions
void remove_entry(){
	cache_entry * cur_head = disk_cache.head ;
	if(cur_head != NULL && cur_head->physical_address != -1){
		if(cur_head->dirty_bit == 1){
			write_block(cur_head->physical_address , (char*) cur_head->block_data) ;
		}
		cur_head->physical_address = -1 ;

	}
}

void * read_block_from_cache(uint32_t physical_address) {
	cache_entry * cur = disk_cache.head ;
	while(cur != NULL){
		if(cur->physical_address == physical_address){
			break ;
		}
		cur = cur->next ;
	}

	if(cur != NULL){
		if(cur->prev != NULL && cur->next != NULL){
			cur->prev->next = cur->next ;
			cur->next->prev = cur->prev ;
			disk_cache.tail->next = cur ;
			cur->prev = disk_cache.tail ;
			cur->next = NULL ;
			disk_cache.tail = cur ;
		}
		else if(cur->next!= NULL){
			disk_cache.head = cur->next ;
			cur->next->prev = cur->prev ;
			disk_cache.tail->next = cur ;
			cur->prev = disk_cache.tail ;
			cur->next = NULL ;
			disk_cache.tail = cur ;
		}
		return cur->block_data ;
	}
	else{
		remove_entry() ;
		cache_entry * new_entry = disk_cache.head ;
		new_entry->physical_address = physical_address ;
		memcpy(new_entry->block_data, read_block(physical_address), super_block.block_size);

		disk_cache.head = new_entry->next ;
		new_entry->prev = disk_cache.tail ;
		disk_cache.tail->next = new_entry ;
		new_entry->next = NULL;
		disk_cache.head->prev = NULL ;
		disk_cache.tail = new_entry ;
		return disk_cache.tail->block_data ;
	}
}

void init_cache(){
	printf("Cache Initialized\n");
	cache_entry * head;
	head = malloc(sizeof(cache_entry)) ;
	head->physical_address = -1 ;
	head->block_data = malloc(4*1024) ;
	head->dirty_bit = 0 ;
	head->prev = NULL ;
	head->next = NULL ;
	disk_cache.head = head ;
	disk_cache.tail = head ;
	int i = 0;
	for(i = 0; i < 15 ; i++){
		cache_entry * new_entry ;
		new_entry = malloc(sizeof(cache_entry)) ;
		new_entry->physical_address = -1 ;
		new_entry->block_data = malloc(4*1024) ;
		new_entry->prev = disk_cache.tail ;
		new_entry->dirty_bit = 0 ;
		new_entry->next = NULL ;
		disk_cache.tail->next = new_entry ;
		disk_cache.tail = new_entry ;
	}
}

void clear_cache(){
	cache_entry * cur = disk_cache.head ;
	while(cur != NULL){
		if(cur->dirty_bit == 1 && cur->physical_address != -1){
			write_block(cur->physical_address , (char*) cur->block_data) ;
			cur->dirty_bit = 0;
		}
		cur->physical_address = -1 ;
		cur = cur->next ;
	}
}

void write_block_to_cache(uint32_t physical_address ,char * data ){

	cache_entry * cur = disk_cache.head ;
	while(cur != NULL){
		if(cur->physical_address == physical_address){
			break ;
		}
		cur = cur->next ;
	}

	if(cur != NULL){

		memcpy(cur->block_data, data, super_block.block_size);
		cur->dirty_bit = 1 ;
		if(cur->prev != NULL & cur->next != NULL){
			cur->prev->next = cur->next ;
			cur->next->prev = cur->prev ;
			disk_cache.tail->next = cur ;
			cur->prev = disk_cache.tail ;
			cur->next = NULL ;
			disk_cache.tail = cur ;
		}
		else if(cur->next != NULL){
			disk_cache.head = cur->next ;
			cur->next->prev = cur->prev ;
			disk_cache.tail->next = cur ;
			cur->prev = disk_cache.tail ;
			cur->next = NULL ;
			disk_cache.tail = cur ;
		}

	}
	else {
		remove_entry() ;
		cache_entry * new_entry = disk_cache.head ;
		new_entry->physical_address = physical_address ;

		memcpy(cur->block_data, data, super_block.block_size);
		disk_cache.head = new_entry->next ;
		new_entry->prev = disk_cache.tail ;
		disk_cache.tail->next = new_entry ;
		new_entry->next = NULL;
		disk_cache.head->prev = NULL ;
		disk_cache.tail = new_entry ;
		disk_cache.tail->dirty_bit = 1 ;
	}
}