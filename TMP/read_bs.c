#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>

SYSCALL read_bs(char *dst, bsd_t bs_id, int page) {

  /* fetch page page from map map_id
     and write beginning at dst.
  */

   void *phy_addr = BACKING_STORE_BASE + (bs_id<<19) + (page*NBPG);

   unsigned long address_val = BACKING_STORE_BASE + (bs_id<<19) + (page*NBPG);

   bcopy(phy_addr, (void*)dst, NBPG);
}
