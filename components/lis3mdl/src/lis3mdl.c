#include "lis3mdl.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include "driver/i2c.h"

static const char *TAG = "AMR_IOT_SENSOR_LIS3MDL";

static esp_err_t i2c_master_init() {
    int i2c_master_port = I2C_PORT_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_RX_BUF_DISABLE, I2C_TX_BUF_DISABLE, 0);
}

esp_err_t lis3mdl_init(const lis3mdl_config_t *config){
    esp_err_t err;

    // Initialize I2C
    err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t reg_value;
    // Check WHO_AM_I_REG
    err = read_register(WHO_AM_I_REG, &reg_value, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I_REG");
        return err;
    }
    if (reg_value != 0x3D) {
        ESP_LOGE(TAG, "Invalid WHO_AM_I_REG value: 0x%X", reg_value);
        return ESP_FAIL;
    }

    // Perform a soft reset
    err = lis3mdl_soft_reset();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to perform soft reset");
        return err;
    }
    // Configure CTRL_REG1
    reg_value = 0x00;
    reg_value |= (config->temp_sensor_enable << 7);
    reg_value |= (config->om_xy_mode);
    reg_value |= (config->odata_rate);
    reg_value |= (config->fast_odr_en << 1);
    reg_value |= config->self_test_en;
    err = write_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }
    // Configure CTRL_REG2
    reg_value = config->fs_config;
    err = write_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }
    // Configure CTRL_REG3
    reg_value = (config->low_power_mode << 5) | config->md_mode;
    err = write_register(CTRL_REG3, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }
    // Configure CTRL_REG4
    reg_value = (config->om_z_mode) | (config->big_endian_enabled << 1);
    err = write_register(CTRL_REG4, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }
    return err;
}

/**
 * @brief Initialize the LIS3MDL sensor with default configuration
 */
esp_err_t lis3mdl_init_default(){
    lis3mdl_config_t default_config = {
        .temp_sensor_enable = true,
        .om_xy_mode = LIS3MDL_OM_XY_UP_MODE,
        .odata_rate = LIS3MDL_ODATA_RATE_80_HZ,
        .fast_odr_en = true,
        .self_test_en = false,
        .fs_config = LIS3MDL_FS_4_GAUSS,
        .low_power_mode = false,
        .md_mode = LIS3MDL_MD_SINGLE_MODE,
        .om_z_mode = LIS3MDL_OM_Z_UP_MODE,
        .big_endian_enabled = false
    };
    return lis3mdl_init(&default_config);
}

/** 
 * @brief Read <count> bytes from the specified register of the LIS3MDL sensor
 * @param reg Register address to read from
 * @param pdata Pointer to the data buffer to store the read data
 * @param count Number of bytes to read
 * @return ESP_OK if successful, ESP_FAIL otherwise
*/
esp_err_t read_register(uint8_t reg, uint8_t *pdata, uint8_t count) {
    return i2c_read_from_address(I2C_PORT_NUM, LIS3MDL_I2C_ADDR, reg, pdata, count);
}

/** 
 * @brief Write <count> bytes to the specified register of the LIS3MDL sensor
 * @param reg Register address to write to
 * @param pdata Pointer to the data buffer to write
 * @param count Number of bytes to write
*/
esp_err_t write_register(uint8_t reg, uint8_t *pdata, uint8_t count) {
    return i2c_write_to_address(I2C_PORT_NUM, LIS3MDL_I2C_ADDR, reg, pdata, count);
}

/**
 * @brief Write <size> bytes to the specified register of the LIS3MDL sensor
 * @param i2c_num I2C port number
 * @param i2c_addr I2C address of the sensor
 * @param i2c_reg Register address to write to
 * @param data_wr Pointer to the data buffer to write
 * @param size Number of bytes to write
 * @return ESP_OK if successful
 * 
 * @details The write sequence for LIS3MDL sensor is as follows:
 * 1. Start
 * 2. Write the sensor device address with the write bit
 * 3. Write the register address to write to (7 LSB indicate the address,  1 MSB indicate auto increment)
 * 4. Write the data to the register
 * 5. Stop
 */
esp_err_t i2c_write_to_address(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c_addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_reg |= 0x80;
    }
    i2c_master_write_byte(cmd, i2c_reg, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/** 
 * @brief Read <count> bytes from the specified register of the LIS3MDL sensor
 * @param i2c_num I2C port number
 * @param i2c_addr I2C address of the sensor
 * @param i2c_reg Register address to read from
 * @param data_rd Pointer to the data buffer to store the read data
 * @param size Number of bytes to read
 * @return ESP_OK if successfuL
 * 
 * @details The read sequence for LIS3MDL sensor is as follows:
 * 1. Start
 * 2. Write the sensor device address with the write bit
 * 3. Write the register address to read from (7 LSB indicate the address,  1 MSB indicate auto increment)
 * 4. Start (repeated start)
 * 5. Write the sensor device address with the read bit
 * 6. Read the data from the register
 * 7. Send NACK to the sensor (After reading the last byte)
*/
esp_err_t i2c_read_from_address(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size) {
    if (size == 0) {
        return ESP_OK;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c_addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_reg |= 0x80;
    }
    i2c_master_write_byte(cmd, i2c_reg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c_addr << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief Read and print the offset values for the X, Y and Z axes
 */
void lis3mdl_read_xyz_offsets(){
    uint8_t offset_data[6];
    esp_err_t err;

    // Read OFFSET_X_REG_L_M to OFFSET_Z_REG_H_M
    err = read_register(OFFSET_X_REG_L_M, offset_data, 6);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read offset registers");
        return;
    }

    // Combine the bytes for each axis
    int16_t offset_x = (int16_t)((offset_data[1] << 8) | offset_data[0]);
    int16_t offset_y = (int16_t)((offset_data[3] << 8) | offset_data[2]);
    int16_t offset_z = (int16_t)((offset_data[5] << 8) | offset_data[4]);

    // Print the offsets
    ESP_LOGI(TAG, "Offset X: %d", offset_x);
    ESP_LOGI(TAG, "Offset Y: %d", offset_y);
    ESP_LOGI(TAG, "Offset Z: %d", offset_z);  
}

/**
 * @brief Set the offset values for the X, Y and Z axes
 */
esp_err_t lis3mdl_set_xyz_offsets(int16_t offset_x, int16_t offset_y, int16_t offset_z){
    uint8_t offset_data[6];

    // Split the offsets into low and high bytes
    offset_data[0] = offset_x & 0xFF;
    offset_data[1] = (offset_x >> 8) & 0xFF;
    offset_data[2] = offset_y & 0xFF;
    offset_data[3] = (offset_y >> 8) & 0xFF;
    offset_data[4] = offset_z & 0xFF;
    offset_data[5] = (offset_z >> 8) & 0xFF;

    // Write the offset data to the registers
    return write_register(OFFSET_X_REG_L_M, offset_data, 6);
}

/**
 * @brief Enables/Disables the internal temperature sensor
 */
esp_err_t lis3mdl_set_temp_sensor(bool enable) {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (enable) {
        reg_value |= (1 << 7);
    } else {
        reg_value &= ~(1 << 7);
    }

    return write_register(CTRL_REG1, &reg_value, 1);
}

/**
 * @brief Get the active status of the internal temperature sensor
 */
bool lis3mdl_get_temp_sensor() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read temperature sensor status");
        return false; // Return false in case of error
    }

    bool enabled = reg_value & (1 << 7);
    ESP_LOGI(TAG, "Temperature sensor is %s", enabled ? "enabled" : "disabled");

    return enabled;
}

/**
 * @brief Read the temperature from the sensor
 * @return Temperature in Celsius
 */
float lis3mdl_read_temperature() {
    uint8_t temp_out[2];
    esp_err_t err;

    // Read TEMP_OUT_L_REG and TEMP_OUT_H_REG
    err = read_register(TEMP_OUT_L_REG, temp_out, 2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read temperature registers");
        return -1.0; // Return -1.0 in case of error
    }

    // Combine the two bytes
    int16_t temp_raw = (int16_t)((temp_out[1] << 8) | temp_out[0]);

    // Calculate the temperature in Celsius
    float temperature = 25.0 + (temp_raw / 8.0);

    ESP_LOGI(TAG, "Temperature is %.2f °C", temperature);
    return temperature;
}

/**
 * @brief Set the XY axis operating mode - Low, Medium, High or Ultra performance
 * @param mode Operating mode to set
 * @return ESP_OK if successful, ESP_ERR_INVALID_ARG if invalid mode is provided
 * 0 - Low performance mode
 * 1 - Medium performance mode
 * 2 - High performance mode
 * 3 - Ultra performance mode
 */
esp_err_t lis3mdl_set_om_xy_mode(uint8_t mode) {

    lis3mdl_om_xy_mode_t om_xy_mode;
    switch (mode) {
        case 0:
            om_xy_mode = LIS3MDL_OM_XY_LP_MODE;
            break;
        case 1:
            om_xy_mode = LIS3MDL_OM_XY_MP_MODE;
            break;
        case 2:
            om_xy_mode = LIS3MDL_OM_XY_HP_MODE;
            break;
        case 3:
            om_xy_mode = LIS3MDL_OM_XY_UP_MODE;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }


    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    reg_value &= ~(0x03 << 5); // Clear the OM bits
    reg_value |= om_xy_mode;  // Set the new OM bits

    return write_register(CTRL_REG1, &reg_value, 1);
}

/**
 * @brief Get and print the XY axis operating mode
 * @return Operating mode of the XY axis
 */
uint8_t lis3mdl_get_om_xy_mode() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        return 4; // Error code
    }
    uint8_t mode = (reg_value >> 5) & 0x03;
    switch (mode) {
        case 0:
            ESP_LOGI(TAG, "XY axis operating mode is Low Power");
            break;
        case 1:
            ESP_LOGI(TAG, "XY axis operating mode is Medium Performance");
            break;
        case 2:
            ESP_LOGI(TAG, "XY axis operating mode is High Performance");
            break;
        case 3:
            ESP_LOGI(TAG, "XY axis operating mode is Ultra High Performance");
            break;
        default:
            return 4; // Error code
    }
    return mode;
}

/**
 * @brief Set the data rate of the sensor
 * @param rate Data rate to set in Hz (0.625, 1.25, 2.5, 5, 10, 20, 40, 80)
 * @return ESP_OK if successful, ESP_ERR_INVALID_ARG if invalid data rate is provided
 */
esp_err_t lis3mdl_set_data_rate(float rate) {
    lis3mdl_output_data_rate_t data_rate;
    switch ((int)(rate * 1000)) {
        case 625:
            data_rate = LIS3MDL_ODATA_RATE_0_625_HZ;
            break;
        case 1250:
            data_rate = LIS3MDL_ODATA_RATE_1_25_HZ;
            break;
        case 2500:
            data_rate = LIS3MDL_ODATA_RATE_2_5_HZ;
            break;
        case 5000:
            data_rate = LIS3MDL_ODATA_RATE_5_HZ;
            break;
        case 10000:
            data_rate = LIS3MDL_ODATA_RATE_10_HZ;
            break;
        case 20000:
            data_rate = LIS3MDL_ODATA_RATE_20_HZ;
            break;
        case 40000:
            data_rate = LIS3MDL_ODATA_RATE_40_HZ;
            break;
        case 80000:
            data_rate = LIS3MDL_ODATA_RATE_80_HZ;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    reg_value &= ~(0x07 << 2); // Clear the ODR bits
    reg_value |= data_rate;    // Set the new ODR bits

    return write_register(CTRL_REG1, &reg_value, 1);
}

/**
 * @brief Get and print the data rate
 */
float lis3mdl_get_data_rate() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read data rate");
        return -1.0; // Return -1.0 in case of error
    }
    // Check if fast data rate is enabled
    bool fast_odr_enabled = reg_value & (1 << 1);
    if (fast_odr_enabled) {
        uint8_t om_xy_mode = reg_value & 0x60;
        float rate;
        switch (om_xy_mode) {
            case LIS3MDL_OM_XY_LP_MODE:
                rate = 1000.0;
                break;
            case LIS3MDL_OM_XY_MP_MODE:
                rate = 560.0;
                break;
            case LIS3MDL_OM_XY_HP_MODE:
                rate = 300.0;
                break;
            case LIS3MDL_OM_XY_UP_MODE:
                rate = 155.0;
                break;
            default:
                ESP_LOGE(TAG, "Unknown XY operating mode");
                return -1.0; // Return -1.0 in case of unknown XY operating mode
        }
        ESP_LOGI(TAG, "Fast data rate is enabled. Data rate is %.1f Hz", rate);
        return rate;
    }

    uint8_t data_rate = (reg_value) & 0x1C;
    float rate;
    switch (data_rate) {
        case LIS3MDL_ODATA_RATE_0_625_HZ:
            rate = 0.625;
            break;
        case LIS3MDL_ODATA_RATE_1_25_HZ:
            rate = 1.25;
            break;
        case LIS3MDL_ODATA_RATE_2_5_HZ:
            rate = 2.5;
            break;
        case LIS3MDL_ODATA_RATE_5_HZ:
            rate = 5.0;
            break;
        case LIS3MDL_ODATA_RATE_10_HZ:
            rate = 10.0;
            break;
        case LIS3MDL_ODATA_RATE_20_HZ:
            rate = 20.0;
            break;
        case LIS3MDL_ODATA_RATE_40_HZ:
            rate = 40.0;
            break;
        case LIS3MDL_ODATA_RATE_80_HZ:
            rate = 80.0;
            break;
        default:
            ESP_LOGE(TAG, "Unknown data rate");
            return -1.0; // Return -1.0 in case of unknown data rate
    }

    ESP_LOGI(TAG, "Data rate is %.3f Hz", rate);
    return rate;
}

/**
 * @brief Enable or disable fast data rate mode
 */
esp_err_t lis3mdl_enable_fast_data_rate(bool enable) {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (enable) {
        reg_value |= (1 << 1);
    } else {
        reg_value &= ~(1 << 1);
    }

    return write_register(CTRL_REG1, &reg_value, 1);
}

/**
 * @brief Enable or disable self-test
 */
esp_err_t lis3mdl_enable_self_test(bool enable) {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG1, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (enable) {
        reg_value |= (1 << 0);
    } else {
        reg_value &= ~(1 << 0);
    }

    return write_register(CTRL_REG1, &reg_value, 1);
}

/**
 * @brief Performs self test procedure according to AN4602 application note
 * @return true if self test is successful, false otherwise
 */
bool lis3mdl_perform_self_test(){
    uint8_t st_command;
    esp_err_t err;
    uint8_t status_reg_bit_to_check = 3;
    bool is_xyz_data_ready = false;
    float x_gauss, y_gauss, z_gauss;
    float X_NOST, Y_NOST, Z_NOST;
    float X_ST, Y_ST, Z_ST;
    float X_diff, Y_diff, Z_diff;
    int i;
    
    float min_x, max_x, min_y, max_y, min_z, max_z;
    min_x = 1.0;
    max_x = 3.0;
    min_y = 1.0;
    max_y = 3.0;
    min_z = 0.1;
    max_z = 1.0;

    ESP_LOGI(TAG, "Starting self test...");
    st_command = 0x1C;
    err = write_register(CTRL_REG1, &st_command, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write 0x1C to CTRL_REG1 - exiting self test");
        return false;
    }

    st_command = 0x40;
    err = write_register(CTRL_REG2, &st_command, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write 0x40 to CTRL_REG2 - exiting self test");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(25));

    st_command = 0x00;
    err = write_register(CTRL_REG3, &st_command, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write 0x00 to CTRL_REG3 - exiting self test");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(25)); // Delay to allow the sensor to get stable output

    ESP_LOGI(TAG, "Checking if new data is available...");
    is_xyz_data_ready = lis3mdl_get_status_reg(status_reg_bit_to_check);

    while(!is_xyz_data_ready){
        ESP_LOGI(TAG, "Checking if new data is available...");
        is_xyz_data_ready = lis3mdl_get_status_reg(status_reg_bit_to_check);
    }
    
    ESP_LOGI(TAG, "New data is available - Reading first data to discard it...");
    err = lis3mdl_read_gauss_magnetic_data(&x_gauss, &y_gauss, &z_gauss);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read raw magnetic data - exiting self test");
        return false;
    }
    ESP_LOGI(TAG, "First data read and discarded - Reading data 5 times before activating self test");

    // Variables that hold the sum of the magnetic field values for each axis
    X_NOST = 0;
    Y_NOST = 0;
    Z_NOST = 0;

    for (i = 0; i < 5; i++) {
        is_xyz_data_ready = false;
        while(!is_xyz_data_ready){
            ESP_LOGI(TAG, "Checking if new data is available...");
            is_xyz_data_ready = lis3mdl_get_status_reg(status_reg_bit_to_check);
        }
        err = lis3mdl_read_gauss_magnetic_data(&x_gauss, &y_gauss, &z_gauss);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read raw magnetic data - exiting self test");
            return false;
        }
        X_NOST += x_gauss;
        Y_NOST += y_gauss;
        Z_NOST += z_gauss;
    }

    // Calculate the average magnetic field values for each axis
    X_NOST = X_NOST / 5;
    Y_NOST = Y_NOST / 5;
    Z_NOST = Z_NOST / 5;

    ESP_LOGI(TAG, "Average magnetic field values for each axis before enabling self test: X = %f, Y = %f, Z = %f", X_NOST, Y_NOST, Z_NOST);

    ESP_LOGI(TAG, "Enabling self test...");
    st_command = 0x1D;
    err = write_register(CTRL_REG1, &st_command, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write 0x1D to CTRL_REG1 - exiting self test");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(65));
    ESP_LOGI(TAG, "Self test enabled...");

    ESP_LOGI(TAG, "Checking if new data is available...");
    is_xyz_data_ready = lis3mdl_get_status_reg(status_reg_bit_to_check);

    while(!is_xyz_data_ready){
        ESP_LOGI(TAG, "Checking if new data is available...");
        is_xyz_data_ready = lis3mdl_get_status_reg(status_reg_bit_to_check);
    }
    
    ESP_LOGI(TAG, "New data is available - Reading first data to discard it...");
    err = lis3mdl_read_gauss_magnetic_data(&x_gauss, &y_gauss, &z_gauss);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read raw magnetic data - exiting self test");
        return false;
    }

    // Variables that hold the sum of the magnetic field values for each axis
    X_ST = 0;
    Y_ST = 0;
    Z_ST = 0;

    ESP_LOGI(TAG, "First data read and discarded - Reading data 5 times after activating self test");
    for (i = 0; i < 5; i++) {
        is_xyz_data_ready = false;
        while(!is_xyz_data_ready){
            ESP_LOGI(TAG, "Checking if new data is available...");
            is_xyz_data_ready = lis3mdl_get_status_reg(status_reg_bit_to_check);
        }
        err = lis3mdl_read_gauss_magnetic_data(&x_gauss, &y_gauss, &z_gauss);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read raw magnetic data - exiting self test");
            return false;
        }
        X_ST += x_gauss;
        Y_ST += y_gauss;
        Z_ST += z_gauss;
    }

    // Calculate the average magnetic field values for each axis
    X_ST = X_ST / 5;
    Y_ST = Y_ST / 5;
    Z_ST = Z_ST / 5;

    ESP_LOGI(TAG, "Average magnetic field values for each axis after enabling self test: X = %f, Y = %f, Z = %f", X_ST, Y_ST, Z_ST);

    // Calculate the difference between the magnetic field values before and after enabling self test
    X_diff = fabs(X_ST - X_NOST);
    Y_diff = fabs(Y_ST - Y_NOST);
    Z_diff = fabs(Z_ST - Z_NOST);

    ESP_LOGI(TAG, "Difference in magnetic field values for each axis: X = %f, Y = %f, Z = %f", X_diff, Y_diff, Z_diff);

    // Check if the difference is within the expected range
    if ((X_diff >= min_x && X_diff <= max_x) && (Y_diff >= min_y && Y_diff <= max_y) && (Z_diff >= min_z && Z_diff <= max_z)) {
        st_command = 0x1C;
        err = write_register(CTRL_REG1, &st_command, 1);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Failed to disable self test");
            return false;
        }
        st_command = 0x03;
        err = write_register(CTRL_REG3, &st_command, 1);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Failed to enter power down mode");
            return false;
        }
        return true;
    } else {
                st_command = 0x1C;
        err = write_register(CTRL_REG1, &st_command, 1);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Failed to disable self test");
            return false;
        }
        st_command = 0x03;
        err = write_register(CTRL_REG3, &st_command, 1);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Failed to enter power down mode");
            return false;
        }
        return false;
    }
}

/**
 * @brief Set the full scale range of the sensor
 * @param scale Full scale range to set in Gauss (4, 8, 12, 16)
 * @return ESP_OK if successful, ESP_ERR_INVALID_ARG if invalid scale is provided
 */
esp_err_t lis3mdl_set_full_scale(int scale) {
    lis3mdl_fs_config_t fs_config;
    switch (scale) {
        case 4:
            fs_config = LIS3MDL_FS_4_GAUSS;
            break;
        case 8:
            fs_config = LIS3MDL_FS_8_GAUSS;
            break;
        case 12:
            fs_config = LIS3MDL_FS_12_GAUSS;
            break;
        case 16:
            fs_config = LIS3MDL_FS_16_GAUSS;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    reg_value &= ~(0x03 << 5); // Clear the FS bits
    reg_value |= fs_config;    // Set the new FS bits

    return write_register(CTRL_REG2, &reg_value, 1);
}

/**
 * @brief Get and print the full scale range
 */
int lis3mdl_get_full_scale() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read full scale");
        return -1; // Return -1 in case of error
    }

    uint8_t fs_config = (reg_value) & 0x60;
    int scale;
    switch (fs_config) {
        case LIS3MDL_FS_4_GAUSS:
            scale = 4;
            break;
        case LIS3MDL_FS_8_GAUSS:
            scale = 8;
            break;
        case LIS3MDL_FS_12_GAUSS:
            scale = 12;
            break;
        case LIS3MDL_FS_16_GAUSS:
            scale = 16;
            break;
        default:
            ESP_LOGE(TAG, "Unknown full scale");
            return -1; // Return -1 in case of unknown full scale
    }

    ESP_LOGI(TAG, "Full scale is %d gauss", scale);
    return scale;
}

/**
 * @brief Perform a memory reboot of the sensor
 */
esp_err_t lis3mdl_reboot() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    reg_value |= (1 << 3); // Set the reboot bit

    err = write_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow the reboot to complete

    return ESP_OK;
}

/**
 * @brief Resets configuration registers and user registers to the default values
 */
esp_err_t lis3mdl_soft_reset() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    reg_value |= (1 << 2); // Set the soft reset bit

    err = write_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow the reset to complete

    return ESP_OK;
}

/**
 * @brief Enable or disable low power mode. Low power mode sets
 * data rate to 0.625 Hz and and the sensor performs the minimum number of averages
 */
esp_err_t lis3mdl_enable_low_power(bool enable) {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG3, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (enable) {
        reg_value |= (1 << 5);
    } else {
        reg_value &= ~(1 << 5);
    }

    return write_register(CTRL_REG3, &reg_value, 1);
}

/**
 * @brief Set the operating mode of the sensor
 * @param mode Operating mode to set (0 - Continuous, 1 - Single, 2 - Power Down)
 * @return ESP_OK if successful, ESP_ERR_INVALID_ARG if invalid mode is provided
 */
esp_err_t lis3mdl_set_operating_mode(uint8_t mode) {
    if (mode > 2) {
        return ESP_ERR_INVALID_ARG;
    }

    lis3mdl_md_mode_t md_mode;
    switch (mode) {
        case 0:
            md_mode = LIS3MDL_MD_CONTINUOUS_MODE;
            break;
        case 1:
            md_mode = LIS3MDL_MD_SINGLE_MODE;
            break;
        case 2:
            md_mode = LIS3MDL_MD_POWER_DOWN_MODE;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG3, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    reg_value &= ~(0x03); // Clear the MD bits
    reg_value |= md_mode; // Set the new MD bits

    return write_register(CTRL_REG3, &reg_value, 1);
}

/**
 * @brief Get and print the System operating mode
 */
uint8_t lis3mdl_get_operating_mode() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG3, &reg_value, 1);
    if (err != ESP_OK) {
        return -1; // Error code
    }

    uint8_t mode = reg_value & 0x03;
    switch (mode) {
        case LIS3MDL_MD_CONTINUOUS_MODE:
            ESP_LOGI(TAG, "Operating mode is Continuous");
            break;
        case LIS3MDL_MD_SINGLE_MODE:
            ESP_LOGI(TAG, "Operating mode is Single");
            break;
        case LIS3MDL_MD_POWER_DOWN_MODE:
            ESP_LOGI(TAG, "Operating mode is Power Down");
            break;
        default:
            ESP_LOGE(TAG, "Unknown Operating mode");
            return -1; // Error code
    }

    return mode;
}

/**
 * @brief Set the Z axis operating mode
 * @param mode Operating mode to set (0 - Low Power, 1 - Medium Performance, 2 - High Performance, 3 - Ultra High Performance)
 * @return ESP_OK if successful, ESP_ERR_INVALID_ARG if invalid mode is provided
 */
esp_err_t lis3mdl_set_om_z_mode(uint8_t mode) {
    lis3mdl_om_z_mode_t om_z_axis;
    switch (mode) {
        case 0:
            om_z_axis = LIS3MDL_OM_Z_LP_MODE;
            break;
        case 1:
            om_z_axis = LIS3MDL_OM_Z_MP_MODE;
            break;
        case 2:
            om_z_axis = LIS3MDL_OM_Z_HP_MODE;
            break;
        case 3:
            om_z_axis = LIS3MDL_OM_Z_UP_MODE;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG4, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    reg_value &= ~(0x03 << 2); // Clear the OM bits
    reg_value |= (om_z_axis << 2); // Set the new OM bits

    return write_register(CTRL_REG4, &reg_value, 1);
}

/**
 * @brief Get and print the Z axis operating mode
 * @return Operating mode of the Z axis (0 - Low Power, 1 - Medium Performance, 2 - High Performance, 3 - Ultra High Performance)
 * -1 in case of error
 */
uint8_t lis3mdl_get_om_z_mode() {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG4, &reg_value, 1);
    if (err != ESP_OK) {
        return -1; // Error code
    }

    uint8_t mode = (reg_value) & 0x0C;
    switch (mode) {
        case LIS3MDL_OM_Z_LP_MODE:
            ESP_LOGI(TAG, "Z axis operating mode is Low Power");
            break;
        case LIS3MDL_OM_Z_MP_MODE:
            ESP_LOGI(TAG, "Z axis operating mode is Medium Performance");
            break;
        case LIS3MDL_OM_Z_HP_MODE:
            ESP_LOGI(TAG, "Z axis operating mode is High Performance");
            break;
        case LIS3MDL_OM_Z_UP_MODE:
            ESP_LOGI(TAG, "Z axis operating mode is Ultra High Performance");
            break;
        default:
            ESP_LOGE(TAG, "Unknown Z axis operating mode");
            return -1; // Error code
    }

    return mode;
}

/**
 * @brief Set the endianess of the sensor data
 * @param endianess Endianess to set (0 - LSB first, 1 - MSB first)
 */
esp_err_t lis3mdl_set_endianess(uint8_t endianess) {
    if (endianess > 1) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG4, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (endianess == 1) {
        reg_value |= (1 << 1); // Set bit 1 for MSB first
    } else {
        reg_value &= ~(1 << 1); // Clear bit 1 for LSB first
    }

    return write_register(CTRL_REG4, &reg_value, 1);
}

/**
 * @brief Enable or disable fast read mode
 * @param enable Enable or disable fast read mode
 */
esp_err_t lis3mdl_enable_fast_read(bool enable) {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG5, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (enable) {
        reg_value |= (1 << 7); // Set bit 7 for fast read
    } else {
        reg_value &= ~(1 << 7); // Clear bit 7 for normal read
    }

    return write_register(CTRL_REG5, &reg_value, 1);
}

/**
 * @brief Enable or disable block data update
 * @param enable Enable block data update if true, disable if false
 */
esp_err_t lis3mdl_enable_block_data_update(bool enable) {
    uint8_t reg_value;
    esp_err_t err = read_register(CTRL_REG5, &reg_value, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (enable) {
        reg_value |= (1 << 6); // Set bit 6 for block data update
    } else {
        reg_value &= ~(1 << 6); // Clear bit 6 for normal data update
    }

    return write_register(CTRL_REG5, &reg_value, 1);
}

/**
 * @brief Configure the interrupt register
 * @param int_on_X Enable interrupt on X axis
 * @param int_on_Y Enable interrupt on Y axis
 * @param int_on_Z Enable interrupt on Z axis
 * @param int_polarity Set interrupt polarity (0 - active low, 1 - active high)
 * @param int_latch Latch interrupt request (0 -  latched, 1 - not latched) (if latched PIN remains in the same state until data is read)
 * @param int_enable Enable interrupt on the INT pin
 */
esp_err_t lis3mdl_configure_interrupt(bool int_on_X, bool int_on_Y, bool int_on_Z, bool int_polarity, bool int_latch, bool int_enable){
    uint8_t reg_value = 0;

    if (int_on_X) {
        reg_value |= (1 << 7); // Set bit 7 to enable interrupt
    }
    if (int_on_Y) {
        reg_value |= (1 << 6); // Set bit 6 to latch interrupt
    }
    if (int_on_Z) {
        reg_value |= (1 << 5); // Set bit 5 to enable interrupt on Z axis
    }
    if (int_polarity) {
        reg_value |= (1 << 2); // Set bit 2 to enable interrupt on Y axis
    }
    if (int_latch) {
        reg_value |= (1 << 1); // Set bit 1 to enable interrupt on X axis
    }
    if (int_enable) {
        reg_value |= (1 << 0); // Set bit 0 to set interrupt polarity
    }

    // Ensure bits 4 and 3 are set to 0 and 1, respectively
    reg_value &= ~(1 << 4); // Clear bit 4
    reg_value |= (1 << 3);  // Set bit 3

    return write_register(INT_CFG_REG, &reg_value, 1); 
}

/**
 * @brief Get the interrupt threshold for X, Y and Z axes
 * @param threshold Unsigned value for interrutp threshold. The sensor will
 * however apply negative threshold as well. Bit 15 must always be 0.
 */
esp_err_t lis3mdl_set_interrupt_threshold(uint16_t threshold) {
    if (threshold > 0x7FFF) {
        return ESP_ERR_INVALID_ARG; // Ensure the MSB is 0
    }

    uint8_t threshold_data[2];

    // Split the threshold into low and high bytes
    threshold_data[0] = threshold & 0xFF;
    threshold_data[1] = (threshold >> 8) & 0xFF;

    // Write the threshold data to the registers
    return write_register(INT_THS_L_REG, threshold_data, 2);
}

/**
 * @brief Get the interrupt threshold for X, Y and Z axes
 * @param threshold Pointer to store the interrupt threshold
 */
esp_err_t lis3mdl_get_interrupt_threshold(uint16_t *threshold) {
    uint8_t threshold_data[2];
    esp_err_t err;

    // Read INT_THS_L_REG and INT_THS_H_REG
    err = read_register(INT_THS_L_REG, threshold_data, 2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read interrupt threshold registers");
        return err;
    }

    // Combine the bytes
    *threshold = (uint16_t)((threshold_data[1] << 8) | threshold_data[0]);

    // Print the threshold
    ESP_LOGI(TAG, "Interrupt Threshold: %u", *threshold);

    return ESP_OK;
}

/**
 * @brief Get the status of a state in STATUS REGISTER
 * @param bit_position Position of the state to check
 * @return true if the state is set, false otherwise
 * - bit 7: X, Y, Z data overrun (new set has overwritten the previous set)
 * - bit 6: Z-axis data overrun (new set has overwritten the previous set)
 * - bit 5: Y-axis data overrun (new set has overwritten the previous set)
 * - bit 4: X-axis data overrun (new set has overwritten the previous set)
 * - bit 3: X, Y, Z new data available
 * - bit 2: Z-axis new data available
 * - bit 1: Y-axis new data available
 * - bit 0: X-axis new data available
 */
bool lis3mdl_get_status_reg(uint8_t bit_position){
    if (bit_position > 7) {
        ESP_LOGE(TAG, "Invalid bit position");
        return false; // Invalid bit position
    }

    uint8_t status;
    esp_err_t err;

    // Read STATUS_REG
    err = read_register(STATUS_REG, &status, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read STATUS_REG");
        return false; // Return false in case of error
    }

    // Check the specified bit
    bool status_condition = status & (1 << bit_position);
    return status_condition;
}

/**
 * @brief Checks if an interrupt event has occured
 */
bool lis3mdl_check_interrupt(){
    uint8_t int_src;
    esp_err_t err;

    // Read INT_SRC_REG
    err = read_register(INT_SRC_REG, &int_src, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read INT_SRC_REG");
        return false; // Return false in case of error
    }

    // Check if bit 0 is set
    bool interrupt_occurred = int_src & (1 << 0);

    ESP_LOGI(TAG, "Interrupt occurred: %s", interrupt_occurred ? "true" : "false");
    return interrupt_occurred;
}

/**
 * @brief Read the magnetic data from the sensor
 * @param x Pointer to store the magnetic data for the X axis
 * @param y Pointer to store the magnetic data for the Y axis
 * @param z Pointer to store the magnetic data for the Z axis
 * @return ESP_OK if successful
 */
esp_err_t lis3mdl_read_raw_magnetic_data(int16_t *x, int16_t *y, int16_t *z){
    uint8_t data[6];
    esp_err_t err;

    // Read OUT_X_L_REG to OUT_Z_H_REG
    err = read_register(OUT_X_L_REG, data, 6);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read magnetic data registers");
        return err;
    }

    // Combine the bytes for each axis
    *x = (int16_t)((data[1] << 8) | data[0]);
    *y = (int16_t)((data[3] << 8) | data[2]);
    *z = (int16_t)((data[5] << 8) | data[4]);

    return ESP_OK;   
}

/**
 * @brief Read the magnetic data from the sensor and return Gauss values
 * @param x Pointer to store the magnetic data for the X axis in Gauss
 * @param y Pointer to store the magnetic data for the Y axis in Gauss
 * @param z Pointer to store the magnetic data for the Z axis in Gauss
 * @return ESP_OK if successful
 */
esp_err_t lis3mdl_read_gauss_magnetic_data(float *x, float *y, float *z) {
    int16_t raw_x, raw_y, raw_z;
    esp_err_t err;

    // Read raw magnetic data
    err = lis3mdl_read_raw_magnetic_data(&raw_x, &raw_y, &raw_z);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read raw magnetic data");
        return err;
    }

    // Read the full-scale configuration
    uint8_t reg_value;
    err = read_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read CTRL_REG2");
        return err;
    }

    // Determine the LSB/gauss based on the full-scale configuration
    float lsb_per_gauss;
    switch ((reg_value) & 0x60) {
        case LIS3MDL_FS_4_GAUSS:
            lsb_per_gauss = 6842.0;
            break;
        case LIS3MDL_FS_8_GAUSS:
            lsb_per_gauss = 3421.0;
            break;
        case LIS3MDL_FS_12_GAUSS:
            lsb_per_gauss = 2281.0;
            break;
        case LIS3MDL_FS_16_GAUSS:
            lsb_per_gauss = 1711.0;
            break;
        default:
            ESP_LOGE(TAG, "Unknown full-scale configuration");
            return ESP_FAIL;
    }

    // Convert raw data to gauss - rounded to 3 decimal places
    *x = roundf((raw_x / lsb_per_gauss) * 1000) / 1000;
    *y = roundf((raw_y / lsb_per_gauss) * 1000) / 1000;
    *z = roundf((raw_z / lsb_per_gauss) * 1000) / 1000;

    return ESP_OK;
}

/**
 * @brief Read the magnetic data from the sensor and return Tesla values
 * @param x Pointer to store the magnetic data for the X axis in Tesla
 * @param y Pointer to store the magnetic data for the Y axis in Tesla
 * @param z Pointer to store the magnetic data for the Z axis in Tesla
 * @return ESP_OK if successful
 */
esp_err_t lis3mdl_read_utesla_magnetic_data(float *x, float *y, float *z){
    int16_t raw_x, raw_y, raw_z;
    esp_err_t err;

    // Read raw magnetic data
    err = lis3mdl_read_raw_magnetic_data(&raw_x, &raw_y, &raw_z);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read raw magnetic data");
        return err;
    }

    // Read the full-scale configuration
    uint8_t reg_value;
    err = read_register(CTRL_REG2, &reg_value, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read CTRL_REG2");
        return err;
    }

    // Determine the LSB/gauss based on the full-scale configuration
    float lsb_per_gauss;
    switch ((reg_value) & 0x60) {
        case LIS3MDL_FS_4_GAUSS:
            lsb_per_gauss = 6842.0;
            break;
        case LIS3MDL_FS_8_GAUSS:
            lsb_per_gauss = 3421.0;
            break;
        case LIS3MDL_FS_12_GAUSS:
            lsb_per_gauss = 2281.0;
            break;
        case LIS3MDL_FS_16_GAUSS:
            lsb_per_gauss = 1711.0;
            break;
        default:
            ESP_LOGE(TAG, "Unknown full-scale configuration");
            return ESP_FAIL;
    }

    // Convert raw data to Tesla (1 Gauss = 1e2 uTesla) - rounded to 3 decimal places
    *x = roundf((raw_x / lsb_per_gauss) * 1e2 * 1000) / 1000;
    *y = roundf((raw_y / lsb_per_gauss) * 1e2 * 1000) / 1000;
    *z = roundf((raw_z / lsb_per_gauss) * 1e2 * 1000) / 1000;

    return ESP_OK;    
}