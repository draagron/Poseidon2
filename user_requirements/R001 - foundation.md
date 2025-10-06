## FEATURE: foundation
This is the foundation for the next generation of the original Poseidon Gateway, included for reference in the examples/poseidon folder.

With this foundation feature implemented the Poseidon Gateway will logon to a predefined known wifi network. The predefined wifi networks are defined in a separate file, and the Posedtion Gateway will try to connect to the first one listed. If that connection fails, it will proceeed and try the next, and so forth. If none of the pre-defined wifi networks connects, it will reboot and repeat the process. 

Ideally the list of wifi networks, with SSID and Paswords, will be stored in an editable file and then uploaded to EEPROM on the ESP32. 

## OTHER CONSIDERATIONS:

- Include README with instructions for setup.
- Include the project structure in the README.





