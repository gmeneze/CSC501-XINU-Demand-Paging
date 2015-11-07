#include <conf.h>
#include <kernel.h>
#include <proc.h>

bs_map_t bsm_tab[16];

int get_bs(bsd_t bs_id, unsigned int npages) {

    if( npages > 100 || npages == 0 )
    {
        return SYSERR; 
    }  

    if( bsm_tab[bs_id].bs_status == BSM_MAPPED )
    {
        if( bsm_tab[bs_id].bs_sem == 1 )
        {
            return -1; 
        }

        bs_map_t *bsm_struct_curr = NULL;

        bsm_struct_curr = &bsm_tab[bs_id];

        while(bsm_struct_curr->next != NULL) 
        {
            bsm_struct_curr = bsm_struct_curr->next;     
        }
        
        bsm_struct_curr->next = &(proctab[currpid].bs_list[bs_id]);
  
        proctab[currpid].bs_list[bs_id].bs_status = BSM_MAPPED;
        proctab[currpid].bs_list[bs_id].bs_pid = currpid;
        proctab[currpid].bs_list[bs_id].bs_npages = bsm_tab[bs_id].bs_npages; 
        proctab[currpid].bs_list[bs_id].bs_vpno = 0;

    }
    else
    {

        bsm_tab[bs_id].bs_status = BSM_MAPPED;
        bsm_tab[bs_id].bs_pid = currpid;
        bsm_tab[bs_id].bs_npages = npages; 
        bsm_tab[bs_id].bs_vpno = 0; // This will be set in xmmap 

        proctab[currpid].bs_list[bs_id] = bsm_tab[bs_id];

    } 

    return bsm_tab[bs_id].bs_npages;
}
