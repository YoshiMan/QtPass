!include(../auto.pri) { error("Couldn't find the auto.pri file!") }

SOURCES += tst_util.cpp \
           util.cpp \
           qtpasssettings.cpp \
           settingsconstants.cpp \
           pass.cpp \
           realpass.cpp \
           imitatepass.cpp \
           executor.cpp \
           simpletransaction.cpp

HEADERS   += util.h \
             qtpasssettings.h \
             settingsconstants.h \
             pass.h \
             realpass.h \
             imitatepass.h \
             executor.h \
             simpletransaction.h

VPATH += ../../../src
INCLUDEPATH += ../../../src

win32 {
    LIBS += -lbcrypt
}
