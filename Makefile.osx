.SUFFIXES : .cpp .o

CC = gcc
LD = gcc
INCDIR = gimicUsbSpcPlay
CFLAGS = -I/usr/local/include/libusb-1.0 -I$(INCDIR) -O3
LFLAGS = -lm -L/usr/local/lib -lstdc++ -lusb-1.0 -Wl,-framework,IOKit -Wl,-framework,Foundation
PROGRAM  = gmcSpcPlay

IFILES = gimicUsbSpcPlay/BulkUsbDevice.h \
				 gimicUsbSpcPlay/FileAccess.h \
				 gimicUsbSpcPlay/SpcControlDevice.h \
				 gimicUsbSpcPlay/SPCFile.h
				 
CFILES = gimicUsbSpcPlay/main.cpp \
				 gimicUsbSpcPlay/BulkUsbDevice.cpp \
				 gimicUsbSpcPlay/FileAccess.cpp \
				 gimicUsbSpcPlay/SpcControlDevice.cpp \
				 gimicUsbSpcPlay/SPCFile.cpp
				 
OFILES = $(CFILES:.cpp=.o)

all: $(PROGRAM)

$(PROGRAM):		$(OFILES)
				$(LD) $(LFLAGS) $? -o $(PROGRAM)

$(OFILES):	$(CFILES) $(IFILES) Makefile

.cpp.o : $(CFILES) $(IFILES)
	$(CC) $(CFLAGS) -c -o $*.o $*.cpp

clean:
	rm -f $(OFILES) $(PROGRAM)
