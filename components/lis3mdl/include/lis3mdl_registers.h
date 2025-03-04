#ifndef LIS3MDL_REGISTERS_H
#define LIS3MDL_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* LIS3MDL ADDRESS */

#define LIS3MDL_I2C_ADDR		0x1C

/* LIS3MDL REGISTERS */

#define WHO_AM_I_REG			0x0F            /*R The default value inside this register should be 0x3D for LIS3MDL*/

#define	CTRL_REG1				0x20            /*R/W*/
#define	CTRL_REG2				0x21            /*R/W*/
#define	CTRL_REG3				0x22            /*R/W*/
#define	CTRL_REG4				0x23            /*R/W*/
#define	CTRL_REG5				0x24            /*R/W*/

#define STATUS_REG				0x27            /*R*/

#define OFFSET_X_REG_L_M        0x05            /*R/W - Represent X(low byte)hard-iron offset in order to compensate environmental effects */
#define OFFSET_X_REG_H_M        0x06            /*R/W - Represent X(high byte)hard-iron offset in order to compensate environmental effects */
#define OFFSET_Y_REG_L_M        0x07            /*R/W - Represent Y(low byte)hard-iron offset in order to compensate environmental effects */
#define OFFSET_Y_REG_H_M        0x08            /*R/W - Represent Y(high byte)hard-iron offset in order to compensate environmental effects */
#define OFFSET_Z_REG_L_M        0x09            /*R/W - Represent Z(low byte)hard-iron offset in order to compensate environmental effects */
#define OFFSET_Z_REG_H_M        0x0A            /*R/W - Represent Z(high byte)hard-iron offset in order to compensate environmental effects */

#define OUT_X_L_REG				0x28            /*R - X-axis (low byte) data output */
#define OUT_X_H_REG				0x29            /*R - X-axis (high byte) data output */
#define OUT_Y_L_REG				0x2A            /*R - Y-axis (low byte) data output */
#define OUT_Y_H_REG				0x2B            /*R - Y-axis (high byte) data output */
#define OUT_Z_L_REG				0x2C            /*R - Z-axis (low byte) data output */
#define OUT_Z_H_REG				0x2D            /*R - Z-axis (high byte) data output */

#define TEMP_OUT_L_REG			0x2E            /*R - Sensor temperature (low byte) */
#define TEMP_OUT_H_REG			0x2F            /*R - Sensor temperature (high byte) */

#define INT_CFG_REG				0x30            /*R/W*/
#define INT_SRC_REG				0x31            /*R*/
#define INT_THS_L_REG			0x32            /*R/W*/
#define INT_THS_H_REG			0x33            /*R/W*/

/* Masks for control registers to enable sensor features and configurations */

// CONTROL REGISTER 1
#define LIS3MDL_TEMP_EN			0x80            /* Temperature sensor enable -  Binary: 1xxx xxxx - (default = disabled) */
#define LIS3MDL_FAST_ODR_EN		0x02            /* Fast ODR mode enable -       Binary: xxxx xx1x - (default = disabled) */
#define LIS3MDL_SELF_TEST_EN    0x01            /* Self-test enable -           Binary: xxxx xxx1 - (default = disabled) */

typedef enum {
    LIS3MDL_OM_XY_LP_MODE = 0x00,               /* Low performance mode -       Binary: x00x xxxx */
    LIS3MDL_OM_XY_MP_MODE = 0x20,               /* Medium performance mode -    Binary: x01x xxxx */
    LIS3MDL_OM_XY_HP_MODE = 0x40,               /* High performance mode -      Binary: x10x xxxx */
    LIS3MDL_OM_XY_UP_MODE = 0x60                /* Ultra performance mode -     Binary: x11x xxxx */
} lis3mdl_om_xy_mode_t;

typedef enum {
    LIS3MDL_ODATA_RATE_0_625_HZ = 0x00,          /* Data rate 0.625 Hz -         Binary: xxx0 00xx */
    LIS3MDL_ODATA_RATE_1_25_HZ = 0x04,           /* Data rate 1.25 Hz -          Binary: xxx0 01xx */
    LIS3MDL_ODATA_RATE_2_5_HZ = 0x08,            /* Data rate 2.5 Hz -           Binary: xxx0 10xx */
    LIS3MDL_ODATA_RATE_5_HZ = 0x0C,              /* Data rate 5 Hz -             Binary: xxx0 11xx */
    LIS3MDL_ODATA_RATE_10_HZ = 0x10,             /* Data rate 10 Hz -            Binary: xxx1 00xx */
    LIS3MDL_ODATA_RATE_20_HZ = 0x14,             /* Data rate 20 Hz -            Binary: xxx1 01xx */
    LIS3MDL_ODATA_RATE_40_HZ = 0x18,             /* Data rate 40 Hz -            Binary: xxx1 10xx */
    LIS3MDL_ODATA_RATE_80_HZ = 0x1C,             /* Data rate 80 Hz -            Binary: xxx1 11xx */
} lis3mdl_output_data_rate_t;

// CONTROL REGISTER 2
#define LIS3MDL_REBOOT          0x08            /* Reboot memory content -  Binary: xxxx 1xxx */
#define LIS3MDL_SOFT_RST		0x04            /* Software reset -         Binary: xxxx x1xx */

typedef enum {
    LIS3MDL_FS_4_GAUSS = 0x00,                  /* Full-scale range +/- 4 Gauss -  Binary: x00x xxxx */
    LIS3MDL_FS_8_GAUSS = 0x20,                  /* Full-scale range +/- 8 Gauss -  Binary: x01x xxxx */
    LIS3MDL_FS_12_GAUSS = 0x40,                 /* Full-scale range +/- 12 Gauss - Binary: x10x xxxx */
    LIS3MDL_FS_16_GAUSS = 0x60                  /* Full-scale range +/- 16 Gauss - Binary: x11x xxxx */
} lis3mdl_fs_config_t;

// CONTROL REGISTER 3
#define LIS3MDL_LP_CONFIG_EN    0x20            /* Low-power mode configuration - Binary: 0010 0xxx */

typedef enum {
    LIS3MDL_MD_CONTINUOUS_MODE = 0x00,          /* Continuous-conversion mode - Binary: 00x0 0x00 */
    LIS3MDL_MD_SINGLE_MODE = 0x01,              /* Single-conversion mode -     Binary: 00x0 0x01 */
    LIS3MDL_MD_POWER_DOWN_MODE = 0x03           /* Power-down mode -            Binary: 00x0 0x11 */
} lis3mdl_md_mode_t;

// CONTROL REGISTER 4
#define LIS3MDL_BIG_ENDIAN     0x02            /* Data endianness selection - Binary: 0000 xx10 */
#define LIS3MDL_LITTLE_ENDIAN  0x00            /* Data endianness selection - Binary: 0000 xx00 */

typedef enum {
    LIS3MDL_OM_Z_LP_MODE = 0x00,                /* Low performance mode -       Binary: 0000 00x0 */
    LIS3MDL_OM_Z_MP_MODE = 0x04,                /* Medium performance mode -    Binary: 0000 01x0 */
    LIS3MDL_OM_Z_HP_MODE = 0x08,                /* High performance mode -      Binary: 0000 10x0 */
    LIS3MDL_OM_Z_UP_MODE = 0x0C                 /* Ultra performance mode -     Binary: 0000 11x0 */
} lis3mdl_om_z_mode_t;

// CONTROL REGISTER 5
#define LIS3MDL_BDU_EN          0x40            /* Block data update - Binary: x100 0000 */
#define LIS3MDL_FAST_READ_EN    0x80            /* Fast read mode -    Binary: 1x00 0000 */

#ifdef __cplusplus
}
#endif

#endif