/*
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_PRJ_H__
#define __MODEM_PRJ_H__

#include <linux/wait.h>
#include <linux/miscdevice.h>
#include <linux/skbuff.h>
#include <linux/completion.h>
#include <linux/wakelock.h>


#define MAX_LINK_DEVTYPE	3
#define MAX_RAW_DEVS		32
#define MAX_NUM_IO_DEV		(MAX_RAW_DEVS + 4)

#define IOCTL_MODEM_ON			_IO('o', 0x19)
#define IOCTL_MODEM_OFF			_IO('o', 0x20)
#define IOCTL_MODEM_RESET		_IO('o', 0x21)
#define IOCTL_MODEM_BOOT_ON		_IO('o', 0x22)
#define IOCTL_MODEM_BOOT_OFF		_IO('o', 0x23)
#define IOCTL_MODEM_START		_IO('o', 0x24)

#define IOCTL_MODEM_PROTOCOL_SUSPEND	_IO('o', 0x25)
#define IOCTL_MODEM_PROTOCOL_RESUME	_IO('o', 0x26)

#define IOCTL_MODEM_STATUS		_IO('o', 0x27)
#define IOCTL_MODEM_GOTA_START		_IO('o', 0x28)
#define IOCTL_MODEM_FW_UPDATE		_IO('o', 0x29)

#define IOCTL_MODEM_NET_SUSPEND		_IO('o', 0x30)
#define IOCTL_MODEM_NET_RESUME		_IO('o', 0x31)

#define IOCTL_MODEM_DUMP_START		_IO('o', 0x32)
#define IOCTL_MODEM_DUMP_UPDATE		_IO('o', 0x33)
#define IOCTL_MODEM_FORCE_CRASH_EXIT	_IO('o', 0x34)
#define IOCTL_MODEM_CP_UPLOAD		_IO('o', 0x35)
#define IOCTL_MODEM_DUMP_RESET		_IO('o', 0x36)

#define IOCTL_DPRAM_SEND_BOOT		_IO('o', 0x40)
#define IOCTL_DPRAM_INIT_STATUS		_IO('o', 0x43)

/* ioctl command definitions. */
#define IOCTL_DPRAM_PHONE_POWON		_IO('o', 0xd0)
#define IOCTL_DPRAM_PHONEIMG_LOAD	_IO('o', 0xd1)
#define IOCTL_DPRAM_NVDATA_LOAD		_IO('o', 0xd2)
#define IOCTL_DPRAM_PHONE_BOOTSTART	_IO('o', 0xd3)

#define IOCTL_DPRAM_PHONE_UPLOAD_STEP1	_IO('o', 0xde)
#define IOCTL_DPRAM_PHONE_UPLOAD_STEP2	_IO('o', 0xdf)

/* modem status */
#define MODEM_OFF		0
#define MODEM_CRASHED		1
#define MODEM_RAMDUMP		2
#define MODEM_POWER_ON		3
#define MODEM_BOOTING_NORMAL	4
#define MODEM_BOOTING_RAMDUMP	5
#define MODEM_DUMPING		6
#define MODEM_RUNNING		7

#define HDLC_HEADER_MAX_SIZE	6 /* fmt 3, raw 6, rfs 6 */

#define PSD_DATA_CHID_BEGIN	0x2A
#define PSD_DATA_CHID_END	0x38

#define IP6VERSION		6

#define SOURCE_MAC_ADDR		{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}

/* Does modem ctl structure will use state ? or status defined below ?*/
enum modem_state {
	STATE_OFFLINE,
	STATE_CRASH_RESET, /* silent reset */
	STATE_CRASH_EXIT, /* cp ramdump */
	STATE_BOOTING,
	STATE_ONLINE,
	STATE_NV_REBUILDING, /* <= rebuilding start */
	STATE_LOADER_DONE,
	STATE_SIM_ATTACH,
	STATE_SIM_DETACH,
};

enum com_state {
	COM_NONE,
	COM_ONLINE,
	COM_HANDSHAKE,
	COM_BOOT,
	COM_CRASH,
};

enum link_mode {
	LINK_MODE_INVALID = 0,
	LINK_MODE_IPC,
	LINK_MODE_BOOT,
	LINK_MODE_DLOAD,
	LINK_MODE_ULOAD,
};

struct sim_state {
	bool online;	/* SIM is online? */
	bool changed;	/* online is changed? */
};

struct header_data {
	char hdr[HDLC_HEADER_MAX_SIZE];
	unsigned len;
	unsigned frag_len;
	char start; /*hdlc start header 0x7F*/
};

struct sipc4_hdlc_fmt_hdr {
	u16 len;
	u8  control;
} __packed;

struct sipc4_fmt_hdr {
	u16 len;
	u8  msg_seq;
	u8  ack_seq;
	u8  main_cmd;
	u8  sub_cmd;
	u8  cmd_type;
} __packed;

struct vnet {
	struct io_device *iod;
};

/* for fragmented data from link devices */
struct fragmented_data {
	struct sk_buff *skb_recv;
	struct header_data h_data;
};
#define fragdata(iod, ld) (&(iod)->fragments[ld->link_type])

/** struct skbuff_priv - private data of struct sk_buff
 * this is matched to char cb[48] of struct sk_buff
 */
struct skbuff_private {
	struct io_device *iod;
	struct link_device *ld;
	struct io_device *real_iod; /* for rx multipdp */
};

static inline struct skbuff_private *skbpriv(struct sk_buff *skb)
{
	BUILD_BUG_ON(sizeof(struct skbuff_private) > sizeof(skb->cb));
	return (struct skbuff_private *)&skb->cb;
}

/** rx_alloc_skb - allocate an skbuff and set skb's iod, ld
 * @length:	length to allocate
 * @gfp_mask:	get_free_pages mask, passed to alloc_skb
 * @iod:	struct io_device *
 * @ld:		struct link_device *
 *
 * %NULL is returned if there is no free memory.
 */
static inline struct sk_buff *rx_alloc_skb(unsigned int length,
		gfp_t gfp_mask, struct io_device *iod, struct link_device *ld)
{
	struct sk_buff *skb = alloc_skb(length, gfp_mask);
	if (likely(skb)) {
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;
	}
	return skb;
}

struct io_device {
	struct list_head list;

	/* Name of the IO device */
	char *name;

	/* Wait queue for the IO device */
	wait_queue_head_t wq;

	/* Misc and net device structures for the IO device */
	struct miscdevice  miscdev;
	struct net_device *ndev;

	/* ID and Format for channel on the link */
	unsigned           id;
	enum modem_link    link_types;
	enum dev_format    format;
	enum modem_io      io_typ;
	enum modem_network net_typ;
	bool               use_handover;	/* handover 2+ link devices */

	atomic_t opened;

	/* Rx queue of sk_buff */
	struct sk_buff_head sk_rx_q;

	/*
	** work for each io device, when delayed work needed
	** use this for private io device rx action
	*/
	struct delayed_work rx_work;

	struct fragmented_data fragments[LINKDEV_MAX];

	/* for multi-frame */
	struct sk_buff *skb[128];

	/* called from linkdevice when a packet arrives for this iodevice */
	int (*recv)(struct io_device *iod, struct link_device *ld,
					const char *data, unsigned int len);

	/* inform the IO device that the modem is now online or offline or
	 * crashing or whatever...
	 */
	void (*modem_state_changed)(struct io_device *iod, enum modem_state);

	/* inform the IO device that the SIM is not inserting or removing */
	void (*sim_state_changed)(struct io_device *iod, bool sim_online);

	struct link_device *link;
	struct modem_ctl   *mc;

	struct wake_lock wakelock;
	long waketime;

	void *private_data;
};
#define to_io_device(misc) container_of(misc, struct io_device, miscdev)

struct io_raw_devices {
	struct io_device *raw_devices[MAX_RAW_DEVS];
	int num_of_raw_devs;
};

struct link_device {
	struct list_head  list;
	char             *name;
	enum modem_link   link_type;
	unsigned          aligned;

	/* Modem data */
	struct modem_data *mdm_data;

	/* Operation mode of the link device */
	enum link_mode mode;

	/* TX queue of socket buffers */
	struct sk_buff_head sk_fmt_tx_q;
	struct sk_buff_head sk_raw_tx_q;
	struct sk_buff_head sk_rfs_tx_q;

	struct sk_buff_head *skb_txq[MAX_DEV_FORMAT];

	bool raw_tx_suspended; /* for misc dev */
	struct completion raw_tx_resumed_by_cp;

	struct workqueue_struct *tx_wq;
	struct work_struct tx_work;
	struct delayed_work tx_delayed_work;

	/*COMMON LINK DEVICE*/
	/* maybe -list of io devices for the link device to use */
	/* to find where to send incoming packets to */
	struct list_head *list_of_io_devices;

	enum com_state com_state;

	/* init communication - setting link driver */
	int (*init_comm)(struct link_device *ld, struct io_device *iod);

	/* terminate communication */
	void (*terminate_comm)(struct link_device *ld, struct io_device *iod);

	/* called by an io_device when it has a packet to send over link
	 * - the io device is passed so the link device can look at id and
	 *   format fields to determine how to route/format the packet
	 */
	int (*send)(struct link_device *ld, struct io_device *iod,
				struct sk_buff *skb);

	int (*gota_start)(struct link_device *ld, struct io_device *iod);

	int (*force_dump)(struct link_device *ld, struct io_device *iod);

	int (*dump_start)(struct link_device *ld, struct io_device *iod);

	int (*modem_update)(
		struct link_device *ld,
		struct io_device *iod,
		unsigned long arg);

	int (*dump_update)(
		struct link_device *ld,
		struct io_device *iod,
		unsigned long arg);

	int (*ioctl)(struct link_device *ld, struct io_device *iod, unsigned cmd
		, unsigned long _arg);
};

struct modemctl_ops {
	int (*modem_on) (struct modem_ctl *);
	int (*modem_off) (struct modem_ctl *);
	int (*modem_reset) (struct modem_ctl *);
	int (*modem_boot_on) (struct modem_ctl *);
	int (*modem_boot_off) (struct modem_ctl *);
	int (*modem_force_crash_exit) (struct modem_ctl *);
	int (*modem_dump_reset) (struct modem_ctl *);
};

struct modem_ctl {
	struct device *dev;
	char *name;
	struct modem_data *mdm_data;

	enum modem_state phone_state;
	struct sim_state sim_state;

	unsigned gpio_cp_on;
	unsigned gpio_reset_req_n;
	unsigned gpio_cp_reset;
	unsigned gpio_pda_active;
	unsigned gpio_phone_active;
	unsigned gpio_cp_dump_int;
	unsigned gpio_flm_uart_sel;
	unsigned gpio_cp_warm_reset;
	unsigned gpio_cp_off;
	unsigned gpio_sim_detect;

	int irq_phone_active;
	int irq_sim_detect;

#ifdef CONFIG_LTE_MODEM_CMC221
	const struct attribute_group *group;
	unsigned gpio_slave_wakeup;
	unsigned gpio_host_wakeup;
	unsigned gpio_host_active;
	int      irq_host_wakeup;

	struct delayed_work dwork;
#endif /*CONFIG_LTE_MODEM_CMC221*/

	struct work_struct work;

#if defined(CONFIG_MACH_U1_KOR_LGT)
	unsigned gpio_cp_reset_msm;
	unsigned gpio_boot_sw_sel;
	void (*vbus_on)(void);
	void (*vbus_off)(void);
	bool usb_boot;
#endif

	struct list_head link_dev_list;

	struct modemctl_ops ops;
	struct io_device *iod;
	struct io_device *bootd;

	void (*gpio_revers_bias_clear)(void);
	void (*gpio_revers_bias_restore)(void);
};

int init_io_device(struct io_device *iod);

#endif
