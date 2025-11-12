This project establishes a CANopen communication link between a Staubli CS8C robot controller and an Arduino-based CANopen node.

The communication is handled through a Molex CANopen interface card installed in the CS8C, connected to the Arduino CAN shield with a custom-made CAN cable that links the Molex card’s CANopen port to the Arduino’s CAN interface.

The Arduino is equipped with an Open Smart Rich Shield, which provides a DHT11 temperature sensor for data acquisition, and two LED outputs used for visual feedback.

The system implements the CANopen protocol as follows:
- The Arduino transmits temperature data to the CS8C controller using a TPDO.
- It receives LED control commands from the CS8C via an RPDO.
- A custom EDS (Arduino_UNO.eds) defines the Arduino’s CANopen communication structure, object dictionary, and PDO mappings.

The configuration and monitoring of the CANopen communication are carried out using ApplicomIO software, which allows the user to import the EDS file, configure PDO mappings, and test real-time data exchange between the CS8C and Arduino.
