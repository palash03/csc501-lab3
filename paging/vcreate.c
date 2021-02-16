/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*------------------------------------------------------------------------
 *  vcreate  -  create a process in virtual memory to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);
	int bs, pid;
	struct mblock *ptr;
	if(hsize > 128 || hsize <= 0)
	{
		restore(ps);
		return(SYSERR);
	}
	if(get_bsm(&bs) == SYSERR)
	{
		restore(ps);
		return(SYSERR);
	}
	pid = create(procaddr,ssize,priority,name,nargs,args);
	if(isbadpid(pid))
	{
		restore(ps);
		return(SYSERR);
	}
	if(bsm_map(pid,4096,bs,hsize) != SYSERR)
	{
		//init_pt(pid);
		//init_pd(pid);
		ptr = (2048 + bs*128)*NBPG;
		ptr->mlen = hsize*NBPG;
		ptr->mnext = NULL;
		proctab[pid].bs[bs] = 1;
		bsm_tab[bs].proc[pid] = 1;
		proctab[pid].vmemlist->mlen = hsize*NBPG;
		proctab[pid].vmemlist->mnext = 4096*NBPG;
		bsm_tab[bs].bs_priv = 1;
		proctab[pid].vhpno = 4096;
		proctab[pid].vhpnpages = hsize;
		restore(ps);
		return pid;
	}
	restore(ps);
	return SYSERR;
}