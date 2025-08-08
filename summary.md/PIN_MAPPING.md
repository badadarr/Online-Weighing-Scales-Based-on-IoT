# Pin Mapping Documentation
## ESP32 GPIO Pin Assignments

### RFID (MFRC522) Pins
| Function | GPIO Pin | Description |
|----------|----------|-------------|
| RST      | 27       | Reset pin |
| SS       | 5        | SPI Slave Select (updated from GPIO 21) |
| SCK      | 18       | SPI Clock |
| MOSI     | 23       | SPI Master Out Slave In |
| MISO     | 19       | SPI Master In Slave Out |
| ~~IRQ~~  | ~~33~~   | ~~Interrupt (not used, GPIO 33 for UP button)~~ |

### HX711 Load Cell Pins
| Function | GPIO Pin | Description |
|----------|----------|-------------|
| DATA     | 25       | Data pin |
| CLOCK    | 26       | Clock pin |

### Button Pins
| Function | GPIO Pin | Description |
|----------|----------|-------------|
| TARE     | 32       | Tare button |
| UP       | 33       | Up button (was RFID IRQ) |
| DOWN     | 34       | Down button |
| ENTER    | 35       | Enter button |

### RGB LED Pins (Common Cathode)
| Function | GPIO Pin | Description |
|----------|----------|-------------|
| RED      | 16       | Red LED (RX2) |
| GREEN    | 4        | Green LED |
| BLUE     | 2        | Blue LED |

### Other Peripherals
| Function | GPIO Pin | Description |
|----------|----------|-------------|
| BUZZER   | 17       | Buzzer (TX2) |
| LCD SDA  | 21       | I2C Data |
| LCD SCL  | 22       | I2C Clock |

### Pin Conflicts Resolved
- **GPIO 33**: Changed from RFID_IRQ_PIN to UP_BUTTON_PIN
- **GPIO 16 & 17**: Used for LED and Buzzer (Serial2 disabled)
- **GPIO 5**: RFID SS pin (changed from GPIO 21)

### System Settings
- **EEPROM Size**: 512 bytes
- **LCD I2C Address**: 0x27
- **Weight Samples**: 10
- **Calibration Factor**: -94659.69f

### Notes
- Serial2 (GPIO 16/17) is disabled due to LED/Buzzer usage
- RFID IRQ functionality removed to free GPIO 33 for button
- All pins tested for conflicts and functionality