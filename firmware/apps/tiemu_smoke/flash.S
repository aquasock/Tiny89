/* SPDX-License-Identifier: GPL-2.0-or-later */
    .section .rodata.tiny89_flash, "a"
    .balign 16
    .globl tiny89_flash_image
    .globl tiny89_flash_image_end
tiny89_flash_image:
    .incbin T89_FLASH_FILE
tiny89_flash_image_end:
    .section .note.GNU-stack, "", @progbits
