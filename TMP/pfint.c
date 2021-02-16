/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
int x;

SYSCALL pfint()
{
    STATWORD ps;
    disable(ps);
    unsigned long faultAddr;
    int i,vp,bs,page,fr,fr2,pd_offset,pg_offset,x;
    pd_t *ptr;
    pt_t *ptr2;
    
    faultAddr = read_cr2();
    vp = faultAddr/NBPG;
    ptr = proctab[currpid].pdbr;
    //kprintf("pdbr: %d",(int)ptr>>12);
    if(bsm_lookup(currpid,faultAddr,&bs,&page) != SYSERR)
    {
        pd_offset = (0xFFC00000 & faultAddr) >> 22;
        pg_offset = (0x003FF000 & faultAddr) >> 12;
        //kprintf(" Yo! ");
        ptr += pd_offset*sizeof(pd_t);
        //kprintf(" pg table: %d ",(int)ptr>>12);
        if(ptr->pd_pres != 1) // pt not present means we have to make pt
        {
            if(get_frm(&fr) == SYSERR)
            {
                restore(ps);
                return SYSERR;
            }
            frm_tab[fr].fr_dirty = 0;
            frm_tab[fr].fr_pid = currpid;
            frm_tab[fr].fr_refcnt = 0;
            frm_tab[fr].fr_status = FRM_MAPPED;
            frm_tab[fr].fr_type = FR_TBL;
            frm_tab[fr].fr_vpno = -1;

            //frm_tab[(unsigned int)ptr/NBPG - FRAME0].fr_refcnt++;
            ptr->pd_acc = 0;
            ptr->pd_avail = 0;
            ptr->pd_base = FRAME0 + fr;
            ptr->pd_fmb = 0;
            ptr->pd_global = 0;
            ptr->pd_mbz = 0;
            ptr->pd_pcd = 0;
            ptr->pd_pres = 1;
            ptr->pd_pwt = 0;
            ptr->pd_user = 0;
            ptr->pd_write = 1;
        }
        //page table is present, now check if page is present or not
        if(get_frm(&fr2) == SYSERR)
        {
            restore(ps);
            return SYSERR;
        }
        ptr2 = (ptr->pd_base)*NBPG + pg_offset*sizeof(pt_t);
        //kprintf("Frame: %d\n",(int)ptr2>>12);
        if(ptr2->pt_pres != 1)
        {
            frm_tab[fr2].fr_dirty = 0;
            frm_tab[fr2].fr_pid = currpid;
            frm_tab[fr2].fr_refcnt = 0;
            frm_tab[fr2].fr_status = FRM_MAPPED;
            frm_tab[fr2].fr_type = FR_PAGE;
            frm_tab[fr2].fr_vpno = vp;
            //read_bs((FRAME0+fr2)*NBPG,bs,page);

            frm_tab[fr2].fr_refcnt++;
            ptr2->pt_acc = 0;
            ptr2->pt_avail = 0;
            ptr2->pt_base = FRAME0 + fr2;
            ptr2->pt_dirty = 0;
            ptr2->pt_global = 0;
            ptr2->pt_mbz = 0;
            ptr2->pt_pcd = 0;
            ptr2->pt_pres = 1;
            ptr2->pt_pwt = 0;
            ptr2->pt_user = 0;
            ptr2->pt_write = 1;
        }
        read_bs((ptr2->pt_base)*NBPG,bs,page);
        write_cr3(proctab[currpid].pdbr);
        restore(ps);
        return OK;
    } 
    kill(currpid);
    restore(ps);
    return SYSERR;
}


