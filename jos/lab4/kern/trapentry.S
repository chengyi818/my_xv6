/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

	TRAPHANDLER_NOEC(handler0, 0)
	TRAPHANDLER_NOEC(handler1, 1)
	TRAPHANDLER_NOEC(handler2, 2)
	TRAPHANDLER_NOEC(handler3, 3)
	TRAPHANDLER_NOEC(handler4, 4)
	TRAPHANDLER_NOEC(handler5, 5)
	TRAPHANDLER_NOEC(handler6, 6)
	TRAPHANDLER_NOEC(handler7, 7)
	TRAPHANDLER(handler8, 8)
	TRAPHANDLER(handler10, 10)
	TRAPHANDLER(handler11, 11)
	TRAPHANDLER(handler12, 12)
	TRAPHANDLER(handler13, 13)
	TRAPHANDLER(handler14, 14)
	TRAPHANDLER_NOEC(handler16, 16)
	TRAPHANDLER(handler17, 17)
	TRAPHANDLER_NOEC(handler18, 18)
	TRAPHANDLER_NOEC(handler19, 19)
	TRAPHANDLER_NOEC(handler32, 32)
	TRAPHANDLER_NOEC(handler33, 33)
	TRAPHANDLER_NOEC(handler34, 34)
	TRAPHANDLER_NOEC(handler35, 35)
	TRAPHANDLER_NOEC(handler36, 36)
	TRAPHANDLER_NOEC(handler37, 37)
	TRAPHANDLER_NOEC(handler38, 38)
	TRAPHANDLER_NOEC(handler39, 39)
	TRAPHANDLER_NOEC(handler40, 40)
	TRAPHANDLER_NOEC(handler41, 41)
	TRAPHANDLER_NOEC(handler42, 42)
	TRAPHANDLER_NOEC(handler43, 43)
	TRAPHANDLER_NOEC(handler44, 44)
	TRAPHANDLER_NOEC(handler45, 45)
	TRAPHANDLER_NOEC(handler46, 46)
	TRAPHANDLER_NOEC(handler47, 47)
	TRAPHANDLER_NOEC(handler48, 48)
	TRAPHANDLER_NOEC(handler51, 51)

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	# build trap frame
	pushl %ds;
	pushl %es;
	pushal ;

	# load GD_KD into %ds and %es
	movw $GD_KD, %ax;
	movw %ax, %ds;
	movw %ax, %es;

	# pushl %esp to pass a pointer to the Trapframe as an argument to trap()
	pushl %esp;

	# call trap
	call trap;
