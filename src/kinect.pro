INCLUDEPATH += ../../../libs/libfreenect/include/ ../../../libs/opencv/include/ ../../../libs/boost/ ../../../libs/cvblob/include/
INCLUDEPATH += ../../../libs/glm/ ../../../libs/glfw/include/ ../../../libs/tbb/include/
INCLUDEPATH += /usr/local/cuda/include/
INCLUDEPATH += "/Developer/GPU Computing/C/common/inc/"
LIBS += -L../../../libs/libfreenect/lib -lfreenect
LIBS += -L../../../libs/opencv/lib/darwin/ -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_calib3d
LIBS += -L../../../libs/boost/bin.v2/libs/thread/darwin/ -lboost_thread
LIBS += -L../../../libs/tbb/lib/darwin/ -ltbb
LIBS += -L../../../libs/cvblob/lib/darwin/ -lcvblob
LIBS += -L../../../libs/glfw/lib/cocoa -lglfw
LIBS += -framework OpenGL -framework Cocoa
LIBS += -L/usr/local/cuda/lib -lcudart -lnpp

QMAKE_LFLAGS_DEBUG += "-Wl,-rpath,/usr/local/cuda/lib/"
QMAKE_LFLAGS_RELEASE += "-Wl,-rpath,/usr/local/cuda/lib/"

#TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp \
    kinect.cpp \
    zsegment.cpp \
    gmm.cpp \
    colorsegment.cpp \
    seed.cpp

HEADERS += \
    kinect.h \
    zsegment.h \
    gmm.h \
    colorsegment.h \
    seed.h

OTHER_FILES += \
    vertex.vert \
    fragment.frag
