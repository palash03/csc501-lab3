/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[];
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
    STATWORD ps;
    disable(ps);
    int i;
    for(i=0;i<NFRAMES;i++)
    {
        frm_tab[i].fr_dirty = -1;
        frm_tab[i].fr_pid = -1;
        frm_tab[i].fr_refcnt = -1;
        frm_tab[i].fr_status = FRM_UNMAPPED;
        frm_tab[i].fr_type = FR_PAGE;
        frm_tab[i].fr_vpno = -1;
    }
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
    STATWORD ps;
    disable(ps);
    int i, flag = 0;
    for(i=0;i<NFRAMES;i++)
    {
        if(frm_tab[i].fr_status == FRM_UNMAPPED)
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
    return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
    STATWORD ps;
    disable(ps);
    if(i < 1024 || i >= 2048)
    {
        restore(ps);
        return SYSERR;
    }
    frm_tab[i].fr_dirty = -1;
    frm_tab[i].fr_pid = -1;
    frm_tab[i].fr_refcnt = -1;
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_type = FR_TBL;
    frm_tab[i].fr_vpno = -1;
    restore(ps);
    return OK;
}

int initglobaltables()
{
    int i, j, a;
    for(i=0;i<4;i++)
    {
        if(get_frm(&a) == SYSERR)
        {
            return SYSERR;
        }
        frm_tab[a].fr_dirty = 0;
        frm_tab[a].fr_pid = -1;
        frm_tab[a].fr_refcnt = 0;
        frm_tab[a].fr_status = FRM_MAPPED;
        frm_tab[a].fr_type = FR_TBL;
        frm_tab[a].fr_vpno = -1;

        pt_t *ptr = (FRAME0 + a)*NBPG;
        for(j=0;j<1024;j++)
        { 
            frm_tab[a].fr_refcnt++;
            ptr->pt_acc = 0;
            ptr->pt_avail = 0;
            ptr->pt_base = FRAME0*i + j;
            ptr->pt_dirty = 0;
            ptr->pt_global = 0;
            ptr->pt_mbz = 0;
            ptr->pt_pcd = 0;
            ptr->pt_pres = 1;
            ptr->pt_pwt = 0;
            ptr->pt_user = 0;
            ptr->pt_write = 1;
            ptr++;
        }
    }
    return OK;
}

init_pd(int pid)
{
    int i, j, a;
    if(get_frm(&a) == SYSERR)
    {
        return SYSERR;
    }
    frm_tab[a].fr_dirty = 0;
    frm_tab[a].fr_pid = pid;
    frm_tab[a].fr_refcnt = 0;
    frm_tab[a].fr_status = FRM_MAPPED;
    frm_tab[a].fr_type = FR_DIR;
    frm_tab[a].fr_vpno = -1;
    pd_t *ptr = (FRAME0 + a)*NBPG;
    //write_cr3(ptr);
    for(i=0;i<1024;i++)
    {
        ptr->pd_acc = 0;
        ptr->pd_avail = 0;
        ptr->pd_base = 0;
        ptr->pd_fmb = 0;
        ptr->pd_global = 0;
        ptr->pd_mbz = 0;
        ptr->pd_pcd = 0;
        ptr->pd_pres = 0;
        ptr->pd_pwt = 0;
        ptr->pd_user = 0;
        ptr->pd_write = 1;
        if(i < 4)
        {
            ptr->pd_pres = 1;
            ptr->pd_base = FRAME0 + i;
            frm_tab[a].fr_refcnt++;
            proctab[pid].pdbr = ptr;
        }
        ptr++;
    }
    //write_cr3(ptr);
    return OK;
} 

init_pt(int pid)
{
    int i, j, a;
    if(get_frm(&a) == SYSERR)
    {
        return SYSERR;
    }
    frm_tab[a].fr_dirty = 0;
    frm_tab[a].fr_pid = pid;
    frm_tab[a].fr_refcnt = 0;
    frm_tab[a].fr_status = FRM_MAPPED;
    frm_tab[a].fr_type = FR_TBL;
    frm_tab[a].fr_vpno = -1;
    pt_t *ptr = (FRAME0 + a)*NBPG;
    proctab[pid].pdbr = ptr;
    for(i=0;i<1024;i++)
    {
        ptr->pt_acc = 0;
        ptr->pt_avail = 0;
        ptr->pt_base = 0;
        ptr->pt_global = 0;
        ptr->pt_dirty = 0;
        ptr->pt_mbz = 0;
        ptr->pt_pcd = 0;
        ptr->pt_pres = 0;
        ptr->pt_pwt = 0;
        ptr->pt_user = 0;
        ptr->pt_write = 1;
        ptr++;
    }
    return OK;
}


