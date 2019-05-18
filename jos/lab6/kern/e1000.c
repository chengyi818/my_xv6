#include <kern/e1000.h>
#include <kern/pci.h>
#include <inc/assert.h>
#include <kern/pmap.h>
#include <inc/string.h>

#define __DEBUG__
#include <inc/cydebug.h>

volatile void *bar_va;

struct e1000_tdh *tdh;
struct e1000_tdt *tdt;
struct e1000_tx_desc tx_desc_array[TXDESCS];
char tx_buffer_array[TXDESCS][TX_PKT_SIZE];

struct e1000_rdh *rdh;
struct e1000_rdt *rdt;
struct e1000_rx_desc rx_desc_array[RXDESCS];
char rx_buffer_array[RXDESCS][RX_PKT_SIZE];

int e1000_attchfn(struct pci_func *pcif) {
	pci_func_enable(pcif);
	DEBUG("base[0]: 0x%x, size[0]: %d\n",
	      pcif->reg_base[0], pcif->reg_size[0]);

	bar_va = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

	uint32_t *status_reg = (uint32_t *)E1000REG(E1000_STATUS);
	assert(*status_reg == 0x80080783);
	DEBUG("status reg: 0x%x", *status_reg);

	e1000_transmit_init();
	e1000_receive_init();

	/*
	 * transmit test
	 */
	/* char *data = "transmit test"; */
	/* e1000_transmit(data, strlen(data)); */

	return 0;
}

static void
e1000_transmit_init()
{
	int i;
	for (i = 0; i < TXDESCS; i++) {
		tx_desc_array[i].addr = PADDR(tx_buffer_array[i]);
		tx_desc_array[i].cmd = 0;
		tx_desc_array[i].status |= E1000_TXD_STAT_DD;
	}

	struct e1000_tdlen *tdlen = (struct e1000_tdlen *)E1000REG(E1000_TDLEN);
	tdlen->len = TXDESCS;

	uint32_t *tdbal = (uint32_t *)E1000REG(E1000_TDBAL);
	*tdbal = PADDR(tx_desc_array);

	uint32_t *tdbah = (uint32_t *)E1000REG(E1000_TDBAH);
	*tdbah = 0;

	tdh = (struct e1000_tdh *)E1000REG(E1000_TDH);
	tdh->tdh = 0;

	tdt = (struct e1000_tdt *)E1000REG(E1000_TDT);
	tdt->tdt = 0;

	struct e1000_tctl *tctl = (struct e1000_tctl *)E1000REG(E1000_TCTL);
	tctl->en = 1;
	tctl->psp = 1;
	tctl->ct = 0x10;
	tctl->cold = 0x40;

	struct e1000_tipg *tipg = (struct e1000_tipg *)E1000REG(E1000_TIPG);
	tipg->ipgt = 10;
	tipg->ipgr1 = 4;
	tipg->ipgr2 = 6;
}

int
e1000_transmit(void *data, size_t len)
{
	uint32_t current = tdt->tdt;
	if(!(tx_desc_array[current].status & E1000_TXD_STAT_DD)) {
		return -E_TRANSMIT_RETRY;
	}
	tx_desc_array[current].length = len;
	tx_desc_array[current].status &= ~E1000_TXD_STAT_DD;
	tx_desc_array[current].cmd |= (E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS);
	memcpy(tx_buffer_array[current], data, len);
	uint32_t next = (current + 1) % TXDESCS;
	tdt->tdt = next;
	return 0;
}

static void
e1000_receive_init()
{
	uint32_t *rdbal = (uint32_t *)E1000REG(E1000_RDBAL);
	uint32_t *rdbah = (uint32_t *)E1000REG(E1000_RDBAH);
	*rdbal = PADDR(rx_desc_array);
	*rdbah = 0;

	int i;
	for (i = 0; i < RXDESCS; i++) {
		rx_desc_array[i].addr = PADDR(rx_buffer_array[i]);
	}

	struct e1000_rdlen *rdlen = (struct e1000_rdlen *)E1000REG(E1000_RDLEN);
	rdlen->len = RXDESCS;

	rdh = (struct e1000_rdh *)E1000REG(E1000_RDH);
	rdt = (struct e1000_rdt *)E1000REG(E1000_RDT);
	rdh->rdh = 0;
	rdt->rdt = RXDESCS-1;

	uint32_t *rctl = (uint32_t *)E1000REG(E1000_RCTL);
	*rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC;

	// hardcode mac address
	uint32_t *ra = (uint32_t *)E1000REG(E1000_RA);
	ra[0] = 0x12005452;
	ra[1] = 0x5634 | E1000_RAH_AV;
}

int
e1000_receive(void *addr, size_t *len)
{
	static int32_t next = 0;
	if(!(rx_desc_array[next].status & E1000_RXD_STAT_DD)) {
		return -E_RECEIVE_RETRY;
	}
	if(rx_desc_array[next].errors) {
		cprintf("receive errors\n");
		return -E_RECEIVE_RETRY;
	}
	*len = rx_desc_array[next].length;
	memcpy(addr, rx_buffer_array[next], *len);

	rdt->rdt = (rdt->rdt + 1) % RXDESCS;
	next = (next + 1) % RXDESCS;
	return 0;
}

// LAB 6: Your driver code here
