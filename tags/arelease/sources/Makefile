mp3val.exe: main.obj mpegparse.obj out.obj report.obj
	cl /O1 /MD main.obj mpegparse.obj out.obj report.obj -o mp3val.exe
main.obj: main.cpp
	cl /O1 /MD /c main.cpp
mpegparse.obj: mpegparse.cpp
	cl /O1 /MD /c mpegparse.cpp
out.obj: out.cpp
	cl /O1 /MD /c out.cpp
report.obj: report.cpp
	cl /O1 /MD /c report.cpp
clean:
	del *.o
	del *.exe
	del *.obj
