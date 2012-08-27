# MacOSX specific include and lib files
macx {
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
}

# Linux specific include and lib files
unix {
INCLUDEPATH += /usr/local/include/libfreenect
INCLUDEPATH += /usr/local/cuda/include
INCLUDEPATH += /home/manu/src/_externals/NVIDIA_GPU_Computing_SDK/CUDALibraries/common/inc
INCLUDEPATH += /opt/intel/tbb/include
LIBS += -lfreenect
LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_calib3d
LIBS += -lboost_thread
LIBS += -L/opt/intel/tbb/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21 -ltbb
LIBS += -lcvblob
LIBS += -lglfw -lGL
LIBS += -L/usr/local/cuda/lib64 -lcudart -lnpp
}

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
