#define _GNU_SOURCE

#include "lazycopy.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>

/** 
 * Global bookeeping structure. 
 */
typedef struct node {
  void * start_address;
  size_t size;
  struct node * next;
} node_t;

typedef struct list {
  node_t * first;
} list_t;
  
/** 
 * Initialize bookkeeping structure.
 */
list_t read_blocks = {NULL};

/** 
 * ADDED FUNCTION: This function is our signal handler.
 */
void copy_handler (int signal, siginfo_t* info, void * ctx) {
  intptr_t seg_address = (intptr_t)info->si_addr;
  node_t * current_node = read_blocks.first;

  node_t * prev_node = NULL;
  while(current_node != NULL){
    if((seg_address >= (intptr_t)current_node->start_address) &&
       (seg_address <= (intptr_t)(current_node->start_address+
                                  current_node->size))) {
          printf("test\n");
      void * test = mmap(current_node->start_address, current_node->size, PROT_READ | PROT_WRITE,
           MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, -1, 0);
      if (current_node->start_address == test) {
        printf("this worked?\n");
      }
      
      memcpy(test, current_node->start_address, current_node->size);
      if(prev_node == NULL){
        read_blocks.first = current_node->next;
        free(current_node);
        return;
      } else {
        prev_node->next = current_node->next;
        free(current_node);
        return;
      }
    }
    
    prev_node = current_node;
    current_node = current_node->next;
  }

  printf("Yeah, no, you actually segfaulted. Sorry.\n");
  exit(EXIT_FAILURE);
}

/**
 * This function will be called at startup so you can set up a signal handler.
 */
void chunk_startup() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = copy_handler;
  sa.sa_flags = SA_SIGINFO;
  
  // Set the signal handler, checking for errors
  if(sigaction(SIGSEGV, &sa, NULL) != 0) {
    perror("sigaction failed");
    exit(2);
  } 
}

/**
 * This function should return a new chunk of memory for use.
 *
 * \returns a pointer to the beginning of a 64KB chunk of memory that can be read, written, and copied
 */
void* chunk_alloc() {
  // Call mmap to request a new chunk of memory. See comments below for description of arguments.
  void* result = mmap(NULL, CHUNKSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  // Arguments:
  //   NULL: this is the address we'd like to map at. By passing null, we're asking the OS to decide.
  //   CHUNKSIZE: This is the size of the new mapping in bytes.
  //   PROT_READ | PROT_WRITE: This makes the new reading readable and writable
  //   MAP_ANONYMOUS | MAP_SHARED: This mapes a new mapping to cleared memory instead of a file,
  //                               which is another use for mmap. MAP_SHARED makes it possible for us
  //                               to create shared mappings to the same memory.
  //   -1: We're not connecting this memory to a file, so we pass -1 here.
  //   0: This doesn't matter. It would be the offset into a file, but we aren't using one.
  
  // Check for an error
  if(result == MAP_FAILED) {
    perror("mmap failed in chunk_alloc");
    exit(2);
  }
  
  // Everything is okay. Return the pointer.
  return result;
}

/**
 * Create a copy of a chunk by copying values eagerly.
 *
 * \param chunk This parameter points to the beginning of a chunk returned from chunk_alloc()
 * \returns a pointer to the beginning of a new chunk that holds a copy of the values from
 *   the original chunk.
 */
void* chunk_copy_eager(void* chunk) {
  // First, we'll allocate a new chunk to copy to
  void* new_chunk = chunk_alloc();
  
  // Now copy the data
  memcpy(new_chunk, chunk, CHUNKSIZE);
  
  // Return the new chunk
  return new_chunk;
}

/**
 * Create a copy of a chunk by copying values lazily.
 *
 * \param chunk This parameter points to the beginning of a chunk returned from chunk_alloc()
 * \returns a pointer to the beginning of a new chunk that holds a copy of the values from
 *   the original chunk.
 */
void* chunk_copy_lazy(void* chunk) {

  size_t size_chunk = sizeof(*chunk);
  // Your implementation should do the following:
  // 1. Use mremap to create a duplicate mapping of the chunk passed in
  void * new_address = mremap(chunk, 0, size_chunk, MREMAP_MAYMOVE);
  if (new_address == NULL) {
    printf("what\n");
  }
  if (new_address == MAP_FAILED) {
    perror("Yikes, something happened: ");
    exit(EXIT_FAILURE);
  }
  // 2. Mark both mappings as read-only
  mprotect(chunk, size_chunk, PROT_READ);
  mprotect(new_address, size_chunk, PROT_READ);
  // 3. Keep some record of both lazy copies so you can make them writable later.
  //    At a minimum, you'll need to know where the chunk begins and ends.

  node_t * current_node;
  current_node = read_blocks.first;
  while (current_node != NULL) {
    current_node = current_node->next;
  }
  current_node = malloc (sizeof (node_t));
  if (current_node == NULL) {
    perror ("Malloc failed: ");
    exit(EXIT_FAILURE);
  }
  current_node->start_address = chunk;
  current_node->size = size_chunk;
  node_t * next_node = malloc (sizeof (node_t));
  current_node->next = next_node;
  next_node->start_address = new_address;   
  next_node->size = size_chunk;
  next_node->next = read_blocks.first;
  read_blocks.first = current_node;
  // Later, if either copy is written to you will need to:
  // 1. Save the contents of the chunk elsewhere (a local array works well)
  // 2. Use mmap to make a writable mapping at the location of the chunk that was written
  // 3. Restore the contents of the chunk to the new writable mapping

  return new_address;
}

