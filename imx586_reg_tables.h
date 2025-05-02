// SPDX-License-Identifier: GPL-2.0-only
/*
 * Register tables for IMX586 – *incomplete demo set*.
 * Replace with full tables from the Sony datasheet or Rockchip driver.
 */
#ifndef _IMX586_REG_TABLES_H
#define _IMX586_REG_TABLES_H

#include <linux/regmap.h>

#define REG_NULL 0xFFFF

static const struct reg_sequence imx586_mode_12mp_30fps[] = {
    {0x0136, 0x12},
    {0x0137, 0x00},
    {0x0138, 0x01},
    {0x0139, 0x00},
    {0x0340, 0x0C}, {0x0341, 0x80},   /* VTS */
    {0x0342, 0x11}, {0x0343, 0x30},   /* HTS */
    {0x0100, 0x00},                   /* standby */
    {REG_NULL, 0x00},
};

/* add more mode tables here … */

#endif /* _IMX586_REG_TABLES_H */
