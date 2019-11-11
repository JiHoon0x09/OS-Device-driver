/***************************************
 * Filename: snull.c
 * Title: Virtual Network Device
 * Desc: network driver module
****************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/interrupt.h> /* mark_bh */
#include <linux/in.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>

#define SNULL_RX_INTR 0x0001
#define SNULL_TX_INTR 0x0002
#define SNULL_TIMEOUT 5

MODULE_LICENSE("GPL");

static int lockup = 0;

struct net_device snull_dev;
    
int snull_init(struct net_device *dev);
int snull_open(struct net_device *dev);
int snull_release(struct net_device *dev);
int snull_tx(struct sk_buff *skb, struct net_device *dev);

void snull_hw_tx(char *buf, int len, struct net_device *dev);
void snull_interrupt(int irq, void *dev_id, struct pt_regs *regs);
    
struct snull_priv {
    struct net_device_stats stats;
    int status;
    int rx_packetlen;
    u8 *rx_packetdata;
    int tx_packetlen;
    u8 *tx_packetdata;
    struct sk_buff *skb;
    spinlock_t lock;
};

int result;

struct net_device snull_devs[2] = {
    {.init   = snull_init},
    {.init   = snull_init},
};

int snull_init(struct net_device *dev)
{
    printk("snull init...\n");
    ether_setup(dev);
    strcpy(dev->name, "sn0");
    dev->open = snull_open;
    dev->stop = snull_release;
    dev->hard_start_xmit = snull_tx;

    return 0;
}

int snull_open(struct net_device *dev)
{
    memcpy(dev->dev_addr, "\0SNUL0", ETH_ALEN);
    
    netif_start_queue(dev);
    printk("snull_open success\n");
    return 0;
}

int snull_release(struct net_device *dev)
{
    printk("release\n");
    netif_stop_queue(dev); /* can't transmit any more */
    return 0;
}

int snull_tx(struct sk_buff *skb, struct net_device *dev)
{
    int len;
    char *data;
    struct snull_priv *priv = (struct snull_priv *) dev->priv;
    
    printk("snull_tx.............................\n");
    
    len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
    data = skb->data;
    dev->trans_start = jiffies; /* save the timestamp */
    priv->skb = skb;
    snull_hw_tx(data, len, dev);
    
    return 0;
}

void snull_hw_tx(char *buf, int len, struct net_device *dev)
{
    struct iphdr *ih;
    struct net_device *dest;
    struct snull_priv *priv;
    u32 *saddr, *daddr;
    
    printk("hw_tx\n");
    if(len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
        printk("snull: Hmm... packet too short(%i octets)\n", len);
        return;
    }
    if(0) {
        int i;
        for(i=14; i<len; i++)
            printk(" %02x", buf[i] & 0xff);
        printk("\n");
    }
    
    ih = (struct iphdr *)(buf+sizeof(struct ethhdr));
    saddr = &ih->saddr;
    daddr = &ih->daddr;
    
    ((u8 *)saddr)[2] ^= 1;
    ((u8 *)daddr)[2] ^= 1;
    
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);
    
    dest = snull_devs + (dev == snull_devs ? 1 : 0);
    priv = (struct snull_priv *) dest->priv;
    priv->status = SNULL_RX_INTR;
    priv->rx_packetlen = len;
    priv->rx_packetdata = buf;
    snull_interrupt(0, dest, NULL);
    
    priv = (struct snull_priv *) dev->priv;
    priv->status = SNULL_TX_INTR;
    priv->tx_packetlen = len;
    priv->tx_packetdata = buf;
    if(lockup && ((priv->stats.tx_packets + 1) % lockup) == 0) {
        netif_stop_queue(dev);
    }
    else
        snull_interrupt(0, dev, NULL);
}

void snull_rx(struct net_device *dev, int len, unsigned char *buf)
{
    struct sk_buff *skb;
    struct snull_priv *priv = (struct snull_priv *) dev->priv;
    printk("rx\n");
    
    skb = dev_alloc_skb(len+2);
    if(!skb) {
        printk("snull rx: low on mem - packet dropped\n");
        priv->stats.rx_dropped++;
        return;
    }
    skb_reserve(skb, 2);
    memcpy(skb_put(skb, len), buf, len);
    
    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    priv->stats.rx_packets++;
#ifndef LINUX_20
    priv->stats.rx_bytes += len;
#endif
    netif_rx(skb);
    return;
}

void snull_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    int statusword;
    struct snull_priv *priv;
    
    struct net_device *dev = (struct net_device *)dev_id;
    printk("interrupt\n");
    if(!dev) return;
    
    priv = (struct snull_priv *)dev->priv;
    spin_lock(&priv->lock);
    
    statusword = priv->status;
    if(statusword & SNULL_RX_INTR)
    {
        snull_rx(dev, priv->rx_packetlen, priv->rx_packetdata);
    }
    if(statusword & SNULL_TX_INTR)
    {
        priv->stats.tx_packets++;
        priv->stats.tx_bytes += priv->tx_packetlen;
        dev_kfree_skb(priv->skb);
    }
    spin_unlock(&priv->lock);
    return;
}

static int snull_init_module(void)
{
    int result;
    printk("snull Module is up...\n");
    
    strcpy(snull_devs[0].name, "sn0");
    strcpy(snull_devs[1].name, "sn1");
    
    result = register_netdev(&snull_devs[0]); // 송신용 
    result = register_netdev(&snull_devs[1]); // 수신용
    
    return 0;
}

static void snull_exit_module(void)
{
    printk("snull down...\n");
    unregister_netdev(&snull_devs[0]);
    unregister_netdev(&snull_devs[1]);
}

module_init(snull_init_module);
module_exit(snull_exit_module);
