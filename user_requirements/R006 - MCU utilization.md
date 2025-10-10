## Feature: MCU Utilization

Based on the principles in the code snippet below, I would like the CPU Idle, currently static at 85%, to be changed to show the average loop frequency over a 5 second interval, and I'd like this to be updated every 5 seconds. I'd like the CPU line on the OLED display to be replaces with a line looking like the following example:  "MCU Loop Frequency:    212 Hz" 

The code snippet to use as inspiration for calculating the loop frequency is as follows, but keep in mind, I do not want any Serial printing. Only report the 5 second avarage loop frequency on the OLED display. 

class SimplePerformanceMonitor {
private:
    uint32_t loop_count = 0;
    uint32_t max_loop_time = 0;
    uint32_t total_loop_time = 0;
    uint32_t last_report = 0;
    uint32_t loop_start_time = 0;

public:
    void startLoop() {
        loop_start_time = micros();
    }
    
    void endLoop() {
        uint32_t loop_time = micros() - loop_start_time;
        loop_count++;
        total_loop_time += loop_time;
        
        if (loop_time > max_loop_time) {
            max_loop_time = loop_time;
        }
        
        // Report every 5 seconds
        if (millis() - last_report >= 5000) {
            printReport();
            resetCounters();
        }
    }
    
    void printReport() {
        Serial.println("=== Performance Report (5s window) ===");
        Serial.printf("Loop iterations: %lu\n", loop_count);
        Serial.printf("Average loop time: %lu µs\n", total_loop_time / loop_count);
        Serial.printf("Max loop time: %lu µs\n", max_loop_time);
        Serial.printf("Loop frequency: %.1f Hz\n", loop_count / 5.0);
        
        // Warning indicators
        if (max_loop_time > 10000) {  // > 10ms
            Serial.println("WARNING: Long loop detected!");
        }
        if (loop_count < 1000) {  // < 200 Hz
            Serial.println("WARNING: Low loop frequency!");
        }
    }
    
private:
    void resetCounters() {
        loop_count = 0;
        max_loop_time = 0;
        total_loop_time = 0;
        last_report = millis();
    }
};
