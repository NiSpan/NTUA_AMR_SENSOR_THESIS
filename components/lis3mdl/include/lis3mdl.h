#ifndef LIS3MDL_H
#define LIS3MDL_H

#include "esp_err.h"
#include "lis3mdl_registers.h"
#include "i2c_defines.h"
#include "driver/i2c.h"

// LIS3MDL configuration structure
typedef struct {
    bool temp_sensor_enable;
    lis3mdl_om_xy_mode_t om_xy_mode;
    lis3mdl_output_data_rate_t odata_rate;
    bool fast_odr_en;
    bool self_test_en;
    lis3mdl_fs_config_t fs_config;
    bool low_power_mode;
    lis3mdl_md_mode_t md_mode;
    lis3mdl_om_z_mode_t om_z_mode;
    bool big_endian_enabled;
} lis3mdl_config_t;

// Function declarations
esp_err_t lis3mdl_init(const lis3mdl_config_t *config);
esp_err_t lis3mdl_init_default();

void lis3mdl_read_xyz_offsets();
esp_err_t lis3mdl_set_xyz_offsets(int16_t offset_x, int16_t offset_y, int16_t offset_z);

esp_err_t lis3mdl_set_temp_sensor(bool enable);
bool lis3mdl_get_temp_sensor();
float lis3mdl_read_temperature();

esp_err_t lis3mdl_set_om_xy_mode(uint8_t mode);
uint8_t lis3mdl_get_om_xy_mode();

esp_err_t lis3mdl_set_data_rate(float rate);
float lis3mdl_get_data_rate();

esp_err_t lis3mdl_enable_fast_data_rate(bool enable);
esp_err_t lis3mdl_enable_self_test(bool enable);
bool lis3mdl_perform_self_test();

esp_err_t lis3mdl_set_full_scale(int scale);
int lis3mdl_get_full_scale();

esp_err_t lis3mdl_reboot();
esp_err_t lis3mdl_soft_reset();

esp_err_t lis3mdl_enable_low_power(bool enable);

esp_err_t lis3mdl_set_operating_mode(uint8_t mode);
uint8_t lis3mdl_get_operating_mode();

esp_err_t lis3mdl_set_om_z_mode(uint8_t mode);
uint8_t lis3mdl_get_om_z_mode();

esp_err_t lis3mdl_set_endianess(uint8_t endianess);

esp_err_t lis3mdl_enable_fast_read(bool enable);
esp_err_t lis3mdl_enable_block_data_update(bool enable);

esp_err_t lis3mdl_configure_interrupt(bool int_on_X, bool int_on_Y, bool int_on_Z, bool int_polarity, bool int_latch, bool int_enable);

esp_err_t lis3mdl_set_interrupt_threshold(uint16_t threshold);
esp_err_t lis3mdl_get_interrupt_threshold(uint16_t *threshold);

bool lis3mdl_get_status_reg(uint8_t bit_position);
bool lis3mdl_check_interrupt();

esp_err_t lis3mdl_read_raw_magnetic_data(int16_t *x, int16_t *y, int16_t *z);
esp_err_t lis3mdl_read_gauss_magnetic_data(float *x, float *y, float *z);
esp_err_t lis3mdl_read_utesla_magnetic_data(float *x, float *y, float *z);

// Internal functions
esp_err_t read_register(uint8_t reg, uint8_t *pdata, uint8_t count);
esp_err_t write_register(uint8_t reg, uint8_t *pdata, uint8_t count);
esp_err_t i2c_write_to_address(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size);
esp_err_t i2c_read_from_address(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size);

#endif // LIS3MDL_H