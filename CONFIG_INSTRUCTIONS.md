# Battery Monitoring Configuration Instructions

This document explains how the battery monitoring system works with both ADC and INA219 approaches.

## Configuration Options

The system supports two battery monitoring approaches:

1. **ADC-based monitoring** (`CONFIG_LOCKBOX_FEATURE_BATTERY_MONITOR=y`)
   - Uses GPIO 6 with a 1:2 voltage divider circuit
   - Measures voltage level only
   - Simple but less accurate for capacity monitoring

2. **INA219-based monitoring** (`CONFIG_LOCKBOX_FEATURE_INA219=y`)
   - Uses INA219 I2C chip for power monitoring
   - Provides accurate capacity monitoring (High/Medium/Low levels)
   - Measures voltage, current, and energy consumption
   - Calculates state of charge (SOC) percentage

## Implementation Logic

### When `CONFIG_LOCKBOX_FEATURE_INA219=y`:
- The INA219 service is initialized and used for all battery status display
- The UI shows battery level as High/Medium/Low with corresponding colors:
  - **High (Green)**: >= 60% SOC
  - **Medium (Yellow)**: 25-60% SOC  
  - **Low (Red)**: < 25% SOC
- ADC-based monitoring is disabled to avoid conflicts

### When `CONFIG_LOCKBOX_FEATURE_BATTERY_MONITOR=y` AND `CONFIG_LOCKBOX_FEATURE_INA219=n`:
- ADC-based monitoring is used for voltage level display
- UI shows battery level as Green/Yellow/Red based on voltage thresholds:
  - **Green**: >= 5.0V
  - **Yellow**: 3.7V - 5.0V
  - **Red**: <= 3.0V

### When Both are Enabled:
- The INA219 approach takes precedence and is used for all battery monitoring
- ADC monitoring is disabled to prevent conflicts
- This ensures consistent behavior with the INA219 approach

## Hardware Requirements

### For ADC-based Monitoring:
- Voltage divider circuit (1:2 ratio) connected to GPIO 6
- Requires 2.5V maximum input voltage

### For INA219-based Monitoring:
- INA219 chip connected via I2C
- Requires proper power supply and pull-up resistors
- Connects to powerbank output to monitor energy consumption

## Usage Notes

1. Only one approach should be enabled at a time for consistency
2. The INA219 approach is recommended for accurate capacity monitoring
3. When using INA219, the system will automatically provide High/Medium/Low status indicators
4. The ADC approach is maintained as a fallback mechanism