.syntax unified
.thumb

/*
 * Hardened nRF52840 revisions close the firmware branch of APPROTECT at
 * reset. This shim runs before the Nordic MBR, opens that branch, and then
 * tail-calls the MBR's original reset handler.
 */
.section .text.approtect_boot_shim, "ax", %progbits
.global approtect_boot_shim
.type approtect_boot_shim, %function
.thumb_func
approtect_boot_shim:
    ldr  r0, =0x40000558
    movs r1, #0x5a
    str  r1, [r0]
    dsb
    isb
    ldr  r0, =0x00000a81
    bx   r0
.size approtect_boot_shim, . - approtect_boot_shim
