#include <m2skernel.h>

void * read_block(int file_block_number){

	void * read_buffer ;
	size_t number_of_byte_read;
	read_buffer = malloc(super_block.block_size) ;
	if(file_block_number >= total_blocks_virtual_mem){
		fatal("read_block : block number out of range");
	}

	fseek(disk_file_pointer, block_number , SEEK_SET) ;
	number_of_byte_read = fread(buf,super_block.block_size, 1, disk_file_pointer);

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

int file_seek(FCB * file_fcb, int offset, int start_point){

	if(offset )
}