all: camera_test

camera_test : camera_test.cc camera_mmal.h
	g++ -Wall -I/opt/vc/include -L/opt/vc/lib/ -o $@ $< -pthread -lvcos -lbcm_host -lmmal -lmmal_core -lmmal_util -lopencv_core -lopencv_imgcodecs

clean :
	rm -f camera_test
