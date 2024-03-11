#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "filesys.h"
#include "terminal.h" 

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test
 * Asserts that first 20 IDT entries are not NULL, and 0x14 to 0x1F IDF entries
 *  filled in nothing
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: idt.c/h
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 20; ++i){							// 20 exception entries should be filled
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	for (i = 0x14; i <= 0x1F; ++i){						// entry 0x14 to 0x1F in the IDT should not fill in anything
		if ((idt[i].offset_15_00 != NULL) && 
			(idt[i].offset_31_16 != NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* div0 Test
 * Asserts that Divide Error Exception leads to correct handler
 * Inputs: None
 * Outputs: Exception message/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: idt.c/h
 */
int div0_test(){
	TEST_HEADER;

	int a = 0;
	int b = 1 / a;										// cause Divide Error Exception
	b++;

	return FAIL;
}

/* dereference nullptr Test
 * Asserts that Page-Fault Exception leads to correct handler
 * Inputs: None
 * Outputs: Exception message/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: idt.c/h
 */
int deref_null_test(){
	TEST_HEADER;

	int* a = NULL;
	int b = *a;											// cause Page-Fault Exception
	b++;

	return FAIL;
}

/* syscall Test
 * Asserts that system call leads to correct handler
 * Inputs: None
 * Outputs: system call message/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: idt.c/h
 */
int syscall_test(){
    TEST_HEADER;

    asm("int	$0x80");								// make system call
    return FAIL;
}

/* Paging Presented Edge cases Test
 * Asserts that all edge cases of pointers' dereferences on valid memory addresses are valid
 * Inputs: None
 * Outputs: Raise message/PASS
 * Side Effects: None
 * Coverage: Paging initializaion: pointer dereference on valid memory address
 * Files: paging.c/h
 */
int paging_test_present_edge()
{
	TEST_HEADER;
	unsigned char* vidmem_start = (unsigned char*) 0xB8000;					// Start address of video memory
	unsigned char* vidmem_end = (unsigned char*) 0xB8FFF;					// End address of video memory
	unsigned char* kernel_start = (unsigned char*) 0x400000;				// Start address of kernel memory
	unsigned char* kernel_end = (unsigned char*) 0x7FFFFF;					// End address of kernel memory
	
	unsigned char result = *vidmem_start;
	result = *vidmem_end;
	result = *kernel_start;
	result = *kernel_end;
	
	return PASS;															// If they are not handled, return PASS
}

/* Paging Presented cases Test
 * Asserts that all pointers' dereferences on valid memory addresses are valid
 * Inputs: None
 * Outputs: Raise failure message/PASS
 * Side Effects: None
 * Coverage: Paging initializaion: all pointers' dereferences on valid memory addresses
 * Files: paging.c/h
 */
int paging_test_present()
{
	TEST_HEADER;
	unsigned char* rdm_vidmem1 = (unsigned char*) 0xB8500;					// Choose some random addresses for video memory
	unsigned char* rdm_vidmem2 = (unsigned char*) 0xB8666;
	unsigned char* rdm_kernel1 = (unsigned char*) 0x405000;					// Choose some random addresses for kernel
	unsigned char* rdm_kernel2 = (unsigned char*) 0x567890;

	unsigned char result = *rdm_vidmem1;
	result = *rdm_vidmem2;
	result = *rdm_kernel1;
	result = *rdm_kernel2;
	
	return PASS;															// If they are not handled, return PASS
}

/* Paging Test for video memory lower bound edge case
 * Asserts that edge case of dereference of lower bound of video memory address is invalid
 * Inputs: None
 * Outputs: Raise failure message/FAIL
 * Side Effects: None
 * Coverage: Paging initializaion: the lower bound of video memory
 * Files: paging.c/h
 */
int paging_test_vidmem_lower()
{
	TEST_HEADER;
	unsigned char* vidl_ptr = (unsigned char*)0xB7FFF;						// The start address of video memory minus 1
	unsigned char result = *vidl_ptr;
	result++;
	return FAIL;															
}

/* Paging Test for video memory upper bound edge case
 * Asserts that edge case of dereference of upper bound of video memory address is invalid
 * Inputs: None
 * Outputs: Raise failure message/FAIL
 * Side Effects: None
 * Coverage: Paging initializaion: the upper bound of video memory
 * Files: paging.c/h
 */
int paging_test_vidmem_upper()
{
	TEST_HEADER;
	unsigned char* vidu_ptr = (unsigned char*)0xB9000;						// The ending address of video memory plus 1
	unsigned char result = *vidu_ptr;
	result++;
	return FAIL;
}

/* Paging Test for video kernel lower bound edge case
 * Asserts that edge case of dereference of lower bound of kernel memory address is invalid
 * Inputs: None
 * Outputs: Raise failure message/FAIL
 * Side Effects: None
 * Coverage: Paging initializaion: the lower bound of kernel memory
 * Files: paging.c/h
 */
int paging_test_kernel_lower()
{
	TEST_HEADER;
	unsigned char* kerl_ptr = (unsigned char*)0x3FFFFF;						// The start address of kernel memory minus 1
	unsigned char result = *kerl_ptr;
	result++;
	return FAIL;
}

/* Paging Test for video kernel upper bound edge case
 * Asserts that edge case of dereference of upper bound of kernel memory address is invalid
 * Inputs: None
 * Outputs: Raise failure message/FAIL
 * Side Effects: None
 * Coverage: Paging initializaion: the upper bound of kernel memory
 * Files: paging.c/h
 */
int paging_test_kernel_upper()
{
	TEST_HEADER;
	unsigned char* keru_ptr = (unsigned char*)0x800000;						// The ending address of kernel memory minus 1
	unsigned char result = *keru_ptr;
	result++;
	return FAIL;
}

/* Checkpoint 2 tests */

/* rtc_test
 * Asserts that rtc can change frequency
 * Inputs: None
 * Outputs: print "1" at different frequency/FAIL
 * Side Effects: None
 * Coverage: rtc_open, rtc_close, rtc_read, rtc_write
 * Files: rtc.c/h
 */
int rtc_test(){
	TEST_HEADER;
	uint32_t i, j;
	if(0 != rtc_open(NULL)) return FAIL;
	for(i = FREQ_DFT; i <= FREQ_MAX; i = i << 1){							// iterate through all possible freq
		rtc_write(0, &i, 0);
		clear();
		printf("Testing rtc frequency %d Hz\n", i);
		for(j = 0; j < i; j++){
			rtc_read(0, NULL, 0);
			printf("1");
		}
	}
	if(0 != rtc_close(NULL)) return FAIL;
	printf("\n");
	return PASS;
}

/* Terminal Test for terminal driver read and write.
 * Asserts that edge case of nbytes equal to 128.
 * Inputs: None
 * Outputs: the user input and output.
 * Side Effects: output some message on the screen
 * Coverage: terminal open, read, write, close.
 * Files: terminal.c, terminal.h, keyboard.c, keyboard.h, lib.c, lib.h
 */
int terminal_read_write_test()
{
	TEST_HEADER;
	int i,cnt;
	char buf[1024];
	clear();
	terminal_open(NULL);
	while(1){
		for (i = 0; i<1024; i++){
			buf[i] = ' ';
		}
		terminal_write(0, "please enter the cmd:\n", 22); // 21 char + '\n' = 22
		cnt = terminal_read(0, buf, 127);				// read user input with maximum 128 bytes, (127 characters + '\n').
		printf("read %d bytes\n",cnt); 
		cnt = terminal_write(0, buf, cnt);				// write the input on the screen.
		printf("\nwrite %d bytes\n",cnt);
	}
	terminal_close(0);
	return PASS;
}

/* dir_test
 * Asserts that directory functions work correctly
 * Inputs: None
 * Outputs: list all filenames in directory/FAIL
 * Side Effects: None
 * Coverage: dir_open, dir_close, dir_read, dir_write, read_dentry_by_name, read_dentry_by_index
 * Files: filesys.c/h
 */
int dir_test(const char* fname){
	TEST_HEADER;
	uint32_t i, c, blank;
	uint8_t buf[MAX_FILENAME_LEN + 1] = {"\0"};								// filename buffer
	dentry_t dentry;
	int32_t nbytes;

	clear();
	if(dir_open((const uint8_t*)fname) == -1){
		printf("The directory  \"%s\"  does not exist!\n", fname);
		return PASS;
	}
	for(i = 0; i < MAX_FILES_NUMBER; i++){									// i -> inode_index
		if(-1 == dir_read(i, buf, nbytes)) continue;
		read_dentry_by_name(buf, &dentry);
		printf("file_name:");
		blank = MAX_FILENAME_LEN - strlen((const int8_t*)buf);
		for(c = 0; c < blank; c++) printf(" ");
		printf("%s", buf);
		printf(", file_type: %d, file size: %d\n", dentry.file_type, (inode_ptr[dentry.inode]).length);
	}
    if(dir_write(0, buf, nbytes) != -1){
        return FAIL;
    }
    if(dir_close(NULL) != 0) return FAIL;
    return PASS;
}

/* file_test
 * Asserts that file functions work correctly
 * Inputs: None
 * Outputs: content of file/FAIL
 * Side Effects: None
 * Coverage: file_open, file_close, file_read, file_write, read_dentry_by_name, read_dentry_by_index, read_data
 * Files: filesys.c/h
 */
int file_test(const char* fname)
{
	TEST_HEADER;
	clear();
	if(file_open((const uint8_t*)fname) == -1){
		printf("The file  \"%s\"  does not exist!\n", fname);
		return PASS;
	}
	dentry_t dentry;
	if (read_dentry_by_name((const uint8_t*)fname, &dentry) == -1){
		printf("oops, something wrong with read_dentry_by_name\n");
		return FAIL;
	}
	uint32_t i;
	uint32_t inode_idx = dentry.inode;
	inode_t* cur_inode = (inode_t*)(inode_ptr + inode_idx);
	uint32_t length = cur_inode->length;
	uint8_t buf[length];

	int32_t num_read = file_read(inode_idx, buf, length);
	for (i = 0; i < num_read; ++i){
		if(buf[i] == NULL){
            continue;
        }
		putc(buf[i]);
	}
    printf("\nfilename \"%s\" \n", fname);

	if(file_write(inode_idx, buf, length) != -1){
		printf("Some miracles happened!\n");
		return FAIL;
	}
	if(file_close(NULL) != 0) return FAIL;
	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	/* Checkpoint 1 tests */
	// TEST_OUTPUT("idt_test", idt_test());
	// TEST_OUTPUT("div0_test", div0_test());
	// TEST_OUTPUT("syscall_test", syscall_test());
	// TEST_OUTPUT("deref_null_test", deref_null_test());
	// TEST_OUTPUT("paging_test_present_edge", paging_test_present_edge());
	// TEST_OUTPUT("paging_test_present", paging_test_present());
	// TEST_OUTPUT("paging_test_vidmem_lower", paging_test_vidmem_lower());
	// TEST_OUTPUT("paging_test_vidmem_upper", paging_test_vidmem_upper());
	// TEST_OUTPUT("paging_test_kernel_lower", paging_test_kernel_lower());
	// TEST_OUTPUT("paging_test_kernel_upper", paging_test_kernel_upper());

	/* Checkpoint 2 tests */
	// TEST_OUTPUT("rtc_test", rtc_test());
	// TEST_OUTPUT("terminal_read_write_test", terminal_read_write_test(NULL));
	TEST_OUTPUT("dir_test", dir_test("."));
	// TEST_OUTPUT("dir_test", dir_test("non-exist-directory"));
	// TEST_OUTPUT("file_test", file_test("frame0.txt"));
	// TEST_OUTPUT("file_test", file_test("frame1.txt"));
	// TEST_OUTPUT("file_test", file_test("grep"));
	// TEST_OUTPUT("file_test", file_test("ls"));
	// TEST_OUTPUT("file_test", file_test("fish"));
	// TEST_OUTPUT("file_test", file_test("verylargetextwithverylongname.tx"));
	// TEST_OUTPUT("file_test", file_test("non-exist-file"));
}
