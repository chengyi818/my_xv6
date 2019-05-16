#include <kern/e1000.h>
#include <kern/pci.h>

int e1000_attchfn(struct pci_func *pcif) {
	pci_func_enable(pcif);
	return 0;
}

// LAB 6: Your driver code here
