	.cpu cortex-m4
	.arch armv7e-m
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 6
	.eabi_attribute 34, 1
	.eabi_attribute 38, 1
	.eabi_attribute 18, 4
	.file	"nrfx_atomic.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.file 1 "C:\\Nrf_sdk\\nRF5_SDK_17.1.0_ddde560\\modules\\nrfx\\soc\\nrfx_atomic.c"
	.section	.text.nrfx_atomic_internal_cmp_exch,"ax",%progbits
	.align	1
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_internal_cmp_exch, %function
nrfx_atomic_internal_cmp_exch:
.LFB176:
	.file 2 "C:\\Nrf_sdk\\nRF5_SDK_17.1.0_ddde560\\modules\\nrfx\\soc\\nrfx_atomic_internal.h"
	.loc 2 289 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4, r5, r6}
	.cfi_def_cfa_offset 12
	.cfi_offset 4, -12
	.cfi_offset 5, -8
	.cfi_offset 6, -4
	sub	sp, sp, #36
	.cfi_def_cfa_offset 48
	str	r0, [sp, #12]
	str	r1, [sp, #8]
	str	r2, [sp, #4]
	.loc 2 290 10
	movs	r3, #0
	strb	r3, [sp, #31]
	.loc 2 295 14
	movs	r3, #0
	str	r3, [sp, #24]
	.loc 2 296 14
	movs	r3, #0
	str	r3, [sp, #20]
	.loc 2 297 5
	ldrb	r1, [sp, #31]
	ldr	r2, [sp, #20]
	ldr	r3, [sp, #24]
	ldr	r0, [sp, #8]
	ldr	r4, [sp, #12]
	ldr	r5, [sp, #4]
	.syntax unified
@ 297 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic_internal.h" 1
	1:     ldrex   r3, [r4]
       ldr     r2, [r0]
       cmp     r3, r2
       ittee   eq
       strexeq r6, r5, [r4]
       moveq   r1, #1
       strexne r6, r3, [r4]
       strne   r3, [r0]
       cmp     r6, #0
       itt     ne
       movne   r1, #0
       bne.n   1b
@ 0 "" 2
	.thumb
	.syntax unified
	strb	r1, [sp, #31]
	str	r2, [sp, #20]
	str	r3, [sp, #24]
	str	r6, [sp, #16]
	.loc 2 324 12
	ldrb	r3, [sp, #31]	@ zero_extendqisi2
	.loc 2 325 1
	mov	r0, r3
	add	sp, sp, #36
	.cfi_def_cfa_offset 12
	@ sp needed
	pop	{r4, r5, r6}
	.cfi_restore 6
	.cfi_restore 5
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE176:
	.size	nrfx_atomic_internal_cmp_exch, .-nrfx_atomic_internal_cmp_exch
	.section	.text.nrfx_atomic_u32_fetch_store,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_fetch_store
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_fetch_store, %function
nrfx_atomic_u32_fetch_store:
.LFB177:
	.loc 1 55 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB2:
	.loc 1 61 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 61 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
mov r0, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE2:
	.loc 1 63 12
	ldr	r3, [sp, #20]
	.loc 1 71 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE177:
	.size	nrfx_atomic_u32_fetch_store, .-nrfx_atomic_u32_fetch_store
	.section	.text.nrfx_atomic_u32_store,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_store
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_store, %function
nrfx_atomic_u32_store:
.LFB178:
	.loc 1 74 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB3:
	.loc 1 81 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 81 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
mov r0, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE3:
	.loc 1 83 12
	ldr	r3, [sp, #16]
	.loc 1 90 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE178:
	.size	nrfx_atomic_u32_store, .-nrfx_atomic_u32_store
	.section	.text.nrfx_atomic_u32_fetch_or,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_fetch_or
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_fetch_or, %function
nrfx_atomic_u32_fetch_or:
.LFB179:
	.loc 1 93 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB4:
	.loc 1 99 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 99 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
orr r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE4:
	.loc 1 101 12
	ldr	r3, [sp, #20]
	.loc 1 109 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE179:
	.size	nrfx_atomic_u32_fetch_or, .-nrfx_atomic_u32_fetch_or
	.section	.text.nrfx_atomic_u32_or,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_or
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_or, %function
nrfx_atomic_u32_or:
.LFB180:
	.loc 1 112 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB5:
	.loc 1 118 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 118 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
orr r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE5:
	.loc 1 120 12
	ldr	r3, [sp, #16]
	.loc 1 128 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE180:
	.size	nrfx_atomic_u32_or, .-nrfx_atomic_u32_or
	.section	.text.nrfx_atomic_u32_fetch_and,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_fetch_and
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_fetch_and, %function
nrfx_atomic_u32_fetch_and:
.LFB181:
	.loc 1 131 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB6:
	.loc 1 137 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 137 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
and r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE6:
	.loc 1 139 12
	ldr	r3, [sp, #20]
	.loc 1 147 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE181:
	.size	nrfx_atomic_u32_fetch_and, .-nrfx_atomic_u32_fetch_and
	.section	.text.nrfx_atomic_u32_and,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_and
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_and, %function
nrfx_atomic_u32_and:
.LFB182:
	.loc 1 150 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB7:
	.loc 1 156 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 156 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
and r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE7:
	.loc 1 158 12
	ldr	r3, [sp, #16]
	.loc 1 166 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE182:
	.size	nrfx_atomic_u32_and, .-nrfx_atomic_u32_and
	.section	.text.nrfx_atomic_u32_fetch_xor,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_fetch_xor
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_fetch_xor, %function
nrfx_atomic_u32_fetch_xor:
.LFB183:
	.loc 1 169 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB8:
	.loc 1 175 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 175 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
eor r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE8:
	.loc 1 177 12
	ldr	r3, [sp, #20]
	.loc 1 185 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE183:
	.size	nrfx_atomic_u32_fetch_xor, .-nrfx_atomic_u32_fetch_xor
	.section	.text.nrfx_atomic_u32_xor,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_xor
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_xor, %function
nrfx_atomic_u32_xor:
.LFB184:
	.loc 1 188 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB9:
	.loc 1 194 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 194 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
eor r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE9:
	.loc 1 196 12
	ldr	r3, [sp, #16]
	.loc 1 204 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE184:
	.size	nrfx_atomic_u32_xor, .-nrfx_atomic_u32_xor
	.section	.text.nrfx_atomic_u32_fetch_add,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_fetch_add
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_fetch_add, %function
nrfx_atomic_u32_fetch_add:
.LFB185:
	.loc 1 207 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB10:
	.loc 1 213 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 213 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
add r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE10:
	.loc 1 215 12
	ldr	r3, [sp, #20]
	.loc 1 223 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE185:
	.size	nrfx_atomic_u32_fetch_add, .-nrfx_atomic_u32_fetch_add
	.section	.text.nrfx_atomic_u32_add,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_add
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_add, %function
nrfx_atomic_u32_add:
.LFB186:
	.loc 1 226 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB11:
	.loc 1 232 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 232 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
add r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE11:
	.loc 1 234 12
	ldr	r3, [sp, #16]
	.loc 1 242 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE186:
	.size	nrfx_atomic_u32_add, .-nrfx_atomic_u32_add
	.section	.text.nrfx_atomic_u32_fetch_sub,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_fetch_sub
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_fetch_sub, %function
nrfx_atomic_u32_fetch_sub:
.LFB187:
	.loc 1 245 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB12:
	.loc 1 251 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 251 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
sub r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE12:
	.loc 1 253 12
	ldr	r3, [sp, #20]
	.loc 1 261 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE187:
	.size	nrfx_atomic_u32_fetch_sub, .-nrfx_atomic_u32_fetch_sub
	.section	.text.nrfx_atomic_u32_sub,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_sub
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_sub, %function
nrfx_atomic_u32_sub:
.LFB188:
	.loc 1 264 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	str	r1, [sp]
.LBB13:
	.loc 1 270 5
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	.syntax unified
@ 270 "C:\Nrf_sdk\nRF5_SDK_17.1.0_ddde560\modules\nrfx\soc\nrfx_atomic.c" 1
	1:     ldrex   r4, [r3]
sub r0, r4, r2
       strex   r1, r0, [r3]
       teq     r1, #0
       bne.n     1b
@ 0 "" 2
	.thumb
	.syntax unified
	str	r4, [sp, #20]
	str	r0, [sp, #16]
	str	r1, [sp, #12]
.LBE13:
	.loc 1 272 12
	ldr	r3, [sp, #16]
	.loc 1 280 1
	mov	r0, r3
	add	sp, sp, #28
	.cfi_def_cfa_offset 4
	@ sp needed
	pop	{r4}
	.cfi_restore 4
	.cfi_def_cfa_offset 0
	bx	lr
	.cfi_endproc
.LFE188:
	.size	nrfx_atomic_u32_sub, .-nrfx_atomic_u32_sub
	.section	.text.nrfx_atomic_u32_cmp_exch,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_cmp_exch
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_cmp_exch, %function
nrfx_atomic_u32_cmp_exch:
.LFB189:
	.loc 1 285 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{lr}
	.cfi_def_cfa_offset 4
	.cfi_offset 14, -4
	sub	sp, sp, #20
	.cfi_def_cfa_offset 24
	str	r0, [sp, #12]
	str	r1, [sp, #8]
	str	r2, [sp, #4]
	.loc 1 294 12
	ldr	r2, [sp, #4]
	ldr	r1, [sp, #8]
	ldr	r0, [sp, #12]
	bl	nrfx_atomic_internal_cmp_exch
	mov	r3, r0
	.loc 1 311 1
	mov	r0, r3
	add	sp, sp, #20
	.cfi_def_cfa_offset 4
	@ sp needed
	ldr	pc, [sp], #4
	.cfi_endproc
.LFE189:
	.size	nrfx_atomic_u32_cmp_exch, .-nrfx_atomic_u32_cmp_exch
	.section	.text.nrfx_atomic_u32_fetch_sub_hs,"ax",%progbits
	.align	1
	.global	nrfx_atomic_u32_fetch_sub_hs
	.syntax unified
	.thumb
	.thumb_func
	.type	nrfx_atomic_u32_fetch_sub_hs, %function
nrfx_atomic_u32_fetch_sub_hs:
.LFB190:
	.loc 1 314 1
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4}
	.cfi_def_cfa_offset 4
	.cfi_offset 4, -4
	sub	sp, sp, #28
	.cfi_def_cfa_offset 32
	str	r0, [sp, #4]
	st