/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[NFRAMES]; // for each frame from 1024 to 2047

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
   int frame_index;

   for(frame_index = 0; frame_index < NFRAMES; frame_index++)
   {
      frm_tab[frame_index].fr_status = FRM_UNMAPPED;
      frm_tab[frame_index].fr_pid = 0;
      frm_tab[frame_index].fr_vpno = 0;
      frm_tab[frame_index].fr_refcnt = 0;
      frm_tab[frame_index].fr_bs_id = -1; 
      frm_tab[frame_index].fr_type = 0;   
      frm_tab[frame_index].fr_dirty = 0;
   } 

   return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
int get_frm()    //(int* avail)
{
   int scan = 0;
   int policy;
   int replace_frm;

   policy = grpolicy(); // fetch replacement policy

/* Implementation for LRU */
   if( policy == LRU || policy == GCM )
   {
      int frame_index = 0;
      unsigned int vpno;
      unsigned int pd_offset;
      unsigned int pt_offset;
      pd_t *pd_entry;
      pt_t *pt_entry; 

      for(frame_index = 0; frame_index < NFRAMES; frame_index++)     
      {
          if(frm_tab[frame_index].fr_status == FRM_MAPPED && frm_tab[frame_index].fr_type == FR_PAGE) 
          {
               vpno = frm_tab[frame_index].fr_vpno;
               pd_offset = (vpno>>10); 
               pt_offset = (vpno & 0x000003ff);

               pd_entry = (pd_t *)(proctab[currpid].pdbr + pd_offset*sizeof(int));  
               pt_entry = (pt_t *)(pd_entry->pd_base*NBPG + pt_offset*sizeof(int));

               if(pt_entry->pt_acc == 1)
               {
                     frm_tab[frame_index].fr_refcnt++;
                     pt_entry->pt_acc = 0; 
               }  
          }

      } 
   }

   while(frm_tab[scan].fr_status == FRM_MAPPED) // looking for unmapped frames
   {
       scan++;
   }
 
   if( scan < NFRAMES ) // map the frame
   {
      frm_tab[scan].fr_status = FRM_MAPPED;
      frm_tab[scan].fr_pid = currpid;
      frm_tab[scan].fr_vpno = 0;   // TO BE IMPLEMENTED
      frm_tab[scan].fr_refcnt = 1; // page is referenced once.
      frm_tab[scan].fr_bs_id = -1;
      frm_tab[scan].fr_fifo_cnt = 0;
   }
   else
   {
         int i;
         int frm_no;
 
         if( policy == FIFO )
         {
             unsigned int phys_page_addr;
             unsigned int virt_page_addr;
             int status, backing_store, pageth;
               
             for(i=1; i<=fifo_cnt; i++)
             {
                  for(frm_no=0;frm_no < NFRAMES; frm_no++)
                  {
                       if(frm_tab[frm_no].fr_status == FRM_MAPPED && frm_tab[frm_no].fr_type == FR_PAGE && frm_tab[frm_no].fr_fifo_cnt == i)
                       {
                             goto jump; 
                             //break; 
                       }
                  } 

             }    
  
             jump: 

             /* If vpno is greater than 4095, Write the frame back to the Backing Store */

             if( frm_tab[frm_no].fr_vpno > 4095 )
             {
                  phys_page_addr = (FRAME0 + frm_no) * 4096;
                  virt_page_addr = frm_tab[frm_no].fr_vpno * 4096;

                  status = bsm_lookup(frm_tab[frm_no].fr_pid, virt_page_addr, &backing_store, &pageth);

                  if( status < 0 )
                  {
                      return SYSERR;
                  }

                  write_bs((char *)phys_page_addr, backing_store, pageth); // write frame back to BS
             }

            /* return the frame */

             frm_tab[frm_no].fr_status = FRM_MAPPED;
             frm_tab[frm_no].fr_pid = currpid;
             frm_tab[frm_no].fr_vpno = 0;   // TO BE IMPLEMENTED
             frm_tab[frm_no].fr_refcnt = 1; // page is referenced once.
             frm_tab[frm_no].fr_bs_id = -1;
             frm_tab[frm_no].fr_fifo_cnt = 0;

             return frm_no;
         } 
         else
         {

             int frame_index = 0;
             unsigned int vpno;
             unsigned int pd_offset;
             unsigned int pt_offset;
             unsigned int phys_page_addr;
             unsigned int virt_page_addr;
             pd_t *pd_entry;
             pt_t *pt_entry;
             int min_refcnt = -1;
             int min_refcnt_frame = -1;
             int first_time_in_loop = 0; 
             int status;
             int backing_store;
             int pageth;

             for(frame_index = 0; frame_index < NFRAMES; frame_index++)
             {
                  if(frm_tab[frame_index].fr_status == FRM_MAPPED && frm_tab[frame_index].fr_type == FR_PAGE) 
                  {
                       vpno = frm_tab[frame_index].fr_vpno;
                       pd_offset = (vpno>>10);
                       pt_offset = (vpno & 0x000003ff);

                       pd_entry = (pd_t *)(proctab[currpid].pdbr + pd_offset*sizeof(int));
                       pt_entry = (pt_t *)(pd_entry->pd_base*NBPG + pt_offset*sizeof(int));

                       if(pt_entry->pt_acc == 0)
                       {
                            if(first_time_in_loop == 0) 
                            {
                                min_refcnt = frm_tab[frame_index].fr_refcnt;
                                min_refcnt_frame = frame_index;    
                                first_time_in_loop++; 
                            }

                            if( frm_tab[frame_index].fr_refcnt < min_refcnt ) 
                            {
                          //      if(frm_tab[frame_index].fr_refcnt == min_refcnt)
                          //      {
                          //          if(frm_tab[frame_index].fr_vpno > frm_tab[min_refcnt_frame].fr_vpno)
                          //          {
                          //              min_refcnt_frame = frame_index;               
                          //          } 
                          //      }  
                          //      else
                          //      {
                                     min_refcnt = frm_tab[frame_index].fr_refcnt;
                                     min_refcnt_frame = frame_index; 
                           //     }
                            }
                       }
           
                  }
              }

             /* If vpno is greater than 4095, Write the frame back to the Backing Store */

             if( frm_tab[min_refcnt_frame].fr_vpno > 4095 )
             {
                  phys_page_addr = (FRAME0 + frm_no) * 4096;
                  virt_page_addr = frm_tab[frm_no].fr_vpno * 4096;

                  status = bsm_lookup(frm_tab[frm_no].fr_pid, virt_page_addr, &backing_store, &pageth);

                  if( status < 0 )
                  {
                      return SYSERR;
                  }

                  write_bs((char *)phys_page_addr, backing_store, pageth); // write frame back to BS
             }

            /* return the frame */

             frm_tab[min_refcnt_frame].fr_status = FRM_MAPPED;
             frm_tab[min_refcnt_frame].fr_pid = currpid;
             frm_tab[min_refcnt_frame].fr_vpno = 0;   // TO BE IMPLEMENTED
             frm_tab[min_refcnt_frame].fr_refcnt = 1; // page is referenced once.
             frm_tab[min_refcnt_frame].fr_bs_id = -1;
             frm_tab[min_refcnt_frame].fr_fifo_cnt = 0;

             return min_refcnt_frame;

      }


   
  } 

  return scan;
}

void create_mapping(int frame_no) // To create PD mapping to PT's 1,2,3 & 4 for a new process
{

    int table_index;

    pd_t *proc_page_dir = (pd_t *)(frame_no*NBPG);

    for(table_index=0; table_index< NFRAMES; table_index++)
    {
          if( table_index < PT_NUM )
          {
              proc_page_dir->pd_pres = 1; // page table present
              proc_page_dir->pd_write = 1; // page is writable
              proc_page_dir->pd_user = 0;  // no use level protection
              proc_page_dir->pd_pwt = 0; // no write through cachine for pt
              proc_page_dir->pd_pcd = 0; // no cache disable for this pt
              proc_page_dir->pd_acc = 0; // page table was NOT accessed
              proc_page_dir->pd_mbz = 0; // not zero
              proc_page_dir->pd_fmb = 0; // not four MB pages
              proc_page_dir->pd_global = 0; // global (ignored)
              proc_page_dir->pd_avail = 0; // for programmer's use
              proc_page_dir->pd_base = 1025 + table_index; // location of page table, starts from page 1025 to 1028
           } 
           else
           {
              proc_page_dir->pd_pres = 0;
              proc_page_dir->pd_base = 0;
           }

          proc_page_dir++;
    } 

/*
    for(table_index=PT_NUM; table_index< NFRAMES; table_index++) // These PDE's are unmapped, no PT's present for these
    {
          proc_page_dir->pd_pres = 0; // page table present
          proc_page_dir->pd_write = 0; // page is writable
          proc_page_dir->pd_user = 0;  // no use level protection
          proc_page_dir->pd_pwt = 0; // no write through cachine for pt
          proc_page_dir->pd_pcd = 0; // no cache disable for this pt
          proc_page_dir->pd_acc = 0; // page table was NOT accessed
          proc_page_dir->pd_mbz = 0; // not zero
          proc_page_dir->pd_fmb = 0; // not four MB pages
          proc_page_dir->pd_global = 0; // global (ignored)
          proc_page_dir->pd_avail = 0; // for programmer's use
          proc_page_dir->pd_base = 0; // location of page table, starts from page 1025 to 1028

          proc_page_dir++;
    }
*/

}

/*----------------------------------------------------------------------------
 * write_back_dirty_frm - write back dirty frames to BS before context switch 
 *----------------------------------------------------------------------------
 */
SYSCALL write_back_dirty_frm(int old_pid, int bsid)
{
  int frm_no;
  int backing_store, pageth;
  unsigned long phys_page_addr;
  unsigned long virt_page_addr;

  int status;

  for( frm_no = 0; frm_no <= NFRAMES; frm_no++ )
  { 
     if( frm_tab[frm_no].fr_status == FRM_MAPPED ) 
     {
        if( frm_tab[frm_no].fr_type == FR_PAGE )
        {
           if( frm_tab[frm_no].fr_vpno > 4095 )
           {
              if( frm_tab[frm_no].fr_pid == old_pid )
              {
                      phys_page_addr = (FRAME0 + frm_no) * 4096;                     
                      virt_page_addr = frm_tab[frm_no].fr_vpno * 4096;

                      status = bsm_lookup(old_pid, virt_page_addr, &backing_store, &pageth);

                      if( status < 0 )
                      {
                          return SYSERR;
                      }   

                      if(backing_store == bsid || bsid == -1)
                      { 
                          write_bs((char *)phys_page_addr, backing_store, pageth);
                      }
              } 
    
           } 

        }
 
      }
   
   }
 
 return OK;
}

/*----------------------------------------------------------------------------
 * read_frames_from_bs - read frames from BS into physical frames 
 *----------------------------------------------------------------------------
 */
SYSCALL read_frames_from_bs()
{
  int frm_no;
  int backing_store, pageth;
  unsigned int phys_page_addr;
  unsigned int virt_page_addr;
  int status;

  for( frm_no = 0; frm_no <= NFRAMES; frm_no++ )
  {

     if( frm_tab[frm_no].fr_status == FRM_MAPPED )
     {

        if( frm_tab[frm_no].fr_vpno > 4095 )
        {

           if( frm_tab[frm_no].fr_type == FR_PAGE )
           {

              if( frm_tab[frm_no].fr_pid == currpid )
              {

                      phys_page_addr = (FRAME0 + frm_no) * 4096;
                      virt_page_addr = frm_tab[frm_no].fr_vpno * 4096;

                      status = bsm_lookup(currpid, virt_page_addr, &backing_store, &pageth);

                      if( status < 0 )
                      {
                          return SYSERR;
                      }  
 
                      char *ptr= BACKING_STORE_BASE + (backing_store<<19) + (pageth*NBPG);
      
                      read_bs((char *)phys_page_addr, backing_store, pageth);

                      *ptr=phys_page_addr;

              }

           }
        }

      }

   }

 return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
      frm_tab[i].fr_status = FRM_UNMAPPED;
      frm_tab[i].fr_pid = 0;
      frm_tab[i].fr_vpno = 0;
      frm_tab[i].fr_refcnt = -1;
      frm_tab[i].fr_type = 0;
      frm_tab[i].fr_dirty = 0;
      frm_tab[i].fr_bs_id = -1;
  
      return OK;
}
