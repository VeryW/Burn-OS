#define ASM     1

.global load_page_directory
.global enable_paging
.global flush_TLB

# load the page directory's address into the CR3 register, where MMU will find it.
.align 4
load_page_directory:
    pushl %ebp
    movl  %esp, %ebp

    movl  8(%ebp), %eax                 # load the parameter         
    movl  %eax, %cr3

    movl  %ebp, %esp
    popl  %ebp
    ret

# enable paging
.align 4
enable_paging:
    pushl %ebp
    movl  %esp, %ebp

    movl  %cr4, %eax
    orl   $0x00000010, %eax             # set bit 4 of CR4 to enable PSE(page size extension) and use 4MB. PSE bit 
    movl  %eax, %cr4

    movl  %cr0, %eax
    orl   $0x80000001, %eax             # set bit 31 to enable paging (PG bit) and bit 0 of CR0 to enable paging protection (PE bit)
    movl  %eax, %cr0

    movl  %ebp, %esp
    popl  %ebp
    ret

# Flush TLB after swapping page
.align 4
flush_TLB:
    pushl   %eax

    movl    %cr3, %eax
    movl    %eax, %cr3

    popl    %eax
    ret
