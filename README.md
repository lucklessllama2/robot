How to run

* Pull the latest version to dog and to a personal computer.  
* Update the IP address in Server/ServerData.json to the Raspberry Pi's IP address.  
* On a personal computer, run MainControlStuff/xboxInput.py.  
* SSH into the dog and run MainControlStuff/MainLoop.py.


Currently working on

* Center of Mass calculation based on motor positions.   
* Refinement of the current class setups for storing positioning   
* Convert to using gaits for walking and other animations  


Next Steps  

* Use Imu for balance calculations.  
* Add a way to detect current battery levels for safe shutdowns when batteries are too low.  
* Add an Extended Kalman Filter to IMU data (EKF).

