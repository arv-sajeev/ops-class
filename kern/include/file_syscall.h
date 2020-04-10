#ifndef _FILESYSCALLH_
#define _FILESYSCALLH_

/*****************

	file_handle 
		- per process file handle for each file that is opened by the process
		- has info about the 
		- vnode, offset, mode, lock and reference count to keep track of references
		- to be shared with children after fork 

******************/
struct file_handle{
	struct vnode* vnode;
	off_t offset;
	struct lock* lock;
	unsigned int reference_count;
	unsigned int mode;
};

//Using int to return error value or 0 and retval to writeback return value 

int sys_open(const char* file_name,int flags,int *retval);












#endif
