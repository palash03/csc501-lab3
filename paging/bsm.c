/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[];
/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    STATWORD(ps);
    disable(ps);
    int i,j;
    for(i=0;i<16;i++)
    {
        bsm_tab[i].bs_npages = -1;
        //bsm_tab[i].bs_pid = -1;
        bsm_tab[i].bs_sem = -1;
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_vpno = -1;
        bsm_tab[i].bs_priv = -1;
        for(j=0;j<50;j++)
        {
            bsm_tab[i].proc[j] = 0;
        }
    }
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD(ps);
    disable(ps);
    int i, flag = 0;
    for(i=0;i<16;i++)
    {
        if(bsm_tab[i].bs_status == BSM_UNMAPPED)
        {
            *avail = i;
            flag = 1;
            break;
        }
    }
    if(flag == 1)
    {
        restore(ps);
        return(OK);
    }
    restore(ps);
    return(SYSERR);
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    STATWORD ps;
    disable(ps);
    int j;
    if(isbadBS(i))
    {
        restore(ps);
        return SYSERR;
    }
    bsm_tab[i].bs_status = BSM_UNMAPPED;
    bsm_tab[i].bs_npages = -1;
    //bsm_tab[i].bs_pid = -1;
    bsm_tab[i].bs_sem = -1;
    bsm_tab[i].bs_vpno = -1;
    bsm_tab[i].bs_priv = -1;
    for(j=0;j<50;j++)
    {
        bsm_tab[j].proc[j] = 0;
    }
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    STATWORD ps;
    disable(ps);
    int i;
    unsigned int frameAddr;
    frameAddr = vaddr/NBPG;
    for(i=0;i<16;i++)
    {
        if(bsm_tab[i].proc[pid] == 1)
        {
            if(bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages < frameAddr || frameAddr < bsm_tab[i].bs_vpno)
            {
                restore(ps);
                return(SYSERR);
            }
            *store = i;
            *pageth = frameAddr - bsm_tab[i].bs_vpno;
            break;
        }
    }
    restore(ps);
    return OK;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);
    if(isbadBS(source) || isbadpid(pid) || npages <= 0 || npages > 128)
    {
        restore(ps);
        return SYSERR;
    }
    if(bsm_tab[source].bs_priv == 1) // BS is private
    {
        restore(ps);
        return SYSERR;
    }
    // process mapping to BS
    struct pentry *ptr = &proctab[pid];
    //ptr->store = source;
    
    if(bsm_tab[source].bs_status == BSM_MAPPED)
    {
        if(bsm_tab[source].proc[pid] == 1)
        {
            restore(ps);
            return SYSERR;
        }
        if(bsm_tab[source].bs_npages >= npages)   
        {
            bsm_tab[source].proc[pid] = 1;
            bsm_tab[source].bs_vpno = vpno;
            bsm_tab[source].bs_sem = -1;
            bsm_tab[source].bs_priv = 0;
            ptr->bs[source] = 1;
            restore(ps);
            return OK;
        }
        restore(ps);
        return SYSERR;
    }
    bsm_tab[source].bs_npages = npages;
    bsm_tab[source].bs_sem = -1;
    bsm_tab[source].bs_status = BSM_MAPPED;
    bsm_tab[source].bs_vpno = vpno;
    bsm_tab[source].proc[pid] = 1;
    bsm_tab[source].bs_priv = 0;
    ptr->bs[source] = 1;
    restore(ps);
    return(OK);
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    STATWORD ps;
    disable(ps);
    if(isbadpid(pid))
    {
        restore(ps);
        return SYSERR;
    }
    int i, j, k, bs, pageOffset;
    unsigned long base;
    struct pentry *ptr = &proctab[pid];
    //kprintf("Did u miss me?????????????????????????????????????????????");
    shutdown();
    for(i=0;i<16;i++)
    {
        if(bsm_tab[i].proc[pid] == 1 && bsm_tab[i].bs_priv == 1) // private bs unmap
        {
            bsm_tab[i].proc[pid] = 0;
            ptr->bs[i] = 0;
            ptr->vmemlist->mlen = 0;
            ptr->vmemlist->mnext = 0;
            for(j=0;j<NFRAMES;j++)
            {
                if(frm_tab[j].fr_pid == pid)
                {
                    if(bsm_lookup(currpid,vpno*NBPG,&bs,&pageOffset)==SYSERR || bsm_tab[i].bs_status == BSM_UNMAPPED)
                    {
                        restore(ps);
                        return SYSERR;
                    }
                    write_bs((FRAME0 + j)*NBPG,bs,pageOffset);
                }    
            }
            bsm_tab[i].bs_npages = -1;
            bsm_tab[i].bs_priv = -1;
            bsm_tab[i].bs_sem = -1;
            bsm_tab[i].bs_status = BSM_UNMAPPED;
            bsm_tab[i].bs_vpno = -1;
            for(k=0;k<NPROC;k++)
            {
                bsm_tab[i].proc[k] = 0;
            }
            /*if(bsm_tab[i].bs_status == BSM_UNMAPPED || bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages < vpno || vpno < bsm_tab[i].bs_vpno)
            {
                restore(ps);
                return SYSERR;
            }
            pageOffset = vpno - bsm_tab[i].bs_vpno;
            
            base = 2048 + (i*128);*/
            /*for(j=0;j<bsm_tab[i].bs_npages;j++)
            {
                bsm_lookup(currpid,vpno*NBPG,&bs,&pageOffset);
                write_bs((FRAME0 + j)*NBPG,bs,pageOffset);
            }*/
            //free_bsm(i);
            restore(bs);
            return OK;
        }
        else if(bsm_tab[i].proc[pid] == 1 && bsm_tab[i].bs_priv == 0) // shared bs unmap
        {
            bsm_tab[i].proc[pid] = 0;
            ptr->bs[i] = 0;
            for(j=0;j<NFRAMES;j++)
            {
                if(frm_tab[j].fr_pid == pid)
                {
                    if(bsm_lookup(currpid,vpno*NBPG,&bs,&pageOffset)==SYSERR || bsm_tab[i].bs_status == BSM_UNMAPPED)
                    {
                        restore(ps);
                        return SYSERR;
                    }
                    write_bs((FRAME0 + j)*NBPG,bs,pageOffset);
                }    
            }
            bsm_tab[i].bs_npages = -1;
            bsm_tab[i].bs_priv = -1;
            bsm_tab[i].bs_sem = -1;
            bsm_tab[i].bs_status = BSM_UNMAPPED;
            bsm_tab[i].bs_vpno = -1;
            for(k=0;k<NPROC;k++)
            {
                bsm_tab[i].proc[k] = 0;
            }
        }
    }
    restore(ps);
    return SYSERR;
}


