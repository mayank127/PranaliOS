#include <m2skernel.h>

char * read_file(FCB * file_fcb, int size){
	int start_block = file_fcb->seek_block;
	int byte_offset = file_fcb->seek_offset;
	uint32 block_address = file_fcb->seek_block_addr;
	
	char * read_buff;
	char * return_buff;
	int j = 0;

	return_buff = malloc(size);

	if(seek_block*super_block.block_size + byte_offset + size <= file_fcb->file_size){
		int num_blocks = (byte_offset + size)/super_block.block_size;
		
		if(num_blocks == 0){		
			read_buff = read_block(block_address);	
			for(int i = 0; i < size; i++){
				return_buff[j] = read_buff[byte_offset + i];
				j++;
			}
			byte_offset = byte_offset + size;
			file_fcb->seek_offset = byte_offset;
		}
		else{
			read_buff = read_block(block_address);
			for(int i = byte_offset; i < super_block.block_size; i++){
				return_buff[j] = read_buff[i];
				j++;
			}
			start_block++;

			for(int i = 1; i <= num_blocks - 1; i++){
				block_address = get_block_address(start_block);
				start_block++;
				read_buff = read_block(block_address);
				for(int k = 0; k < super_block.block_size; k++){
					return_buff[j] = read_buff[k];
					j++;
				}
			}

			block_address = get_block_address(file_fcb, start_block);
			read_buff = read_block(block_address);
			for(int k = 0; j < size; k++){
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

uint32 get_block_address(FCB * file_fcb, int block_number){
	if(block_number <= 11){
		return block_address[block_number];
	}
	else if(block_number <= 11 + 1024){
		uint32 * address_buff = read_block(block_address[12]);
		return address_buff[block_number - 12]; 
	}
	else if(block_number <= 11 + 1024 + 1024 * 1024){
		uint32 * address_buff = read_block(block_address[13]);
		uint32 * address_buff_level2 = address_buff[(block_number - 11 - 1024) / 1024]);
		return address_buff_level2[(block_number - 11 - 1024) % 1024]		
	}
}

void seek_file(FCB * file_fcb, int size){
	int start_block = file_fcb->seek_block;
	int byte_offset = file_fcb->seek_offset;
	uint32 block_address = file_fcb->seek_block_addr;

	if(seek_block*super_block.block_size + byte_offset + size <= file_fcb->file_size){
		int num_blocks = (byte_offset + size)/super_block.block_size;
		if(num_blocks == 0){		
			byte_offset = byte_offset + size;
		}
		else{
			file_fcb->seek_block = start_block + num_blocks;
			file_fcb->seek_offset = (byte_offset + size)%super_block.block_size;;
			file_fcb->seek_block_addr = get_block_address(file_fcb->seek_block);
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

	int num_blocks = (byte_offset + size)/super_block.block_size;
		
	char * write_buff;
	write_buff = malloc(super_block.block_size);
	if(num_blocks == 0){		
		write_buff = read_block(block_address);	
			for(int i = 0; i < size; i++){
				return_buff[j] = read_buff[byte_offset + i];
				j++;
			}
			byte_offset = byte_offset + size;
			file_fcb->seek_offset = byte_offset;
		}
		else{
			read_buff = read_block(block_address);
			for(int i = byte_offset; i < super_block.block_size; i++){
				return_buff[j] = read_buff[i];
				j++;
			}
			start_block++;

			for(int i = 1; i <= num_blocks - 1; i++){
				block_address = get_block_address(start_block);
				start_block++;
				read_buff = read_block(block_address);
				for(int k = 0; k < super_block.block_size; k++){
					return_buff[j] = read_buff[k];
					j++;
				}
			}

			block_address = get_block_address(file_fcb, start_block);
			read_buff = read_block(block_address);
			for(int k = 0; j < size; k++){
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
}