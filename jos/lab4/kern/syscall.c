/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>

/* #define __DEBUG__ */
#include <inc/cydebug.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	/* panic("sys_exofork not implemented"); */

	struct Env* ep = NULL;
	int r = env_alloc(&ep, curenv->env_id);
	if(r < 0){
		return r;
	}

	ep->env_status = ENV_NOT_RUNNABLE;
	memcpy(&ep->env_tf, &curenv->env_tf, sizeof(struct Trapframe));
	ep->env_tf.tf_regs.reg_eax = 0;
	DEBUG("env_id: %d\n", ep->env_id);
	return ep->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	1. -E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	2. -E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	/* panic("sys_env_set_status not implemented"); */

	// 检查入参
	if(status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE) {
		DEBUG("1");
		return -E_INVAL;
	}

	struct Env* ep = NULL;
	int r = envid2env(envid, &ep, 1);
	if(r < 0) {
		DEBUG("2");
		return r;
	}
	ep->env_status = status;
	DEBUG("set envid: %d runnable\n", envid);
	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	1. -E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	/* panic("sys_env_set_pgfault_upcall not implemented"); */

	DEBUG("envid: %d, func: %p\n", envid, func);
	struct Env* ep = NULL;
	int r = envid2env(envid, &ep, 1);
	if(r < 0) {
		DEBUG("1");
		return r;
	}
	ep->env_pgfault_upcall = func;
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	1. -E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	2. -E_INVAL if va >= UTOP, or va is not page-aligned.
//	3. -E_INVAL if perm is inappropriate (see above).
//	4. -E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	// 检查入参
	// 1
	int r;
	struct Env* ep = NULL;
	r = envid2env(envid, &ep, 1);
	DEBUG("env_id: %d\n", ep->env_id);
	if(r < 0) {
		DEBUG("1 envid:%d\n", envid);
		return r;
	}

	// 2
	if((uintptr_t)va >= UTOP || (uintptr_t)va%PGSIZE != 0) {
		DEBUG("2");
		return -E_INVAL;
	}

	// 3
	if((perm&(PTE_U | PTE_P)) != (PTE_U | PTE_P)) {
		DEBUG("3");
		return -E_INVAL;
	}
	if((perm&~(PTE_U|PTE_P|PTE_AVAIL|PTE_W)) != 0) {
		DEBUG("3");
		return -E_INVAL;
	}
	// 4
	struct PageInfo* pp = page_alloc(ALLOC_ZERO);
	if(!pp) {
		DEBUG("4");
		return -E_NO_MEM;
	}
	r = page_insert(ep->env_pgdir, pp, va, perm);
	if(r < 0) {
		return r;
	}
	DEBUG("sys_page_alloc return 0\n");
	return 0;

	/* panic("sys_page_alloc not implemented"); */
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	1. -E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	2. -E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	3. -E_INVAL is srcva is not mapped in srcenvid's address space.
//	4. -E_INVAL if perm is inappropriate (see sys_page_alloc).
//	5. -E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	6. -E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.
	int r;
	pte_t* pte;
	struct PageInfo * pp;
	struct Env* esrc = NULL;
	struct Env* edst = NULL;
	// 1
	r = envid2env(srcenvid, &esrc, 1);
	if(r < 0) {
		return r;
	}
	r = envid2env(dstenvid, &edst, 1);
	if(r < 0) {
		return r;
	}

	// 2
	if((uintptr_t)srcva >= UTOP || (uintptr_t)srcva%PGSIZE != 0) {
		DEBUG("srcva: %p", srcva);
		return -E_INVAL;
	}
	if((uintptr_t)dstva >= UTOP || (uintptr_t)dstva%PGSIZE != 0) {
		DEBUG("2 2");
		return -E_INVAL;
	}

	// 3
	pp = page_lookup(esrc->env_pgdir, srcva, &pte);
	if(!pp) {
		DEBUG("3");
		return -E_INVAL;
	}

	// 4
	if((perm&(PTE_U | PTE_P)) != (PTE_U | PTE_P)) {
		DEBUG("4");
		return -E_INVAL;
	}
	if((perm&~(PTE_U|PTE_P|PTE_AVAIL|PTE_W)) != 0) {
		DEBUG("4");
		return -E_INVAL;
	}

	// 5
	if(perm&PTE_W) {
		if(!(*pte&PTE_W)) {
			DEBUG("5");
			return -E_INVAL;
		}
	}

	// 6
	r = page_insert(edst->env_pgdir, pp, dstva, perm);
	if(r < 0) {
		DEBUG("6");
		return r;
	}
	return 0;
	// LAB 4: Your code here.
	/* panic("sys_page_map not implemented"); */
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	1. -E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	2. -E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().
	int r;
	struct Env* ep = NULL;
	// 1
	r = envid2env(envid, &ep, 1);
	if(r < 0) {
		return r;
	}

	// 2
	if((uintptr_t)va >= UTOP || (uintptr_t)va%PGSIZE != 0) {
		return -E_INVAL;
	}

	page_remove(ep->env_pgdir, va);
	return 0;

	// LAB 4: Your code here.
	/* panic("sys_page_unmap not implemented"); */
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	1. -E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	2. -E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	3. -E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	4. -E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	5. -E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	6. -E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	7. -E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	int r;
	bool send_page = false;
	bool recive_page = false;
	struct Env* dstEnv = NULL;
	pte_t* pte = NULL;
	struct PageInfo* pp;

	r = envid2env(envid, &dstEnv, 0);
	// 1
	if(r < 0) {
		DEBUG("1");
		return r;
	}

	// 2
	if(dstEnv->env_ipc_recving != 1) {
		DEBUG("2");
		return -E_IPC_NOT_RECV;
	}
	if((uintptr_t)srcva < UTOP) {
		// 3
		if(PGOFF(srcva) != 0) {
			DEBUG("3");
			return -E_INVAL;
		}

		// 4
		if((perm&(PTE_U | PTE_P)) != (PTE_U | PTE_P)) {
			DEBUG("4_1");
			return -E_INVAL;
		}
		if((perm&~(PTE_U|PTE_P|PTE_AVAIL|PTE_W)) != 0) {
			DEBUG("4_2");
			return -E_INVAL;
		}

		pp = page_lookup(curenv->env_pgdir, srcva, &pte);
		// 5
		if(!pp) {
			DEBUG("5");
			return -E_INVAL;
		}
		// 6
		if((perm & PTE_W) && !(*pte & PTE_W)) {
			DEBUG("6");
			return -E_INVAL;
		}
		send_page = true;
	}

	if((uintptr_t)dstEnv->env_ipc_dstva < UTOP) {
		if(PGOFF(dstEnv->env_ipc_dstva) == 0) {
			recive_page = true;
		}
	}

	if(send_page && recive_page) {
		dstEnv->env_ipc_perm = perm;
		r = page_insert(dstEnv->env_pgdir, pp, dstEnv->env_ipc_dstva, perm);
		// 7
		if(r < 0) {
			DEBUG("7");
			return r;
		}
	} else {
		dstEnv->env_ipc_perm = 0;
	}

	dstEnv->env_ipc_recving = 0;
	dstEnv->env_ipc_from = curenv->env_id;
	dstEnv->env_ipc_value = value;
	dstEnv->env_tf.tf_regs.reg_eax = 0;
	dstEnv->env_status = ENV_RUNNABLE;

	return 0;
	// LAB 4: Your code here.
	/* panic("sys_ipc_try_send not implemented"); */
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	if((uintptr_t)dstva<UTOP) {
		if(PGOFF(dstva) != 0) {
			return -E_INVAL;
		}

		curenv->env_ipc_dstva = dstva;
	}

	// LAB 4: Your code here.
	curenv->env_ipc_recving = 1;
	curenv->env_status = ENV_NOT_RUNNABLE;

	sched_yield();
	/* panic("sys_ipc_recv not implemented"); */
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	/* panic("syscall not implemented"); */
	switch (syscallno) {
	case SYS_cputs:
		sys_cputs((char*)a1, a2);
		return 0;
	case SYS_cgetc:
		return sys_cgetc();
	case SYS_getenvid:
		return  sys_getenvid();
	case SYS_env_destroy:
		return sys_env_destroy(a1);
	case SYS_yield:
		sys_yield();
	case SYS_page_alloc:
		return sys_page_alloc(a1, (void*)a2, a3);
	case SYS_page_map:
		return sys_page_map(a1, (void*)a2, a3, (void*)a4, a5);
	case SYS_page_unmap:
		return sys_page_unmap(a1, (void*)a2);
	case SYS_exofork:
		return sys_exofork();
	case SYS_env_set_status:
		return sys_env_set_status(a1, a2);
	case SYS_env_set_pgfault_upcall:
		return sys_env_set_pgfault_upcall(a1, (void*)a2);
	case SYS_ipc_try_send:
		return sys_ipc_try_send(a1, a2, (void*)a3, a4);
	case SYS_ipc_recv:
		return sys_ipc_recv((void*)a1);
	default:
		return  -E_INVAL;
	}
}
