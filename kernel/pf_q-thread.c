/*
 * Copyright (c) 2014 Bonelli Nicola <nicola.bonelli@cnit.it>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/kthread.h>

#include <pf_q-memory.h>
#include <pf_q-sock.h>
#include <pf_q-transmit.h>

#include <pf_q-common.h>

int
pfq_tx_thread(void *data)
{
        struct pfq_sock *so = (struct pfq_sock *)data;
        struct pfq_tx_opt *to = &so->tx_opt;

        printk(KERN_INFO "[PFQ] tx-thread started on node %d\n", to->cpu_index);

        while(!kthread_should_stop())
        {
                struct pfq_pkt_hdr * h;
                struct sk_buff *skb;
                struct net_device *dev;
                size_t len;
                int index;

                index = pfq_spsc_read_index(to->queue_info);

                dev = dev_get_by_index(sock_net(&so->sk), to->if_index);

                for(; likely(index != -1); index = pfq_spsc_read_index(to->queue_info))
                {

                        if (unlikely(index >= to->size))
                        {
                                if(printk_ratelimit())
                                        printk(KERN_WARNING "[PFQ] bogus spsc index! q->size=%lu index=%d\n", to->size, index);
                                goto out;
                        }

                        h = (struct pfq_pkt_hdr *) (to->base_addr + index * to->queue_info->slot_size);

                        skb = pfq_alloc_skb(to->maxlen, GFP_KERNEL);
                        if (skb == NULL) {
                                goto out;
                        }

                        skb->dev = dev;

                        /* copy packet to this skb: */

                        len =  min_t(size_t, h->len, to->queue_info->max_len);

                        memcpy(skb->data, h+1, len);

                        /* release the slot */

                        pfq_spsc_read_commit(to->queue_info);

                        /* set the tail */

                        skb_reset_tail_pointer(skb);

                        skb->len = 0;

                        skb_put(skb, len);

                        /* send the packet... */

                        dev_queue_xmit(skb);

                        // TODO:
                        // pfq_queue_xmit(skb, to->if_index, to->hw_queue);

                }
        out:
                dev_put(dev);
                set_current_state(TASK_INTERRUPTIBLE);
                schedule();
        }

        printk(KERN_INFO "[PFQ] tx-thread stopped.\n");
        return 0;
}