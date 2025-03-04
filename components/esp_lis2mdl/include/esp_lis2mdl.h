/**
 * @file esp_lis2mdl.h
 * @brief ESP32 LIS2MDL Magnetometer Driver
 *
 * @note This driver depends on the ESP-IDF I2C driver.
 
 */

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef ESP_LIS2MDL_H
#define ESP_LIS2MDL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "esp_err.h"
#include "i2c_defines.h"
#include "driver/i2c_master.h"

/* Private macroS ------------------------------------------------------------*/

#define    BOOT_TIME                    20  // ms
#define    SELF_TEST_WAIT               60  // ms
#define    RESET_TIME                   5   // us 
#define    I2C_TIMEOUT                  100 // ms

#define    LIS2MDL_PROPERTY_ENABLE      0x01
#define    LIS2MDL_PROPERTY_DISABLE     0x00

/* Address definitions -------------------------------------------------------*/

/** I2C Device Address 8 bit format **/

#define LIS2MDL_I2C_ADD                 0x1EU   //  0011110b (7-bit address)

/** Device Identification (Who am I) **/

#define LIS2MDL_ID                      0x40U

/** LIS2MDL Register addresses definition **/

#define LIS2MDL_OFFSET_X_REG_L          0x45U
#define LIS2MDL_OFFSET_X_REG_H          0x46U
#define LIS2MDL_OFFSET_Y_REG_L          0x47U
#define LIS2MDL_OFFSET_Y_REG_H          0x48U
#define LIS2MDL_OFFSET_Z_REG_L          0x49U
#define LIS2MDL_OFFSET_Z_REG_H          0x4AU

#define LIS2MDL_WHO_AM_I                0x4FU

#define LIS2MDL_INT_THS_L_REG           0x65U
#define LIS2MDL_INT_THS_H_REG           0x66U

#define LIS2MDL_OUTX_L_REG              0x68U
#define LIS2MDL_OUTX_H_REG              0x69U
#define LIS2MDL_OUTY_L_REG              0x6AU
#define LIS2MDL_OUTY_H_REG              0x6BU
#define LIS2MDL_OUTZ_L_REG              0x6CU
#define LIS2MDL_OUTZ_H_REG              0x6DU
#define LIS2MDL_TEMP_OUT_L_REG          0x6EU
#define LIS2MDL_TEMP_OUT_H_REG          0x6FU

/** CFG_REG_A
 * 
 * |  Bit  |        7       |    6   |     5    |  4  |  3   |  2   |  1  |  0  |
 * |-------|--------------- |--------|----------|-----|------|------|-----|-----|
 * | Name  | Comp_temp_en   | REBOOT | SOFT_RST |  LP | ODR1 | ODR0 | MD1 | MD0 |
 * 
 * Bit Descriptions:
 * - Bit 7 (COMP_TEMP_EN):  Enables the magnetometer temperature compensation. Default value: 0
 * - Bit 6 (REBOOT):        Reboots magnetometer memory content. Default value: 0
 * - Bit 5 (SOFT_RST):      When this bit is set, the configuration registers and user registers are reset
 *                          Flash registers keep their values
 * - Bit 4 (LP):            Enables low-power mode. Default value: 0
 * - Bit 3 (ODR1):          Output data rate configuration, see datasheet. Default value: 00
 * - Bit 2 (ODR0):          Output data rate configuration, see datasheet. Default value: 00
 * - Bit 1 (MD1):           These bits select the mode of operation of the device, see datasheet. Default value: 11
 * - Bit 0 (MD0):           These bits select the mode of operation of the device, see datasheet. Default value: 11
 */

#define LIS2MDL_CFG_REG_A               0x60U

/** CFG_REG_B
 * 
 * |  Bit  |  7  |  6  |  5  |         4         |        3       |     2    |    1     |  0  |
 * |-------|-----|-----|-----|-------------------|----------------|----------|----------|-----|
 * | Name  |  0  |  0  |  0  | OFF_CANC_ONE_SHOT | INT_on_DataOFF | Set_FREQ | OFF_CANC | LPF |
 * 
 * Bit Descriptions:
 * - Bit 7 (0): Not used
 * - Bit 6 (0): Not used
 * - Bit 5 (0): Not used
 * - Bit 4 (OFF_CANC_ONE_SHOT): Enables offset cancellation in single measurement mode. The OFF_CANC bit must be set to 1
                                when enabling offset cancellation in single measurement mode. Default value: 0
 * - Bit 3 (INT_on_DataOFF):    If 1, the interrupt block recognition checks data after the hard-iron correction 
                                to discover the interrupt. Default value: 0
 * - Bit 2 (Set_FREQ):          Selects the frequency of the set pulse. Default value: 0
 * - Bit 1 (OFF_CANC):          Enables offset cancellation. Default value: 0
 * - Bit 0 (LPF):               Enables low-pass filter. Default value: 0
 */

#define LIS2MDL_CFG_REG_B               0x61U

/** CFG_REG_C
 * 
 * |  Bit  |  7  |      6     |    5    |  4  |  3  |   2   |    1      |      0      |
 * |-------|-----|------------|---------|-----|-----|-------|-----------|-------------|
 * | Name  | 0   | INT_on_PIN | I2C_DIS | BDU | BLE | 4WSPI | Self_test | DRDY_on_PIN |
 * 
 * Bit Descriptions:
 * - Bit 7 (0): Not used
 * - Bit 6 (INT_on_PIN):    If 1, the INTERRUPT signal (INT bit in INT_SOURCE_REG (64h)) is driven to 
 *                          the INT/DRDY pin. Default value: 0
 * - Bit 5 (I2C_DIS):       If 1, the I2C interface is inhibited. Only the SPI interface can be used
 * - Bit 4 (BDU):           Block data update. Default value: 0
 * - Bit 3 (BLE):           Big/little endian data selection. Default value: 0 (data LSB @ lower address)
 * - Bit 2 (4WSPI):         Set to 1 to enable SDO line on pin 7
 * - Bit 1 (Self_test):     Enable self-test. Default value: 0
 * - Bit 0 (DRDY_on_PIN):   If 1, the data-ready signal (Zyxda bit in STATUS_REG (67h)) is driven on 
 *                          the INT/DRDY pin. Default value: 0
 */

#define LIS2MDL_CFG_REG_C               0x62U

/** INT_CTRL_REG 
 * 
 * |  Bit  |  7   |  6   |  5   |  4  |  3  |  2  |  1  |  0  |
 * |-------|------|------|------|-----|-----|-----|-----|-----|
 * | Name  | XIEN | YIEN | ZIEN |  0  |  0  | IEA | IEL | IEN |
 * 
 * Bit Descriptions:
 * - Bit 7 (XIEN):  Enables the interrupt detection for the X-axis. Default value: 1
 * - Bit 6 (YIEN):  Enables the interrupt detection for the Y-axis. Default value: 1
 * - Bit 5 (ZIEN):  Enables the interrupt detection for the Z-axis. Default value: 1
 * - Bit 4 (0):     Not used, MUST be 0
 * - Bit 3 (0):     Not used, MUST be 0
 * - Bit 2 (IEA):   Controls the polarity of the INT bit (INT_SOURCE_REG (64h)) when an interrupt occurs. Default: 0
 *                  If IEA = 1, then INT = 1 signals an interrupt
 * - Bit 1 (IEL):   Controls whether the INT bit (INT_SOURCE_REG (64h)) is latched or pulsed. Default: 0
 *                  If IEL = 1, then INT is latched.
 * - Bit 0 (IEN):   Enables the interrupt. When set, enables the generation of the interrupt. The INT bit 
 *                  is in INT_SOURCE_REG (64h). Default: 0
 */

#define LIS2MDL_INT_CTRL_REG            0x63U

/** INT_SOURCE_REG 
 * 
 * |  Bit  |     7    |     6    |     5    |     4    |     3    |     2    |   1  |  0  |
 * |-------|----------|----------|----------|----------|----------|----------|------|-----|
 * | Name  | P_TH_S_X | P_TH_S_Y | P_TH_S_Z | N_TH_S_X | N_TH_S_Y | N_TH_S_Z | MROI | INT |
 * 
 * Bit Descriptions:
 * - Bit 7 (P_TH_S_X):  X-axis value exceeds the threshold positive side
 * - Bit 6 (P_TH_S_Y):  Y-axis value exceeds the threshold positive side
 * - Bit 5 (P_TH_S_Z):  Z-axis value exceeds the threshold positive side
 * - Bit 4 (N_TH_S_X):  X-axis value exceeds the threshold negative side
 * - Bit 3 (N_TH_S_Y):  Y-axis value exceeds the threshold negative side
 * - Bit 2 (N_TH_S_Z):  Z-axis value exceeds the threshold negative side
 * - Bit 1 (MROI):      MROI flag generation is always enabled.This flag is reset by reading INT_SOURCE_REG (64h).
 * - Bit 0 (INT):       This bit signals when the interrupt event occurs
 */

#define LIS2MDL_INT_SOURCE_REG          0x64U

/** STATUS_REG 
 * 
 * |  Bit  |   7   |  6  |  5  |  4  |   3   |  2  |  1  |  0  |
 * |-------|-------|-----|-----|-----|-------|-----|-----|-----|
 * | Name  | Zyxor | zor | yor | xor | Zyxda | zda | yda | xda |
 * 
 * Bit Descriptions:
 * - Bit 7 (Zyxor): X-, Y-, and Z-axis data overrun. Default value: 0
 * - Bit 6 (zor):   Z-axis data overrun. Default value: 0
 * - Bit 5 (yor):   Y-axis data overrun. Default value: 0
 * - Bit 4 (xor):   X-axis data overrun. Default value: 0
 * - Bit 3 (Zyxda): X-, Y-, and Z-axis new data available. Default value: 0
 * - Bit 2 (zda):   Z-axis new data available. Default value: 0
 * - Bit 1 (yda):   Y-axis new data available. Default value: 0
 * - Bit 0 (xda):   X-axis new data available. Default value: 0
 */

#define LIS2MDL_STATUS_REG              0x67U

/* Defines ------------------------------------------------------------------*/

#define LIS2MDL_TEMP_COMP_ENABLE        0x80  // CFG_REG_A bit 7 set
#define LIS2MDL_REBOOT_ENABLE           0x40  // CFG_REG_A bit 6 set
#define LIS2MDL_SOFT_RESET_ENABLE       0x20  // CFG_REG_A bit 5 set

#define LIS2MDL_SINGLE_OFF_CANC_ENABLE  0x10  // CFG_REG_B bit 4 set
#define LIS2MDL_INT_ON_DATAOFF_ENABLE   0x08  // CFG_REG_B bit 3 set
#define LIS2MDL_CONT_OFF_CANC_ENABLE    0x02  // CFG_REG_B bit 1 set
#define LIS2MDL_LPF_ENABLE              0x01  // CFG_REG_B bit 0 set

#define LIS2MDL_INT_TO_PIN_ENABLE       0x40  // CFG_REG_C bit 6 set
#define LIS2MDL_BDU_ENABLE              0x10  // CFG_REG_C bit 4 set
#define LIS2MDL_SELF_TEST_ENABLE        0x02  // CFG_REG_C bit 1 set
#define LIS2MDL_DRDY_TO_PIN_ENABLE      0x01  // CFG_REG_C bit 0 set


#define LIS2MDL_INT_ON_X_ENABLE         0x80  // INT_CTRL_REG bit 7 set
#define LIS2MDL_INT_ON_Y_ENABLE         0x40  // INT_CTRL_REG bit 6 set
#define LIS2MDL_INT_ON_Z_ENABLE         0x20  // INT_CTRL_REG bit 5 set
#define LIS2MDL_INT_ENABLE              0x01  // INT_CTRL_REG bit 0 set

/* Enums --------------------------------------------------------------------*/

typedef enum {
    LIS2MDL_HIGH_RESOLUTION     = 0x00U,    // xxx0 xxxx Binary
    LIS2MDL_LOW_POWER           = 0x10U,    // xxx1 xxxx Binary
} lis2mdl_power_mode_t;

typedef enum {
    LIS2MDL_ODR_10Hz            = 0x00U,    // xxxx 00xx Binary
    LIS2MDL_ODR_20Hz            = 0x04U,    // xxxx 01xx Binary
    LIS2MDL_ODR_50Hz            = 0x08U,    // xxxx 10xx Binary
    LIS2MDL_ODR_100Hz           = 0x0CU,    // xxxx 11xx Binary
} lis2mdl_odr_t;

typedef enum {
    LIS2MDL_CONTINUOUS_MODE     = 0x00U,    // xxxx xx00 Binary
    LIS2MDL_SINGLE_TRIGGER      = 0x01U,    // xxxx xx01 Binary
    LIS2MDL_POWER_DOWN          = 0x02U,    // xxxx xx10 Binary
    NOT_USED_POWER_DOWN         = 0x03U,    // xxxx xx11 Binary
} lis2mdl_op_mode_t;

typedef enum {
    LIS2MDL_SET_PUL_ODR         = 0x00U,    // xxxx xx0x Binary
    LIS2MDL_SET_PUL_APD         = 0x04U,    // xxxx x1xx Binary
} lis2mdl_set_rst_t;

typedef enum{
  LIS2MDL_I2C_ENABLE            = 0x00U,    // xx0x xxxx Binary
  LIS2MDL_I2C_DISABLE           = 0x20U,    // xx1x xxxx Binary
} lis2mdl_i2c_dis_t;

typedef enum {
    LIS2MDL_LITTLE_ENDIAN        = 0x00U,    // xxxx 0xxx Binary
    LIS2MDL_BIG_ENDIAN           = 0x08U,    // xxxx 1xxx Binary
} lis2mdl_endianness_t;

typedef enum {
  LIS2MDL_SPI_4_WIRE            = 0x04U,    // xxxx x1xx Binary
  LIS2MDL_SPI_3_WIRE            = 0x00U,    // xxxx x0xx Binary
} lis2mdl_sim_t;

typedef enum {
    LIS2MDL_POLARITY_POS        = 0x04U,    // xxxx x1xx Binary
    LIS2MDL_POLARITY_NEG        = 0x00U,    // xxxx x0xx Binary
} lis2md_int_polarity_t;

typedef enum {
    LIS2MDL_LATCH_SET           = 0x02U,   // xxxx xx1x Binary
    LIS2MDL_LATCH_UNSET         = 0x00U,   // xxxx xx0x Binary
} lis2md_int_latch_t;

/* Structs ------------------------------------------------------------------*/

// Struc to group the hard iron offset values
// For storing in sensor registers - not for use in calibration function
typedef struct {
    int16_t hi_x;
    int16_t hi_y;
    int16_t hi_z;
} lis2mdl_hard_iron_off_t;

typedef struct {
    int16_t x_th;
    int16_t y_th;
    int16_t z_th;
} lis2mdl_threshold_val_t;

// Struc to group the magnetometer output values, in different format
typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    float mgauss_x;
    float mgauss_y;
    float mgauss_z;
    float uTesla_x;
    float uTesla_y;
    float uTesla_z;
} lis2mdl_mag_values_t;

// Struc to group the temperature values, in different format
typedef struct {
    float celsius;
    int16_t raw;
} lis2mdl_temp_values_t;

typedef struct {
    uint8_t                         temp_compensation;
    lis2mdl_power_mode_t            power_mode;
    lis2mdl_odr_t                   data_rate;
    lis2mdl_op_mode_t               op_mode;
    uint8_t                         offset_cancellation_single;
    uint8_t                         interrupt_on_data_off;
    lis2mdl_set_rst_t               set_pulse_freq;
    uint8_t                         offset_cancellation_cont;
    uint8_t                         low_pass_filter;
    lis2mdl_endianness_t            endianness;
    uint8_t                         int_to_pin;
    uint8_t                         drdy_to_pin;
    uint8_t                         block_data_update;
    lis2mdl_i2c_dis_t               i2c_disable;
    lis2mdl_sim_t                   four_wire_spi_mode;
    uint8_t                         x_int_enable;
    uint8_t                         y_int_enable;
    uint8_t                         z_int_enable;
    lis2md_int_polarity_t           int_polarity;
    lis2md_int_latch_t              int_latch;
    uint8_t                         int_enable;
} lis2mdl_config_t;

typedef struct {
    bool                            p_th_s_x;
    bool                            p_th_s_y;
    bool                            p_th_s_z;
    bool                            n_th_s_x;
    bool                            n_th_s_y;
    bool                            n_th_s_z;
    bool                            signal_int;
} lis2mdl_interrupt_src_t;

typedef struct {
    bool                            drdy_xyz;
    bool                            dovr_xyz;
} lis2mdl_data_status_t;
 
typedef struct{
    lis2mdl_hard_iron_off_t         hi_offset;
    int16_t                         int_threshold;
    lis2mdl_temp_values_t           temp_value;
    lis2mdl_mag_values_t            mag_value;
    lis2mdl_config_t                config;
    lis2mdl_interrupt_src_t         interrupt_src;
    lis2mdl_data_status_t           data_status;
    bool                            boot_enabled;
    bool                            srst_enabled;
    bool                            self_test_enabled;
} lis2mdl_sensor_handle_t;

/* User accessible functions for LIS2MDL -------------------------------------*/

/** Initialization **/

esp_err_t esp_lis2mdl_init(const lis2mdl_config_t *config);
esp_err_t esp_lis2mdl_init_default();

/** CFG_REG_A **/

/**
 * @brief Sets the temperature compensation bit in the CFG_REG_A register
 * @param enable Enable or disable the temperature compensation bit
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_offset_temp_comp_set(bool enable);

/**
 * @brief Gets if the temperature compensation is active or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.temp_compensation
 */
esp_err_t esp_lis2mdl_offset_temp_comp_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Sets the sensor reboot bit in the CFG_REG_A register
 * @param enable Enable or disable the sensor reboot bit
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_reboot_set(bool enable);

/**
 * @brief Gets if the sensor reboot bit is active or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->boot_enabled
 */
esp_err_t esp_lis2mdl_reboot_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Sets the soft reset bit in the CFG_REG_A register
 * @param enable Enable or disable the soft reset bit
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_soft_reset_set(bool enable);

/**
 * @brief Gets if the soft reset bit is active or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->srst_enabled
 */
esp_err_t esp_lis2mdl_soft_reset_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Sets the power mode of the sensor
 * @param power_mode Power mode:
 *                   LIS2MDL_HIGH_RESOLUTION: High resolution mode - LIS2MDL_LOW_POWER: Low power mode
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL power mode
 */
esp_err_t esp_lis2mdl_power_mode_set(lis2mdl_power_mode_t power_mode);

/**
 * @brief Gets if low power mode is enabled or in high resolution mode
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.power_mode
 */
esp_err_t esp_lis2mdl_power_mode_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Sets the outpute data rate of the sensor
 * @param data_rate Output data rates:
 *                   LIS2MDL_ODR_10Hz - LIS2MDL_ODR_20Hz - LIS2MDL_ODR_50Hz - LIS2MDL_ODR_100Hz
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL data rate
 */
esp_err_t esp_lis2mdl_data_rate_set(lis2mdl_odr_t data_rate);

/**
 * @brief Returns the output data rate of the sensor
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_INVALID_STATE: Not valid LIS2MDL data rate
 * @note The value is stored in the sensor_handle->config.data_rate
 */
esp_err_t esp_lis2mdl_data_rate_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Sets the operation mode of the sensor
 * @param op_mode Operation modes:
 *                   LIS2MDL_CONTINUOUS_MODE - LIS2MDL_SINGLE_TRIGGER - LIS2MDL_POWER_DOWN - (use previous)NOT_USED_POWER_DOWN
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL operation mode
 */
esp_err_t esp_lis2mdl_op_mode_set(lis2mdl_op_mode_t op_mode);

/**
 * @brief Gets the operating mode of the sensor
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_INVALID_STATE: Not valid LIS2MDL data rate
 * @note The value is stored in the sensor_handle->config.op_mode
 */
esp_err_t esp_lis2mdl_op_mode_get(lis2mdl_sensor_handle_t *sensor_handle);

/** CFG_REG_B **/

/**
 * @brief Enable or disable offset cancellation in single operating mode. 
 * @param enable Enable or disable offset cancellation in single operating mode (Reverses the polarity between 2 set pulses)
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note Also enables continuous offset cancellation bit
 */
esp_err_t esp_lis2mdl_offset_cancellation_single_set(bool enable);

/**
 * @brief Gets if offset cancellation in single operating mode is enabled or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.offset_cancellation_single
 */
esp_err_t esp_lis2mdl_offset_cancellation_single_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Configure if the interrupt should take into account hard iron offsets
 * @param enable If enabled, the interrupt block recognition checks data after the hard-iron correction to discover the interrupt
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_interrupt_on_data_off_set(bool enable);

/**
 * @brief Gets if interrupt should take into account hard iron offsets
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.interrupt_on_data_off
 */
esp_err_t esp_lis2mdl_interrupt_on_data_off_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Sets the set pulse frequency for measurements
 * @param set_freq Operation modes:
 *                   LIS2MDL_SET_PUL_ODR: Set pulse every ODR - LIS2MDL_SET_PUL_APD: Set pulse only after power on
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL set frequency
 */
esp_err_t esp_lis2mdl_set_pulse_freq_set(lis2mdl_set_rst_t set_freq);

/**
 * @brief Gets the set pulse frequency for measurements
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_INVALID_STATE: Not valid LIS2MDL set frequency
 * @note The value is stored in the sensor_handle->config.set_pulse_freq
 */
esp_err_t esp_lis2mdl_set_pulse_freq_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Enable or disable offset cancellation in continuous operating mode
 * @param enable Enable or disable offset cancellation in continuous operating mode
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_offset_cancellation_cont_set(bool enable);

/**
 * @brief Gets if offset cancellation in continuous operating mode is enabled or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.offset_cancellation_cont
 */
esp_err_t esp_lis2mdl_offset_cancellation_cont_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Enable or disable the low pass filter of the sensor. While filter is enabled, RMS noise is reduced, ODR is ODR/4
 * @param enable Enable or disable the low pass filter
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The offset cancellation feature works as a two-level moving average filter, 
 *       thus giving the same bandwidth and noise performance as with the LPF filter enabled
 */
esp_err_t esp_lis2mdl_bw_lpf_set(bool enable);

/**
 * @brief Gets if the low pass filter is enabled or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.low_pass_filter
 */
esp_err_t esp_lis2mdl_bw_lpf_get(lis2mdl_sensor_handle_t *sensor_handle);

/** CFG_REG_C **/

/**
 * @brief Drive the interrupt signal (INT bit in INT_SOURCE_REG (64h)) to the INT/DRDY pin
 * @param enable Enable the interrupt signal to the INT/DRDY pin
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_int_to_pin_set(bool enable);

/**
 * @brief Gets if interrupt is driven to the INT/DRDY pin
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.int_to_pin
 */
esp_err_t esp_lis2mdl_int_to_pin_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Enable/Disable I2C interface
 * @param i2c_enable LIS2MDL_I2C_ENABLE: i2c activated - LIS2MDL_I2C_DISABLE: i2c inactive
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL parameter
 */
esp_err_t esp_lis2mdl_i2c_disable_set(lis2mdl_i2c_dis_t i2c_enable);

/**
 * @brief Gets I2C is enabled or disabled
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.i2c_disable
 */
esp_err_t esp_lis2mdl_i2c_disable_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Enable or disable the block data update feature. If enabled, the output registers are not updated until MSB and LSB are read
 * @param enable Enable means the BDU is active
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_block_data_update_set(bool enable); 

/**
 * @brief Gets block data update feature is enabled or disabled
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.block_data_update
 */
esp_err_t esp_lis2mdl_block_data_update_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Set the endianness of the sensor output data registers
 * @param endianness LITTLE_ENDIAN: LSB First - BIG_ENDIAN: MSB First
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL parameter
 */
esp_err_t esp_lis2mdl_endianness_set(lis2mdl_endianness_t endianness);

/**
 * @brief Gets if output data registers are in little or big endian
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.endianness
 */
esp_err_t esp_lis2mdl_endianness_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Set SPI mode as 4 or 3 wire
 * @param spi_mode LIS2MDL_SPI_4_WIRE - LIS2MDL_SPI_3_WIRE
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL parameter
 */
esp_err_t esp_lis2mdl_spi_mode_set(lis2mdl_sim_t spi_mode);

/**
 * @brief Gets if SPI is in 4 or 3 wire mode
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.four_wire_spi_mode
 */
esp_err_t esp_lis2mdl_spi_mode_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Enable or disable the self test feature.
 * @param enable Enable means activation of self test
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_self_test_set(bool enable);

/**
 * @brief Gets if self test bit is set or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->self_test_enabled
 */
esp_err_t esp_lis2mdl_self_test_get(lis2mdl_sensor_handle_t *sensor_handle);
/**  TO DO Implement self test function bool esp_lis2mdl_execute_self_test */

/**
 * @brief Drive the data-ready signal (Zyxda bit in STATUS_REG (67h)) to the INT/DRDY pin
 * @param enable Enable the data-ready signal to the INT/DRDY pin
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_drdy_to_pin_set(bool enable); 

/**
 * @brief Gets if data-ready signal is driven to the INT/DRDY pin
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the sensor_handle->config.drdy_to_pin
 */
esp_err_t esp_lis2mdl_drdy_to_pin_get(lis2mdl_sensor_handle_t *sensor_handle);

/** INT_CTRL_REG **/

/**
 * @brief Write <size> bytes to the specified register of the LIS2MDL sensor
 * @param int_config Pointer to 8-bit configuration of INT_CTRL_REG
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * 
 * @details The bit of INT_CTRL_REG are set as follows:
 * bit.7 = XIEN enable interrupt generation on X-axis
 * bit.6 = YIEN enable interrupt generation on Y-axis
 * bit.5 = ZIEN enable interrupt generation on Z-axis
 * bit.4 = 0 not used - MUST be 0
 * bit.3 = 0 not used - MUST be 0
 * bit.2 = IEA polarity of INT bit (INT_SOURCE_REG (64h)) when an interrupt occurs
 * bit.1 = IEL latch or pulse the INT bit (INT_SOURCE_REG (64h))
 * bit.0 = IEN enable interrupt generation
 */
esp_err_t esp_lis2mdl_interrupt_configure_set(uint8_t *int_config);

/**
 * @brief Reads the interrupt control register and sets the sensor config variables to the values read
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The values are stored in the corresponding fields of sensor_handle->config
 */
esp_err_t esp_lis2mdl_interrupt_configure_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Controls the polarity of the INT bit (INT_SOURCE_REG (64h)) when an interrupt occur
 * @param iea_polarity LIS2MDL_POLARITY_NEG: 0 signals an interrupt - LIS2MDL_POLARITY_POS: 1 signals an interrupt
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL parameter
 */
esp_err_t esp_lis2mdl_interrupt_polarity_set(lis2md_int_polarity_t iea_polarity);

/**
 * @brief Gets the interrupt polarity configuration
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the corresponding fields of sensor_handle->config.int_polarity
 */
esp_err_t esp_lis2mdl_interrupt_polarity_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Controls whether the INT bit (INT_SOURCE_REG (64h)) is latched or pulsed
 * @param iel_latch LIS2MDL_LATCH_UNSET: Interrupt not latched - LIS2MDL_LATCH_SET: Interrupt latched
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 *         - ESP_ERR_NOT_ALLOWED: Not valid LIS2MDL parameter
 * @note Once latched, the INT bit remains high until the INT_SOURCE_REG (64h) is read
 */
esp_err_t esp_lis2mdl_interrupt_latch_set(lis2md_int_latch_t iel_latch);

/**
 * @brief Gets the interrupt latch configuration
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the corresponding fields of sensor_handle->config.int_latch
 */
esp_err_t esp_lis2mdl_interrupt_latch_get(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Enables the interrupt. When set, enables the generation of the interrupt. The INT bit is in INT_SOURCE_REG (64h).
 * @param enable Enable interrupt
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid.
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 */
esp_err_t esp_lis2mdl_interrupt_enable_set(bool enable);

/**
 * @brief Gets if interrupt bit is enabled or not
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in the corresponding fields of sensor_handle->config.int_enable
 */
esp_err_t esp_lis2mdl_interrupt_enable_get(lis2mdl_sensor_handle_t *sensor_handle);

/** INT_SOURCE_REG **/

/**
 * @brief Reads the interrupt source register and sets the sensor config variables to the values read
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The values are stored in the corresponding fields of sensor_handle->interrupt_src
 */
esp_err_t esp_lis2mdl_get_interrupt_source(lis2mdl_sensor_handle_t *sensor_handle);

/** STATUS_REG **/

/**
 * @brief Gets if a new set of data has overwritten the previous set, for X/Y/Z axis
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in sensor_handle->data_status.dovr_xyz
 */
esp_err_t esp_lis2mdl_get_data_overrun(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Gets if a new set of data is available, for X/Y/Z axis
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * @note The value is stored in sensor_handle->data_status.drdy_xyz
 */
esp_err_t esp_lis2mdl_get_data_ready(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Gets if a new set of data is available, for X/Y/Z axis
 * @return 8 if new data is ready, 0 if not, -1 on error
 */
uint8_t esp_lis2mdl_get_data_ready_status();

/** Hard iron offset */

esp_err_t esp_lis2mdl_hard_iron_offset_set(lis2mdl_sensor_handle_t *sensor_handle);
esp_err_t esp_lis2mdl_hard_iron_offset_get(lis2mdl_sensor_handle_t *sensor_handle);

/** Interrupt threshold, common for X,Y,Z */

esp_err_t esp_lis2mdl_int_threshold_set(lis2mdl_sensor_handle_t *sensor_handle);
esp_err_t esp_lis2mdl_int_threshold_get(lis2mdl_sensor_handle_t *sensor_handle);

/** Data readings */

esp_err_t esp_lis2mdl_get_magnetic_raw(lis2mdl_sensor_handle_t *sensor_handle);
esp_err_t esp_lis2mdl_get_magnetic_mgauss(lis2mdl_sensor_handle_t *sensor_handle);
esp_err_t esp_lis2mdl_get_magnetic_utesla(lis2mdl_sensor_handle_t *sensor_handle);

esp_err_t esp_lis2mdl_get_temperature_raw(lis2mdl_sensor_handle_t *sensor_handle);
esp_err_t esp_lis2mdl_get_temperature_celsius(lis2mdl_sensor_handle_t *sensor_handle);

/** Misc functions */

/**
 * @brief Applies soft and hard iron calibration to magnetic data and returns uTesla values
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * 
 * @note The hard iron and soft iron offsets are defined in esp_lis2mdl.c file. Must be calculated by external software
 */
esp_err_t esp_lis2mdl_get_calibrated_uTesla(lis2mdl_sensor_handle_t *sensor_handle);

/**
 * @brief Averages "samples" values and applies soft and hard iron calibration to magnetic data and returns uTesla values
 * @param sensor_handle Pointer to the lis2mdl sensor handle
 * @param samples Number of samples to average
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * 
 * @note The hard iron and soft iron offsets are defined in esp_lis2mdl.c file. Must be calculated by external software
 */
esp_err_t esp_lis2mdl_get_avg_calibrated_uTesla(lis2mdl_sensor_handle_t *sensor_handle, uint8_t samples);

/** Helper functions */

/** Internal functions that implement the I2C sequence for Read/Write operations of LIS2MDL **/

/**
 * @brief Write <size> bytes to the specified register of the LIS2MDL sensor
 * @param reg_address Register address to read from
 * @param data_wr Pointer to the data buffer to write
 * @param size Number of bytes to write
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * 
 * @details The write sequence for LIS3MDL sensor is as follows:
 * 1. Start
 * 2. Write the sensor device address with the write bit
 * 3. Write the register address to write to (7 LSB indicate the address,  1 MSB indicate auto increment)
 * 4. Write the data to the register
 * 5. Stop
 */
static esp_err_t esp_lis2mdl_write_reg(uint8_t reg_address, uint8_t* data_wr, uint16_t size);

/** 
 * @brief Read <count> bytes from the specified register of the LIS3MDL sensor
 * @param reg_address Register address to read from
 * @param data_rd Pointer to the data buffer to store the read data
 * @param size Number of bytes to read
 * @return ESP_OK if successful - ESP_ERR_INVALID_ARG: I2C master transmit parameter invalid. 
 *         - ESP_ERR_TIMEOUT: Operation timeout(larger than xfer_timeout_ms) because the bus is busy or hardware crash.
 * 
 * @details The read sequence for LIS2MDL sensor is as follows:
 * 1. Start
 * 2. Write the sensor device address with the write bit
 * 3. Write the register address to read from (7 LSB indicate the address,  1 MSB indicate auto increment)
 * 4. Start (repeated start)
 * 5. Write the sensor device address with the read bit
 * 6. Read the data from the register
 * 7. Send NACK to the sensor (After reading the last byte)
*/
static esp_err_t esp_lis2mdl_read_reg(uint8_t reg_address, uint8_t* data_rd, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif // ESP_LIS2MDL_H