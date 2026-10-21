#include <cpu.h>
#include <printf.h>
#include <isr.h>

static const char* const exception_names[32] = {
    "Divide Error",                   // 0
    "Debug",                          // 1
    "Non-Maskable Interrupt (NMI)",   // 2
    "Breakpoint",                     // 3
    "Overflow",                       // 4
    "Bound Range Exceeded",           // 5
    "Invalid Opcode",                 // 6
    "Device Not Available",           // 7
    "Double Fault",                   // 8
    "Coprocessor Segment Overrun",    // 9 (legacy, no longer generated)
    "Invalid TSS",                    // 10
    "Segment Not Present",            // 11
    "Stack-Segment Fault",            // 12
    "General Protection Fault",       // 13
    "Page Fault",                     // 14
    "Reserved",                       // 15
    "x87 Floating-Point Exception",   // 16
    "Alignment Check",                // 17
    "Machine Check",                  // 18
    "SIMD Floating-Point Exception",  // 19
    "Virtualization Exception",       // 20
    "Control Protection Exception",   // 21
    "Reserved",                       // 22
    "Reserved",                       // 23
    "Reserved",                       // 24
    "Reserved",                       // 25
    "Reserved",                       // 26
    "Reserved",                       // 27
    "Hypervisor Injection Exception", // 28
    "VMM Communication Exception",    // 29
    "Security Exception",             // 30
    "Reserved"                        // 31
};

__attribute__((noreturn)) void exception_handler(isr_frame_t* frame) {
    printf("\033[91m");
    printf("== Kernel Panic ========\n");
    printf("Location: 0x%.16llx\n", frame->rip);
    printf(
        "Reason: %s (%d) (err=0x%x)\n", exception_names[frame->vector], frame->vector, frame->err);
    printf("\n");
    printf("rax=0x%.16llx rbx=0x%.16llx rcx=0x%.16llx\n", frame->rax, frame->rbx, frame->rcx);
    printf("rdx=0x%.16llx rdi=0x%.16llx rsi=0x%.16llx\n", frame->rdx, frame->rdi, frame->rsi);
    printf("rbp=0x%.16llx cs=0x%.16llx  ss=0x%.16llx\n", frame->rbp, frame->cs, frame->ss);
    printf("rsp=0x%.16llx rflags=0x%.16llx\n", frame->rsp, frame->rflags);
    printf("========================\n");
    __asm__ volatile("cli");
    halt();
}