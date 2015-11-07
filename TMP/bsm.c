/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>


bs_map_t bsm_tab[16];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
   int bs_index;

   for(bs_index = 0; bs_index <= MAX_ID; bs_index++)
   {
      bsm_tab[bs_index].bs_status = BSM_UNMAPPED;
      bsm_tab[bs_index].bs_pid = 0;
      bsm_tab[bs_index].bs_vpno = 0;
      bsm_tab[bs_index].bs_npages = 0;
      bsm_tab[bs_index].bs_sem = 0;
   }

   return OK;    
 
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
int get_bsm(int hsize)      //(int* avail) 
{
   int scan = 0;

   while(bsm_tab[scan].bs_status == BSM_MAPPED) // looking for unmapped frames
   {
       scan++;
   }

   if( scan <= MAX_ID ) // map the frame, here scan is the BS number
   {

      bsm_tab[scan].bs_status = BSM_MAPPED;
      bsm_tab[scan].bs_pid = currpid;
      bsm_tab[scan].bs_vpno = 0;
      bsm_tab[scan].bs_npages = hsize;
      bsm_tab[scan].bs_sem = 0;
   }
   else
   {
      return -1; // This should be a case of failure. No BS available
   }

  return scan; // return BS number

}

/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{

   bsm_tab[i].bs_status = BSM_UNMAPPED;
   bsm_tab[i].bs_pid = 0;
   bsm_tab[i].bs_vpno = 0;
   bsm_tab[i].bs_npages = 0;
   bsm_tab[i].bs_sem = 0;
   bsm_tab[i].next = NULL;

}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, unsigned long vaddr, int* store, int* pageth)
{
/* Check if the input address is present in one of the BS of the process */

  int bs_id;
  int page_no; 

  for( bs_id = 0; bs_id <= MAX_ID; bs_id++ )
  {
      if( (proctab[pid].bs_list[bs_id].bs_status == BSM_MAPPED) && (proctab[pid].bs_list[bs_id].bs_vpno * NBPG <= vaddr) 
          && ((proctab[pid].bs_list[bs_id].bs_vpno + proctab[pid].bs_list[bs_id].bs_npages) * NBPG >= vaddr)) 
      {
               *store = bs_id;

               for(page_no = proctab[pid].bs_list[bs_id].bs_vpno; page_no < (proctab[pid].bs_list[bs_id].bs_vpno + proctab[pid].bs_list[bs_id].bs_npages); page_no ++)
               {
                       if( vaddr < page_no * NBPG )
                       {
                           *pageth = (page_no - proctab[pid].bs_list[bs_id].bs_vpno) - 1;

                           return OK;
                       }
                
                       if( vaddr == page_no * NBPG )
                       {
                           *pageth =  page_no - proctab[pid].bs_list[bs_id].bs_vpno;

                           return OK;
                       }
               } 
      }

  }

  return SYSERR; 

}

/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
}

/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
}


