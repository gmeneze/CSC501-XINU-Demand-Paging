/* initialize.c - nulluser, sizmem, sysinit */

#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <sleep.h>
#include <mem.h>
#include <tty.h>
#include <q.h>
#include <io.h>

/*#define DETAIL */
#define HOLESIZE	(600)	
#define	HOLESTART	(640 * 1024)
#define	HOLEEND		((1024 + HOLESIZE) * 1024)  
/* Extra 600 for bootp loading, and monitor */

extern	int	main();	/* address of user's main prog	*/

extern	int	start();

LOCAL		sysinit();

/* Declarations of major kernel variables */
struct	pentry	proctab[NPROC]; /* process table			*/
int	nextproc;		/* next process slot to use in create	*/
struct	sentry	semaph[NSEM];	/* semaphore table			*/
int	nextsem;		/* next sempahore slot to use in screate*/
struct	qent	q[NQENT];	/* q table (see queue.c)		*/
int	nextqueue;		/* next slot in q structure to use	*/
char	*maxaddr;		/* max memory address (set by sizmem)	*/
struct	mblock	memlist;	/* list of free memory blocks		*/
#ifdef	Ntty
struct  tty     tty[Ntty];	/* SLU buffers and mode control		*/
#endif

/* active system status */
int	numproc;		/* number of live user processes	*/
int	currpid;		/* id of currently running process	*/
int	reboot = 0;		/* non-zero after first boot		*/

int	rdyhead,rdytail;	/* head/tail of ready list (q indicies)	*/
char 	vers[80];
int	console_dev;		/* the console device			*/



/*  added for the demand paging */
int page_replace_policy = FIFO;

/* Glen's structures & constants */
SYSCALL pfintr();
void enable_paging();
/* End of Glen's structures */


/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED, and      ***/
/***   must eventually be enabled explicitly.  This routine turns     ***/
/***   itself into the null process after initialization.  Because    ***/
/***   the null process must always remain ready to run, it cannot    ***/
/***   execute code that might cause it to be suspended, wait for a   ***/
/***   semaphore, or put to sleep, or exit.  In particular, it must   ***/
/***   not do I/O unless it uses kprintf for polled output.           ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  nulluser  -- initialize system and become the null process (id==0)
 *------------------------------------------------------------------------
 */
nulluser()				/* babysit CPU when no one is home */
{
        int userpid;
  
        int pdbr_null_proc;

	console_dev = SERIAL0;		/* set console to COM0 */

	initevec();

	kprintf("system running up!\n");
	sysinit();

        fifo_cnt = 0; // initialize frm_cnt used in FIFO replacement policy

        init_bsm();

        init_frm();

	enable();		/* enable interrupts */

        pdbr_null_proc = initialize_frames();

        set_evec(14,pfintr);

        proctab[currpid].pdbr = pdbr_null_proc; /* Set PDBR for null process */

        write_cr3(pdbr_null_proc); /* Set PDBR for null process */

        enable_paging();

	sprintf(vers, "PC Xinu %s", VERSION);
	kprintf("\n\n%s\n", vers);
	if (reboot++ < 1)
		kprintf("\n ");
	else
		kprintf("   (reboot %d)\n", reboot);

	kprintf("%d bytes real mem\n",
		(unsigned long) maxaddr+1);

#ifdef DETAIL	
	kprintf("    %d", (unsigned long) 0);
	kprintf(" to %d\n", (unsigned long) (maxaddr) );
#endif	

	kprintf("%d bytes Xinu code\n",
		(unsigned long) ((unsigned long) &end - (unsigned long) start));
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) start);
	kprintf(" to %d\n", (unsigned long) &end );
#endif

#ifdef DETAIL	
	kprintf("%d bytes user stack/heap space\n",
		(unsigned long) ((unsigned long) maxaddr - (unsigned long) &end));
	kprintf("    %d", (unsigned long) &end);
	kprintf(" to %d\n", (unsigned long) maxaddr);
#endif	
	
	kprintf("clock %sabled\n", clkruns == 1?"en":"dis");


	/* create a process to execute the user's main program */

        kprintf(" before calling main \n");
	userpid = create(main,INITSTK,INITPRIO,INITNAME,INITARGS);
        kprintf(" after calling main \n"); 
	resume(userpid);

	while (TRUE)
		/* empty */;
}

int initialize_frames()
{

	pd_t *null_proc_page_dir = (pd_t *)(FRAME0*NBPG);
	pt_t *page_table = (pt_t *)((FRAME0+1)*NBPG);
        int scan=0;
        int table_index;

        for(table_index=0; table_index<NFRAMES; table_index++)
        {

           if(table_index < PT_NUM)
           { 
               null_proc_page_dir->pd_pres = 1; // page table present
               null_proc_page_dir->pd_write = 1; // page is writable
               null_proc_page_dir->pd_user = 0;  // no use level protection
               null_proc_page_dir->pd_pwt = 0; // no write through cachine for pt
               null_proc_page_dir->pd_pcd = 0; // no cache disable for this pt
               null_proc_page_dir->pd_acc = 0; // page table was NOT accessed
               null_proc_page_dir->pd_mbz = 0; // not zero 
               null_proc_page_dir->pd_fmb = 0; // not four MB pages 
               null_proc_page_dir->pd_global = 0; // global (ignored) 
               null_proc_page_dir->pd_avail = 0; // for programmer's use 
               null_proc_page_dir->pd_base = 1025 + table_index; // location of page table, starts from page 1025 to 1028
 
           } 
           else
           {
               null_proc_page_dir->pd_pres = 0;
               null_proc_page_dir->pd_base = 0;
           }

           null_proc_page_dir++;

        } 


        frm_tab[scan].fr_status = FRM_MAPPED;
        frm_tab[scan].fr_pid = currpid;
        frm_tab[scan].fr_vpno = scan;
        frm_tab[scan].fr_refcnt = 1;
        frm_tab[scan].fr_type = FR_DIR;
        frm_tab[scan].fr_dirty = 0;
        frm_tab[scan].fr_bs_id = -1;

        for(table_index=0;table_index<PT_NUM;table_index++) 
        {
             for(scan=0;scan<NFRAMES;scan++)
             {
                 page_table->pt_pres = 1; // page present
                 page_table->pt_write = 1; // page is writable
                 page_table->pt_user = 0;  // no use level protection
                 page_table->pt_pwt = 0; // no write through for this page
                 page_table->pt_pcd = 0; // no cache disable for this page
                 page_table->pt_acc = 0; // page was NOT accessed
                 page_table->pt_dirty = 0; // page was NOT written
                 page_table->pt_mbz = 0; // is NOT zero 
                 page_table->pt_global = 0; // should be zero in 586 
                 page_table->pt_avail = 0; // for programmer's use 
                 page_table->pt_base = table_index*NFRAMES + scan; // location of page 
        
                 page_table++;
             }
        
        }

        for(scan=1; scan <= PT_NUM; scan++) // set frm_tab for Page table frames
        { 
            frm_tab[scan].fr_status = FRM_MAPPED;
            frm_tab[scan].fr_pid = currpid;
            frm_tab[scan].fr_vpno = scan;
            frm_tab[scan].fr_refcnt = 1;
            frm_tab[scan].fr_type = FR_TBL;
            frm_tab[scan].fr_dirty = 0;
            frm_tab[scan].fr_bs_id = -1;

        }

        return  FRAME0*NBPG;

}
/*------------------------------------------------------------------------
 *  sysinit  --  initialize all Xinu data structeres and devices
 *------------------------------------------------------------------------
 */
LOCAL
sysinit()
{
	static	long	currsp;
	int	i,j;
	struct	pentry	*pptr;
	struct	sentry	*sptr;
	struct	mblock	*mptr;
	SYSCALL pfintr();

	

	numproc = 0;			/* initialize system variables */
	nextproc = NPROC-1;
	nextsem = NSEM-1;
	nextqueue = NPROC;		/* q[0..NPROC-1] are processes */

	/* initialize free memory list */
	/* PC version has to pre-allocate 640K-1024K "hole" */
	if (maxaddr+1 > HOLESTART) {
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = (struct mblock *)HOLEEND;
		mptr->mlen = (int) truncew(((unsigned) HOLESTART -
	     		 (unsigned)&end));
        mptr->mlen -= 4;

		mptr = (struct mblock *) HOLEEND;
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - HOLEEND -
	      		NULLSTK);
/*
		mptr->mlen = (int) truncew((unsigned)maxaddr - (4096 - 1024 ) *  4096 - HOLEEND - NULLSTK);
*/
	} else {
		/* initialize free memory list */
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - (int)&end -
			NULLSTK);
	}
	

	for (i=0 ; i<NPROC ; i++)	/* initialize process table */
		proctab[i].pstate = PRFREE;


#ifdef	MEMMARK
	_mkinit();			/* initialize memory marking */
#endif

#ifdef	RTCLOCK
	clkinit();			/* initialize r.t.clock	*/
#endif

	mon_init();     /* init monitor */

#ifdef NDEVS
	for (i=0 ; i<NDEVS ; i++ ) {	    
	    init_dev(i);
	}
#endif

	pptr = &proctab[NULLPROC];	/* initialize null process entry */
	pptr->pstate = PRCURR;
	for (j=0; j<7; j++)
		pptr->pname[j] = "prnull"[j];
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK;
	pptr->pbase = (WORD) maxaddr - 3;
/*
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK - (4096 - 1024 )*4096;
	pptr->pbase = (WORD) maxaddr - 3 - (4096-1024)*4096;
*/
	pptr->pesp = pptr->pbase-4;	/* for stkchk; rewritten before used */
	*( (int *)pptr->pbase ) = MAGIC;
	pptr->paddr = (WORD) nulluser;
	pptr->pargs = 0;
	pptr->pprio = 0;
	currpid = NULLPROC;

	for (i=0 ; i<NSEM ; i++) {	/* initialize semaphores */
		(sptr = &semaph[i])->sstate = SFREE;
		sptr->sqtail = 1 + (sptr->sqhead = newqueue());
	}

	rdytail = 1 + (rdyhead=newqueue());/* initialize ready list */


	return(OK);
}

stop(s)
char	*s;
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* empty */;
}

delay(n)
int	n;
{
	DELAY(n);
}


#define	NBPG	4096

/*------------------------------------------------------------------------
 * sizmem - return memory size (in pages)
 *------------------------------------------------------------------------
 */
long sizmem()
{
	unsigned char	*ptr, *start, stmp, tmp;
	int		npages;

	/* at least now its hacked to return
	   the right value for the Xinu lab backends (16 MB) */

	return 4096; 

	start = ptr = 0;
	npages = 0;
	stmp = *start;
	while (1) {
		tmp = *ptr;
		*ptr = 0xA5;
		if (*ptr != 0xA5)
			break;
		*ptr = tmp;
		++npages;
		ptr += NBPG;
		if ((int)ptr == HOLESTART) {	/* skip I/O pages */
			npages += (1024-640)/4;
			ptr = (unsigned char *)HOLEEND;
		}
	}
	return npages;
}
