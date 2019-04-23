# BuildFaceModel.py:
# This program buids face model based on camera pictures captured.
# Uses the lbphcascades model, as this is contour based and is
# much more effective for small data sets
# The training photos are all under sub directories of a parent directory
# Ex :               CAMERA
#                      |
#                      v
#        ---------------------------------------
#        |                 |                   |
#      Person1           Person2            Person3
#        |                 |                   |
#        v                 v                   v
#     photo1.jpg        photo1.jpg          photo1.jpg
#     photo2.jpg        photo2.jpg          photo2.jpg
#        ...              ...                 ...
#     phtooN.jpg        photoM.jpg          photoP.jpg
#
# For good results :
#     - Ensure that photos are captured with as 'big'face as possible
#     - Ensure that all phots are are frontal ... No side and poses
#     - Ensire that background is contrasting the face. For example
#       use a dark black curtain when photos are taken
#     - We find you need a minimum 20 to 30 good photos for this to work well
#
# The out model is creted as a 'face_model' loadable file for future usage.

import sys
import cv2
import os
import numpy as np

program_name = sys.argv[0]+': '

# Configuration variables
classifier_xml_file  = './lbpcascade_frontalface.xml' #Contains configuration data & contour data for how facial recognition is to be performed.
face_model_file = './face_model' # output file for facial model

# Gme oflobal data ...
subjects = [""] #directory name for the subjects. Initialized to nothing.     

def draw_rectangle(img, rect): #draws rectangle on detected face bounds.
	(x, y, w, h) = rect
	cv2.rectangle(img, (x, y), (x+w, y+h), (0, 255, 0), 2) #actual rectangle that is drawn.

def detect_face(gray_img):
	face_cascade = cv2.CascadeClassifier(classifier_xml_file) #sets up Cascade Classifier in OpenCV.
	faces = face_cascade.detectMultiScale(gray_img, scaleFactor=1.2, minNeighbors=5, minSize=(10,10)) 
	#First param passes in a gray image version of the image taken as input. 
	#scaleFactor is the scalar used to resize the image taken by 20% (0.2) smaller than what was taken (Helps algorithm match larger faces to smaller faces in model if needed). 
	#minNeighbors=5 scales the classifier to omit any erroneous faces detected in an image (higher value = more chance that bounds found includes an actual face.Lower values found `faces` in random parts of input.) 
	#minSize refers to the smallest image that is allowed to be detected as a face. Denotes 10x10 pixel image for the reduced size of the face. This is set to a low number to accomodate for the low pixel density of the camera in our system. However, we still omit faces in our classifier if they are under 100x100 pixels. 
	if (len(faces) == 0): #Given there are no faces found, return None.
		return None, None

	print program_name, 'detected a  face!!! faces[0]', faces[0] #If there are faces found, print that it has been detected & rectangle bounds.
	(x, y, w, h) = faces[0] #Assume first face is for the PersonN that we are looking for.
	return gray_img[y:y+w, x:x+h], faces[0] #Return grayscale version of the face that we have found. Not entire image, just face in rectangle bounds.


def prepare_training_data(data_folder_path):

	#Initialization of DATA_FOLDER_PATH & variables for use in function.
        dirs = os.listdir(data_folder_path)

	faces = []
	labels = []

	label = 0
	good_pictures = 0
	bad_pictures = 0

	#
	for dir_name in dirs:
		label = label + 1
	 	subjects.append(dir_name) #append directory name to subjects array. Will become name of subject.	
		subject_dir_path = data_folder_path + "/" + dir_name #initialize the subject directory path.
		subject_images_names = os.listdir(subject_dir_path) #returns list of images in directory path.
		print program_name, 'Image names ', subject_images_names

		for image_name in subject_images_names: #image_name <--- Ajay, Krishna, AnPage, etc. per person.
			image_path = subject_dir_path + "/" + image_name
			print(program_name, ' Image path ', image_path)
			image = cv2.imread(image_path, 0) #Using OpenCV to read image at imagepath.
			face, rect = detect_face(image) #Call detect face function above to detect face.
			print(program_name, 'Drawing rectangle in area !!!! rect = Press Key', rect) 
			draw_rectangle(image, rect) #Draw rectangle around face bounds.
			cv2.imshow("Training on image... press key", image)
			cv2.waitKey(0) #Make this into a sleep
 
			if face is not None: #Given that we have found a face...
				print ('program_name, Found Face')
				#Append face and label to the respective arrays.
				faces.append(face)
				labels.append(label)

				print(program_name, ' rect type', type(rect))
				print(program_name, ' rect', rect[0], rect[1], rect[2], rect[3]) #Print coordinates of the rectangle found.
				width = rect[2]
				height = rect[3]
				#Run checks to ensure that the face width/height has atleast 100x100 px.
				if (width < 100):
					print(program_name, "Width of the image is too small .. Ignoring")
					bad_pictures += 1
					continue	

				if (height < 100):
					print(program_name, "Height of the image is too small.. Ignoring")
					bad_pictures += 1
					continue
				#Increment good_pictures counter.
				good_pictures += 1
			else:
				#Increment bad_pictures counter.
				bad_pictures += 1
				print(program_name, 'No match')
	cv2.destroyAllWindows()
	cv2.waitKey(1)
	cv2.destroyAllWindows()
 
	return faces, labels, good_pictures, bad_pictures


def print_help_message():
	print' '
	print program_name, ' builds face recognition model.'
	print '	Takes following args'
	print '	', program_name, ' --help : To display this message'
	print '	', program_name, ' <directory>'
	print ' '
	print '	The frontal face captures should be under separate directories'
	print '	Under parent <directory>'
	print '	example : ~/CAMERA/Person1/, ~/CAMERA/Person2/ etc.'
	print '	where Person1 , Person2 etc are directories in which frontal'
	print '	face photographs resides for Person1, Person2 and so on.'
	print 'The output model file is ', face_model_file
	
# Main program begins
argc = len(sys.argv)
if (argc != 2):
	print_help_message()
	exit(0)

training_dir = sys.argv[1]
if(training_dir == '--help'):
	print_help_message()
	exit(0)
	
print(program_name, "Preparing data...")
faces, labels, good, bad = prepare_training_data(training_dir) #left-hand side is what is returned by prepare_training_data.
print(program_name, "Data prepared")

#print total faces and labels
print program_name, "Total faces: ", len(faces), "Good ones =", good, "bad ones = " , bad
#print(program_name, "Total labels: ", len(labels))
#print(program_name, "labels: ", labels)
print program_name, "Subjects: ", subjects

#create our LBPH face recognizer 
face_recognizer = cv2.createLBPHFaceRecognizer() #Instantiates our LBPH classifier with some predefined constants in parameter fields. Dont need to touch these for now

face_recognizer.train(faces, np.array(labels)) #Train model based of face images, map to labels
face_recognizer.save(face_model_file) #Save final model to the face_model_file saved under ./face_model
print("model saved under ./face_model")

cv2.waitKey(0)
cv2.destroyAllWindows()
