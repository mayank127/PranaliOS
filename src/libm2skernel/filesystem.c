#include <m2skernel.h>
#include <string.h>

SuperBlock super_block;


void * read_block(int file_block_number){

	void * read_buffer ;
	size_t number_of_byte_read;
	read_buffer = malloc(super_block.block_size) ;
	if(file_block_number >= total_blocks_virtual_mem){
		fatal("read_block : block number out of range");
	}
	fseek(disk_file_pointer, file_block_number , SEEK_SET) ;
	number_of_byte_read = fread(read_buffer, super_block.block_size, 1, disk_file_pointer);

	if (number_of_byte_read != super_block.block_size) {
		fatal("read error");
	}

	return read_buffer ;

}


void write_block(int file_block_number, char * buffer){

	if(file_block_number >= total_blocks_virtual_mem){
		fatal("read_block : block number out of range");
	}

	fseek(disk_file_pointer, file_block_number,
		SEEK_SET);
	fwrite(buffer, super_block.block_size, 1, disk_file_pointer);

}

void init_super_block(){
	super_block.number_of_blocks = total_blocks_virtual_mem;
	super_block.block_size = 2048;
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

	super_block.size = size/2048 + 1;
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
	int number_of_entries = directory->file_size / sizeof(struct FCB);
	int i = 0;
	for (i=0;i<number_of_entries;i++) {
		struct FCB * temp = (struct FCB *) malloc (sizeof(struct FCB));
		seek_file(directory,i*sizeof(struct FCB));
		temp = (FCB *) read_file(directory, sizeof(struct FCB));
		if (strcmp(name,temp->name)==0) {
			return temp;
		}
	}
	return NULL;
}

// search file
struct FCB * search_file_or_directory(char * path) {
	if (strcmp(path,"root")==0)
		return &(super_block.FCB_root);
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
	root->seek_block_addr = root->block_address[0];
	root->seek_offset = 0;
	strcpy(root->path, "root");
	return root;
}

// create a new file/directory FCB and add to its parent directory
void create_file(char * path, char * name, int type) {
	/*
		type = 0 :- directory creation
		type = 1 :- file creation
	*/
	struct FCB * cur_directory = search_file_or_directory(path);
	struct FCB * new_file = (struct FCB *) (malloc(sizeof(struct FCB)));
	new_file->index = get_free_FCB_index();
	new_file->file_size = 0;
	strcmp(new_file->name,name);
	update_time_stamps(new_file,1);
	// new_file->uid = get_uid();
	new_file->type = type;
	new_file->seek_block = 0;
	new_file->seek_block_addr = new_file->block_address[0];
	new_file->seek_offset = 0;
	strcpy(new_file->path, path);
	strcat(new_file->path, "/");
	strcat(new_file->path, name);

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
	//TODO truncate_file(directory);
	struct FCB_list * itr = &cur_list;
	while(itr->next != NULL){
		if (strcmp(itr->cur->name, file->name) != 0){
			seek_file(directory, directory->file_size);
			write_file(directory,sizeof(struct FCB),(char *)(itr->cur));
			directory->file_size += sizeof(struct FCB);
		}
	}
}

// remove a directory (recursively deleting all its contents)
void remove_directory(char * path) {
	struct FCB * cur_directory = search_file_or_directory(path);
	if (cur_directory->type==0) {
		// now first recursively delete all the contents
		int number_of_entries = cur_directory->file_size/sizeof(struct FCB);
		int i = 0;
		for (i=0;i<number_of_entries;i++) {
			struct FCB * temp = (struct FCB *) malloc (sizeof(struct FCB));
			seek_file(cur_directory, i*sizeof(struct FCB));
			temp = (struct FCB *)read_file(cur_directory, sizeof(struct FCB));
			remove_directory(temp->path);
		}
	}
	// now delete this FCB (can be a directory or file) [base case]
	delete_file(path);
}


char * read_file(FCB * file_fcb, int size){
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
			read_buff = read_block(block_address);
			int i = 0;
			for(i = 0; i < size; i++){
				return_buff[j] = read_buff[byte_offset + i];
				j++;
			}
			byte_offset = byte_offset + size;
			file_fcb->seek_offset = byte_offset;
		}
		else{
			read_buff = read_block(block_address);
			int i = 0;
			for(i = byte_offset; i < super_block.block_size; i++){
				return_buff[j] = read_buff[i];
				j++;
			}
			start_block++;

			for(i = 1; i <= num_blocks - 1; i++){
				block_address = get_block_address(file_fcb, start_block);
				start_block++;
				read_buff = read_block(block_address);
				int k = 0;
				for(k = 0; k < super_block.block_size; k++){
					return_buff[j] = read_buff[k];
					j++;
				}
			}

			block_address = get_block_address(file_fcb, start_block);
			read_buff = read_block(block_address);
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
		uint32_t * address_buff = (uint32_t *) read_block(file_fcb->block_address[12]);
		return address_buff[block_number - 12];
	}
	else if(block_number <= 11 + 1024 + 1024 * 1024){
		uint32_t * address_buff = (uint32_t *) read_block(file_fcb->block_address[13]);
		uint32_t * address_buff_level2 = (uint32_t *) read_block(address_buff[(block_number - 11 - 1024) / 1024]);
		return address_buff_level2[(block_number - 11 - 1024) % 1024];
	}
}

void seek_file(FCB * file_fcb, int size){
	int start_block = file_fcb->seek_block;
	int byte_offset = file_fcb->seek_offset;
	// uint32_t block_address = file_fcb->seek_block_addr;

	if(start_block*super_block.block_size + byte_offset + size <= file_fcb->file_size){
		int num_blocks = (byte_offset + size)/super_block.block_size;
		if(num_blocks == 0){
			byte_offset = byte_offset + size;
		}
		else{
			file_fcb->seek_block = start_block + num_blocks;
			file_fcb->seek_offset = (byte_offset + size)%super_block.block_size;;
			file_fcb->seek_block_addr = get_block_address(file_fcb, file_fcb->seek_block);
		}
	}
	else{
		fatal("seek_file: size out of range");
	}
}

void write_file(FCB * file_fcb, int size, char * data){
	int start_block = file_fcb->seek_block;
	int byte_offset = file_fcb->seek_offset;
	uint32 block_address = file_fcb->seek_block_addr;

	int j = 0;

    int num_blocks = ceil((byte_offset + size)/super_block.block_size);
	int total_num_blocks = ceil(file_fcb->file_size / super_block.block_size);

	char * write_buff;
	write_buff = malloc(super_block.block_size);

	if(start_block >= total_num_blocks){
		block_address = allocate_block(file_fcb, start_block);
	}
	if(num_blocks == 1){
		write_buff = read_block(block_address);
		int i = 0;
		for(i = byte_offset; j < size; i++){
			write_buff[i] = data[j];
			j++;
		}
		file_fcb->seek_offset = byte_offset + size;
		write_block(block_address, write_buff);
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
					write_buff = read_block(block_address);
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
				write_block(block_address, write_buff);
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
		uint32 * address_buff = (uint32_t *) read_block(block_address[12]);
		address_buff[block_number - 12] = new_block_addr;
		write_block(block_address[12], address_buff);
	}
	else if(block_number <= 11 + 1024 + 1024 * 1024){
		if(block_number == 12 + 1024){
			file_fcb->block_address[13] = new_block_addr;
			new_block_addr = (uint32_t) get_free_block();
		}
		uint32 * address_buff = read_block(block_address[13]);
		if((block_number - 12 - 1024) % 1024 == 0){
			address_buff[(block_number - 12 - 1024) / 1024] = new_block_addr;
			new_block_addr = (uint32_t) get_free_block();
			write_block(block_address[13], address_buff);
		}
		uint32 * address_buff_level2 = (uint32_t *) read_block(address_buff[(block_number - 11 - 1024) / 1024]);
		address_buff_level2[(block_number - 11 - 1024) % 1024] = new_block_addr;
		write_block(address_buff[(block_number - 11 - 1024) / 1024], address_buff_level2);
	}
	return new_block_addr;
}