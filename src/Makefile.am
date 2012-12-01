bin_PROGRAMS = papersegment

papersegment = \
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
	-std=gnu++0x \
	$(BOOST_CPPFLAGS) \
	$(FREENECT_CPPFLAGS) \
	$(OPENCV_CFLAGS)

papersegment_LDADD = \
	$(BOOST_THREADS_LIBS) \
	$(FREENECT_LIBS) \
	$(OPENCV_LIBS)
