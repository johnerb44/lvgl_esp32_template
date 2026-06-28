Here's a clean, platform-agnostic C implementation. I've abstracted the underlying I2C calls 
with clear comments so you can easily map them to your specific environment (Linux `i2c-dev`, 
Arduino `Wire`, STM32 HAL, etc.).

Reads the SC16IS752's GP0 pin but can be modified to read other SC16IS752 GPx pins.

```c
#include <stdint.h>
#include <stdio.h>

/*
 * PLATFORM-SPECIFIC I2C STUBS
 * Replace these with your actual I2C library calls.
 * They must support the standard I2C register access sequence:
 * I2C_START -> I2C_WRITE(dev_addr) -> I2C_WRITE(reg_addr) -> I2C_STOP
 * I2C_START -> I2C_WRITE(dev_addr) -> I2C_READ(data) -> I2C_STOP
 */
static int i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    // e.g., Linux: i2c_smbus_write_byte_data(fd, reg_addr, data)
    return 0; // 0 = success
}

static int i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data) {
    // e.g., Linux: i2c_smbus_read_byte_data(fd, reg_addr)
    // Note: Most I2C libraries handle the repeated-start automatically.
    *data = 0xFF; // Dummy placeholder
    return 0;     // 0 = success
}

/**
 * Reads the digital state of the SC16IS752's GP0 pin.
 * @return 1 if HIGH, 0 if LOW, or negative on I2C error.
 */
int read_gp0_state(void) {
    uint8_t gpio_data = 0;
    int ret;

    // 1. Switch to Page 2 (GP0-GP5 GPIO control page)
    // Register 0x07 on Page 0 is the Page Register.
    // Writing 0x02 selects Page 2.
    ret = i2c_write_reg(0x4D, 0x07, 0x02);
    if (ret != 0) return -1; // I2C write failed

    // 2. Read the GP0-GP5 Data Register (Page 2, Register 0x01)
    ret = i2c_read_reg(0x4D, 0x01, &gpio_data);
    if (ret != 0) return -2; // I2C read failed

    // 3. Extract GP0 state (Bit 0)
    // GP0 = Bit 0, GP1 = Bit 1, ..., GP5 = Bit 5
    if (gpio_data & 0x01) {
        return 1; // HIGH
    } else {
        return 0; // LOW
    }
}

/* Example usage */
int main(void) {
    // I2C is assumed to be initialized elsewhere
    
    int state = read_gp0_state();
    
    if (state == 1) {
        printf("GP0 is HIGH\n");
    } else if (state == 0) {
        printf("GP0 is LOW\n");
    } else {
        fprintf(stderr, "I2C error reading GP0!\n");
    }
    
    return 0;
}
```

### 🔑 Key Implementation Notes:
1. **Register Mapping**: 
   - `0x07` (Page 0) → Page Register. Writing `0x02` switches to Page 2.
   - `0x01` (Page 2) → `GP0-GP5 DATA` register. Bit 0 directly reflects GP0's input state.

2. **Direction Configuration**: By default, GP0 is configured as an input after reset. If 
you've previously configured it as an output, you must set Bit 0 of the **Direction Register** 
(`Page 2, Reg 0x00`) to `0` before reading.

3. **I2C Address**: `0x4D` is the **7-bit** device address (ADDR pin tied HIGH). Most modern 
I2C libraries automatically handle the R/W bit and don't require shifting.

4. **Voltage Levels**: Ensure your GPIO signal matches the SC16IS752's supply voltage (`VCC`, 
typically 3.3V or 5V). The chip is not 5V-tolerant on all pins if powered at 3.3V.

5. **Alternate Functions**: GP0 can also serve as an alternative UART function (e.g., RTS/CTS). 
If your application uses UART features mapped to GP0, the GPIO read may reflect the internal 
UART state rather than the external pin. Ensure the UART alternate function is disabled if 
you're using GP0 as a pure GPIO input.

You can directly paste the stub functions and map them to your platform's I2C API. The core 
logic remains identical across microcontrollers and host systems.

