/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <bsp_s_memory.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/vmalloc.h>
#include <bsp_print.h>
#include <securec.h>

#undef THIS_MODU
#define THIS_MODU mod_mperf
/*lint -e528*/
#define s_mem_pr_err(fmt, ...) (bsp_err("<%s> " fmt, __FUNCTION__, ##__VA_ARGS__))

#ifdef CONFIG_OF_RESERVED_MEM

#define MAX_RESERVED_REGIONS 16
static struct reserve_mem_node g_reserved_mems[MAX_RESERVED_REGIONS];
static int g_reserved_mem_count = 0;


static int __init modem_mperf_reserve_area(struct reserved_mem *rmem)
{
    char *status = NULL;
    errno_t ret = 0;
    s_mem_pr_err("enter modem_mperf_reserve_area 0x%x, size is 0x%x\n", (unsigned int)(uintptr_t)rmem->base,
        (unsigned int)(uintptr_t)rmem->size);

    status = (char *)of_get_flat_dt_prop(rmem->fdt_node, "status", NULL); /*lint !e838*/
    if (status && (strncmp(status, "ok", strlen("ok")) != 0)) {
        return 0;
    }
    if (g_reserved_mem_count < 0 || g_reserved_mem_count >= MAX_RESERVED_REGIONS) {
        s_mem_pr_err("Array out of bounds\n");
        return -1;
    }
    ret = strncpy_s(g_reserved_mems[g_reserved_mem_count].name, sizeof(g_reserved_mems[g_reserved_mem_count].name),
        rmem->name,
        min(sizeof(g_reserved_mems[g_reserved_mem_count].name),
        strnlen(rmem->name, sizeof(g_reserved_mems[g_reserved_mem_count].name)))); /*lint !e666*/
    if (ret) {
        s_mem_pr_err("strncpy_s err\n");
    }
    g_reserved_mems[g_reserved_mem_count].base = rmem->base;
    g_reserved_mems[g_reserved_mem_count].size = rmem->size;
    g_reserved_mem_count++;

    s_mem_pr_err("kernel reserved buffer name %s\n", rmem->name);

    s_mem_pr_err("kernel reserved buffer is useful, base 0x%x, size is 0x%x\n", (unsigned int)(uintptr_t)rmem->base,
        (unsigned int)(uintptr_t)rmem->size);

    return 0;
}

/*lint -save -e611*/
RESERVEDMEM_OF_DECLARE(modem_mperf, "modem_mperf", modem_mperf_reserve_area); /*lint !e528*/
/*lint -restore +e611*/
struct reserve_mem_node *bsp_reserve_mem_mperf_get(const char *name)
{
    int i;
    for (i = 0; i < g_reserved_mem_count; i++) {
        if (strncmp(g_reserved_mems[i].name, name, strlen(name)) == 0)
            return &g_reserved_mems[i];
    }
    return NULL;
}

EXPORT_SYMBOL(bsp_reserve_mem_mperf_get);

void *bsp_mem_remap_uncached(unsigned long phys_addr, unsigned int size)
{
    unsigned long i;
    u8 *vaddr = NULL;
    unsigned long npages = (PAGE_ALIGN((phys_addr & (PAGE_SIZE - 1)) + size) >> PAGE_SHIFT);
    unsigned long offset = phys_addr & (PAGE_SIZE - 1);
    struct page **pages;

    pages = vmalloc(sizeof(struct page *) * npages);
    if (pages == NULL) {
        return NULL;
    }

    pages[0] = phys_to_page(phys_addr);

    for (i = 0; i < npages - 1; i++) {
        pages[i + 1] = pages[i] + 1;
    }

    vaddr = (u8 *)vmap(pages, (unsigned int)npages, (unsigned long)VM_MAP, pgprot_writecombine(PAGE_KERNEL));
    if (vaddr != NULL) {
        vaddr += offset;
    }

    vfree(pages);

    return (void *)vaddr;
}

EXPORT_SYMBOL(bsp_mem_remap_uncached);

void bsp_mem_unmap_uncached(const void *virt_addr)
{
    vunmap(virt_addr);
}

EXPORT_SYMBOL(bsp_mem_unmap_uncached);
#endif

static struct mperf_info g_mperf_info = { 0 };

int bsp_mem_init_mperf_info(void)
{
    s32 ret;
    struct device_node *np = NULL;

    np = of_find_compatible_node(NULL, NULL, "mperf_driver"); /*lint !e838*/
    if (np == NULL) {
        s_mem_pr_err("can not match mperf_driver in gator_events_mperf_init. defaultcpu_nr=%d\n",
            g_mperf_info.nr_cpus);
        return -1;
    }

    ret = of_property_read_u32(np, "nr_cpus", &g_mperf_info.nr_cpus); /*lint !e838*/
    if (ret == 0) {
        g_mperf_info.inited = 1;
        s_mem_pr_err("mperf cntl.cpu_nr=%d\n", g_mperf_info.nr_cpus);
    }
    return 0;
}

struct mperf_info *bsp_mem_get_mperf_info(void)
{
    if (g_mperf_info.inited) {
        return &g_mperf_info;
    } else {
        return NULL;
    }
}
EXPORT_SYMBOL(bsp_mem_get_mperf_info);
#if !IS_ENABLED(CONFIG_MODEM_DRIVER)
module_init(bsp_mem_init_mperf_info); /*lint !e528*/
#endif
