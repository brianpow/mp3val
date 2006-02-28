#include <windows.h>
#include <commctrl.h>

LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
	char szAppName[]="MP3ValGUI";
	char szMainWindowCaption[]="MP3valGUI 0.0.0+ (not for publiñ release)";
	HWND hWnd;
	MSG msg;
	WNDCLASS wndclass;
	BOOL msgstatus;
	
	InitCommonControls();

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	RegisterClass(&wndclass);

	hWnd=CreateWindow(szAppName,                    // window class name
				szMainWindowCaption,          // window caption
				WS_OVERLAPPEDWINDOW,          // window style
				CW_USEDEFAULT,                // initial x position
				CW_USEDEFAULT,                // initial y position
				CW_USEDEFAULT,                // initial x size
				CW_USEDEFAULT,                // initial y size
				NULL,                         // parent window handle
				NULL,                         // window menu handle
				hInstance,                    // program instance handle
				NULL);                        // creation parameters
	
	ShowWindow(hWnd,iCmdShow);
	UpdateWindow(hWnd);

	msgstatus=FALSE;

	while((msgstatus=GetMessage(&msg, NULL, 0, 0))&&msgstatus!=-1) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(msgstatus==-1) return -1;

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	RECT clientrect;
	HWND hListView;
	HWND hEdit;
	
	switch(message) {
	case WM_CREATE:
		GetClientRect(hWnd,&clientrect);
		hListView=CreateWindow(WC_LISTVIEW,"",WS_CHILD|WS_VISIBLE|LVS_LIST,4,4,clientrect.right-clientrect.left-8,clientrect.bottom-clientrect.top-104,hWnd,NULL,GetModuleHandle(NULL),NULL);
		hEdit=CreateWindow(WC_EDIT,"",WS_VISIBLE|WS_CHILD|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL,4,clientrect.bottom-96,clientrect.right-clientrect.left-8,92,hWnd,NULL,GetModuleHandle(NULL),NULL);
		return 0;
	case WM_COMMAND:
		return 0;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	
	return DefWindowProc(hWnd,message,wParam,lParam);
}
