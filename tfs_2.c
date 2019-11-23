/* Programmer:	Tom Birdsong
 * Course:	ECE 3220 F2019
 * Assignment:	Program 3
 * Purpose:	Implement read/write/delete operations
		for simple file system
 */

#include "tfs.h"


/* Implementation of helper functions */

/* tfs_offset (derived from tfs_1.c)
 * 
 * returns the file offset for an active directory entry
 *
 * preconditions:
 * 	(1) the file descriptor is in range
 *	(2) the directory entry is active
 *
 * postconditions:
 *	there are no changes to the file data structure
 *
 * input parameter is a file descriptor
 *
 * return value is the file offset when successful or MAX_FILE_SIZE + 1
 *	when failure
 */

unsigned int tfs_offset ( unsigned int file_descriptor) {
	if( !tfs_check_fd_in_range( file_descriptor) ) return (MAX_FILE_SIZE + 1);
	if( directory[file_descriptor].status == UNUSED) return (MAX_FILE_SIZE + 1 );
	return (directory[file_descriptor].byte_offset );
}

/* num_blocks
 * 
 * returns the number of blocks currently allocated to the file
 *
 * input: file descriptor
 * 
 * returns: number of blocks
 */
unsigned int num_blocks( unsigned int file_descriptor) {

	// Round up to get the number of blocks currently allocated for the file
	return (tfs_size(file_descriptor) + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
}

/* nth_block
 *
 * returns the zero-index nth block in the file block list for the descriptor
 *
 * input: file descriptor, block counter
 *
 * returns: index of nth block or 0 if the desciptor is invalid
 */

unsigned int nth_block( unsigned int file_descriptor, unsigned int n) {

	// Check preconditions
	if( !tfs_check_fd_in_range( file_descriptor) ) return 0;
	if( directory[file_descriptor].status == UNUSED) return 0;
	if( directory[file_descriptor].first_block == FREE) return 0;

	// Short-circuit if accessing 0th block
	if( n == 0 ) return directory[file_descriptor].first_block;	

	// Declare variables
	unsigned int i;
	unsigned int block_index = directory[file_descriptor].first_block;

	// Step through file allocation table	
	for(i = 0; i < n && file_allocation_table[block_index] >= FIRST_VALID_BLOCK; i++) {
		block_index = file_allocation_table[block_index];
	}

	// If fewer than n blocks were in the list return 0
	if(i == n)
		return block_index;
	else
		return 0;
	
}


/* implementation of assigned functions */


/* tfs_delete()
 *
 * deletes a closed directory entry having the given file descriptor
 *   (changes the status of the entry to unused) and releases all
 *   allocated file blocks
 *
 * preconditions:
 *   (1) the file descriptor is in range
 *   (2) the directory entry is closed
 *
 * postconditions:
 *   (1) the status of the directory entry is set to unused
 *   (2) all file blocks have been set to free
 *
 * input parameter is a file descriptor
 *
 * return value is TRUE when successful or FALSE when failure
 */

unsigned int tfs_delete( unsigned int file_descriptor ){

	// Steps:
	// 1. Check preconditions and initialize variables
	if( !tfs_check_fd_in_range(file_descriptor)) return FALSE;

	if(directory[file_descriptor].status == OPEN) {
		printf("Attempted to delete an open file descriptor!\n");
		return FALSE;
	} else if(directory[file_descriptor].status == UNUSED) {
		printf("Attempted to delete an unused file descriptor.\n");
		return FALSE;
	}
	
	unsigned char fb, next_fb;

	// 2. Release file blocks
	fb = nth_block(file_descriptor, 0);

	// Walk file allocation table and release blocks along the way
	while(fb != FREE && fb != LAST_BLOCK) {
		next_fb = file_allocation_table[fb];
		file_allocation_table[fb] = FREE;
		fb = next_fb;
	}

	// 3. Set entry status to unused	
	directory[file_descriptor].first_block = FREE;
	directory[file_descriptor].size = 0;
	directory[file_descriptor].byte_offset = 0;
	directory[file_descriptor].status = UNUSED;
	
	return TRUE;
}

/* tfs_read()
 *
 * reads a specified number of bytes from a file starting
 *   at the byte offset in the directory into the specified
 *   buffer; the byte offset in the directory entry is
 *   incremented by the number of bytes transferred
 *
 * depending on the starting byte offset and the specified
 *   number of bytes to transfer, the transfer may cross two
 *   or more file blocks
 *
 * the function will read fewer bytes than specified if the
 *   end of the file is encountered before the specified number
 *   of bytes have been transferred
 *
 * preconditions:
 *   (1) the file descriptor is in range
 *   (2) the directory entry is open
 *   (3) the file has allocated file blocks
 *
 * postconditions:
 *   (1) the buffer contains bytes transferred from file blocks
 *   (2) the specified number of bytes has been transferred
 *         except in the case that end of file was encountered
 *         before the transfer was complete
 *
 * input parameters are a file descriptor, the address of a
 *   buffer of bytes to transfer, and the count of bytes to
 *   transfer
 *
 * return value is the number of bytes transferred
 */

unsigned int tfs_read( unsigned int file_descriptor,
                       char *buffer,
                       unsigned int byte_count ){

	// Steps:
	// 1. Check preconditions and initialize variables
	if( !tfs_check_fd_in_range(file_descriptor)) return 0;
	if( !tfs_check_file_is_open(file_descriptor)) return 0;

	int i;
	unsigned int block_offset, updated_byte_count;
	unsigned char fb, *fb_ptr, *fptr;

	// 2. Get address of first byte to write
	block_offset = tfs_offset(file_descriptor) / BLOCK_SIZE;
	fb = nth_block( file_descriptor, block_offset);
	
	fb_ptr = (char *)(blocks + fb);
	fptr = fb_ptr + tfs_offset(file_descriptor) % BLOCK_SIZE;
	
	// 3. Iteratively read into buffer, adjusting for overflow
	updated_byte_count = byte_count;
	for( i = 0; i < updated_byte_count; i++, fptr++) {
	
		// If the next read is beyond the block
		// then move to the next block
		if(fptr >= fb_ptr + BLOCK_SIZE) {

			block_offset++;
			fb = nth_block(file_descriptor, block_offset);
		
			// Check for EOF
			if(fb < FIRST_VALID_BLOCK) {
				// Record number of bytes read and exit
				updated_byte_count = i;
			} else {
				// Point to new block
				fb_ptr = (char *)(blocks + fb);
				fptr = fb_ptr;
			}
		}

		// Write from the buffer into the block
		if(fb >= FIRST_VALID_BLOCK) buffer[i] = *fptr;
	}

	// Update offset in directory
	directory[file_descriptor].byte_offset += updated_byte_count;

	return updated_byte_count;
}

/* tfs_write()
 *
 * writes a specified number of bytes from a specified buffer
 *   into a file starting at the byte offset in the directory;
 *   the byte offset in the directory entry is incremented by
 *   the number of bytes transferred
 *
 * depending on the starting byte offset and the specified
 *   number of bytes to transfer, the transfer may cross two
 *   or more file blocks
 *
 * furthermore, depending on the starting byte offset and the
 *   specified number of bytes to transfer, additional file
 *   blocks, if available, will be added to the file as needed;
 *   in this case, the size of the file will be adjusted
 *   based on the number of bytes transferred beyond the
 *   original size of the file
 *
 * the function will read fewer bytes than specified if file
 *   blocks are not available
 *
 * preconditions:
 *   (1) the file descriptor is in range
 *   (2) the directory entry is open
 *   (3) the file has allocated file blocks
 *
 * postconditions:
 *   (1) the file contains bytes transferred from the buffer
 *   (2) the specified number of bytes has been transferred
 *         except in the case that file blocks needed to
 *         complete the transfer were not available
 *   (3) the size of the file is increased as appropriate
 *         when transferred bytes extend beyond the previous
 *         end of the file
 *
 * input parameters are a file descriptor, the address of a
 *   buffer of bytes to transfer, and the count of bytes to
 *   transfer
 *
 * return value is the number of bytes transferred
 */

unsigned int tfs_write( unsigned int file_descriptor,
                        char *buffer,
                        unsigned int byte_count ){

	// Steps:
	// 1. Check preconditions and initialize variables
	if( !tfs_check_fd_in_range(file_descriptor)) return 0;
	if( !tfs_check_file_is_open(file_descriptor)) return 0;

	int i;
	unsigned char fb, new_block, last_block, *fb_ptr, *fptr;
	unsigned int end_write_pos, end_blocks, block_offset, size, updated_byte_count;
	
	// 2. Allocate the necessary amount of blocks so that
	//	offset + byte_count <= size

	last_block = 0;
	end_write_pos = tfs_offset(file_descriptor) + byte_count;
	end_blocks = num_blocks(file_descriptor) * BLOCK_SIZE;

	while( end_write_pos > end_blocks) {

		// Make sure that the previous size accounts for all current blocks
		// before adding new ones
		directory[file_descriptor].size = end_blocks;

		// Make a new block and append it to the file list
		new_block = tfs_new_block();

		// If allocation fails try to continue with write on existing blocks
		if(new_block < FIRST_VALID_BLOCK) break;

		file_allocation_table[new_block] = LAST_BLOCK;	

		if(directory[file_descriptor].first_block == FREE) {
			directory[file_descriptor].first_block = new_block;
			last_block = new_block;
		} else {
			// Move pointer to end of the file block list
			if(last_block == 0) {
				last_block = directory[file_descriptor].first_block;
				while(file_allocation_table[last_block] != LAST_BLOCK) {
					last_block = file_allocation_table[last_block];
				}
			}

			// Add file block to the end of the list
			file_allocation_table[last_block] = new_block;
			last_block = new_block;
		}

		// num_blocks does not update until the size is adjusted
		end_blocks = (num_blocks(file_descriptor) + 1) * BLOCK_SIZE;
			
		if( end_write_pos >= end_blocks)
			directory[file_descriptor].size += BLOCK_SIZE;
		else
			directory[file_descriptor].size += end_write_pos % BLOCK_SIZE;
	}

	// If allocation failed for a file of size 0 return immediately
	if(directory[file_descriptor].first_block == FREE) return 0;

	// 3. Get address of first byte to write
	block_offset = tfs_offset(file_descriptor) / BLOCK_SIZE;
	fb = nth_block( file_descriptor, block_offset);

	// if the nth block does not exist then the offset must be at the
	// end of the file and no more blocks are available to allocate,
	// so no bytes can be written
	updated_byte_count = (fb == 0) ? 0 : byte_count;

	fb_ptr = (char *)(blocks + fb);
	fptr = fb_ptr + tfs_offset(file_descriptor) % BLOCK_SIZE;
	
	// 4. Iteratively write into buffer, adjusting for overflow
	for( i = 0; i < updated_byte_count; i++, fptr++) {
		// If the next write will overflow the block
		// then move to the next block
		if(fptr >= fb_ptr + BLOCK_SIZE) {
			block_offset++;
			fb = nth_block(file_descriptor, block_offset);
		
			// Check for EOF
			if(fb < FIRST_VALID_BLOCK) {
				updated_byte_count = i;
			} else {
				fb_ptr = (char *)(blocks + fb);
				fptr = fb_ptr;
			}
		}

		// Write from the buffer into the block
		if(fb >= FIRST_VALID_BLOCK) *fptr = buffer[i];
	}

	// Move offset to new offset
	directory[file_descriptor].byte_offset += updated_byte_count;

	// 5. Return number of bytes read
	return updated_byte_count;
}
