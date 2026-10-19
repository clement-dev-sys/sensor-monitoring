#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "bme280.h"
#include "i2c_bus.h"

#include "sensor.h"

#define I2C_PORT  I2C_NUM_0
#define I2C_SDA_GPIO  GPIO_NUM_21
#define I2C_SCL_GPIO  GPIO_NUM_22
#define I2C_FREQ_HZ  100000

static i2c_bus_handle_t s_i2c_bus = NULL;
static bme280_handle_t s_bme280 = NULL;

static void sensor_release(void)
{
    if (s_bme280 != NULL) {
        bme280_delete(&s_bme280);
    }
    if (s_i2c_bus != NULL) {
        i2c_bus_delete(&s_i2c_bus);
    }
}

esp_err_t sensor_init(void)
{
    sensor_release();

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SCL_GPIO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    s_i2c_bus = i2c_bus_create(I2C_PORT, &i2c_conf);
    if (s_i2c_bus == NULL) {
        ESP_LOGE("SENSOR", "Erreur i2c_bus_create");
        return ESP_FAIL;
    }

    s_bme280 = bme280_create(s_i2c_bus, BME280_I2C_ADDRESS_DEFAULT);
    if (s_bme280 == NULL) {
        ESP_LOGE("SENSOR", "Erreur bme280_create");
        i2c_bus_delete(&s_i2c_bus);
        return ESP_FAIL;
    }

    esp_err_t err = bme280_default_init(s_bme280);
    if (err != ESP_OK) {
        ESP_LOGE("SENSOR", "Erreur bme280_default_init: %s", esp_err_to_name(err));
        sensor_release();
        return err;
    }

    ESP_LOGI("SENSOR", "BME280 configuré");
    return ESP_OK;
}

bool sensor_ready(void)
{
    return s_bme280 != NULL;
}

esp_err_t sensor_read(sensor_reading_t *out)
{
    if (s_bme280 == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = bme280_read_temperature(s_bme280, &out->temperature);
    if (err != ESP_OK) {
        return err;
    }

    err = bme280_read_humidity(s_bme280, &out->humidity);
    if (err != ESP_OK) {
        return err;
    }

    err = bme280_read_pressure(s_bme280, &out->pressure);
    if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}
