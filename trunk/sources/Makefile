mp3val.exe: main.obj mpegparse.obj out.obj report.obj crossapi.obj
	cl /O1 /GX /MD /DWIN32 main.obj mpegparse.obj out.obj report.obj crossapi.obj -o mp3val.exe
main.obj: main.cpp
	cl /O1 /GX /MD /c /DWIN32 main.cpp
mpegparse.obj: mpegparse.cpp
	cl /O1 /GX /MD /c /DWIN32 mpegparse.cpp
out.obj: out.cpp
	cl /O1 /GX /MD /c /DWIN32 out.cpp
report.obj: report.cpp
	cl /O1 /GX /MD /c /DWIN32 report.cpp
crossapi.obj: crossapi.cpp
	cl /O1 /GX /MD /c /DWIN32 crossapi.cpp
clean:
	del *.o
	del *.exe
	del *.obj
