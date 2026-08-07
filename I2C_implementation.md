Your GitHub Copilot is correct about why the error happens, but its proposed solution—trying to call i2c_driver_install multiple times and just "tolerating" the failure—is an anti-pattern in ESP-IDF development.

In ESP-IDF, the I2C driver is global to the microcontroller port (I2C_NUM_0). It should only be initialized exactly once during system startup, before any individual device driver (DS3231, INA219, etc.) attempts to use it.

Here is exactly why your set_date is failing and how to structure your code properly to fix it.

Why set_date is Failing

When Copilot tells you that the system "tolerates" the error, it forgets that your ds3231_rtc.c code is likely also calling i2c_param_config().

If i2c_param_config() is executed on an already active, running I2C port, it can disrupt the timing generator, scramble the internal state of the ESP32-S3 I2C hardware, and corrupt ongoing communications. This is why your subsequent commands like set_date fail.

The Correct Architecture: "Shared Bus" Pattern

Instead of letting each device file (ds3231_rtc.c, ina219.c, etc.) try to initialize the whole hardware port, you must separate Bus Initialization from Device Initialization.

[ app_main.c ] ───> 1. Initialise I2C_NUM_0 Hardware (Once)

                           │

                           ├───> 2. Init DS3231 Driver Instance (Using I2C_NUM_0)

                           ├───> 3. Init INA219 Driver Instance (Using I2C_NUM_0)

                           └───> 4. Init SC16IS752 Driver Instance (Using I2C_NUM_0)

How to Refactor Your Code

1. Create a Centralized I2C Init Function

Move i2c_param_config and i2c_driver_install out of your individual device source files completely. Put them into your main.c (or a dedicated i2c_bus.c).

// Run this ONCE in app_main() before anything else

esp_err_t init_shared_i2c_bus(void) {

    i2c_config_t conf = {

        .mode = I2C_MODE_MASTER,

        .sda_io_num = GPIO_NUM_4,     // Your chosen SDA pin

        .scl_io_num = GPIO_NUM_5,     // Your chosen SCL pin

        .sda_pullup_en = GPIO_PULLUP_ENABLE,

        .scl_pullup_en = GPIO_PULLUP_ENABLE,

        .master.clk_speed = 100000,   // 100kHz standard mode

    };



    esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);

    if (err != ESP_OK) return err;



    return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

}

2. Modify ds3231_rtc.c and Other Drivers

Strip the driver installation code out of ds3231_rtc.c. Instead, your device initialization functions should just accept the pre-configured port number and the device's specific address.

// Inside ds3231_rtc.c

esp_err_t ds3231_init(i2c_port_t port, uint8_t dev_addr) {

    // DO NOT call i2c_driver_install here!

    // Just verify communication by reading the RTC status register

    uint8_t reg = 0x0F; // Status register

    uint8_t data = 0;



    esp_err_t err = i2c_master_write_read_device(port, dev_addr, &reg, 1, &data, 1, pdMS_TO_TICKS(100));

    return err;

}

3. Update Your app_main Order of Operations

In your primary application entry point, explicitly dictate the order of operations so there is zero ambiguity:

void app_main(void) {

    // Step 1: Initialize the physical bus hardware

    ESP_ERROR_CHECK(init_shared_i2c_bus());



    // Step 2: Initialize the device drivers using the active bus

    if (ds3231_init(I2C_NUM_0, 0x68) == ESP_OK) {

        ESP_LOGI("MAIN", "DS3231 RTC Initialized.");

        // Now you can safely call set_date!

    }



    ina219_init(I2C_NUM_0, 0x41);

    sc16is752_init(I2C_NUM_0, 0x4D);

    pca9685_init(I2C_NUM_0, 0x40);

}

Summary Checklist for Copilot

Tell your Copilot the following constraints to get better code generation:

    "We are treating I2C_NUM_0 as a centralized, shared bus."
    "Remove all i2c_driver_install and i2c_param_config calls from individual device component files."
    "Provide helper functions for the devices that take i2c_port_t port as an argument."
- 