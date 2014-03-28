#include <m2skernel.h>

SuperBlock super_block;

void init_super_block(){
	super_block.number_of_blocks = total_blocks_virtual_mem;
	super_block.block_size = 2048;
	super_block.blocks_in_track = blocks_in_track;
	super_block.FCB_root = * (create_root_FCB());

	super_block.free_block_count = 0;
	super_block.highest_FCB_index_used = 0;

	super_block.free_block_pointer = NULL;
	super_block.free_FCB_list_head = NULL;

	int size = sizeof(super_block);
	size += 8;
	size += total_blocks_virtual_mem * 4;
	size += 1024 * 4;			// 1024 is number of FCBs
	size += total_blocks_virtual_mem * sizeof(int);

	super_block.size = size/2048 + 1;
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


	//free_block_list write
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
		int num = super_block.free_FCB_list_head;
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
	if(super_block.free_block_list != NULL){
		int num = super_block.free_block_list;
		super_block.free_block_list = super_block.free_block_list->next;
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
	temp->next = super_block.free_block_list;
	super_block.free_block_list = temp;
}
void add_free_FCB(int fcb){
	int_list * temp = (int_list*) (malloc(sizeof(int_list)));
	temp->num = fcb;
	temp->next = super_block.free_FCB_list_head;
	super_block.free_FCB_list_head = temp;
}