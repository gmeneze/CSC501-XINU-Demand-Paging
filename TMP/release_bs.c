#include <conf.h>
#include <kernel.h>
#include <proc.h>

bs_map_t bsm_tab[16];

SYSCALL release_bs(bsd_t bs_id) {

   /* Initialize bsm_tab for the bs_id */

   int status;

   status = unmap_bs(bs_id, currpid); 
  
   if(status < 0)
   {
      kprintf("Failed in umap_bs \n");
      return SYSERR;
   }

/*
        if( proctab[currpid].bs_list[bs_id].bs_status == BSM_MAPPED )
        {
                if( bsm_tab[bs_id].bs_pid == currpid )  // first entry in bsm_tab is not linked to proctab, so UNMMAP in both
                {
                      bsm_tab[bs_id].bs_status = BSM_UNMAPPED;

                      proctab[currpid].bs_list[bs_id].bs_status = BSM_UNMAPPED;  // unmap the BS from the process

                      if( bsm_tab[bs_id].next != NULL )
                      {
                           bsm_tab[bs_id] = *(bsm_tab[bs_id].next);
                      }
                      else
                      {
                           free_bsm(bs_id);       // release the backing store, no process is using it.
                      }

                      return OK;
                }

                bs_map_t *bsm_struct_curr = NULL;
                bs_map_t *bsm_struct_prev = NULL;

                bsm_struct_curr = &bsm_tab[bs_id];

                while( bsm_struct_curr != NULL )
                {
                     if( bsm_struct_curr->bs_pid == currpid )
                     {
                          bsm_struct_prev->next = bsm_struct_curr->next;
                          bsm_struct_curr->bs_status = BSM_UNMAPPED;
                          return OK;
                     }

                     bsm_struct_prev = bsm_struct_curr;

                     bsm_struct_curr = bsm_struct_curr->next;
                }

        }

  kprintf("xmunmap error: BS is not mapped to this process \n");
  return SYSERR;

*/

return OK;
}

SYSCALL unmap_bs(bsd_t bs_id, int process_id)
{

        if( proctab[process_id].bs_list[bs_id].bs_status == BSM_MAPPED )
        {
                if( bsm_tab[bs_id].bs_pid == process_id )  // first entry in bsm_tab is not linked to proctab, so UNMMAP in both
                {
                      bsm_tab[bs_id].bs_status = BSM_UNMAPPED;

                      proctab[process_id].bs_list[bs_id].bs_status = BSM_UNMAPPED;  // unmap the BS from the process

                      if( bsm_tab[bs_id].next != NULL )
                      {
                           bsm_tab[bs_id] = *(bsm_tab[bs_id].next);
                      }
                      else
                      {
                           free_bsm(bs_id);       // release the backing store, no process is using it.
                      }

                      return OK;
                }

                bs_map_t *bsm_struct_curr = NULL;
                bs_map_t *bsm_struct_prev = NULL;

                bsm_struct_curr = &bsm_tab[bs_id];

                while( bsm_struct_curr != NULL )
                {
                     if( bsm_struct_curr->bs_pid == process_id )
                     {
                          bsm_struct_prev->next = bsm_struct_curr->next;
                          bsm_struct_curr->bs_status = BSM_UNMAPPED;
                          return OK;
                     }

                     bsm_struct_prev = bsm_struct_curr;

                     bsm_struct_curr = bsm_struct_curr->next;
                }

        }

  kprintf("xmunmap error: BS is not mapped to this process \n");
  return SYSERR;
}
