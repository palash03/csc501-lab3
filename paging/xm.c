/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

bs_map_t bsm_tab[];
/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
    STATWORD ps;
    disable(ps);
    if(bsm_map(currpid,virtpage,source,npages) != SYSERR)
    {
        restore(ps);
        return OK;
    }
    restore(ps);
    return SYSERR;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    STATWORD ps;
    disable(ps);
    if(virtpage < 4096)
    {
        restore(ps);
        return SYSERR;
    }
    if(bsm_unmap(currpid,virtpage,1) != SYSERR)
    {
        restore(ps);
        return OK;
    }
    restore(ps);
    return SYSERR;
}
