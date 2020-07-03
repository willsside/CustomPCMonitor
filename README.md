# CustomPCMonitor
Custom PC Stats Monitor and Clock with Arduino Uno and a TFT screen. 

ARDUINO:
1. Open the .ino file in the Arduino IDE and make changes if you'd like
2. Upload the program to the Arduino Uno and keep it connected. Note the COM port that the Arduino is connected to. Additionally, make sure that you do NOT have the Serial monitor open, because it will not allow the .NET app to access the COM port
3. The Arduino should say "Waiting for Time Sync..."

PC:
1. Make sure OpenHardwareMonitor is running in the background (you can minimize it to the system tray)
2. Get the Port number from the Arduino IDE (Tools > Port). Write this port number (and only the port number) in the file "INSERT_ARDIUNO_PORT_HERE.txt". Mine was on "COM4".
3. Make sure the Arduino Uno is on and the display states "Waiting for Time Sync..."
4. Run the executable "CPUTemp2Arduino.exe"

Communication between the arduino and the PC is done through the serial line. If a time update is sent from the computer, the message consists of a "T" then the unix time with no space (mine was changed slightly to match Easter Standard Time). To update the temperatures, the message consists of a "C" then the CPU temp (2 digits) and GPU temp (2 digits) with no space. The data streams from the PC and the Arduino only reads; it does not respond. This is not the best / most elegant way of handling communications but it works!
