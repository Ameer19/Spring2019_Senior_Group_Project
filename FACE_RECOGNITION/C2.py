# camera.py : camera and acts as server.
# 	Acts both as server as well s camera input module.
#	Once the  a new photo is 'clicked', it is analyzed,
#	if matches, sends an 'OPEN' message to esp
#
#	Main program starts 2 threads.
#	    run_camera_app()
#                         Runs 'cheese' program'
#           track_camera_click()
#                        This tracks the last photograph that was clicked
#                        copies this into target directory via Camera.sh script
#
#       The following proceduere processes and sends OPEN/CLOS message to ESP32
#           process_picture()
#                        Acts as the server.
#                        This is where the captured picture is validated
#                        against model and sends OPEN/CLOSE to esp32.

import cv2
import os
import numpy as np
import socket
import glob
import sys
import tty, termios
import time
import threading

program_name = sys.argv[0]+': '
face_model_file = './face_model'
classifier_xml_file  = './lbpcascade_frontalface.xml'

# Network server Configuration variables
# You should change these according to what your network provides as your IP address !!!
server_address = ('10.0.0.231', 10034)
#server_address = ('172.20.10.3', 10034)

# Camera prediction threshold value between 0 and 100.
# Ideally we expect it to be 0. But given less no of pics 30.0 
# seems to work.
error_threshold = 30.0

def lock_door():
	duration = 1  # seconds
	freq = 200  # Hz
	os.system('play -nq -t alsa synth {} sine {}'.format(duration, freq))
	os.system("aplay -q ./locked.wav")

def unlock_door():
	duration = 1  # seconds
	freq = 10000  # Hz
	os.system('play -nq -t alsa synth {} sine {}'.format(duration, freq))
	os.system("aplay ./unlocked.wav")

#function to detect face using OpenCV
def detect_face(gray_img):
	face_cascade = cv2.CascadeClassifier(classifier_xml_file) #Understand
	faces = face_cascade.detectMultiScale(gray_img, scaleFactor=1.2, minNeighbors=5, minSize=(10,10))#me!
	if (len(faces) == 0):
		return None, None

	print program_name, ' detected a  face!!! faces[0]',  faces[0]
	(x, y, w, h) = faces[0]
	return gray_img[y:y+w, x:x+h], faces[0]

def draw_rectangle(img, rect):
	(x, y, w, h) = rect
	cv2.rectangle(img, (x, y), (x+w, y+h), (0, 255, 0), 2)
 
def draw_text(img, text, x, y):
	cv2.putText(img, text, (x, y), cv2.FONT_HERSHEY_PLAIN, 1.5, (0, 255, 0), 2)

def predict(test_img): #Main prediction. Input: test_img.
	error = 100.0 # Assume bad image to begin with
	print program_name, 'Prediction begins'
	img = test_img.copy()
	face, rect = detect_face(img)
	if face is not None:
		width = rect[2]
		height = rect[3]
		if (width < 100):
			print(program_name, "Width of the image is too small .. Ignoring")
			return error	

		if (height < 100):
			print(program_name, "Height of the image is too small.. Ignoring")
			return error	
		#If program reaches here, run image prediction. At this point, LBPH classifier is loaded and ready for predictions.
		label, error = face_recognizer.predict(face) #Matches the face to a label, assigns a error score between 0 - 100 for input face.
		print program_name, ' predict label = error = ',label, error
		draw_rectangle(img, rect)
		draw_text(img, '', rect[0], rect[1]-5)
		return error # Lower the number corresponds to a better match
	return error

def process_picture(picture):
	print program_name, 'Picture = ', picture, 
	test_img = cv2.imread(picture, 0)
	error = predict(test_img) #Calls predict function, error value is returned.
	print program_name, 'Accuracy in prediction : Confidence = ', 100.0-error,'%', 'Error = ', error, '%', 'Threshold = ', error_threshold,'%'
	if error < error_threshold: #if error is 0 - threshold, send UNLK message. Face is found by detection! 
		message =  'UNLK'
		print  program_name, ' sending UNLK to ESP',message
		sent = lock_conn.send(message, 4)
		unlock_door()
	else:                       #if error is threshold - 100, send LOCK message. Face has not been found.
		message =  'LOCK'
		print program_name, 'CLOSING the gates'
		print  program_name, ' sending LOCK to ESP',message
		sent = lock_conn.send(message, 4)
		lock_door()

def track_camera_click(): #Thread - Image Tracking in Camera Directory.
	global program_exit	
	print program_name, 'Photo Capture Track'

	inotify_cmd = 'inotifywait -q -q -t 1 -e close ' + test_dir + '/' #inode notification. given there is an inode change, notify program. Need to wait until the image is done being created. Do not start camera tracking until the image inputted has COMPLETED write and is closed. [Understand me & params]

	program_exit = 0
	while program_exit == 0:
		ret_val = os.system(inotify_cmd) #starts another thread and uses cmd in order to perform the call above.
		ret_val = ret_val >> 8 # Note ret_val is 16 bits MSB 8 is actual ret val, lsb is signal, if command was interrupted
		# [8 bits return code | 8 bits signal return code]. Need to just get the bits for the return code of the command.
		if(ret_val == 2):
			continue
		search_files = test_dir +'/*' #if this executes, we have the file created and we can start to look for the images & process them
		list_of_files = glob.glob(search_files) #List all files to search though.
		print 'Search files = ', search_files
#		print 'List of files = ' , list_of_files
		latest_file = max(list_of_files, key=os.path.getctime) #Find out the file with the highest timestamp - Latest file created. This corresponds to the latest image that was captured by user.
		print 'Latest file = ', latest_file 
		process_picture(latest_file); #Call process_picture.

	print "picture_tracking thread - Exit program_exit = ", program_exit

def run_camera_app(): # Thread - Cheese camera application.
	global program_exit
	os.system("cheese") #Call cheese via cmd. After photo is taken, and application is closed, continue & make program_exit = 1.
	program_exit = 1
	print "picture_capture thread - Exit"

def print_help_message():
	print' '
	print program_name, ' face recognition program.'
	print '	Takes following args'
	print '	', program_name, ' --help : To display this message'
	print '	', program_name, ' <photo_capture_directory>'
	print '	<photo_capture_directory> is the place where camera'
	print '	utility cheese captures the photo clicked'
	
# Main program begins
argc = len(sys.argv)
if (argc != 2):
	print_help_message()
	exit(0)

test_dir = sys.argv[1]
if(test_dir == '--help'):
	print_help_message()
	exit(0)

#Set up all global variables for capture ready

print program_name, "load our model ..."
print test_dir

#create our LBPH face recognizer  and load our model

face_recognizer = cv2.createLBPHFaceRecognizer()
face_recognizer.load(face_model_file)

#Start the server
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print program_name, 'TCP server Starting up on pn %s port %s', server_address
server_socket.bind(server_address)

#Listen to incoming (lock) connection 
server_socket.listen(1)
print program_name, 'TCP Server: Waiting for Lock to connect' 
lock_conn, lock_client_address = server_socket.accept()
print program_name, 'TCP Server: Out of Waiting for Lock connection', lock_conn, lock_client_address

print program_name, 'Now  lock and Camera Server are connected !!!'

#Launch seperate threads for run_camera_app (runs cheese) and track_camera_click (latest image)
t1 = threading.Thread(target=run_camera_app)
t2 = threading.Thread(target=track_camera_click)

t1.start()
t2.start()

#End point after threads T1 and T2 been exited
t1.join()
t2.join()

#Send Lock Signal given that the program has finished execution
message =  'LOCK'
print  program_name, ' sending LOCK to ESP',message
print program_name, 'CLOSING the gates'
sent = lock_conn.send(message, 4)
lock_door()
server_socket.close()
print 'Picture processing and server thread Exit program_exit = ', program_exit

print program_name, 'All threads exitted. Main thread exits too'
