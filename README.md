# cphw-esp32

This is an ESP32 app to unlock the Copenhagen Wheel. It uses BLE to connect to the wheel, send the passcode and unlocks the wheel with it. After this the ESP32 controller can be switched off. 

The app uses a hard wired passcode. Before using it, the app must be run once to find the wheel identification number. With this, the wheel passcode can be found in passcode.cvs. 

See the wiki for more information: https://github.com/slviajero/cphw-esp32/wiki
