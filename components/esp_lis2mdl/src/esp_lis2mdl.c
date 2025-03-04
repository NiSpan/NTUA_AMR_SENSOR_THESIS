#include "esp_lis2mdl.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <esp_log.h>

static const char *TAG = "AMR_IOT_SENSOR_LIS2MDL";

static i2c_master_dev_handle_t lis2mdl_sensor_handle;

/* Calibration data -------------------------------------------------------------*/

#define SENSOR_2

#if defined(SENSOR_1)
// in corners
// Values for sensor 1 in uTesla
// const float HARD_IRON_OFFSET[3] = {21.52f, -23.38f, 3.63f};
// const float SOFT_IRON_MATRIX[3][3] = {
//    {0.94f, 0.00f, 0.00f},
//    {0.00f, 1.01f, 0.00f},
//    {0.00f, 0.00f, 1.04f}
//};

// in middle
const float HARD_IRON_OFFSET[3] = {9.00f, -24.19f, 4.89f};
const float SOFT_IRON_MATRIX[3][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}};

#elif defined(SENSOR_2)
// in corners
// Values for sensor 2 in uTesla
// const float HARD_IRON_OFFSET[3] = {4.61f, 0.53f, 2.15f};
// const float SOFT_IRON_MATRIX[3][3] = {
//    {0.85f, 0.00f, 0.00f},
//    {0.00f, 1.07f, 0.00f},
//    {0.00f, 0.00f, 1.09f}
//};

// in middle
const float HARD_IRON_OFFSET[3] = {-8.71f, -0.25f, 2.78f};
const float SOFT_IRON_MATRIX[3][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}};

#elif defined(SENSOR_3)
// in corners
// Values for sensor 3 in uTesla
// const float HARD_IRON_OFFSET[3] = {4.82f, 1.78f, 1.56f};
// const float SOFT_IRON_MATRIX[3][3] = {
//    {0.86f, 0.00f, 0.00f},
//    {0.00f, 1.07f, 0.00f},
//    {0.00f, 0.00f, 1.09f}
//};

// in middle
const float HARD_IRON_OFFSET[3] = {-7.82f, 0.84f, 1.22f};
const float SOFT_IRON_MATRIX[3][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}};

#else
// Default values in uTesla
const float HARD_IRON_OFFSET[3] = {0.0f, 0.0f, 0.0f};
const float SOFT_IRON_MATRIX[3][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}};

#endif

/* Internal --------------------------------------------------------------------*/

static esp_err_t esp_lis2mdl_write_reg(uint8_t reg_address, uint8_t *data_wr, uint16_t size)
{
    esp_err_t ret;

    // Set the autoincrement bit to 1 if size to write > 1
    reg_address |= (size > 1) ? 0x80 : 0x00;

    uint8_t write_buffer[size + 1];
    write_buffer[0] = reg_address;
    memcpy(&write_buffer[1], data_wr, size);

    ret = i2c_master_transmit(lis2mdl_sensor_handle, write_buffer, size + 1, I2C_TIMEOUT);

    return ret;
}

static esp_err_t esp_lis2mdl_read_reg(uint8_t reg_address, uint8_t *data_rd, uint16_t size)
{
    esp_err_t ret;
    uint8_t SUB[1];

    reg_address |= (size > 1) ? 0x80 : 0x00;
    // Set the autoincrement bit to 1 if size to read > 1
    SUB[0] = reg_address;

    ret = i2c_master_transmit_receive(lis2mdl_sensor_handle, SUB, 1, data_rd, size, I2C_TIMEOUT);

    return ret;
}

static esp_err_t esp_lis2mdl_i2c_master_init()
{
    esp_err_t ret;
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT_NUM,
        .scl_io_num = I2C_SCL_IO,
        .sda_io_num = I2C_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LIS2MDL_I2C_ADD,
        .scl_speed_hz = I2C_FREQ_HZ,
        .flags.disable_ack_check = false,
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &lis2mdl_sensor_handle);

    return ret;
}

/* Init -----------------------------------------------------------------------*/

esp_err_t esp_lis2mdl_init(const lis2mdl_config_t *config)
{
    esp_err_t ret;
    uint8_t reg_value = 0;

    // Initialize I2C master device
    ret = esp_lis2mdl_i2c_master_init(&lis2mdl_sensor_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize I2C master device");
        return ret;
    }
    // Wait for the sensor to boot up
    vTaskDelay(pdMS_TO_TICKS(BOOT_TIME));

    // Check if the correct device is connected
    ret = esp_lis2mdl_read_reg(LIS2MDL_WHO_AM_I, &reg_value, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I register");
        return ret;
    }

    // Return - wronmg chip connected
    if (reg_value != LIS2MDL_ID)
    {
        ESP_LOGE(TAG, "Invalid WHO_AM_I_REG value: 0x%X", reg_value);
        return ESP_FAIL;
    }

    // Perform a soft reset
    ret = esp_lis2mdl_soft_reset_set(LIS2MDL_PROPERTY_ENABLE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to perform soft reset");
        return ESP_FAIL;
    }
    // Wait for the sensor to reset
    vTaskDelay(pdMS_TO_TICKS(RESET_TIME));

    // Configure CFG_REG_A
    reg_value = 0;
    reg_value = (config->op_mode) |
                (config->data_rate) |
                (config->power_mode) |
                (config->temp_compensation);
    ret = esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write to CFG_REG_A");
        return ESP_FAIL;
    }

    // Configure CFG_REG_B
    reg_value = 0;
    reg_value = (config->offset_cancellation_single) |
                (config->interrupt_on_data_off) |
                (config->set_pulse_freq) |
                (config->offset_cancellation_cont) |
                (config->low_pass_filter);
    ret = esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write to CFG_REG_B");
        return ESP_FAIL;
    }

    // Configure CFG_REG_C
    reg_value = 0;
    reg_value = (config->endianness) |
                (config->int_to_pin) |
                (config->drdy_to_pin) |
                (config->block_data_update) |
                (config->i2c_disable) |
                (config->four_wire_spi_mode);
    ret = esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write to CFG_REG_C");
        return ESP_FAIL;
    }

    // Configure INT_CTRL_REG
    reg_value = 0;
    reg_value = (config->x_int_enable) |
                (config->y_int_enable) |
                (config->z_int_enable) |
                (config->int_polarity) |
                (config->int_latch) |
                (config->int_enable);
    ret = esp_lis2mdl_interrupt_configure_set(&reg_value);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write to INT_CTRL_REG");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t esp_lis2mdl_init_default()
{
    lis2mdl_config_t lis2mdl_conf_default = {
        .temp_compensation = LIS2MDL_TEMP_COMP_ENABLE,
        .power_mode = LIS2MDL_HIGH_RESOLUTION,
        .data_rate = LIS2MDL_ODR_100Hz,
        .op_mode = LIS2MDL_CONTINUOUS_MODE,
        .offset_cancellation_single = LIS2MDL_PROPERTY_DISABLE,
        .interrupt_on_data_off = LIS2MDL_PROPERTY_DISABLE,
        .set_pulse_freq = LIS2MDL_PROPERTY_DISABLE,
        .offset_cancellation_cont = LIS2MDL_CONT_OFF_CANC_ENABLE,
        .low_pass_filter = LIS2MDL_LPF_ENABLE,
        .endianness = LIS2MDL_LITTLE_ENDIAN,
        .int_to_pin = LIS2MDL_PROPERTY_DISABLE,
        .drdy_to_pin = LIS2MDL_DRDY_TO_PIN_ENABLE,
        .block_data_update = LIS2MDL_BDU_ENABLE,
        .i2c_disable = LIS2MDL_I2C_ENABLE,
        .four_wire_spi_mode = LIS2MDL_SPI_3_WIRE,
        .x_int_enable = LIS2MDL_INT_ON_X_ENABLE,
        .y_int_enable = LIS2MDL_INT_ON_Y_ENABLE,
        .z_int_enable = LIS2MDL_INT_ON_Z_ENABLE,
        .int_polarity = LIS2MDL_POLARITY_POS,
        .int_latch = LIS2MDL_LATCH_SET,
        .int_enable = LIS2MDL_INT_ENABLE,
    };

    return esp_lis2mdl_init(&lis2mdl_conf_default);
}

/* CFG_REG_A ------------------------------------------------------------------*/

esp_err_t esp_lis2mdl_offset_temp_comp_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_TEMP_COMP_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_TEMP_COMP_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
}

esp_err_t esp_lis2mdl_offset_temp_comp_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.temp_compensation = reg_value & LIS2MDL_TEMP_COMP_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_reboot_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_REBOOT_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_REBOOT_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
}

esp_err_t esp_lis2mdl_reboot_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->boot_enabled = reg_value & LIS2MDL_REBOOT_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_soft_reset_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_SOFT_RESET_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_SOFT_RESET_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
}

esp_err_t esp_lis2mdl_soft_reset_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->boot_enabled = reg_value & LIS2MDL_SOFT_RESET_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_power_mode_set(lis2mdl_power_mode_t power_mode)
{
    uint8_t reg_value;
    esp_err_t ret;

    if (power_mode != LIS2MDL_LOW_POWER && power_mode != LIS2MDL_HIGH_RESOLUTION)
    {
        return ESP_ERR_NOT_ALLOWED;
    }

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    reg_value |= (uint8_t)power_mode;
    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
}

esp_err_t esp_lis2mdl_power_mode_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.power_mode = reg_value & LIS2MDL_LOW_POWER;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_data_rate_set(lis2mdl_odr_t data_rate)
{
    uint8_t reg_value;
    esp_err_t ret;

    switch (data_rate)
    {
    case LIS2MDL_ODR_10Hz:
        break;
    case LIS2MDL_ODR_20Hz:
        break;
    case LIS2MDL_ODR_50Hz:
        break;
    case LIS2MDL_ODR_100Hz:
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    reg_value |= (uint8_t)data_rate;
    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
}

esp_err_t esp_lis2mdl_data_rate_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }
    reg_value &= 0x0C;

    switch (reg_value)
    {
    case LIS2MDL_ODR_10Hz:
        sensor_handle->config.data_rate = LIS2MDL_ODR_10Hz;
        break;
    case LIS2MDL_ODR_20Hz:
        sensor_handle->config.data_rate = LIS2MDL_ODR_20Hz;
        break;
    case LIS2MDL_ODR_50Hz:
        sensor_handle->config.data_rate = LIS2MDL_ODR_50Hz;
        break;
    case LIS2MDL_ODR_100Hz:
        sensor_handle->config.data_rate = LIS2MDL_ODR_100Hz;
        break;
    default:
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

esp_err_t esp_lis2mdl_op_mode_set(lis2mdl_op_mode_t op_mode)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    switch (op_mode)
    {
    case LIS2MDL_CONTINUOUS_MODE:
        reg_value &= ~(0x03);
        break;
    case LIS2MDL_SINGLE_TRIGGER:
        reg_value &= ~(0x01);
        break;
    case LIS2MDL_POWER_DOWN:
        reg_value |= (uint8_t)op_mode;
        break;
    case NOT_USED_POWER_DOWN:
        reg_value |= (uint8_t)op_mode;
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    reg_value |= (uint8_t)op_mode;
    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
}

esp_err_t esp_lis2mdl_op_mode_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_A, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }
    reg_value &= 0x03;

    switch (reg_value)
    {
    case LIS2MDL_CONTINUOUS_MODE:
        sensor_handle->config.op_mode = LIS2MDL_CONTINUOUS_MODE;
        break;
    case LIS2MDL_SINGLE_TRIGGER:
        sensor_handle->config.op_mode = LIS2MDL_SINGLE_TRIGGER;
        break;
    case LIS2MDL_POWER_DOWN:
        sensor_handle->config.op_mode = LIS2MDL_POWER_DOWN;
        break;
    case NOT_USED_POWER_DOWN:
        sensor_handle->config.op_mode = NOT_USED_POWER_DOWN;
        break;
    default:
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

/* CFG_REG_B ------------------------------------------------------------------*/

esp_err_t esp_lis2mdl_offset_cancellation_single_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_SINGLE_OFF_CANC_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_SINGLE_OFF_CANC_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
}

esp_err_t esp_lis2mdl_offset_cancellation_single_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.offset_cancellation_single = reg_value & LIS2MDL_SINGLE_OFF_CANC_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_interrupt_on_data_off_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Continuous offset cancellation must be enabled
    ret = esp_lis2mdl_offset_cancellation_cont_set(LIS2MDL_PROPERTY_ENABLE);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_INT_ON_DATAOFF_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_INT_ON_DATAOFF_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
}

esp_err_t esp_lis2mdl_interrupt_on_data_off_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.interrupt_on_data_off = reg_value & LIS2MDL_INT_ON_DATAOFF_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_set_pulse_freq_set(lis2mdl_set_rst_t set_freq)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    switch (set_freq)
    {
    case LIS2MDL_SET_PUL_ODR:
        reg_value &= (uint8_t)LIS2MDL_SET_PUL_APD;
        break;
    case LIS2MDL_SET_PUL_APD:
        reg_value |= (uint8_t)set_freq;
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
}

esp_err_t esp_lis2mdl_set_pulse_freq_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.set_pulse_freq = reg_value & LIS2MDL_SET_PUL_APD;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_offset_cancellation_cont_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_CONT_OFF_CANC_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_CONT_OFF_CANC_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
}

esp_err_t esp_lis2mdl_offset_cancellation_cont_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.offset_cancellation_cont = reg_value & LIS2MDL_CONT_OFF_CANC_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_bw_lpf_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_LPF_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_LPF_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
}

esp_err_t esp_lis2mdl_bw_lpf_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_B, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.low_pass_filter = reg_value & LIS2MDL_LPF_ENABLE;

    return ESP_OK;
}

/* CFG_REG_C ------------------------------------------------------------------*/

esp_err_t esp_lis2mdl_int_to_pin_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_INT_TO_PIN_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_INT_TO_PIN_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
}

esp_err_t esp_lis2mdl_int_to_pin_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.int_to_pin = reg_value & LIS2MDL_INT_TO_PIN_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_i2c_disable_set(lis2mdl_i2c_dis_t i2c_enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    switch (i2c_enable)
    {
    case LIS2MDL_I2C_ENABLE:
        reg_value &= ~(uint8_t)LIS2MDL_I2C_DISABLE;
        break;
    case LIS2MDL_I2C_DISABLE:
        reg_value |= (uint8_t)i2c_enable;
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
}

esp_err_t esp_lis2mdl_i2c_disable_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.i2c_disable = reg_value & LIS2MDL_I2C_DISABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_block_data_update_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_BDU_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_BDU_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
}

esp_err_t esp_lis2mdl_block_data_update_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.block_data_update = reg_value & LIS2MDL_BDU_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_endianness_set(lis2mdl_endianness_t endianness)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    switch (endianness)
    {
    case LIS2MDL_LITTLE_ENDIAN:
        reg_value &= ~(uint8_t)LIS2MDL_BIG_ENDIAN;
        break;
    case LIS2MDL_BIG_ENDIAN:
        reg_value |= (uint8_t)endianness;
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
}

esp_err_t esp_lis2mdl_endianness_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.endianness = reg_value & LIS2MDL_BIG_ENDIAN;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_spi_mode_set(lis2mdl_sim_t spi_mode)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    switch (spi_mode)
    {
    case LIS2MDL_SPI_3_WIRE:
        reg_value &= ~(uint8_t)LIS2MDL_SPI_4_WIRE;
        break;
    case LIS2MDL_SPI_4_WIRE:
        reg_value |= (uint8_t)spi_mode;
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
}

esp_err_t esp_lis2mdl_spi_mode_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.four_wire_spi_mode = reg_value & LIS2MDL_SPI_4_WIRE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_self_test_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_SELF_TEST_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_SELF_TEST_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
}

esp_err_t esp_lis2mdl_self_test_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->self_test_enabled = reg_value & LIS2MDL_SELF_TEST_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_drdy_to_pin_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_DRDY_TO_PIN_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_DRDY_TO_PIN_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
}

esp_err_t esp_lis2mdl_drdy_to_pin_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_CFG_REG_C, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.drdy_to_pin = reg_value & LIS2MDL_DRDY_TO_PIN_ENABLE;

    return ESP_OK;
}

/* INT_CTRL_REG ---------------------------------------------------------------*/

esp_err_t esp_lis2mdl_interrupt_configure_set(uint8_t *int_config)
{
    return esp_lis2mdl_write_reg(LIS2MDL_INT_CTRL_REG, int_config, 1);
}

esp_err_t esp_lis2mdl_interrupt_configure_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.x_int_enable = reg_value & LIS2MDL_INT_ON_X_ENABLE;
    sensor_handle->config.y_int_enable = reg_value & LIS2MDL_INT_ON_Y_ENABLE;
    sensor_handle->config.z_int_enable = reg_value & LIS2MDL_INT_ON_Z_ENABLE;
    sensor_handle->config.int_polarity = reg_value & LIS2MDL_POLARITY_POS;
    sensor_handle->config.int_latch = reg_value & LIS2MDL_LATCH_SET;
    sensor_handle->config.int_enable = reg_value & LIS2MDL_INT_ENABLE;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_interrupt_polarity_set(lis2md_int_polarity_t iea_polarity)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    switch (iea_polarity)
    {
    case LIS2MDL_POLARITY_NEG:
        reg_value &= ~(uint8_t)LIS2MDL_POLARITY_POS;
        break;
    case LIS2MDL_POLARITY_POS:
        reg_value |= (uint8_t)iea_polarity;
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
}

esp_err_t esp_lis2mdl_interrupt_polarity_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.int_polarity = reg_value & LIS2MDL_POLARITY_POS;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_interrupt_latch_set(lis2md_int_latch_t iel_latch)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    switch (iel_latch)
    {
    case LIS2MDL_LATCH_UNSET:
        reg_value &= ~(uint8_t)LIS2MDL_LATCH_SET;
        break;
    case LIS2MDL_LATCH_SET:
        reg_value |= (uint8_t)iel_latch;
        break;
    default:
        return ESP_ERR_NOT_ALLOWED;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
}

esp_err_t esp_lis2mdl_interrupt_latch_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.int_latch = reg_value & LIS2MDL_LATCH_SET;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_interrupt_enable_set(bool enable)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        reg_value |= LIS2MDL_INT_ENABLE;
    }
    else
    {
        reg_value &= ~LIS2MDL_INT_ENABLE;
    }

    return esp_lis2mdl_write_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
}

esp_err_t esp_lis2mdl_interrupt_enable_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_CTRL_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->config.int_enable = reg_value & LIS2MDL_INT_ENABLE;

    return ESP_OK;
}

/* INT_SOURCE_REG --------------------------------------------------------------*/

esp_err_t esp_lis2mdl_get_interrupt_source(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_INT_SOURCE_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->interrupt_src.p_th_s_x = reg_value & 0x80;
    sensor_handle->interrupt_src.p_th_s_y = reg_value & 0x40;
    sensor_handle->interrupt_src.p_th_s_z = reg_value & 0x20;
    sensor_handle->interrupt_src.n_th_s_x = reg_value & 0x10;
    sensor_handle->interrupt_src.n_th_s_y = reg_value & 0x08;
    sensor_handle->interrupt_src.n_th_s_z = reg_value & 0x04;
    sensor_handle->interrupt_src.signal_int = reg_value & 0x01;

    return ESP_OK;
}

/* STATUS_REG ------------------------------------------------------------------*/

esp_err_t esp_lis2mdl_get_data_overrun(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_STATUS_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->data_status.dovr_xyz = reg_value & 0x80;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_get_data_ready(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_STATUS_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->data_status.drdy_xyz = reg_value & 0x08;

    return ESP_OK;
}

uint8_t esp_lis2mdl_get_data_ready_status()
{
    uint8_t reg_value;
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_STATUS_REG, &reg_value, 1);
    if (ret != ESP_OK)
    {
        return -1;
    }

    return reg_value & 0x08;
}

/* Threshold and offsets -------------------------------------------------------*/

esp_err_t esp_lis2mdl_hard_iron_offset_set(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t hi_offset_data[6];

    hi_offset_data[0] = (uint8_t)sensor_handle->hi_offset.hi_x & 0xFF;
    hi_offset_data[1] = (uint8_t)(sensor_handle->hi_offset.hi_x >> 8) & 0xFF;
    hi_offset_data[2] = (uint8_t)sensor_handle->hi_offset.hi_y & 0xFF;
    hi_offset_data[3] = (uint8_t)(sensor_handle->hi_offset.hi_y >> 8) & 0xFF;
    hi_offset_data[4] = (uint8_t)sensor_handle->hi_offset.hi_z & 0xFF;
    hi_offset_data[5] = (uint8_t)(sensor_handle->hi_offset.hi_z >> 8) & 0xFF;

    return esp_lis2mdl_write_reg(LIS2MDL_OFFSET_X_REG_L, hi_offset_data, 6);
}

esp_err_t esp_lis2mdl_hard_iron_offset_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t hi_offset_data[6];

    esp_err_t ret = esp_lis2mdl_read_reg(LIS2MDL_OFFSET_X_REG_L, hi_offset_data, 6);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->hi_offset.hi_x = (int16_t)(hi_offset_data[1] << 8 | hi_offset_data[0]);
    sensor_handle->hi_offset.hi_y = (int16_t)(hi_offset_data[3] << 8 | hi_offset_data[2]);
    sensor_handle->hi_offset.hi_z = (int16_t)(hi_offset_data[5] << 8 | hi_offset_data[4]);

    return ESP_OK;
}
esp_err_t esp_lis2mdl_int_threshold_set(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t int_threshold_data[2];

    int_threshold_data[0] = (uint8_t)sensor_handle->int_threshold & 0xFF;
    int_threshold_data[1] = (uint8_t)(sensor_handle->int_threshold >> 8) & 0xFF;

    return esp_lis2mdl_write_reg(LIS2MDL_INT_THS_L_REG, int_threshold_data, 2);
}

esp_err_t esp_lis2mdl_int_threshold_get(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t int_threshold_data[2];

    esp_err_t ret = esp_lis2mdl_read_reg(LIS2MDL_INT_THS_L_REG, int_threshold_data, 2);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->int_threshold = (int16_t)(int_threshold_data[1] << 8 | int_threshold_data[0]);

    return ESP_OK;
}

/* Data readings ---------------------------------------------------------------*/

esp_err_t esp_lis2mdl_get_magnetic_raw(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t data_buffer[6];
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_OUTX_L_REG, data_buffer, 6);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->mag_value.raw_x = (int16_t)(data_buffer[1] << 8 | data_buffer[0]);
    sensor_handle->mag_value.raw_y = (int16_t)(data_buffer[3] << 8 | data_buffer[2]);
    sensor_handle->mag_value.raw_z = (int16_t)(data_buffer[5] << 8 | data_buffer[4]);

    return ESP_OK;
}

esp_err_t esp_lis2mdl_get_magnetic_mgauss(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t data_buffer[6];
    esp_err_t ret;
    const float sensitivity = 1.5f; // sensitivity in mGauss/LSB

    ret = esp_lis2mdl_read_reg(LIS2MDL_OUTX_L_REG, data_buffer, 6);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->mag_value.raw_x = (int16_t)(data_buffer[1] << 8 | data_buffer[0]);
    sensor_handle->mag_value.raw_y = (int16_t)(data_buffer[3] << 8 | data_buffer[2]);
    sensor_handle->mag_value.raw_z = (int16_t)(data_buffer[5] << 8 | data_buffer[4]);

    sensor_handle->mag_value.mgauss_x = (float)sensor_handle->mag_value.raw_x * sensitivity;
    sensor_handle->mag_value.mgauss_y = (float)sensor_handle->mag_value.raw_y * sensitivity;
    sensor_handle->mag_value.mgauss_z = (float)sensor_handle->mag_value.raw_z * sensitivity;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_get_magnetic_utesla(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t data_buffer[6];
    esp_err_t ret;
    const float sensitivity = 1.5f;       // sensitivity in mGauss/LSB
    const float conversion_factor = 0.1f; // 1 mGauss = 0.1 µTesla

    ret = esp_lis2mdl_read_reg(LIS2MDL_OUTX_L_REG, data_buffer, 6);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->mag_value.raw_x = (int16_t)(data_buffer[1] << 8 | data_buffer[0]);
    sensor_handle->mag_value.raw_y = (int16_t)(data_buffer[3] << 8 | data_buffer[2]);
    sensor_handle->mag_value.raw_z = (int16_t)(data_buffer[5] << 8 | data_buffer[4]);

    sensor_handle->mag_value.uTesla_x = (float)sensor_handle->mag_value.raw_x * sensitivity * conversion_factor;
    sensor_handle->mag_value.uTesla_y = (float)sensor_handle->mag_value.raw_y * sensitivity * conversion_factor;
    sensor_handle->mag_value.uTesla_z = (float)sensor_handle->mag_value.raw_z * sensitivity * conversion_factor;

    return ESP_OK;
}

esp_err_t esp_lis2mdl_get_temperature_raw(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t data_buffer[2];
    esp_err_t ret;

    ret = esp_lis2mdl_read_reg(LIS2MDL_TEMP_OUT_L_REG, data_buffer, 2);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->temp_value.raw = (int16_t)(data_buffer[1] << 8 | data_buffer[0]);

    return ESP_OK;
}

esp_err_t esp_lis2mdl_get_temperature_celsius(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t data_buffer[2];
    esp_err_t ret;
    const float sensitivity = 8.0f;  // sensitivity in digits/Celsius
    const float zero_offset = 25.0f; // zero value means 25 Celsius

    ret = esp_lis2mdl_read_reg(LIS2MDL_TEMP_OUT_L_REG, data_buffer, 2);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->temp_value.raw = (int16_t)(data_buffer[1] << 8 | data_buffer[0]);
    sensor_handle->temp_value.celsius = (sensor_handle->temp_value.raw / sensitivity) + zero_offset;

    return ESP_OK;
}

/* Misc functions --------------------------------------------------------------*/

esp_err_t esp_lis2mdl_get_calibrated_uTesla(lis2mdl_sensor_handle_t *sensor_handle)
{
    uint8_t data_buffer[6];
    esp_err_t ret;
    const float conversion_factor = 0.1f; // 1 mGauss = 0.1 µTesla
    const float sensitivity = 1.5f;       // sensitivity in mGauss/LSB

    ret = esp_lis2mdl_read_reg(LIS2MDL_OUTX_L_REG, data_buffer, 6);
    if (ret != ESP_OK)
    {
        return ret;
    }

    sensor_handle->mag_value.raw_x = (int16_t)(data_buffer[1] << 8 | data_buffer[0]);
    sensor_handle->mag_value.raw_y = (int16_t)(data_buffer[3] << 8 | data_buffer[2]);
    sensor_handle->mag_value.raw_z = (int16_t)(data_buffer[5] << 8 | data_buffer[4]);

    sensor_handle->mag_value.uTesla_x = (float)sensor_handle->mag_value.raw_x * sensitivity * conversion_factor;
    sensor_handle->mag_value.uTesla_y = (float)sensor_handle->mag_value.raw_y * sensitivity * conversion_factor;
    sensor_handle->mag_value.uTesla_z = (float)sensor_handle->mag_value.raw_z * sensitivity * conversion_factor;

    // Hard iron offset cancellation
    sensor_handle->mag_value.uTesla_x -= HARD_IRON_OFFSET[0];
    sensor_handle->mag_value.uTesla_y -= HARD_IRON_OFFSET[1];
    sensor_handle->mag_value.uTesla_z -= HARD_IRON_OFFSET[2];

    float utesla_x = sensor_handle->mag_value.uTesla_x;
    float utesla_y = sensor_handle->mag_value.uTesla_y;
    float utesla_z = sensor_handle->mag_value.uTesla_z;

    // Soft iron offset cancellation
    sensor_handle->mag_value.uTesla_x = (SOFT_IRON_MATRIX[0][0] * utesla_x) +
                                        (SOFT_IRON_MATRIX[0][1] * utesla_y) +
                                        (SOFT_IRON_MATRIX[0][2] * utesla_z);

    sensor_handle->mag_value.uTesla_y = (SOFT_IRON_MATRIX[1][0] * utesla_x) +
                                        (SOFT_IRON_MATRIX[1][1] * utesla_y) +
                                        (SOFT_IRON_MATRIX[1][2] * utesla_z);

    sensor_handle->mag_value.uTesla_z = (SOFT_IRON_MATRIX[2][0] * utesla_x) +
                                        (SOFT_IRON_MATRIX[2][1] * utesla_y) +
                                        (SOFT_IRON_MATRIX[2][2] * utesla_z);

    return ESP_OK;
}

esp_err_t esp_lis2mdl_get_avg_calibrated_uTesla(lis2mdl_sensor_handle_t *sensor_handle, uint8_t samples)
{
    uint8_t data_buffer[6];
    esp_err_t ret;
    const float conversion_factor = 0.1f; // 1 mGauss = 0.1 µTesla
    const float sensitivity = 1.5f;       // sensitivity in mGauss/LSB
    uint8_t drd = 0;
    int16_t buf_x, buf_y, buf_z;
    int32_t AVG_X = 0;
    int32_t AVG_Y = 0;
    int32_t AVG_Z = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        while (drd == 0)
        {
            drd = esp_lis2mdl_get_data_ready_status();
        }
        ret = esp_lis2mdl_read_reg(LIS2MDL_OUTX_L_REG, data_buffer, 6);
        if (ret != ESP_OK)
        {
            return ret;
        }
        buf_x = (int16_t)(data_buffer[1] << 8 | data_buffer[0]);
        buf_y = (int16_t)(data_buffer[3] << 8 | data_buffer[2]);
        buf_z = (int16_t)(data_buffer[5] << 8 | data_buffer[4]);

        AVG_X += buf_x;
        AVG_Y += buf_y;
        AVG_Z += buf_z;
        drd = 0;
    }

    sensor_handle->mag_value.raw_x = (int16_t)(AVG_X / samples);
    sensor_handle->mag_value.raw_y = (int16_t)(AVG_Y / samples);
    sensor_handle->mag_value.raw_z = (int16_t)(AVG_Z / samples);

    sensor_handle->mag_value.uTesla_x = (float)sensor_handle->mag_value.raw_x * sensitivity * conversion_factor;
    sensor_handle->mag_value.uTesla_y = (float)sensor_handle->mag_value.raw_y * sensitivity * conversion_factor;
    sensor_handle->mag_value.uTesla_z = (float)sensor_handle->mag_value.raw_z * sensitivity * conversion_factor;

    // Hard iron offset cancellation
    sensor_handle->mag_value.uTesla_x -= HARD_IRON_OFFSET[0];
    sensor_handle->mag_value.uTesla_y -= HARD_IRON_OFFSET[1];
    sensor_handle->mag_value.uTesla_z -= HARD_IRON_OFFSET[2];

    float utesla_x = sensor_handle->mag_value.uTesla_x;
    float utesla_y = sensor_handle->mag_value.uTesla_y;
    float utesla_z = sensor_handle->mag_value.uTesla_z;

    // Soft iron offset cancellation
    sensor_handle->mag_value.uTesla_x = (SOFT_IRON_MATRIX[0][0] * utesla_x) +
                                        (SOFT_IRON_MATRIX[0][1] * utesla_y) +
                                        (SOFT_IRON_MATRIX[0][2] * utesla_z);

    sensor_handle->mag_value.uTesla_y = (SOFT_IRON_MATRIX[1][0] * utesla_x) +
                                        (SOFT_IRON_MATRIX[1][1] * utesla_y) +
                                        (SOFT_IRON_MATRIX[1][2] * utesla_z);

    sensor_handle->mag_value.uTesla_z = (SOFT_IRON_MATRIX[2][0] * utesla_x) +
                                        (SOFT_IRON_MATRIX[2][1] * utesla_y) +
                                        (SOFT_IRON_MATRIX[2][2] * utesla_z);

    return ESP_OK;
}
