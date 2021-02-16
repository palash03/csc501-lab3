#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

bs_map_t bsm_tab[];

int get_bs(bsd_t bs_id, unsigned int npages) 
{
    if(isbadpid(bs_id) || npages <= 0 || npages > 128)
    {
        return SYSERR;
    }
    if(bsm_tab[bs_id].bs_priv == 1)
    {
        return SYSERR;
    }
    if(bsm_tab[bs_id].bs_status == BSM_MAPPED)
    {
        return bsm_tab[bs_id].bs_npages;
    }
    //bsm_tab[bs_id].bs_status = BSM_MAPPED;
    //bsm_tab[bs_id].bs_npages = npages;
    return npages;
}


