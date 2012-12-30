bin_PROGRAMS = papersegment

papersegment_SOURCES = \
	main.cpp \
	colorsegment.cpp \
	gmm.cpp \
	kinect.cpp \
	seed.cpp \
	zsegment.cpp

noinst_HEADERS = \
	colorsegment.h \
	gmm.h \
	kinect.h \
	seed.h \
	zsegment.h

papersegment_CXXFLAGS = \
	-std=c++11 \
	$(BOOST_CPPFLAGS) \
	$(CUDA_CFLAGS) \
	-I/usr/local/cuda/samples/common/inc \
	$(CVBLOB_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLFW_CPPFLAGS) \
	$(FREENECT_CPPFLAGS) \
	$(OPENCV_CFLAGS)

papersegment_LDADD = \
	$(BOOST_THREAD_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(CUDA_LIBS) \
	-L/usr/local/cuda/lib64 -lcudart \
	-lnpp \
	$(CVBLOB_LIBS) \
	$(GL_LIBS) \
	$(GLFW_LIBS) \
	$(FREENECT_LIBS) \
	$(OPENCV_LIBS)
