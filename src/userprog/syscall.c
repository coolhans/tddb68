#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
//-----------------------------------------------------------------------------------------------
/*struct fd{ //file descriptor
  tid_t owner;
  struct file* file;
  bool available;
  bool is_open;
};
*/

bool checkfile(int fd){ //check if valid fd
	if(fd<0 || fd>=FD_SIZE) {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------------------------
static void halt (void);
static bool create (const char *file, unsigned initial_size);
static int open (const char *file);
static void close (int fd);
static int read (int fd, void *buffer, unsigned size);
static int write (int fd, const void *buffer, unsigned size);
static void exit (int status);
//-----------------------------------------------------------------------------------------------
static void syscall_handler (struct intr_frame *);

void syscall_init (void){
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

//Shuts down the whole system. Use power_off() for that (declared in "threads/init.h").
//Do not use this system call to terminate your user program!
void halt (void){
  printf("Halted\n");
  power_off();
}

//Creates a new file called file initially initial_size bytes in size.
//Returns true if successful, false otherwise.
bool create (const char *file, unsigned initial_size){
  ASSERT (file != NULL);
  printf("File created\n");
  return filesys_create(file, initial_size);
}


int open (const char *file){
  ASSERT (file != NULL);
  struct file **opened_files = thread_current()->fd_table;
  int fd;
  for(fd=2;fd<FD_SIZE;fd++){
    //  fd_table[i].fd_number = i; //fd_number cant be 0 or 1
    if(opened_files[fd] == NULL){
      opened_files[fd] = filesys_open(file);
			if(opened_files[fd] == NULL){
				return -1;
			}
			else{
      printf("Opened file : %d\n", fd);
      return fd;
			}
    }
  }
  printf("File not opened : %d\n", fd);
  return -1;
}

/*
    Opens the file called file. Returns a nonnegative integer handle
    called a "file descriptor" (fd), or -1 if the file could not be opened.

    File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is
    standard input, fd 1 (STDOUT_FILENO) is standard output.
    You do not have to open the standard input and output before using them.

    Each process has an independent set of file descriptors. A user program should be able to have
    up to 132 files open at the same time. File descriptors are not inherited by child processes.
    Note that pintos does not manage file descriptors yet, you need to implement this feature yourself.

    When a single file is opened more than once, whether by a single process or different processes,
    each open returns a new file descriptor. Different file descriptors for a single file are
    closed independently in separate calls to close and they do not share a file position.

    If you add code inside "threads" protect it with the following (to make lab 2 work later on):
    #ifdef USERPROG
    ..
    #endif
*/
void close (int fd){
  if(checkfile(fd)){
    file_close(thread_current()->fd_table[fd]); //func from "filesys/file.c"
		thread_current()->fd_table[fd] = NULL;
		printf("File closed\n");
    return;
  }
printf("File not closed, bad fd\n");
return;
}

    //Closes file descriptor fd. Exiting or terminating a process implicitly closes all
    //its open file descriptors,as if by calling this function for each one.

int read (int fd, void *buffer, unsigned size){
  if(checkfile(fd)){ //check if valid fd
		//ASSERT(thread_current()->fd_table[fd]!=NULL);
    if(fd==0){ //fd 0 reads from keyboard
      int read = 0;
      //char *temp_buffer = buffer; //char *tmp = (char *)buffer
      int i;
      for(i=0; i<size; i++){
        //temp_buffer[i] = input_getc();
				input_getc();
        read++;
      }
      printf("File read\n");
      return read;
		}

		else if(thread_current()->fd_table[fd] != NULL){
		return file_read(thread_current()->fd_table[fd], buffer, size);
  }
}
  printf("File not read, bad fd\n");
  return -1;
}
    //Reads size bytes from the file open as fd into buffer. Returns the number of bytes actually read,
    //or -1 if the file could not be read (due to a condition other than end of file).
    //Fd 0 reads from the keyboard using input_getc() (defined in "devices/input.h").

int write (int fd, const void *buffer, unsigned size){
	struct file **opened_files = thread_current()->fd_table;
  if(checkfile(fd)){ //ASSERT valid fd
    if(fd==1){ //input
      putbuf(buffer, size);
      return size;
		}
		else if(opened_files[fd] != NULL){ //if file exists
      printf("File written\n");
			return file_write(opened_files[fd], buffer, size);
      //return file_write(thread_current()->fd_table[fd], buffer, size); //func from file.c
	}
	printf("File not written %d\n",fd);
	return -1;
}
}

    //Writes size bytes from buffer to the open file fd. Returns the number of bytes actually
    //written or -1 if the file could not be written.

    //Writing past end-of-file would normally extend the file, but file growth
    //is not implemented by the basic file system. The expected behavior is to write as
    //many bytes as possible up to end-of-file and return the actual number
    //written or -1 if no bytes could be written at all.

    //When fd=1 then the system call should write to the console. Your code which writes to
    //the console should write all of buffer in one callto putbuf()
    //(check "lib/kernel/stdio.h" and "lib/kernel/console.c"), at least as long as size is not
    //bigger than a few hundred bytes. (It is reasonable to break up larger buffers.)
    //Otherwise, lines of text output by different processes may end up interleaved on the console,
    //confusing the user.

void exit (int status){
  printf("Exit\n");
  thread_exit();
}
//------------------------------------------------------------------------
static void
syscall_handler (struct intr_frame *f UNUSED){
  int *p = (int*)f->esp;

  switch(*p){
    case SYS_HALT:
    {
      halt();
      break;
    }
    case SYS_CREATE:
    {
      const char *file = (const char*)p[1];
      unsigned initial_size = (unsigned)p[2];
      f->eax = create(file, initial_size);
      break;
    }
    case SYS_OPEN:
    {
      const char *file = (const char*)p[1];
      f->eax = open(file);
      break;
    }
    case SYS_CLOSE:
    {
      int fd = (int)p[1];
      close(fd);
      break;
    }
    case SYS_READ:
    {
      int fd = (int)p[1];
      void *buffer = (void*)p[2];
      unsigned size = (unsigned)p[3];
      f->eax = read(fd, buffer, size);
      break;
    }
    case SYS_WRITE:
    {
      int fd = (int)p[1];
      void *buffer = (void*)p[2];
      unsigned size = (unsigned)p[3];
      f->eax = write(fd, buffer, size);
      break;
    }
    case SYS_EXIT:
    {
      int status = (int)p[1];
      exit(status);
      break;
    }
    default:
    printf("Invalid system call\n" );
  }
}

    //Terminates the current user program, returning status to the kernel
    //(you don't have to worry about status for Lab1, it will be covered in Lab3).
    //Conventionally, a status of 0 indicates success and nonzero values indicate errors.
    //Remember to free all the resources that will be not needed anymore.
    //This system call will be improved in the following labs.
