// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

/* #define __DEBUG__ */
#include <inc/cydebug.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	unsigned pn = (uintptr_t)addr/PGSIZE;
	pte_t* pte = (pte_t*)uvpt + pn;
	void* align_addr = (void*)ROUNDDOWN(addr, PGSIZE);

	DEBUG("err: 0x%08x, addr: %p\n", err, addr);
	if(!(err& FEC_WR))
		panic("Not a write protect page fault\n");
	if(!(*pte&PTE_COW)) {
		panic("Not a PTE_COW page\n");
	}

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	r=sys_page_alloc(0, PFTEMP, PTE_U|PTE_W|PTE_P);
	if(r < 0)
		panic("sys_page_alloc: %e", r);
	memmove(PFTEMP, align_addr, PGSIZE);
	if((r=sys_page_map(0, PFTEMP, 0, align_addr,
					   PTE_U|PTE_W|PTE_P)) < 0)
		panic("sys_page_map: %e", r);

	// LAB 4: Your code here.

	/* panic("pgfault not implemented"); */
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 4: Your code here.
	/* panic("duppage not implemented"); */
	/* DEBUG("dumpage reutrn 0\n"); */
	int r;
	uintptr_t vaddr = pn*PGSIZE;
	pte_t* pte = (pte_t*)uvpt + pn;
	if(!(*pte & PTE_P))
		return 0;
	if(pn == (UXSTACKTOP-PGSIZE)/PGSIZE)
		return 0;

	/* DEBUG("dumpage envid: %d pn: %d\n", envid, pn); */
	if(*pte & PTE_SHARE) {
		DEBUG("duppage pn: %u\n", pn);
		if((r=sys_page_map(0, (void*)vaddr, envid, (void*)vaddr,
				   ((*pte)&PTE_SYSCALL)|PTE_SHARE)) < 0) {
			panic("sys_page_map: %e", r);
		}
	} else if((*pte & PTE_W) || (*pte & PTE_COW)) {
		if((r=sys_page_map(0, (void*)vaddr, envid, (void*)vaddr,
						   PTE_U | PTE_P | PTE_COW)) < 0) {
			panic("sys_page_map: %e", r);
		}

		if((r=sys_page_map(0, (void*)vaddr, 0, (void*)vaddr,
						   PTE_U | PTE_P | PTE_COW)) < 0) {
			panic("sys_page_map: %e", r);
		}
	} else {
		// PTE_P
		if((r=sys_page_map(0, (void*)vaddr, envid, (void*)vaddr,
						   PGOFF(*pte))) < 0) {
			panic("sys_page_map: %e", r);
		}
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// 1. Set up our page fault handler appropriately.
// 2. Create a child.
// 3. Copy our address space
// 4. and page fault handler setup to the child.
// 5. Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   6. Use uvpd, uvpt, and duppage.
//   7. Remember to fix "thisenv" in the child process.
//   8. Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	/* panic("fork not implemented"); */
	int r;
	uint32_t pdeno, pteno;
	envid_t envid;

	// 1
	if(!thisenv->env_pgfault_upcall) {
		set_pgfault_handler(pgfault);
	}

	// 2
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		// We're the child.

		// 7
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		envid_t env_id = sys_getenvid();
		thisenv = &envs[ENVX(env_id)];
		DEBUG("child: %d\n", thisenv->env_id);

	}

	// We're the parent
	// 3
	for(pdeno=0; pdeno<PDX(UTOP); pdeno++) {
		pde_t* p = (pde_t*)uvpd + pdeno;
		if(!(*p & PTE_P))
			continue;

		// 6
		for(pteno=0; pteno<1024; pteno++) {
			unsigned pn = 1024*pdeno+pteno;

			if((r = duppage(envid, pn)) < 0)
				panic("duppage: %e", r);
		}
	}

	// 8
	if((r=sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE),
						 PTE_U|PTE_P|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	// 4
	sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);

	// 5
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
