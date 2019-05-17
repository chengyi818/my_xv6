#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver


	int perm;
	uint32_t req;
	envid_t who;
	while(1) {
		req = ipc_recv(&who, &nsipcbuf, &perm);
		/* cprintf("%x got %d from %x\n", sys_getenvid(), req, who); */
		if(req != NSREQ_OUTPUT) {
			cprintf("not a nsreq output\n");
			continue;
		}

		while(sys_pkt_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len) < 0) {
			sys_yield();
		}
	}
}
