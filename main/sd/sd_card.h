#ifndef _SD_CARD_
#define _SD_CARD_

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/i2c.h"

// I2C configuration
//#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL /*!< GPIO number used for I2C master clock */
//#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_SCL_IO 9                     /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO 8                    /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM 0                        /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ 400000               /*!< I2C master clock frequency      */
#define I2C_MASTER_TX_BUF_DISABLE 0             /*!< I2C master doesn't need buffer  */
#define I2C_MASTER_RX_BUF_DISABLE 0             /*!< I2C master doesn't need buffer  */
#define I2C_MASTER_TIMEOUT_MS 1000              /*!< I2C master timeout in milliseconds */

// Maximum character size for file operations
#define EXAMPLE_MAX_CHAR_SIZE 64

// Mount point for the SD card
#define MOUNT_POINT "/sdcard"

// Pin assignments for SD SPI interface
// #define PIN_NUM_MISO CONFIG_EXAMPLE_PIN_MISO /*!< Pin number for MISO    */
// #define PIN_NUM_MOSI CONFIG_EXAMPLE_PIN_MOSI /*!< Pin number for MOSI   */
// #define PIN_NUM_CLK CONFIG_EXAMPLE_PIN_CLK   /*!< Pin number for CLK    */
// #define PIN_NUM_CS CONFIG_EXAMPLE_PIN_CS     /*!< Pin number for CS CS  */
#define PIN_NUM_MISO 13                       /*!< Pin number for MISO    */
#define PIN_NUM_MOSI 11                       /*!< Pin number for MOSI   */
#define PIN_NUM_CLK 12                        /*!< Pin number for CLK    */
#define PIN_NUM_CS -1                         /*!< Pin number for CS CS  */

// Function prototypes for initializing and testing SD card functions
esp_err_t waveshare_sd_card_init();
esp_err_t waveshare_sd_card_test();
esp_err_t waveshare_sd_card_info();
esp_err_t waveshare_sd_card_list_files();
sdmmc_card_t* waveshare_sd_card_get_card();

#endif
