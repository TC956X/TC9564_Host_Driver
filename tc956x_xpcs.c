/*
 * TC956x XPCS layer
 *
 * tc956x_xpcs.c
 *
 * Copyright (C) 2021 Toshiba Electronic Devices & Storage Corporation
 *
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
 *  05 Nov 2020 : Initial version
 *  VERSION     : 00-01
 *
 *  15 Mar 2021 : Base lined
 *  VERSION     : 01-00
 */

#include "common.h"
#include "tc956xmac.h"
#include "tc956x_xpcs.h"
#ifdef TC956X
u32 tc956x_xpcs_read(void __iomem *xpcsaddr, u32 pcs_reg_num)
{
	u32 reg_value;
	u16 base_address, offset;

	base_address = pcs_reg_num >> XPCS_REG_BASE_ADDR;
	offset = pcs_reg_num & XPCS_REG_OFFSET;

	KPRINT_INFO("XPCS Indirect Access Base Register : %x, offset : %x", base_address, offset);
	/*write base address to (PCS address + 0x3FC) register*/
	writel(base_address, (xpcsaddr + XPCS_IND_ACCESS));

	/*Access to offset address (PCS address + offset)*/
	reg_value = readl(xpcsaddr + offset);
	KPRINT_INFO("XPCS register %x indirect read access value : %x", pcs_reg_num, reg_value);

	return reg_value;
}

u32 tc956x_xpcs_write(void __iomem *xpcsaddr, u32 pcs_reg_num, u32 value)
{
	u16 base_address, offset;

	base_address = pcs_reg_num >> XPCS_REG_BASE_ADDR;
	offset = pcs_reg_num & XPCS_REG_OFFSET;

	KPRINT_INFO("XPCS Indirect Access Base Register : %x, offset : %x", base_address, offset);
	/*write base address to (PCS address + 0x3FC) register*/
	writel(base_address, (xpcsaddr + XPCS_IND_ACCESS));

	/*Access to offset address (PCS address + offset)*/
	writel(value, xpcsaddr + offset);
	KPRINT_INFO("XPCS register %x indirect write access value : %x", pcs_reg_num, value);

	return 0;
}


int tc956x_xpcs_init(struct tc956xmac_priv *priv, void __iomem *xpcsaddr)
{
	u32 reg_value;

	reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_SR_MII_CTRL);
	if (reg_value & XGMAC_SOFT_RST)
		return -1;

	/*Clause 37 autoneg related settings*/
	if (priv->plat->interface == PHY_INTERFACE_MODE_SGMII) {
		//DK2
		//PCS Type Select SR_XS_PCS_CTRL2  PCS_TYPE_SEL -> 1
		reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_SR_XS_PCS_CTRL2);
		reg_value &= XGMAC_PCS_TYPE_SEL;
		reg_value |= 0x1;
		tc956x_xpcs_write(xpcsaddr, XGMAC_SR_XS_PCS_CTRL2, reg_value);

		reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_MII_AN_CTRL);
		reg_value &= XGMAC_PCS_MODE_MASK;
		reg_value |= XGMAC_SGMII_MODE;/*SGMII PCS MODE*/
		tc956x_xpcs_write(xpcsaddr, XGMAC_VR_MII_AN_CTRL, reg_value);

#ifdef TC956X_SGMII_2P5_GBPS_TEST
		reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_XS_PCS_DIG_CTRL1);
		reg_value &= ~(0x4);
		reg_value |= 0x4; /*EN_2_5G_MODE*/
		tc956x_xpcs_write(xpcsaddr, XGMAC_VR_XS_PCS_DIG_CTRL1, reg_value);
#endif
	}

	if (priv->plat->interface == PHY_INTERFACE_MODE_USXGMII) {
		reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_SR_XS_PCS_CTRL2);
		reg_value &= XGMAC_PCS_TYPE_SEL;/*PCS_TYPE_SEL as 10GBASE-R PCS */
		tc956x_xpcs_write(xpcsaddr, XGMAC_SR_XS_PCS_CTRL2, reg_value);

		reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_XS_PCS_DIG_CTRL1);
		reg_value |= XGMAC_USXG_EN;/*set USXG_EN*/
		tc956x_xpcs_write(xpcsaddr, XGMAC_VR_XS_PCS_DIG_CTRL1, reg_value);

		reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_XS_PCS_KR_CTRL);
		reg_value &= ~XGMAC_USXG_MODE;/*USXG_MODE : 0x000*/
		tc956x_xpcs_write(xpcsaddr, XGMAC_VR_XS_PCS_KR_CTRL, reg_value);

		reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_XS_PCS_DIG_CTRL1);
		reg_value |= XGMAC_VR_RST;/*set VR_RST*/
		tc956x_xpcs_write(xpcsaddr, XGMAC_VR_XS_PCS_DIG_CTRL1, reg_value);

		/*Wait for Reset to clear*/
		do {
			reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_XS_PCS_DIG_CTRL1);
		} while ((XGMAC_VR_RST & reg_value) == XGMAC_VR_RST);

	}

	reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_MII_AN_CTRL);
	reg_value &= XGMAC_TX_CFIG_INTR_EN_MASK;/*TX_CONFIG MAC SIDE*/
	reg_value |= XGMAC_MII_AN_INTR_EN;/*MII_AN_INTR_EN enabe*/
	tc956x_xpcs_write(xpcsaddr, XGMAC_VR_MII_AN_CTRL, reg_value);

	reg_value = tc956x_xpcs_read(xpcsaddr, XGMAC_VR_MII_DIG_CTRL1);
	reg_value &= ~XGMAC_MAC_AUTO_SW_EN;/*MAC_AUTO_SW enable*/
#ifndef TC956X_SGMII_2P5_GBPS_TEST
	reg_value |= XGMAC_MAC_AUTO_SW_EN;
#endif
	tc956x_xpcs_write(xpcsaddr, XGMAC_VR_MII_DIG_CTRL1, reg_value);

	return 0;
}

void tc956x_xpcs_ctrl_ane(struct tc956xmac_priv *priv, bool ane)
{
	u32 reg_value;

	if (ane) {
		reg_value = tc956x_xpcs_read(priv->xpcsaddr, XGMAC_SR_MII_CTRL);
#ifndef TC956X_SGMII_2P5_GBPS_TEST
		reg_value |= XGMAC_AN_37_ENABLE;
#else
		reg_value &= (~XGMAC_AN_37_ENABLE);
#endif
		tc956x_xpcs_write(priv->xpcsaddr, XGMAC_SR_MII_CTRL, reg_value);
	}

}
#endif
