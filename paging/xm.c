/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>

bs_map_t bsm_tab[16];
/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages) // Ask Raunaq about npages
{

  /* sanity check ! */
   if ((virtpage < 4096) || ( source < 0 ) || ( source > MAX_ID) ||(npages < 1) || ( npages >100)){
        kprintf("xmmap call error: parameter error! \n");
        return SYSERR;
   }


   if( proctab[currpid].bs_list[source].bs_npages < npages )
   {
        return SYSERR; 
   }

   if( proctab[currpid].bs_list[source].bs_status == BSM_MAPPED ) /* check given BS is mapped to this process */
   {
         if( bsm_tab[source].bs_pid == currpid ) /* first entry in bsm_tab is not linked to proctab */
              bsm_tab[source].bs_vpno = virtpage;
          
         proctab[currpid].bs_list[source].bs_vpno = virtpage; 

         return OK;  
    }

   return SYSERR;

}

/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage ) /* PENDING - need to unmap from from page table. */
{
  int bs_id;
  int frame_index;
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
	kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
	return SYSERR;
  }

  for(bs_id = 0; bs_id < MAX_ID; bs_id++) 
  {
        if(proctab[currpid].bs_list[bs_id].bs_vpno == virtpage && proctab[currpid].bs_list[bs_id].bs_status == BSM_MAPPED ) 
        {
                write_back_dirty_frm(currpid,bs_id);

                if( bsm_tab[bs_id].bs_pid == currpid )  // first entry in bsm_tab is not linked to proctab, so UNMMAP in both
                {

                      bsm_tab[bs_id].bs_vpno = 0; 
                      proctab[currpid].bs_list[bs_id].bs_vpno = 0;

                      for(frame_index = 0; frame_index < NFRAMES; frame_index++)
                      {

                           if( frm_tab[frame_index].fr_pid == currpid )
                           {

                               if(frm_tab[frame_index].fr_bs_id == bs_id)
                               {
                                      if(frm_tab[frame_index].fr_type == FR_PAGE)
                                      {
                                           free_frm(frame_index);
                                      }
                               }

                            }

                      }

                      return OK; 
                }

                proctab[currpid].bs_list[bs_id].bs_vpno = 0;

               for(frame_index = 0; frame_index < NFRAMES; frame_index++)
               {

                    if( frm_tab[frame_index].fr_pid == currpid )
                    {

                         if(frm_tab[frame_index].fr_bs_id == bs_id)
                         {

                               if(frm_tab[frame_index].fr_type == FR_PAGE)
                               {
                                    free_frm(frame_index);
                               }
                         }

                     }

                }


               return OK;
        }
  } 

  return SYSERR;
}

