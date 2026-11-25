# Runtime for programs compiled by compiler-dungeon (RV64, Linux, no libc).
# Every rt_print_* receives the value in a0 and prints `result: <value>\n` to stdout, the exact
# text `--interpret` prints for the same program.

    .text

# write(1, a0, a1): a0 = string address, a1 = length
print_str:
    li a7, 64
    mv a2, a1
    mv a1, a0
    li a0, 1
    ecall
    ret

    .globl rt_print_unit
rt_print_unit:
    addi sp, sp, -16
    sd ra, 8(sp)
    la a0, msg_unit
    li a1, 11
    call print_str
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

    .globl rt_print_bool
rt_print_bool:
    addi sp, sp, -16
    sd ra, 8(sp)
    beqz a0, 1f
    la a0, msg_true
    li a1, 13
    j 2f
1:  la a0, msg_false
    li a1, 14
2:  call print_str
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

    .globl rt_print_signed
rt_print_signed:
    addi sp, sp, -48
    sd ra, 40(sp)
    sd a0, 0(sp)         # value
    la a0, msg_result
    li a1, 8
    call print_str
    ld a0, 0(sp)
    bgez a0, 1f
    neg a0, a0           # |value| (INT64_MIN stays 2^63, correct as unsigned)
    sd a0, 0(sp)
    la a0, msg_minus
    li a1, 1
    call print_str
    ld a0, 0(sp)
1:  call print_digits
    ld ra, 40(sp)
    addi sp, sp, 48
    ret

    .globl rt_print_unsigned
rt_print_unsigned:
    addi sp, sp, -48
    sd ra, 40(sp)
    sd a0, 0(sp)
    la a0, msg_result
    li a1, 8
    call print_str
    ld a0, 0(sp)
    call print_digits
    ld ra, 40(sp)
    addi sp, sp, 48
    ret

# prints a0 as an unsigned decimal followed by a newline
print_digits:
    addi sp, sp, -48
    sd ra, 40(sp)
    addi t0, sp, 32      # buffer end (digits are written backwards from sp+31)
    li t1, 10
    li t2, 10
    addi t0, t0, -1
    sb t2, 0(t0)         # '\n'
1:  remu t2, a0, t1
    divu a0, a0, t1
    addi t2, t2, 48
    addi t0, t0, -1
    sb t2, 0(t0)
    bnez a0, 1b
    addi t2, sp, 32
    sub a1, t2, t0       # length
    mv a0, t0
    call print_str
    ld ra, 40(sp)
    addi sp, sp, 48
    ret

    .section .rodata
msg_result: .ascii "result: "
msg_minus:  .ascii "-"
msg_true:   .ascii "result: true\n"
msg_false:  .ascii "result: false\n"
msg_unit:   .ascii "result: ()\n"
