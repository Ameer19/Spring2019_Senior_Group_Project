<-------------------->
TODO:
Go through python program. Comment out all code besides the import functions to determine packages to install on new system (Jetson).
- Need: inotify, cheese, and anything else that is missing. 
- Change IP address and try to get the existing code to work with your Android phone for communication.

<-------------------->
2 programs:
//First, run Build Face Model program.
- Build Face Model - Builds face model. 
    - Building: python BuildFaceModel <directory>/    [Training_Data directory ]
	- <directory/Person1>, <directory/PersonN> <-- Contains facial images to create classifier.
	- Output: ./face_model <-- Final face model for all faces inputted.
	- Keep stepping through photos via enter key (change to Sleep for 2 seconds). 
	- Photo of face needs to be roughly 100x100 pixels.

<---------------------->
//This program should run after the Face Model program is run.
- Cheese <-- Application for taking photos.
- C2.py <-- Main program that launches our Camera and TCP Server programs. (2 seperate threads)
   - C2 execution: python C2.py <directory where Cheese stores images> <--- Pictures/Webcam.
   - After this is executed,
       - Start ESP device.
   - Either:
       - Take a photo with the Webcam OR
       - Copy a new photo directly from a source image folder over into Pictures/Webcam.
       - Either way, the inotify command will wait for the image with the latest timestamp in order to run image detection based on prexisting models from the first script that is run (Build Face Model).	   
	   
<---------------------->
- ESP32 Side of things:
    - 2 programs: hello_world.c [rename to TCP Client.c] / servo.c
	- Start Camera/Server first. Provide the address of the computer with the Camera connected to the ESP32 code. This is our 'HOST IP ADDRESS'. 
    - Messages: LOCK and UNLK are sent from TCP Server (Jetson) to TCP Client.

	
	