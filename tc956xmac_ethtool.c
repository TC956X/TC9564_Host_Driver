/*
 * TC956X ethernet driver.
 *
 * tc956xmac_ethtool.c - Ethtool support
 *
 * Copyright (C) 2007-2009  STMicroelectronics Ltd
 * Copyright (C) 2021 Toshiba Electronic Devices & Storage Corporation
 *
 * This file has been derived from the STMicro Linux driver,
 * and developed or modified for TC956X.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*! History:
 *  20 Jan 2021 : Initial Version
 *  VERSION     : 00-01
 *
 *  15 Mar 2021 : Base lined
 *  VERSION     : 01-00
 */

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/interrupt.h>
#include <linux/mii.h>
#include <linux/phylink.h>
#include <linux/net_tstamp.h>
#include <asm/io.h>

#include "tc956xmac.h"
#include "dwxgmac2.h"

#define REG_SPACE_SIZE	11512/*Total Reg Len*/
#define MAC100_ETHTOOL_NAME	"tc956x_mac100"
#define GMAC_ETHTOOL_NAME	"tc956x_gmac"
#define XGMAC_ETHTOOL_NAME	"tc956x_xgmac"

#define ETHTOOL_DMA_OFFSET	55

struct tc956xmac_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

#define TC956XMAC_STAT(m)	\
	{ #m, sizeof_field(struct tc956xmac_extra_stats, m),	\
	offsetof(struct tc956xmac_priv, xstats.m)}

static const struct tc956xmac_stats tc956xmac_gstrings_stats[] = {
	/* Transmit errors */
	TC956XMAC_STAT(tx_underflow),
	TC956XMAC_STAT(tx_carrier),
	TC956XMAC_STAT(tx_losscarrier),
	TC956XMAC_STAT(vlan_tag),
	TC956XMAC_STAT(tx_deferred),
	TC956XMAC_STAT(tx_vlan),
	TC956XMAC_STAT(tx_jabber),
	TC956XMAC_STAT(tx_frame_flushed),
	TC956XMAC_STAT(tx_payload_error),
	TC956XMAC_STAT(tx_ip_header_error),
	/* Receive errors */
	TC956XMAC_STAT(rx_desc),
	TC956XMAC_STAT(sa_filter_fail),
	TC956XMAC_STAT(overflow_error),
	TC956XMAC_STAT(ipc_csum_error),
	TC956XMAC_STAT(rx_collision),
	TC956XMAC_STAT(rx_crc_errors),
	TC956XMAC_STAT(dribbling_bit),
	TC956XMAC_STAT(rx_length),
	TC956XMAC_STAT(rx_mii),
	TC956XMAC_STAT(rx_multicast),
	TC956XMAC_STAT(rx_gmac_overflow),
	TC956XMAC_STAT(rx_watchdog),
	TC956XMAC_STAT(da_rx_filter_fail),
	TC956XMAC_STAT(sa_rx_filter_fail),
	TC956XMAC_STAT(rx_missed_cntr),
	TC956XMAC_STAT(rx_overflow_cntr),
	TC956XMAC_STAT(rx_vlan),
	TC956XMAC_STAT(rx_split_hdr_pkt_n),
	/* Tx/Rx IRQ error info */
	TC956XMAC_STAT(tx_undeflow_irq),
	TC956XMAC_STAT(tx_process_stopped_irq[0]),
	TC956XMAC_STAT(tx_process_stopped_irq[1]),
	TC956XMAC_STAT(tx_process_stopped_irq[2]),
	TC956XMAC_STAT(tx_process_stopped_irq[3]),
	TC956XMAC_STAT(tx_process_stopped_irq[4]),
	TC956XMAC_STAT(tx_process_stopped_irq[5]),
	TC956XMAC_STAT(tx_process_stopped_irq[6]),
	TC956XMAC_STAT(tx_process_stopped_irq[7]),
	TC956XMAC_STAT(tx_jabber_irq),
	TC956XMAC_STAT(rx_overflow_irq),
	TC956XMAC_STAT(rx_buf_unav_irq[0]),
	TC956XMAC_STAT(rx_buf_unav_irq[1]),
	TC956XMAC_STAT(rx_buf_unav_irq[2]),
	TC956XMAC_STAT(rx_buf_unav_irq[3]),
	TC956XMAC_STAT(rx_buf_unav_irq[4]),
	TC956XMAC_STAT(rx_buf_unav_irq[5]),
	TC956XMAC_STAT(rx_buf_unav_irq[6]),
	TC956XMAC_STAT(rx_buf_unav_irq[7]),
	TC956XMAC_STAT(rx_process_stopped_irq),
	TC956XMAC_STAT(rx_watchdog_irq),
	TC956XMAC_STAT(tx_early_irq),
	TC956XMAC_STAT(fatal_bus_error_irq[0]),
	TC956XMAC_STAT(fatal_bus_error_irq[1]),
	TC956XMAC_STAT(fatal_bus_error_irq[2]),
	TC956XMAC_STAT(fatal_bus_error_irq[3]),
	TC956XMAC_STAT(fatal_bus_error_irq[4]),
	TC956XMAC_STAT(fatal_bus_error_irq[5]),
	TC956XMAC_STAT(fatal_bus_error_irq[6]),
	TC956XMAC_STAT(fatal_bus_error_irq[7]),
	/* Tx/Rx IRQ Events */
	TC956XMAC_STAT(rx_early_irq),
	TC956XMAC_STAT(threshold),
	TC956XMAC_STAT(tx_pkt_n[0]),
	TC956XMAC_STAT(tx_pkt_n[1]),
	TC956XMAC_STAT(tx_pkt_n[2]),
	TC956XMAC_STAT(tx_pkt_n[3]),
	TC956XMAC_STAT(tx_pkt_n[4]),
	TC956XMAC_STAT(tx_pkt_n[5]),
	TC956XMAC_STAT(tx_pkt_n[6]),
	TC956XMAC_STAT(tx_pkt_n[7]),
	TC956XMAC_STAT(tx_pkt_errors_n[0]),
	TC956XMAC_STAT(tx_pkt_errors_n[1]),
	TC956XMAC_STAT(tx_pkt_errors_n[2]),
	TC956XMAC_STAT(tx_pkt_errors_n[3]),
	TC956XMAC_STAT(tx_pkt_errors_n[4]),
	TC956XMAC_STAT(tx_pkt_errors_n[5]),
	TC956XMAC_STAT(tx_pkt_errors_n[6]),
	TC956XMAC_STAT(tx_pkt_errors_n[7]),
	TC956XMAC_STAT(rx_pkt_n[0]),
	TC956XMAC_STAT(rx_pkt_n[1]),
	TC956XMAC_STAT(rx_pkt_n[2]),
	TC956XMAC_STAT(rx_pkt_n[3]),
	TC956XMAC_STAT(rx_pkt_n[4]),
	TC956XMAC_STAT(rx_pkt_n[5]),
	TC956XMAC_STAT(rx_pkt_n[6]),
	TC956XMAC_STAT(rx_pkt_n[7]),
	TC956XMAC_STAT(normal_irq_n[0]),
	TC956XMAC_STAT(normal_irq_n[1]),
	TC956XMAC_STAT(normal_irq_n[2]),
	TC956XMAC_STAT(normal_irq_n[3]),
	TC956XMAC_STAT(normal_irq_n[4]),
	TC956XMAC_STAT(normal_irq_n[5]),
	TC956XMAC_STAT(normal_irq_n[6]),
	TC956XMAC_STAT(normal_irq_n[7]),
	TC956XMAC_STAT(rx_normal_irq_n[0]),
	TC956XMAC_STAT(rx_normal_irq_n[1]),
	TC956XMAC_STAT(rx_normal_irq_n[2]),
	TC956XMAC_STAT(rx_normal_irq_n[3]),
	TC956XMAC_STAT(rx_normal_irq_n[4]),
	TC956XMAC_STAT(rx_normal_irq_n[5]),
	TC956XMAC_STAT(rx_normal_irq_n[6]),
	TC956XMAC_STAT(rx_normal_irq_n[7]),
	TC956XMAC_STAT(napi_poll_tx[0]),
	TC956XMAC_STAT(napi_poll_tx[1]),
	TC956XMAC_STAT(napi_poll_tx[2]),
	TC956XMAC_STAT(napi_poll_tx[3]),
	TC956XMAC_STAT(napi_poll_tx[4]),
	TC956XMAC_STAT(napi_poll_tx[5]),
	TC956XMAC_STAT(napi_poll_tx[6]),
	TC956XMAC_STAT(napi_poll_tx[7]),
	TC956XMAC_STAT(napi_poll_rx[0]),
	TC956XMAC_STAT(napi_poll_rx[1]),
	TC956XMAC_STAT(napi_poll_rx[2]),
	TC956XMAC_STAT(napi_poll_rx[3]),
	TC956XMAC_STAT(napi_poll_rx[4]),
	TC956XMAC_STAT(napi_poll_rx[5]),
	TC956XMAC_STAT(napi_poll_rx[6]),
	TC956XMAC_STAT(napi_poll_rx[7]),
	TC956XMAC_STAT(tx_normal_irq_n[0]),
	TC956XMAC_STAT(tx_normal_irq_n[1]),
	TC956XMAC_STAT(tx_normal_irq_n[2]),
	TC956XMAC_STAT(tx_normal_irq_n[3]),
	TC956XMAC_STAT(tx_normal_irq_n[4]),
	TC956XMAC_STAT(tx_normal_irq_n[5]),
	TC956XMAC_STAT(tx_normal_irq_n[6]),
	TC956XMAC_STAT(tx_normal_irq_n[7]),
	TC956XMAC_STAT(tx_clean[0]),
	TC956XMAC_STAT(tx_clean[1]),
	TC956XMAC_STAT(tx_clean[2]),
	TC956XMAC_STAT(tx_clean[3]),
	TC956XMAC_STAT(tx_clean[4]),
	TC956XMAC_STAT(tx_clean[5]),
	TC956XMAC_STAT(tx_clean[6]),
	TC956XMAC_STAT(tx_clean[7]),
	TC956XMAC_STAT(tx_set_ic_bit),
	TC956XMAC_STAT(irq_receive_pmt_irq_n),
	/* MMC info */
	TC956XMAC_STAT(mmc_tx_irq_n),
	TC956XMAC_STAT(mmc_rx_irq_n),
	TC956XMAC_STAT(mmc_rx_csum_offload_irq_n),
	/* EEE */
	TC956XMAC_STAT(irq_tx_path_in_lpi_mode_n),
	TC956XMAC_STAT(irq_tx_path_exit_lpi_mode_n),
	TC956XMAC_STAT(irq_rx_path_in_lpi_mode_n),
	TC956XMAC_STAT(irq_rx_path_exit_lpi_mode_n),
	TC956XMAC_STAT(phy_eee_wakeup_error_n),
	/* Extended RDES status */
	TC956XMAC_STAT(ip_hdr_err),
	TC956XMAC_STAT(ip_payload_err),
	TC956XMAC_STAT(ip_csum_bypassed),
	TC956XMAC_STAT(ipv4_pkt_rcvd),
	TC956XMAC_STAT(ipv6_pkt_rcvd),
	TC956XMAC_STAT(no_ptp_rx_msg_type_ext),
	TC956XMAC_STAT(ptp_rx_msg_type_sync),
	TC956XMAC_STAT(ptp_rx_msg_type_follow_up),
	TC956XMAC_STAT(ptp_rx_msg_type_delay_req),
	TC956XMAC_STAT(ptp_rx_msg_type_delay_resp),
	TC956XMAC_STAT(ptp_rx_msg_type_pdelay_req),
	TC956XMAC_STAT(ptp_rx_msg_type_pdelay_resp),
	TC956XMAC_STAT(ptp_rx_msg_type_pdelay_follow_up),
	TC956XMAC_STAT(ptp_rx_msg_type_announce),
	TC956XMAC_STAT(ptp_rx_msg_type_management),
	TC956XMAC_STAT(ptp_rx_msg_pkt_reserved_type),
	TC956XMAC_STAT(ptp_frame_type),
	TC956XMAC_STAT(ptp_ver),
	TC956XMAC_STAT(timestamp_dropped),
	TC956XMAC_STAT(av_pkt_rcvd),
	TC956XMAC_STAT(av_tagged_pkt_rcvd),
	TC956XMAC_STAT(vlan_tag_priority_val),
	TC956XMAC_STAT(l3_filter_match),
	TC956XMAC_STAT(l4_filter_match),
	TC956XMAC_STAT(l3_l4_filter_no_match),
	/* PCS */
	TC956XMAC_STAT(irq_pcs_ane_n),
	TC956XMAC_STAT(irq_pcs_link_n),
	TC956XMAC_STAT(irq_rgmii_n),
	/* DEBUG */
	TC956XMAC_STAT(mtl_tx_status_fifo_full),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[0]),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[1]),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[2]),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[3]),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[4]),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[5]),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[6]),
	TC956XMAC_STAT(mtl_tx_fifo_not_empty[7]),
	TC956XMAC_STAT(mmtl_fifo_ctrl[0]),
	TC956XMAC_STAT(mmtl_fifo_ctrl[1]),
	TC956XMAC_STAT(mmtl_fifo_ctrl[3]),
	TC956XMAC_STAT(mmtl_fifo_ctrl[4]),
	TC956XMAC_STAT(mmtl_fifo_ctrl[5]),
	TC956XMAC_STAT(mmtl_fifo_ctrl[6]),
	TC956XMAC_STAT(mmtl_fifo_ctrl[7]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[0]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[1]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[2]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[3]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[4]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[5]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[6]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_write[7]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[0]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[1]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[2]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[3]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[4]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[5]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[6]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_wait[7]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[0]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[1]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[2]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[3]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[4]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[5]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[6]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_read[7]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[0]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[1]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[2]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[3]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[4]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[5]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[6]),
	TC956XMAC_STAT(mtl_tx_fifo_read_ctrl_idle[7]),
	TC956XMAC_STAT(mac_tx_in_pause[0]),
	TC956XMAC_STAT(mac_tx_in_pause[1]),
	TC956XMAC_STAT(mac_tx_in_pause[2]),
	TC956XMAC_STAT(mac_tx_in_pause[3]),
	TC956XMAC_STAT(mac_tx_in_pause[4]),
	TC956XMAC_STAT(mac_tx_in_pause[5]),
	TC956XMAC_STAT(mac_tx_in_pause[6]),
	TC956XMAC_STAT(mac_tx_in_pause[7]),
	TC956XMAC_STAT(mac_tx_frame_ctrl_xfer),
	TC956XMAC_STAT(mac_tx_frame_ctrl_idle),
	TC956XMAC_STAT(mac_tx_frame_ctrl_wait),
	TC956XMAC_STAT(mac_tx_frame_ctrl_pause),
	TC956XMAC_STAT(mac_gmii_tx_proto_engine),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[0]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[1]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[2]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[3]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[4]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[5]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[6]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_full[7]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[0]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[1]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[2]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[3]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[4]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[5]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[6]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_above_thresh[7]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[0]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[1]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[2]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[3]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[4]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[5]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[6]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_below_thresh[7]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[0]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[1]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[2]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[3]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[4]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[5]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[6]),
	TC956XMAC_STAT(mtl_rx_fifo_fill_level_empty[7]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[0]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[1]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[2]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[3]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[4]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[5]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[6]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_flush[7]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[0]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[1]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[2]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[3]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[4]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[5]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[6]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_read[7]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[0]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[1]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[2]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[3]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[4]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[5]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[6]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_status[7]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[0]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[1]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[2]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[3]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[4]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[5]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[6]),
	TC956XMAC_STAT(mtl_rx_fifo_read_ctrl_idle[7]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[0]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[1]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[2]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[3]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[4]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[5]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[6]),
	TC956XMAC_STAT(mtl_rx_fifo_ctrl_active[7]),
	TC956XMAC_STAT(mac_rx_frame_ctrl_fifo),
	TC956XMAC_STAT(mac_gmii_rx_proto_engine),
	/* TSO */
	TC956XMAC_STAT(tx_tso_frames[0]),
	TC956XMAC_STAT(tx_tso_frames[1]),
	TC956XMAC_STAT(tx_tso_frames[2]),
	TC956XMAC_STAT(tx_tso_frames[3]),
	TC956XMAC_STAT(tx_tso_frames[4]),
	TC956XMAC_STAT(tx_tso_frames[5]),
	TC956XMAC_STAT(tx_tso_frames[6]),
	TC956XMAC_STAT(tx_tso_frames[7]),
	TC956XMAC_STAT(tx_tso_nfrags[0]),
	TC956XMAC_STAT(tx_tso_nfrags[1]),
	TC956XMAC_STAT(tx_tso_nfrags[2]),
	TC956XMAC_STAT(tx_tso_nfrags[3]),
	TC956XMAC_STAT(tx_tso_nfrags[4]),
	TC956XMAC_STAT(tx_tso_nfrags[5]),
	TC956XMAC_STAT(tx_tso_nfrags[6]),
	TC956XMAC_STAT(tx_tso_nfrags[7]),

	/* Tx Desc statistics */
	TC956XMAC_STAT(txch_status[0]),
	TC956XMAC_STAT(txch_status[1]),
	TC956XMAC_STAT(txch_status[2]),
	TC956XMAC_STAT(txch_status[3]),
	TC956XMAC_STAT(txch_status[4]),
	TC956XMAC_STAT(txch_status[5]),
	TC956XMAC_STAT(txch_status[6]),
	TC956XMAC_STAT(txch_status[7]),
	TC956XMAC_STAT(txch_control[0]),
	TC956XMAC_STAT(txch_control[1]),
	TC956XMAC_STAT(txch_control[2]),
	TC956XMAC_STAT(txch_control[3]),
	TC956XMAC_STAT(txch_control[4]),
	TC956XMAC_STAT(txch_control[5]),
	TC956XMAC_STAT(txch_control[6]),
	TC956XMAC_STAT(txch_control[7]),
	TC956XMAC_STAT(txch_desc_list_haddr[0]),
	TC956XMAC_STAT(txch_desc_list_haddr[1]),
	TC956XMAC_STAT(txch_desc_list_haddr[2]),
	TC956XMAC_STAT(txch_desc_list_haddr[3]),
	TC956XMAC_STAT(txch_desc_list_haddr[4]),
	TC956XMAC_STAT(txch_desc_list_haddr[5]),
	TC956XMAC_STAT(txch_desc_list_haddr[6]),
	TC956XMAC_STAT(txch_desc_list_haddr[7]),
	TC956XMAC_STAT(txch_desc_list_laddr[0]),
	TC956XMAC_STAT(txch_desc_list_laddr[1]),
	TC956XMAC_STAT(txch_desc_list_laddr[2]),
	TC956XMAC_STAT(txch_desc_list_laddr[3]),
	TC956XMAC_STAT(txch_desc_list_laddr[4]),
	TC956XMAC_STAT(txch_desc_list_laddr[5]),
	TC956XMAC_STAT(txch_desc_list_laddr[6]),
	TC956XMAC_STAT(txch_desc_list_laddr[7]),
	TC956XMAC_STAT(txch_desc_ring_len[0]),
	TC956XMAC_STAT(txch_desc_ring_len[1]),
	TC956XMAC_STAT(txch_desc_ring_len[2]),
	TC956XMAC_STAT(txch_desc_ring_len[3]),
	TC956XMAC_STAT(txch_desc_ring_len[4]),
	TC956XMAC_STAT(txch_desc_ring_len[5]),
	TC956XMAC_STAT(txch_desc_ring_len[6]),
	TC956XMAC_STAT(txch_desc_ring_len[7]),
	TC956XMAC_STAT(txch_desc_curr_haddr[0]),
	TC956XMAC_STAT(txch_desc_curr_haddr[1]),
	TC956XMAC_STAT(txch_desc_curr_haddr[2]),
	TC956XMAC_STAT(txch_desc_curr_haddr[3]),
	TC956XMAC_STAT(txch_desc_curr_haddr[4]),
	TC956XMAC_STAT(txch_desc_curr_haddr[5]),
	TC956XMAC_STAT(txch_desc_curr_haddr[6]),
	TC956XMAC_STAT(txch_desc_curr_haddr[7]),
	TC956XMAC_STAT(txch_desc_curr_laddr[0]),
	TC956XMAC_STAT(txch_desc_curr_laddr[1]),
	TC956XMAC_STAT(txch_desc_curr_laddr[2]),
	TC956XMAC_STAT(txch_desc_curr_laddr[3]),
	TC956XMAC_STAT(txch_desc_curr_laddr[4]),
	TC956XMAC_STAT(txch_desc_curr_laddr[5]),
	TC956XMAC_STAT(txch_desc_curr_laddr[6]),
	TC956XMAC_STAT(txch_desc_curr_laddr[7]),
	TC956XMAC_STAT(txch_desc_tail[0]),
	TC956XMAC_STAT(txch_desc_tail[1]),
	TC956XMAC_STAT(txch_desc_tail[2]),
	TC956XMAC_STAT(txch_desc_tail[3]),
	TC956XMAC_STAT(txch_desc_tail[4]),
	TC956XMAC_STAT(txch_desc_tail[5]),
	TC956XMAC_STAT(txch_desc_tail[6]),
	TC956XMAC_STAT(txch_desc_tail[7]),
	TC956XMAC_STAT(txch_desc_buf_haddr[0]),
	TC956XMAC_STAT(txch_desc_buf_haddr[1]),
	TC956XMAC_STAT(txch_desc_buf_haddr[2]),
	TC956XMAC_STAT(txch_desc_buf_haddr[3]),
	TC956XMAC_STAT(txch_desc_buf_haddr[4]),
	TC956XMAC_STAT(txch_desc_buf_haddr[5]),
	TC956XMAC_STAT(txch_desc_buf_haddr[6]),
	TC956XMAC_STAT(txch_desc_buf_haddr[7]),
	TC956XMAC_STAT(txch_desc_buf_laddr[0]),
	TC956XMAC_STAT(txch_desc_buf_laddr[1]),
	TC956XMAC_STAT(txch_desc_buf_laddr[2]),
	TC956XMAC_STAT(txch_desc_buf_laddr[3]),
	TC956XMAC_STAT(txch_desc_buf_laddr[4]),
	TC956XMAC_STAT(txch_desc_buf_laddr[5]),
	TC956XMAC_STAT(txch_desc_buf_laddr[6]),
	TC956XMAC_STAT(txch_desc_buf_laddr[7]),
	TC956XMAC_STAT(txch_sw_cur_tx[0]),
	TC956XMAC_STAT(txch_sw_cur_tx[1]),
	TC956XMAC_STAT(txch_sw_cur_tx[2]),
	TC956XMAC_STAT(txch_sw_cur_tx[3]),
	TC956XMAC_STAT(txch_sw_cur_tx[4]),
	TC956XMAC_STAT(txch_sw_cur_tx[5]),
	TC956XMAC_STAT(txch_sw_cur_tx[6]),
	TC956XMAC_STAT(txch_sw_cur_tx[7]),
	TC956XMAC_STAT(txch_sw_dirty_tx[0]),
	TC956XMAC_STAT(txch_sw_dirty_tx[1]),
	TC956XMAC_STAT(txch_sw_dirty_tx[2]),
	TC956XMAC_STAT(txch_sw_dirty_tx[3]),
	TC956XMAC_STAT(txch_sw_dirty_tx[4]),
	TC956XMAC_STAT(txch_sw_dirty_tx[5]),
	TC956XMAC_STAT(txch_sw_dirty_tx[6]),
	TC956XMAC_STAT(txch_sw_dirty_tx[7]),

	/* Rx Desc statistics */
	TC956XMAC_STAT(rxch_status[0]),
	TC956XMAC_STAT(rxch_status[1]),
	TC956XMAC_STAT(rxch_status[2]),
	TC956XMAC_STAT(rxch_status[3]),
	TC956XMAC_STAT(rxch_status[4]),
	TC956XMAC_STAT(rxch_status[5]),
	TC956XMAC_STAT(rxch_status[6]),
	TC956XMAC_STAT(rxch_status[7]),
	TC956XMAC_STAT(rxch_control[0]),
	TC956XMAC_STAT(rxch_control[1]),
	TC956XMAC_STAT(rxch_control[2]),
	TC956XMAC_STAT(rxch_control[3]),
	TC956XMAC_STAT(rxch_control[4]),
	TC956XMAC_STAT(rxch_control[5]),
	TC956XMAC_STAT(rxch_control[6]),
	TC956XMAC_STAT(rxch_control[7]),
	TC956XMAC_STAT(rxch_desc_list_haddr[0]),
	TC956XMAC_STAT(rxch_desc_list_haddr[1]),
	TC956XMAC_STAT(rxch_desc_list_haddr[2]),
	TC956XMAC_STAT(rxch_desc_list_haddr[3]),
	TC956XMAC_STAT(rxch_desc_list_haddr[4]),
	TC956XMAC_STAT(rxch_desc_list_haddr[5]),
	TC956XMAC_STAT(rxch_desc_list_haddr[6]),
	TC956XMAC_STAT(rxch_desc_list_haddr[7]),
	TC956XMAC_STAT(rxch_desc_list_laddr[0]),
	TC956XMAC_STAT(rxch_desc_list_laddr[1]),
	TC956XMAC_STAT(rxch_desc_list_laddr[2]),
	TC956XMAC_STAT(rxch_desc_list_laddr[3]),
	TC956XMAC_STAT(rxch_desc_list_laddr[4]),
	TC956XMAC_STAT(rxch_desc_list_laddr[5]),
	TC956XMAC_STAT(rxch_desc_list_laddr[6]),
	TC956XMAC_STAT(rxch_desc_list_laddr[7]),
	TC956XMAC_STAT(rxch_desc_ring_len[0]),
	TC956XMAC_STAT(rxch_desc_ring_len[1]),
	TC956XMAC_STAT(rxch_desc_ring_len[2]),
	TC956XMAC_STAT(rxch_desc_ring_len[3]),
	TC956XMAC_STAT(rxch_desc_ring_len[4]),
	TC956XMAC_STAT(rxch_desc_ring_len[5]),
	TC956XMAC_STAT(rxch_desc_ring_len[6]),
	TC956XMAC_STAT(rxch_desc_ring_len[7]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[0]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[1]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[2]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[3]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[4]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[5]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[6]),
	TC956XMAC_STAT(rxch_desc_curr_haddr[7]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[0]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[1]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[2]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[3]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[4]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[5]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[6]),
	TC956XMAC_STAT(rxch_desc_curr_laddr[7]),
	TC956XMAC_STAT(rxch_desc_tail[0]),
	TC956XMAC_STAT(rxch_desc_tail[1]),
	TC956XMAC_STAT(rxch_desc_tail[2]),
	TC956XMAC_STAT(rxch_desc_tail[3]),
	TC956XMAC_STAT(rxch_desc_tail[4]),
	TC956XMAC_STAT(rxch_desc_tail[5]),
	TC956XMAC_STAT(rxch_desc_tail[6]),
	TC956XMAC_STAT(rxch_desc_tail[7]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[0]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[1]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[2]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[3]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[4]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[5]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[6]),
	TC956XMAC_STAT(rxch_desc_buf_haddr[7]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[0]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[1]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[2]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[3]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[4]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[5]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[6]),
	TC956XMAC_STAT(rxch_desc_buf_laddr[7]),
	TC956XMAC_STAT(rxch_sw_cur_rx[0]),
	TC956XMAC_STAT(rxch_sw_cur_rx[1]),
	TC956XMAC_STAT(rxch_sw_cur_rx[2]),
	TC956XMAC_STAT(rxch_sw_cur_rx[3]),
	TC956XMAC_STAT(rxch_sw_cur_rx[4]),
	TC956XMAC_STAT(rxch_sw_cur_rx[5]),
	TC956XMAC_STAT(rxch_sw_cur_rx[6]),
	TC956XMAC_STAT(rxch_sw_cur_rx[7]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[0]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[1]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[2]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[3]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[4]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[5]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[6]),
	TC956XMAC_STAT(rxch_sw_dirty_rx[7]),

};
#define TC956XMAC_STATS_LEN ARRAY_SIZE(tc956xmac_gstrings_stats)

/* HW MAC Management counters (if supported) */
#define TC956XMAC_MMC_STAT(m)	\
	{ #m, sizeof_field(struct tc956xmac_counters, m),	\
	offsetof(struct tc956xmac_priv, mmc.m)}

static const struct tc956xmac_stats tc956xmac_mmc[] = {
	TC956XMAC_MMC_STAT(mmc_tx_octetcount_gb),
	TC956XMAC_MMC_STAT(mmc_tx_framecount_gb),
	TC956XMAC_MMC_STAT(mmc_tx_broadcastframe_g),
	TC956XMAC_MMC_STAT(mmc_tx_multicastframe_g),
	TC956XMAC_MMC_STAT(mmc_tx_64_octets_gb),
	TC956XMAC_MMC_STAT(mmc_tx_65_to_127_octets_gb),
	TC956XMAC_MMC_STAT(mmc_tx_128_to_255_octets_gb),
	TC956XMAC_MMC_STAT(mmc_tx_256_to_511_octets_gb),
	TC956XMAC_MMC_STAT(mmc_tx_512_to_1023_octets_gb),
	TC956XMAC_MMC_STAT(mmc_tx_1024_to_max_octets_gb),
	TC956XMAC_MMC_STAT(mmc_tx_unicast_gb),
	TC956XMAC_MMC_STAT(mmc_tx_multicast_gb),
	TC956XMAC_MMC_STAT(mmc_tx_broadcast_gb),
	TC956XMAC_MMC_STAT(mmc_tx_underflow_error),
	TC956XMAC_MMC_STAT(mmc_tx_singlecol_g),
	TC956XMAC_MMC_STAT(mmc_tx_multicol_g),
	TC956XMAC_MMC_STAT(mmc_tx_deferred),
	TC956XMAC_MMC_STAT(mmc_tx_latecol),
	TC956XMAC_MMC_STAT(mmc_tx_exesscol),
	TC956XMAC_MMC_STAT(mmc_tx_carrier_error),
	TC956XMAC_MMC_STAT(mmc_tx_octetcount_g),
	TC956XMAC_MMC_STAT(mmc_tx_framecount_g),
	TC956XMAC_MMC_STAT(mmc_tx_excessdef),
	TC956XMAC_MMC_STAT(mmc_tx_pause_frame),
	TC956XMAC_MMC_STAT(mmc_tx_vlan_frame_g),
	TC956XMAC_MMC_STAT(mmc_rx_framecount_gb),
	TC956XMAC_MMC_STAT(mmc_rx_octetcount_gb),
	TC956XMAC_MMC_STAT(mmc_rx_octetcount_g),
	TC956XMAC_MMC_STAT(mmc_rx_broadcastframe_g),
	TC956XMAC_MMC_STAT(mmc_rx_multicastframe_g),
	TC956XMAC_MMC_STAT(mmc_rx_crc_error),
	TC956XMAC_MMC_STAT(mmc_rx_align_error),
	TC956XMAC_MMC_STAT(mmc_rx_run_error),
	TC956XMAC_MMC_STAT(mmc_rx_jabber_error),
	TC956XMAC_MMC_STAT(mmc_rx_undersize_g),
	TC956XMAC_MMC_STAT(mmc_rx_oversize_g),
	TC956XMAC_MMC_STAT(mmc_rx_64_octets_gb),
	TC956XMAC_MMC_STAT(mmc_rx_65_to_127_octets_gb),
	TC956XMAC_MMC_STAT(mmc_rx_128_to_255_octets_gb),
	TC956XMAC_MMC_STAT(mmc_rx_256_to_511_octets_gb),
	TC956XMAC_MMC_STAT(mmc_rx_512_to_1023_octets_gb),
	TC956XMAC_MMC_STAT(mmc_rx_1024_to_max_octets_gb),
	TC956XMAC_MMC_STAT(mmc_rx_unicast_g),
	TC956XMAC_MMC_STAT(mmc_rx_length_error),
	TC956XMAC_MMC_STAT(mmc_rx_autofrangetype),
	TC956XMAC_MMC_STAT(mmc_rx_pause_frames),
	TC956XMAC_MMC_STAT(mmc_rx_fifo_overflow),
	TC956XMAC_MMC_STAT(mmc_rx_vlan_frames_gb),
	TC956XMAC_MMC_STAT(mmc_rx_watchdog_error),
	TC956XMAC_MMC_STAT(mmc_rx_ipc_intr_mask),
	TC956XMAC_MMC_STAT(mmc_rx_ipc_intr),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_gd),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_hderr),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_nopay),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_frag),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_udsbl),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_gd_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_hderr_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_nopay_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_frag_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv4_udsbl_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv6_gd_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv6_hderr_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv6_nopay_octets),
	TC956XMAC_MMC_STAT(mmc_rx_ipv6_gd),
	TC956XMAC_MMC_STAT(mmc_rx_ipv6_hderr),
	TC956XMAC_MMC_STAT(mmc_rx_ipv6_nopay),
	TC956XMAC_MMC_STAT(mmc_rx_udp_gd),
	TC956XMAC_MMC_STAT(mmc_rx_udp_err),
	TC956XMAC_MMC_STAT(mmc_rx_tcp_gd),
	TC956XMAC_MMC_STAT(mmc_rx_tcp_err),
	TC956XMAC_MMC_STAT(mmc_rx_icmp_gd),
	TC956XMAC_MMC_STAT(mmc_rx_icmp_err),
	TC956XMAC_MMC_STAT(mmc_rx_udp_gd_octets),
	TC956XMAC_MMC_STAT(mmc_rx_udp_err_octets),
	TC956XMAC_MMC_STAT(mmc_rx_tcp_gd_octets),
	TC956XMAC_MMC_STAT(mmc_rx_tcp_err_octets),
	TC956XMAC_MMC_STAT(mmc_rx_icmp_gd_octets),
	TC956XMAC_MMC_STAT(mmc_rx_icmp_err_octets),
	TC956XMAC_MMC_STAT(mmc_tx_fpe_fragment_cntr),
	TC956XMAC_MMC_STAT(mmc_tx_hold_req_cntr),
	TC956XMAC_MMC_STAT(mmc_rx_packet_assembly_err_cntr),
	TC956XMAC_MMC_STAT(mmc_rx_packet_smd_err_cntr),
	TC956XMAC_MMC_STAT(mmc_rx_packet_assembly_ok_cntr),
	TC956XMAC_MMC_STAT(mmc_rx_fpe_fragment_cntr),
};
#define TC956XMAC_MMC_STATS_LEN ARRAY_SIZE(tc956xmac_mmc)

static const char tc956x_priv_flags_strings[][ETH_GSTRING_LEN] = {
#define TC956XMAC_TX_FCS	BIT(0)
"tx-fcs",
};

#define TC956X_PRIV_FLAGS_STR_LEN ARRAY_SIZE(tc956x_priv_flags_strings)

static void tc956xmac_ethtool_getdrvinfo(struct net_device *dev,
				      struct ethtool_drvinfo *info)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	struct tc956x_version *fw_version;
	int reg = 0;
	char fw_version_str[32];

#ifdef TC956X
	reg = readl(priv->tc956x_SRAM_pci_base_addr + TC956X_M3_DBG_VER_START);
#endif

	fw_version = (struct tc956x_version *)(&reg);
	sprintf(fw_version_str, "Firmware Version %s_%d.%d-%d", (fw_version->rel_dbg == 'D')?"DBG":"REL",
								fw_version->major, fw_version->minor,
								fw_version->sub_minor);

	strlcpy(info->fw_version, fw_version_str, sizeof(info->fw_version));

	if (priv->plat->has_gmac || priv->plat->has_gmac4)
		strlcpy(info->driver, GMAC_ETHTOOL_NAME, sizeof(info->driver));
	else if (priv->plat->has_xgmac)
		strlcpy(info->driver, XGMAC_ETHTOOL_NAME, sizeof(info->driver));
	else
		strlcpy(info->driver, MAC100_ETHTOOL_NAME,
			sizeof(info->driver));

	strlcpy(info->version, DRV_MODULE_VERSION, sizeof(info->version));

	info->n_priv_flags = TC956X_PRIV_FLAGS_STR_LEN;
}

static int tc956xmac_ethtool_get_link_ksettings(struct net_device *dev,
					     struct ethtool_link_ksettings *cmd)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	if (priv->hw->pcs & TC956XMAC_PCS_RGMII ||
	    priv->hw->pcs & TC956XMAC_PCS_SGMII ||
	    priv->hw->pcs & TC956XMAC_PCS_USXGMII) {
		struct rgmii_adv adv;
		u32 supported, advertising, lp_advertising;

		if (!priv->xstats.pcs_link) {
			cmd->base.speed = SPEED_UNKNOWN;
			cmd->base.duplex = DUPLEX_UNKNOWN;
			return 0;
		}
		cmd->base.duplex = priv->xstats.pcs_duplex;

		cmd->base.speed = priv->xstats.pcs_speed;

		/* Get and convert ADV/LP_ADV from the HW AN registers */
		if (tc956xmac_pcs_get_adv_lp(priv, priv->ioaddr, &adv))
			return -EOPNOTSUPP;	/* should never happen indeed */

		/* Encoding of PSE bits is defined in 802.3z, 37.2.1.4 */

		ethtool_convert_link_mode_to_legacy_u32(
			&supported, cmd->link_modes.supported);
		ethtool_convert_link_mode_to_legacy_u32(
			&advertising, cmd->link_modes.advertising);
		ethtool_convert_link_mode_to_legacy_u32(
			&lp_advertising, cmd->link_modes.lp_advertising);

		if (adv.pause & TC956XMAC_PCS_PAUSE)
			advertising |= ADVERTISED_Pause;
		if (adv.pause & TC956XMAC_PCS_ASYM_PAUSE)
			advertising |= ADVERTISED_Asym_Pause;
		if (adv.lp_pause & TC956XMAC_PCS_PAUSE)
			lp_advertising |= ADVERTISED_Pause;
		if (adv.lp_pause & TC956XMAC_PCS_ASYM_PAUSE)
			lp_advertising |= ADVERTISED_Asym_Pause;

		/* Reg49[3] always set because ANE is always supported */
		cmd->base.autoneg = ADVERTISED_Autoneg;
		supported |= SUPPORTED_Autoneg;
		advertising |= ADVERTISED_Autoneg;
		lp_advertising |= ADVERTISED_Autoneg;

		if (adv.duplex) {
			supported |= (SUPPORTED_1000baseT_Full |
				      SUPPORTED_100baseT_Full |
				      SUPPORTED_10baseT_Full);
			advertising |= (ADVERTISED_1000baseT_Full |
					ADVERTISED_100baseT_Full |
					ADVERTISED_10baseT_Full);
		} else {
			supported |= (SUPPORTED_1000baseT_Half |
				      SUPPORTED_100baseT_Half |
				      SUPPORTED_10baseT_Half);
			advertising |= (ADVERTISED_1000baseT_Half |
					ADVERTISED_100baseT_Half |
					ADVERTISED_10baseT_Half);
		}
		if (adv.lp_duplex)
			lp_advertising |= (ADVERTISED_1000baseT_Full |
					   ADVERTISED_100baseT_Full |
					   ADVERTISED_10baseT_Full);
		else
			lp_advertising |= (ADVERTISED_1000baseT_Half |
					   ADVERTISED_100baseT_Half |
					   ADVERTISED_10baseT_Half);
		cmd->base.port = PORT_OTHER;

		ethtool_convert_legacy_u32_to_link_mode(
			cmd->link_modes.supported, supported);
		ethtool_convert_legacy_u32_to_link_mode(
			cmd->link_modes.advertising, advertising);
		ethtool_convert_legacy_u32_to_link_mode(
			cmd->link_modes.lp_advertising, lp_advertising);

		return 0;
	}

	if (!netif_running(dev))
		return -EBUSY;

	if (dev->phydev == NULL)
		return -ENODEV;

	return phylink_ethtool_ksettings_get(priv->phylink, cmd);
}

static int
tc956xmac_ethtool_set_link_ksettings(struct net_device *dev,
				  const struct ethtool_link_ksettings *cmd)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	if (priv->hw->pcs & TC956XMAC_PCS_RGMII ||
	    priv->hw->pcs & TC956XMAC_PCS_SGMII ||
	    priv->hw->pcs & TC956XMAC_PCS_USXGMII) {
		u32 mask = ADVERTISED_Autoneg | ADVERTISED_Pause;

		/* Only support ANE */
		if (cmd->base.autoneg != AUTONEG_ENABLE)
			return -EINVAL;

		mask &= (ADVERTISED_1000baseT_Half |
			ADVERTISED_1000baseT_Full |
			ADVERTISED_100baseT_Half |
			ADVERTISED_100baseT_Full |
			ADVERTISED_10baseT_Half |
			ADVERTISED_10baseT_Full);

		mutex_lock(&priv->lock);
#ifdef TC956X
		tc956x_xpcs_ctrl_ane(priv, 1);
#else
		tc956xmac_pcs_ctrl_ane(priv, priv->ioaddr, 1, priv->hw->ps, 0);
#endif
		mutex_unlock(&priv->lock);

		return 0;
	}

	if (!dev->phydev)
		return -ENODEV;
	return phylink_ethtool_ksettings_set(priv->phylink, cmd);
}

static u32 tc956xmac_ethtool_getmsglevel(struct net_device *dev)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	return priv->msg_enable;
}

static void tc956xmac_ethtool_setmsglevel(struct net_device *dev, u32 level)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	priv->msg_enable = level;

}

static int tc956xmac_check_if_running(struct net_device *dev)
{
	if (!netif_running(dev))
		return -EBUSY;
	return 0;
}

static int tc956xmac_ethtool_get_regs_len(struct net_device *dev)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	if (priv->plat->has_xgmac)
		return XGMAC_REGSIZE * 4;
	return REG_SPACE_SIZE * sizeof(u32);
}

static void tc956xmac_ethtool_gregs(struct net_device *dev,
			  struct ethtool_regs *regs, void *space)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	u32 *reg_space = (u32 *) space;

	tc956xmac_dump_mac_regs(priv, priv->hw, reg_space);
	tc956xmac_dump_dma_regs(priv, priv->ioaddr, reg_space);

#ifndef TC956X
	if (!priv->plat->has_xgmac && !priv->plat->has_gmac4) {
		/* Copy DMA registers to where ethtool expects them */
		memcpy(&reg_space[ETHTOOL_DMA_OFFSET],
		       &reg_space[DMA_BUS_MODE / 4],
		       NUM_DWMAC1000_DMA_REGS * 4);
	}
#endif
}

#ifdef TC956X_UNSUPPORTED_UNTESTED_FEATURE
static int tc956xmac_nway_reset(struct net_device *dev)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	return phylink_ethtool_nway_reset(priv->phylink);
}
#endif

static void
tc956xmac_get_pauseparam(struct net_device *netdev,
		      struct ethtool_pauseparam *pause)
{
	struct tc956xmac_priv *priv = netdev_priv(netdev);
	struct rgmii_adv adv_lp;

	if (priv->hw->pcs &&
	    !tc956xmac_pcs_get_adv_lp(priv, priv->ioaddr, &adv_lp)) {
		pause->autoneg = 1;
		if (!adv_lp.pause)
			return;
	} else {
		phylink_ethtool_get_pauseparam(priv->phylink, pause);
		pause->rx_pause = (priv->flow_ctrl & FLOW_RX);
		pause->tx_pause = (priv->flow_ctrl & FLOW_TX);
	}
}

static int
tc956xmac_set_pauseparam(struct net_device *netdev,
		      struct ethtool_pauseparam *pause)
{
	struct tc956xmac_priv *priv = netdev_priv(netdev);
	int new_pause = FLOW_OFF;
	struct rgmii_adv adv_lp;
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	struct phy_device *phy = netdev->phydev;

	if (priv->hw->pcs &&
	    !tc956xmac_pcs_get_adv_lp(priv, priv->ioaddr, &adv_lp)) {
		pause->autoneg = 1;
		if (!adv_lp.pause)
			return -EOPNOTSUPP;
		return 0;
	} else {
		phylink_ethtool_set_pauseparam(priv->phylink, pause);
	}
	if (pause->rx_pause)
		new_pause |= FLOW_RX;
	if (pause->tx_pause)
		new_pause |= FLOW_TX;
	priv->flow_ctrl = new_pause;

	tc956xmac_flow_ctrl(priv, priv->hw, phy->duplex, priv->flow_ctrl,
				 priv->pause, tx_cnt);
	return 0;
}

static void tc956xmac_get_ethtool_stats(struct net_device *dev,
				 struct ethtool_stats *dummy, u64 *data)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 tx_queues_count = priv->plat->tx_queues_to_use;
	unsigned long count;
	int i, j = 0, ret;

	if (priv->dma_cap.asp) {
		for (i = 0; i < TC956XMAC_SAFETY_FEAT_SIZE; i++) {
			if (!tc956xmac_safety_feat_dump(priv, &priv->sstats, i,
						&count, NULL))
				data[j++] = count;
		}
	}

	/* Update the DMA HW counters for dwmac10/100 */
	ret = tc956xmac_dma_diagnostic_fr(priv, &dev->stats,
					(void *)&priv->xstats, priv->ioaddr);
	if (ret) {
		/* If supported, for new GMAC chips expose the MMC counters */
		if (priv->dma_cap.rmon) {
			tc956xmac_mmc_read(priv, priv->mmcaddr, &priv->mmc);

			for (i = 0; i < TC956XMAC_MMC_STATS_LEN; i++) {
				char *p;

				p = (char *)priv + tc956xmac_mmc[i].stat_offset;

				data[j++] = (tc956xmac_mmc[i].sizeof_stat ==
					     sizeof(u64)) ? (*(u64 *)p) :
					     (*(u32 *)p);
			}
		}
		if (priv->eee_enabled) {
			int val = phylink_get_eee_err(priv->phylink);

			if (val)
				priv->xstats.phy_eee_wakeup_error_n = val;
		}

		if (priv->synopsys_id >= DWMAC_CORE_3_50 ||
			priv->synopsys_id == DWXGMAC_CORE_3_01) {
			tc956xmac_mac_debug(priv, priv->ioaddr,
					(void *)&priv->xstats,
					rx_queues_count, tx_queues_count);

			tc956xmac_dma_desc_stats(priv, priv->ioaddr);
		}
	}
	for (i = 0; i < TC956XMAC_STATS_LEN; i++) {
		char *p = (char *)priv + tc956xmac_gstrings_stats[i].stat_offset;

		data[j++] = (tc956xmac_gstrings_stats[i].sizeof_stat ==
			     sizeof(u64)) ? (*(u64 *)p) : (*(u32 *)p);
	}
}

static int tc956xmac_get_sset_count(struct net_device *netdev, int sset)
{
	struct tc956xmac_priv *priv = netdev_priv(netdev);
	int i, len, safety_len = 0;

	switch (sset) {
	case ETH_SS_STATS:
		len = TC956XMAC_STATS_LEN;

		if (priv->dma_cap.rmon)
			len += TC956XMAC_MMC_STATS_LEN;
		if (priv->dma_cap.asp) {
			for (i = 0; i < TC956XMAC_SAFETY_FEAT_SIZE; i++) {
				if (!tc956xmac_safety_feat_dump(priv,
							&priv->sstats, i,
							NULL, NULL))
					safety_len++;
			}

			len += safety_len;
		}

		return len;
	case ETH_SS_TEST:
		return tc956xmac_selftest_get_count(priv);
	case ETH_SS_PRIV_FLAGS:
		return TC956X_PRIV_FLAGS_STR_LEN;
		break;
	default:
		return -EOPNOTSUPP;
	}
}

static void tc956xmac_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	int i;
	u8 *p = data;
	struct tc956xmac_priv *priv = netdev_priv(dev);

	switch (stringset) {
	case ETH_SS_STATS:
		if (priv->dma_cap.asp) {
			for (i = 0; i < TC956XMAC_SAFETY_FEAT_SIZE; i++) {
				const char *desc;

				if (!tc956xmac_safety_feat_dump(priv,
							&priv->sstats, i,
							NULL, &desc)) {
					memcpy(p, desc, ETH_GSTRING_LEN);
					p += ETH_GSTRING_LEN;
				}
			}
		}
		if (priv->dma_cap.rmon)
			for (i = 0; i < TC956XMAC_MMC_STATS_LEN; i++) {
				memcpy(p, tc956xmac_mmc[i].stat_string,
				       ETH_GSTRING_LEN);
				p += ETH_GSTRING_LEN;
			}
		for (i = 0; i < TC956XMAC_STATS_LEN; i++) {
			memcpy(p, tc956xmac_gstrings_stats[i].stat_string,
				ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		break;
	case ETH_SS_TEST:
		tc956xmac_selftest_get_strings(priv, p);
		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(data, tc956x_priv_flags_strings,
				TC956X_PRIV_FLAGS_STR_LEN * ETH_GSTRING_LEN);
		break;
	default:
		WARN_ON(1);
		break;
	}
}

static int tc956xmac_ethtool_op_get_eee(struct net_device *dev,
				     struct ethtool_eee *edata)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	if (!priv->dma_cap.eee)
		return -EOPNOTSUPP;

	edata->eee_enabled = priv->eee_enabled;
	edata->eee_active = priv->eee_active;
	edata->tx_lpi_timer = priv->tx_lpi_timer;

	return phylink_ethtool_get_eee(priv->phylink, edata);
}

static int tc956xmac_ethtool_op_set_eee(struct net_device *dev,
				     struct ethtool_eee *edata)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	int ret;

	if (!edata->eee_enabled) {
		tc956xmac_disable_eee_mode(priv);
	} else {
		/* We are asking for enabling the EEE but it is safe
		 * to verify all by invoking the eee_init function.
		 * In case of failure it will return an error.
		 */
		if (priv->tx_lpi_timer != edata->tx_lpi_timer)
			priv->tx_lpi_timer = edata->tx_lpi_timer;

		edata->eee_enabled = tc956xmac_eee_init(priv);
		if (!edata->eee_enabled)
			return -EOPNOTSUPP;
	}

	ret = phylink_ethtool_set_eee(priv->phylink, edata);
	if (ret)
		return ret;

	priv->eee_enabled = edata->eee_enabled;
	priv->tx_lpi_timer = edata->tx_lpi_timer;
	return 0;
}

static u32 tc956xmac_usec2riwt(u32 usec, struct tc956xmac_priv *priv)
{
	unsigned long clk = clk_get_rate(priv->plat->tc956xmac_clk);
	u32 value, mult = 256;

	if (!clk) {
		clk = TC956X_PTP_SYSCLOCK;
		if (!clk)
			return 0;
	}

	for (mult = 256; mult <= 2048; mult *= 2) {
		value = (usec * (clk / 1000000)) / mult;
		if (value <= 0xff)
			break;
	}

	return value;
}

static u32 tc956xmac_riwt2usec(u32 riwt, struct tc956xmac_priv *priv)
{
	unsigned long clk = clk_get_rate(priv->plat->tc956xmac_clk);
	u32 mult = 256;

	if (!clk) {
		clk = TC956X_PTP_SYSCLOCK;
		if (!clk)
			return 0;
	}

	if (riwt > (1024 * 0xff))
		mult = 2048;
	else if (riwt > (512 * 0xff))
		mult = 1024;
	else if (riwt > (256 * 0xff))
		mult = 512;
	else
		mult = 256;

	return (riwt * mult) / (clk / 1000000);
}

static int tc956xmac_get_coalesce(struct net_device *dev,
			       struct ethtool_coalesce *ec)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	ec->tx_coalesce_usecs = priv->tx_coal_timer;
	ec->tx_max_coalesced_frames = priv->tx_coal_frames;

	if (priv->use_riwt) {
		ec->rx_max_coalesced_frames = priv->rx_coal_frames;
		ec->rx_coalesce_usecs = tc956xmac_riwt2usec(priv->rx_riwt, priv);
	}

	return 0;
}

static int tc956xmac_set_coalesce(struct net_device *dev,
			       struct ethtool_coalesce *ec)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	u32 rx_cnt = priv->plat->rx_queues_to_use;
	unsigned int rx_riwt;

	/* Check not supported parameters  */
	if ((ec->rx_coalesce_usecs_irq) ||
	    (ec->rx_max_coalesced_frames_irq) || (ec->tx_coalesce_usecs_irq) ||
	    (ec->use_adaptive_rx_coalesce) || (ec->use_adaptive_tx_coalesce) ||
	    (ec->pkt_rate_low) || (ec->rx_coalesce_usecs_low) ||
	    (ec->rx_max_coalesced_frames_low) || (ec->tx_coalesce_usecs_high) ||
	    (ec->tx_max_coalesced_frames_low) || (ec->pkt_rate_high) ||
	    (ec->tx_coalesce_usecs_low) || (ec->rx_coalesce_usecs_high) ||
	    (ec->rx_max_coalesced_frames_high) ||
	    (ec->tx_max_coalesced_frames_irq) ||
	    (ec->stats_block_coalesce_usecs) ||
	    (ec->tx_max_coalesced_frames_high) || (ec->rate_sample_interval))
		return -EOPNOTSUPP;

	if (priv->use_riwt && (ec->rx_coalesce_usecs > 0)) {
		rx_riwt = tc956xmac_usec2riwt(ec->rx_coalesce_usecs, priv);

		if ((rx_riwt > MAX_DMA_RIWT) || (rx_riwt < MIN_DMA_RIWT)) {
			KPRINT_DEBUG1("Invalid rx_usecs value 0x%X\n", ec->rx_coalesce_usecs);
			return -EINVAL;
		}

		priv->rx_riwt = rx_riwt;
		tc956xmac_rx_watchdog(priv, priv->ioaddr, priv->rx_riwt, rx_cnt);
	}

	if (ec->rx_max_coalesced_frames > TC956XMAC_RX_MAX_FRAMES) {
		KPRINT_DEBUG1("Invalid rx_frames value 0x%X\n", ec->rx_max_coalesced_frames);
		return -EINVAL;
	}

	if ((ec->tx_coalesce_usecs == 0) &&
	    (ec->tx_max_coalesced_frames == 0))
		return -EINVAL;

	if ((ec->tx_coalesce_usecs > TC956XMAC_MAX_COAL_TX_TICK) ||
	    (ec->tx_max_coalesced_frames > TC956XMAC_TX_MAX_FRAMES))
		return -EINVAL;

	/* Only copy relevant parameters, ignore all others. */
	priv->tx_coal_frames = ec->tx_max_coalesced_frames;
	priv->tx_coal_timer = ec->tx_coalesce_usecs;
	priv->rx_coal_frames = ec->rx_max_coalesced_frames;
	return 0;
}

#ifndef TC956X
static int tc956xmac_get_rxnfc(struct net_device *dev,
			    struct ethtool_rxnfc *rxnfc, u32 *rule_locs)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	switch (rxnfc->cmd) {
	case ETHTOOL_GRXRINGS:
		rxnfc->data = priv->plat->rx_queues_to_use;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static u32 tc956xmac_get_rxfh_key_size(struct net_device *dev)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	return sizeof(priv->rss.key);
}

static u32 tc956xmac_get_rxfh_indir_size(struct net_device *dev)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	return ARRAY_SIZE(priv->rss.table);
}

static int tc956xmac_get_rxfh(struct net_device *dev, u32 *indir, u8 *key,
			   u8 *hfunc)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	int i;

	if (indir) {
		for (i = 0; i < ARRAY_SIZE(priv->rss.table); i++)
			indir[i] = priv->rss.table[i];
	}

	if (key)
		memcpy(key, priv->rss.key, sizeof(priv->rss.key));
	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;

	return 0;
}

static int tc956xmac_set_rxfh(struct net_device *dev, const u32 *indir,
			   const u8 *key, const u8 hfunc)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	int i;

	if ((hfunc != ETH_RSS_HASH_NO_CHANGE) && (hfunc != ETH_RSS_HASH_TOP))
		return -EOPNOTSUPP;

	if (indir) {
		for (i = 0; i < ARRAY_SIZE(priv->rss.table); i++)
			priv->rss.table[i] = indir[i];
	}

	if (key)
		memcpy(priv->rss.key, key, sizeof(priv->rss.key));

	return tc956xmac_rss_configure(priv, priv->hw, &priv->rss,
				    priv->plat->rx_queues_to_use);
}
#endif

static int tc956xmac_get_ts_info(struct net_device *dev,
			      struct ethtool_ts_info *info)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	if ((priv->dma_cap.time_stamp || priv->dma_cap.atime_stamp)) {

		info->so_timestamping = SOF_TIMESTAMPING_TX_SOFTWARE |
					SOF_TIMESTAMPING_TX_HARDWARE |
					SOF_TIMESTAMPING_RX_SOFTWARE |
					SOF_TIMESTAMPING_RX_HARDWARE |
					SOF_TIMESTAMPING_SOFTWARE |
					SOF_TIMESTAMPING_RAW_HARDWARE;

		if (priv->ptp_clock)
			info->phc_index = ptp_clock_index(priv->ptp_clock);

		info->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);

		info->rx_filters = ((1 << HWTSTAMP_FILTER_NONE) |
				    (1 << HWTSTAMP_FILTER_PTP_V1_L4_EVENT) |
				    (1 << HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
				    (1 << HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_L4_EVENT) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_EVENT) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_SYNC) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_DELAY_REQ) |
				    (1 << HWTSTAMP_FILTER_ALL));
		return 0;
	} else
		return ethtool_op_get_ts_info(dev, info);
}

#ifndef TC956X
static int tc956xmac_get_tunable(struct net_device *dev,
			      const struct ethtool_tunable *tuna, void *data)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	int ret = 0;

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		*(u32 *)data = priv->rx_copybreak;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int tc956xmac_set_tunable(struct net_device *dev,
			      const struct ethtool_tunable *tuna,
			      const void *data)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	int ret = 0;

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		priv->rx_copybreak = *(u32 *)data;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#endif

#ifdef TC956X
static int tc956x_set_priv_flag(struct net_device *dev, u32 priv_flag)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);

	if (priv_flag & TC956XMAC_TX_FCS)
		priv->tx_crc_pad_state = TC956X_TX_CRC_PAD_INSERT;
	else
		priv->tx_crc_pad_state = TC956X_TX_CRC_PAD_DISABLE;
	KPRINT_INFO("tx_crc_pad_state : %x", priv->tx_crc_pad_state);

	return 0;
}

static u32 tc956x_get_priv_flag(struct net_device *dev)
{
	struct tc956xmac_priv *priv = netdev_priv(dev);
	u32 ret;

	if (priv->tx_crc_pad_state == TC956X_TX_CRC_PAD_INSERT)
		ret = 1;
	else
		ret = 0;
	KPRINT_INFO("tx_crc_pad_state : %x", priv->tx_crc_pad_state);
	return ret;
}
#endif

static const struct ethtool_ops tc956xmac_ethtool_ops = {
	.begin = tc956xmac_check_if_running,
	.get_drvinfo = tc956xmac_ethtool_getdrvinfo,
	.get_msglevel = tc956xmac_ethtool_getmsglevel,
	.set_msglevel = tc956xmac_ethtool_setmsglevel,
	.get_regs = tc956xmac_ethtool_gregs,
	.get_regs_len = tc956xmac_ethtool_get_regs_len,
	.get_link = ethtool_op_get_link,
#ifdef TC956X_UNSUPPORTED_UNTESTED_FEATURE
	.nway_reset = tc956xmac_nway_reset,
#endif
	.get_pauseparam = tc956xmac_get_pauseparam,
	.set_pauseparam = tc956xmac_set_pauseparam,
	.self_test = tc956xmac_selftest_run,
	.get_ethtool_stats = tc956xmac_get_ethtool_stats,
	.get_strings = tc956xmac_get_strings,
	.get_eee = tc956xmac_ethtool_op_get_eee,
	.set_eee = tc956xmac_ethtool_op_set_eee,
	.get_sset_count	= tc956xmac_get_sset_count,
#ifndef TC956X
	.get_rxnfc = tc956xmac_get_rxnfc,
	.get_rxfh_key_size = tc956xmac_get_rxfh_key_size,
	.get_rxfh_indir_size = tc956xmac_get_rxfh_indir_size,
	.get_rxfh = tc956xmac_get_rxfh,
	.set_rxfh = tc956xmac_set_rxfh,
#endif
	.get_ts_info = tc956xmac_get_ts_info,
	.get_coalesce = tc956xmac_get_coalesce,
	.set_coalesce = tc956xmac_set_coalesce,
#ifndef TC956X
	.get_tunable = tc956xmac_get_tunable,
	.set_tunable = tc956xmac_set_tunable,
#endif
	.get_link_ksettings = tc956xmac_ethtool_get_link_ksettings,
	.set_link_ksettings = tc956xmac_ethtool_set_link_ksettings,
#ifdef TC956X
	.set_priv_flags = tc956x_set_priv_flag,
	.get_priv_flags = tc956x_get_priv_flag,
#endif
};

void tc956xmac_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &tc956xmac_ethtool_ops;
}
