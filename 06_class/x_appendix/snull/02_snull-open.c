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

MODULE_LICENSE("GPL");

struct net_device snull_dev;
    
int snull_init(struct net_device *dev);
int snull_open(struct net_device *dev);
int snull_release(struct net_device *dev);

int result;

struct net_device snull_dev = {
    .init   = snull_init,
};

int snull_init(struct net_device *dev)
{
    printk("snull init...\n");
    ether_setup(dev);
    strcpy(dev->name, "sn0");
    dev->open = snull_open;
    dev->stop = snull_release;

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

static int snull_init_module(void)
{
    result = register_netdev(&snull_dev);
    printk("snull Module is up...\n");
    return 0;
}

static void snull_exit_module(void)
{
    printk("snull down...\n");
    unregister_netdev(&snull_dev);
}

module_init(snull_init_module);
module_exit(snull_exit_module);
