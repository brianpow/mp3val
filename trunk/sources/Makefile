mp3val.exe: main.obj mpegparse.obj out.obj report.obj crossapi.obj crc.obj
	cl /O1 /GX /MD main.obj mpegparse.obj out.obj report.obj crossapi.obj crc.obj user32.lib -o mp3val.exe
main.obj: main.cpp
	cl /O1 /GX /MD /c main.cpp
mpegparse.obj: mpegparse.cpp
	cl /O1 /GX /MD /c mpegparse.cpp
out.obj: out.cpp
	cl /O1 /GX /MD /c out.cpp
report.obj: report.cpp
	cl /O1 /GX /MD /c report.cpp
crossapi.obj: crossapi.cpp
	cl /O1 /GX /MD /c crossapi.cpp
crc.obj: crc.cpp crc.h
	cl /O1 /GX /MD /c crc.cpp
clean:
	del *.o
	del *.exe
	del *.obj
