## FEATURE: OLED Basic info

Add OLED info display. The display should show the start-up progress, indicating status for different subsystems, including:
- wifi, including ip adress
- filesystem/eeprom 
- webserver

More subsystems will be added later, but for now only report on the ones that are implemented. 

The info display should also show size of uploaded code, i.e. using ESP.getSketchSize(), and it should show free program space, i.e. using ESP.getFreeSketchSpace().

Also, once startup is completed, give indication that the system is running and alive. Maybe a visual indication that the loop code is running, and that the ESP32 is not stuck somewhere. 

Also, once started, continue to indicate the wifi status. It may drop, and reconnect occasionally, and that should be informed on the OLED display. 

Also, the display should give an indication of free RAM, i.e. using ESP.getFreeHeap() or similar. 

Finally some indication of MCU utilization, maybe an indication of number of loop iterations per second, or better metrics if you can think of some. 


In a future feature enhancement, the OLED display should be able to show multiple 'pages' of information, allowing the user to cycle through the pages using the Button attached to GPIO 13. Make sure to take appropriate consideration to filter out button noise, i.e. some debounce handling required. 
The first page is the only one the we are concerned about in this feature, but the code should be prepared for additional pages in a later development iteration., 


The implementation should:
- Avoid over-abstraction and not create unnecessary layers and wrapper functions
- Avoid library bloat, and not includefull libraries when only small functionality is needed
- Make efficient use of data types, e.g. using `uint8_t` or `uint16_t` instead of  `int`  when possible. 
- Make use of PROGMEM directives to store constants in flash memory rather than RAM. 
- Leverage ESP32-specific optimizations



There is an example of using the OLED display in the file examples/poseidongw/src/main.cpp. This can be used for reference and inspiration, but do not just copy. 