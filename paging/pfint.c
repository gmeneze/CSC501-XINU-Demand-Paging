/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
   /* Check if BS is assigned to currpid. If not fail else fetch pages from the BS, for PT? PD? 
      and add this page to the page table ? Page replacement in get_frm?, shouldn't this be done in get_frm?*/

    STATWORD ps;
    disable(ps);
    int backing_store, pageth;
    int status;
    pd_t *pd_entry;
    unsigned long vaddr; 

    vaddr = read_cr2();

    unsigned int pt_offset = (vaddr & 0x003FF000) >> 12;

    unsigned int pd_offset = vaddr >> 22;

    unsigned int offset = (vaddr & 0x00000FFF); 

    unsigned int vpno = vaddr >> 12;  // fetch top 20 bits

    status = bsm_lookup(currpid, vaddr, &backing_store, &pageth);

    if( status < 0 )
    {
       kprintf("pfint : Did not find backing store for vaddr \n");
       return SYSERR;
    }

    /* Check if PD entry is present */

    pd_entry = (pd_t *)(proctab[currpid].pdbr + pd_offset * sizeof(int));

    if( pd_entry->pd_pres == 0 )  
    {
       int frame, pt_frame, phys_page;
       int k;
       unsigned int phys_page_addr;
       unsigned int pt_base_addr;
       pt_t *pte_entry;
       

       frame = get_frm();  

       if ( frame < 0 )
       {
          kprintf("get_frm() failed! \n");
          return SYSERR; 
       }

       pd_entry->pd_pres = 1;
         
       pt_frame = FRAME0 + frame; 
       pd_entry->pd_base = pt_frame;        

       frm_tab[frame].fr_type = FR_TBL; 
       frm_tab[frame].fr_vpno = vpno;   // set vpno
       frm_tab[frame].fr_bs_id = backing_store; 
       pt_base_addr = pt_frame * NBPG;           /* The base adrress of new Page table */

       for(k=0; k<1024; k++)
       {
          pte_entry = (pt_t *)(pt_base_addr + k*sizeof(int));     
          pte_entry->pt_pres = 0;
          pte_entry->pt_write = 0;
          pte_entry->pt_base = 0;
       }

       pte_entry = (pt_t *)(pt_base_addr + pt_offset*sizeof(int));

       /* Mark the page as present in page table */
       pte_entry->pt_pres = 1; // page present
       pte_entry->pt_write = 1; // page is writable
       pte_entry->pt_user = 0;  // no use level protection
       pte_entry->pt_pwt = 0; // no write through for this page
       pte_entry->pt_pcd = 0; // no cache disable for this page
       pte_entry->pt_acc = 0; // page was NOT accessed
       pte_entry->pt_dirty = 0; // page was NOT written
       pte_entry->pt_mbz = 0; // is NOT zero
       pte_entry->pt_global = 0; // should be zero in 586
       pte_entry->pt_avail = 0; // for programmer's use 

       /* Map an empty frame to page table */
       frame = get_frm();

       if ( frame < 0 )
       {
          kprintf("get_frm() failed! \n");
          return SYSERR;
       }
 
       fifo_cnt++;  // incrementing fifo_cnt for pages

       frm_tab[frame].fr_type = FR_PAGE; 
       frm_tab[frame].fr_fifo_cnt = fifo_cnt;
       frm_tab[frame].fr_vpno = vpno;   
       frm_tab[frame].fr_bs_id = backing_store;

       phys_page = FRAME0 + frame; 
       pte_entry->pt_base = phys_page;   

       phys_page_addr = phys_page*4096; 

       read_bs((char *)phys_page_addr, backing_store, pageth);   

    }
    else
    {
       int frame, phys_page;
       pt_t *pte_entry; 
       unsigned int phys_page_addr;

          frame = get_frm();

          if ( frame < 0 )
          {
             kprintf("get_frm() failed! \n");
             return SYSERR;
          }

          pte_entry = (pt_t *)(pd_entry->pd_base*NBPG + pt_offset*sizeof(int));

          fifo_cnt++;  // increment count to assign to a page frame.

          frm_tab[frame].fr_type = FR_PAGE;
          frm_tab[frame].fr_fifo_cnt = fifo_cnt;          
          frm_tab[frame].fr_vpno = vpno;
          frm_tab[frame].fr_bs_id = backing_store; 
          phys_page = FRAME0 + frame;
          pte_entry->pt_base = phys_page;
          pte_entry->pt_pres = 1;

          phys_page_addr = phys_page*4096; 

          read_bs((char *)phys_page_addr, backing_store, pageth); 

       /* Code to handle PT fault */
    } 

    return OK;
}
