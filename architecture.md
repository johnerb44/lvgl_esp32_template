 ### ARCHITECTURE.md

 ```markdown
   # Architecture Overview for Secure Lockbox

   ## Introduction
   This document provides an overview of the hardware and software architecture for the Secure Lockbox project. The lockbox allows access via facial recognition or fingerprint scanning and requires a PIN code to unlock.

   ## Hardware Architecture

   ### Components
   1. **Main Controller**
      - **Model**: Waveshare ESP32-S3-Touch-LCD-4.3 (800x480 touch LCD, ESP32-S3)
      - **Role**: Central processing unit for the lockbox.

   2. **Facial Recognition Module**
      - **Model**: HLK-510
      - **Role**: Captures and processes facial data for authentication.

   3. **Fingerprint Module**
      - **Model**: R503
      - **Role**: Captures and processes fingerprint data for authentication.

   4. **Custom Interface Module**
      - **Role**: Interfaces with the main controller to handle various input/output operations.

   5. **Servo-Based Locking Mechanism**
      - **Role**: Controls the physical locking mechanism based on authentication results.

   ### Diagram
   ```plaintext
   +---------------------+
   | Main Controller     |
   | (ESP32-S3)          |
   |                     |
   | +-----------------+ |
   | | Touch LCD       | |
   | | (800x480)       | |
   | +-----------------+ |
   | +-----------------+ |
   | | UART            | |
   | +-----------------+ |
   | +-----------------+ |
   | | I2C             | |
   | +-----------------+ |
   +---------+-----------+
             |
             v
   +---------+-----------+
   | Facial Recognition|
   | Module (HLK-510)    |
   |                     |
   | +-----------------+ |
   | | Camera          | |
   | +-----------------+ |
   | +-----------------+ |
   | | UART            | |
   +---------+-----------+
             |
             v
   +---------+-----------+
   | Fingerprint Module  |
   | (R503)              |
   |                     |
   | +-----------------+ |
   | | Sensor          | |
   | +-----------------+ |
   | +-----------------+ |
   | | UART            | |
   +---------+-----------+
             |
             v
   +---------+-----------+
   | Custom Interface    |
   | Module                |
   |                     |
   | +-----------------+ |
   | | GPIO            | |
   | +-----------------+ |
   | +-----------------+ |
   | | SPI             | |
   +---------+-----------+
             |
             v
   +---------+-----------+
   | Servo-Based Locking |
   | Mechanism           |
   |                     |
   | +-----------------+ |
   | | PWM             | |
   +---------------------+
 ```

 Software Architecture

 ### Components

 1. Main Application
     - Language: C
     - Role: Manages the overall system operations, including authentication and GUI handling.
 2. LVGL Library
     - Version: Latest stable release
     - Role: Provides the graphical user interface for interaction with the lockbox.
 3. Facial Recognition Library
     - Library: Custom or third-party library (e.g., OpenCV)
     - Role: Handles facial recognition tasks.
 4. Fingerprint Recognition Library
     - Library: Custom or third-party library
     - Role: Handles fingerprint recognition tasks.
 5. PIN Authentication Module
     - Language: C
     - Role: Verifies the PIN code entered by the user.

 ### Diagram

 ```plaintext
   +---------------------+
   | Main Application    |
   | (C)                 |
   |                     |
   | +-----------------+ |
   | | GUI             | | <-- LVGL Library
   | +-----------------+ |
   | +-----------------+ |
   | | Authentication  | |
   | | - Facial        | | <-- Facial Recognition Library
   | | - Fingerprint   | | <-- Fingerprint Recognition Library
   | | - PIN           | | <-- PIN Authentication Module
   | +-----------------+ |
   +---------+-----------+
             |
             v
   +---------+-----------+
   | LVGL Library        |
   | (GUI)               |
   |                     |
   | +-----------------+ |
   | | Display         | |
   +---------------------+

   +---------+-----------+
   | Facial Recognition|
   | Library             |
   |                     |
   | +-----------------+ |
   | | Capture         | |
   | | Process         | |
   | | Compare         | |
   +---------------------+

   +---------+-----------+
   | Fingerprint       |
   | Recognition       |
   | Library           |
   |                     |
   | +-----------------+ |
   | | Capture         | |
   | | Process         | |
   | | Compare         | |
   +---------------------+

   +---------+-----------+
   | PIN Authentication|
   | Module            |
   |                     |
   | +-----------------+ |
   | | Verify          | |
   +---------------------+
 ```

 ### Interfaces

 1. UART Interface
     - Used for communication between the ESP32-S3 and both the facial recognition module and the fingerprint module.
 2. GPIO Interface
     - Used for communication between the custom interface module and the ESP32-S3.
 3. PWM Interface
     - Used to control the servo-based locking mechanism.

 ### Data Flow

 1. Authentication Process:
     - User triggers authentication via facial recognition or fingerprint scanning.
     - Facial recognition module captures and processes data, sending results to the main application.
     - Fingerprint module captures and processes data, sending results to the main application.
     - Main application verifies authentication and prompts for PIN if necessary.
 2. GUI Interaction:
     - User interacts with the touch LCD to enter the PIN code.
     - LVGL library handles user input and updates the display accordingly.

 ### Security Considerations

 - Secure storage of facial recognition data and fingerprint templates.
 - Robust encryption for PIN codes.
 - Regular security audits and testing.
