.SUFFIXES : .cpp .o

CC = gcc
LD = gcc
INCDIR = gimicUsbSpcPlay -Isnes_spc
CFLAGS = -I$(INCDIR) -O3
LFLAGS = -lm
LIBS =  -lstdc++ -lusb-1.0
PROGRAM  = gmcSpcPlay

IFILES = gimicUsbSpcPlay/BulkUsbDevice.h \
				 gimicUsbSpcPlay/FileAccess.h \
				 gimicUsbSpcPlay/SpcControlDevice.h \
				 gimicUsbSpcPlay/SPCFile.h
				 
CFILES = gimicUsbSpcPlay/main.cpp \
				 gimicUsbSpcPlay/BulkUsbDevice.cpp \
				 gimicUsbSpcPlay/FileAccess.cpp \
				 gimicUsbSpcPlay/SpcControlDevice.cpp \
				 gimicUsbSpcPlay/SPCFile.cpp \
				 gimicUsbSpcPlay/DspRegFIFO.cpp \
				 snes_spc/dsp.cpp \
				 snes_spc/SNES_SPC.cpp \
				 snes_spc/SNES_SPC_misc.cpp \
				 snes_spc/SNES_SPC_state.cpp \
				 snes_spc/spc.cpp \
				 snes_spc/SPC_DSP.cpp \
				 snes_spc/SPC_Filter.cpp

OFILES = $(CFILES:.cpp=.o)

all: $(PROGRAM)

$(PROGRAM):		$(OFILES)
				$(LD) $(LFLAGS) $? -o $(PROGRAM) $(LIBS)

$(OFILES):	$(CFILES) $(IFILES) Makefile

.cpp.o : $(CFILES) $(IFILES)
	$(CC) $(CFLAGS) -c -o $*.o $*.cpp

clean:
	rm -f $(OFILES) $(PROGRAM)
