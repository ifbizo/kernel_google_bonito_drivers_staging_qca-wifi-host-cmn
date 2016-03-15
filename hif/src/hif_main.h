/*
 * Copyright (c) 2013-2016 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

/*
 * NB: Inappropriate references to "HTC" are used in this (and other)
 * HIF implementations.  HTC is typically the calling layer, but it
 * theoretically could be some alternative.
 */

/*
 * This holds all state needed to process a pending send/recv interrupt.
 * The information is saved here as soon as the interrupt occurs (thus
 * allowing the underlying CE to re-use the ring descriptor). The
 * information here is eventually processed by a completion processing
 * thread.
 */

#ifndef __HIF_MAIN_H__
#define __HIF_MAIN_H__

#include <qdf_atomic.h>         /* qdf_atomic_read */
#include "qdf_lock.h"
#include "cepci.h"
#include "hif.h"
#include "multibus.h"

#define HIF_MIN_SLEEP_INACTIVITY_TIME_MS     50
#define HIF_SLEEP_INACTIVITY_TIMER_PERIOD_MS 60

/*
 * This macro implementation is exposed for efficiency only.
 * The implementation may change and callers should
 * consider the targid to be a completely opaque handle.
 */
#define TARGID_TO_PCI_ADDR(targid) (*((A_target_id_t *)(targid)))

#ifdef QCA_WIFI_3_0
#define DISABLE_L1SS_STATES 1
#endif
#ifdef CONFIG_SLUB_DEBUG_ON
#define MAX_NUM_OF_RECEIVES 100 /* Maximum number of Rx buf to process before*
				 * break out in SLUB debug builds */
#elif defined(FEATURE_NAPI)
#define MAX_NUM_OF_RECEIVES HIF_NAPI_MAX_RECEIVES
#else /* no SLUBS, no NAPI */
/* Maximum number of Rx buf to process before break out */
#define MAX_NUM_OF_RECEIVES 1000
#endif /* SLUB_DEBUG_ON / FEATURE_NAPI */

#ifdef QCA_WIFI_3_0_ADRASTEA
#define ADRASTEA_BU 1
#else
#define ADRASTEA_BU 0
#endif

#ifdef QCA_WIFI_3_0
#define HAS_FW_INDICATOR 0
#else
#define HAS_FW_INDICATOR 1
#endif


#define AR9888_DEVICE_ID (0x003c)
#define AR6320_DEVICE_ID (0x003e)
#define AR6320_FW_1_1  (0x11)
#define AR6320_FW_1_3  (0x13)
#define AR6320_FW_2_0  (0x20)
#define AR6320_FW_3_0  (0x30)
#define AR6320_FW_3_2  (0x32)
#define ADRASTEA_DEVICE_ID (0xabcd)
#define ADRASTEA_DEVICE_ID_P2_E12 (0x7021)
#if (defined(QVIT))
#define QCA6180_DEVICE_ID (0xabcd)
#else
#define QCA6180_DEVICE_ID (0x041)
#endif

#define HIF_GET_PCI_SOFTC(scn) ((struct hif_pci_softc *)scn)
#define HIF_GET_CE_STATE(scn) ((struct HIF_CE_state *)scn)
#define HIF_GET_SOFTC(scn) ((struct hif_softc *)scn)
#define GET_HIF_OPAQUE_HDL(scn) ((struct hif_opaque_softc *)scn)

struct hif_ce_stats {
	int hif_pipe_no_resrc_count;
	int ce_ring_delta_fail_count;
};

struct hif_softc {
	struct hif_opaque_softc osc;
	struct hif_config_info hif_config;
	struct hif_target_info target_info;
	void __iomem *mem;
	enum qdf_bus_type bus_type;
	struct hif_bus_ops bus_ops;
	void *ce_id_to_state[CE_COUNT_MAX];
	qdf_device_t qdf_dev;
	bool hif_init_done;
	bool request_irq_done;
	/* Packet statistics */
	struct hif_ce_stats pkt_stats;
	ol_target_status target_status;

	struct targetdef_s *targetdef;
	struct ce_reg_def *target_ce_def;
	struct hostdef_s *hostdef;
	struct host_shadow_regs_s *host_shadow_regs;

	bool recovery;
	bool notice_send;
	uint32_t ce_irq_summary;
	/* No of copy engines supported */
	unsigned int ce_count;
	atomic_t active_tasklet_cnt;
	atomic_t link_suspended;
	uint32_t *vaddr_rri_on_ddr;
	int linkstate_vote;
	int fastpath_mode_on;
	atomic_t tasklet_from_intr;
	int htc_endpoint;
	qdf_dma_addr_t mem_pa;
	bool athdiag_procfs_inited;
#ifdef FEATURE_NAPI
	struct qca_napi_data napi_data;
#endif /* FEATURE_NAPI */
	struct hif_callbacks callbacks;
	uint32_t hif_con_param;
};

A_target_id_t hif_get_target_id(struct hif_softc *scn);
void hif_dump_pipe_debug_count(struct hif_softc *scn);

bool hif_target_forced_awake(struct hif_softc *scn);
bool hif_max_num_receives_reached(struct hif_softc *scn, unsigned int count);
int hif_bus_configure(struct hif_softc *scn);
int hif_config_ce(struct hif_softc *scn);
void hif_unconfig_ce(struct hif_softc *scn);
void hif_ce_prepare_config(struct hif_softc *scn);
int hif_set_hia(struct hif_softc *scn);
QDF_STATUS hif_ce_open(struct hif_softc *scn);
void hif_ce_close(struct hif_softc *scn);
int hif_wlan_enable(struct hif_softc *scn);
void hif_wlan_disable(struct hif_softc *scn);
int athdiag_procfs_init(void *scn);
void athdiag_procfs_remove(void);
/* routine to modify the initial buffer count to be allocated on an os
 * platform basis. Platform owner will need to modify this as needed
 */
qdf_size_t init_buffer_count(qdf_size_t maxSize);

irqreturn_t hif_fw_interrupt_handler(int irq, void *arg);
int hif_get_target_type(struct hif_softc *ol_sc, struct device *dev,
	void *bdev, const hif_bus_id *bid, uint32_t *hif_type,
	uint32_t *target_type);
int hif_get_device_type(uint32_t device_id,
			uint32_t revision_id,
			uint32_t *hif_type, uint32_t *target_type);
/*These functions are exposed to HDD*/
bool hif_targ_is_awake(struct hif_softc *scn, void *__iomem *mem);
void hif_nointrs(struct hif_softc *scn);
void hif_bus_close(struct hif_softc *ol_sc);
QDF_STATUS hif_bus_open(struct hif_softc *ol_sc,
	enum qdf_bus_type bus_type);
QDF_STATUS hif_enable_bus(struct hif_softc *ol_sc, struct device *dev,
	void *bdev, const hif_bus_id *bid, enum hif_enable_type type);
void hif_disable_bus(struct hif_softc *scn);
void hif_bus_prevent_linkdown(struct hif_softc *scn, bool flag);
int hif_bus_get_context_size(void);
void hif_read_phy_mem_base(struct hif_softc *scn, qdf_dma_addr_t *bar_value);
uint32_t hif_get_conparam(struct hif_softc *scn);
struct hif_callbacks *hif_get_callbacks_handle(struct hif_softc *scn);
bool hif_is_driver_unloading(struct hif_softc *scn);
bool hif_is_load_or_unload_in_progress(struct hif_softc *scn);
bool hif_is_recovery_in_progress(struct hif_softc *scn);
#endif /* __HIF_MAIN_H__ */
