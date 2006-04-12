/*
 * MP3valGUI - a frontend for MP3val program
 * Copyright (C) 2005-2006 Alexey Kuznetsov (ring0)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <commctrl.h>
#include <cstdio>

#include "commands.h"
#include "resource.h"

HWND hWnd,hListView,hEdit;

int InitListView();
int ArrangeWindows();

LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
	char szAppName[]="MP3ValGUI";
	char szMainWindowCaption[]="MP3valGUI 0.0.0+ (not for publiñ release)";
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
	wndclass.hbrBackground = (HBRUSH)GetSysColorBrush(COLOR_3DFACE);
	wndclass.lpszMenuName  = MAKEINTRESOURCE(IDM_APPMENU);
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
	HDC hDC;
	PAINTSTRUCT ps;
	
	switch(message) {
	case WM_CREATE:
		hListView=CreateWindow(WC_LISTVIEW,"",WS_CHILD|WS_VISIBLE|LVS_REPORT|WS_BORDER|WS_VSCROLL|WS_HSCROLL,0,0,10,10,hWnd,NULL,GetModuleHandle(NULL),NULL);
		hEdit=CreateWindow("Edit","",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_READONLY|WS_VSCROLL,0,0,100,100,hWnd,NULL,GetModuleHandle(NULL),NULL);
		ArrangeWindows();
		InitListView();
		InitCommands();
		return 0;
	case WM_PAINT:
		hDC=BeginPaint(hWnd,&ps);
		EndPaint(hWnd,&ps);
		return 0;
	case WM_SIZE:
		ArrangeWindows();
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDM_FILE_ADDFILE:
			DoFileAddFile();
			break;
		case IDM_FILE_ADDDIR:
			DoFileAddDir();
			break;
		case IDM_FILE_QUIT:
			DoFileQuit();
			break;
		
		case IDM_ACTIONS_SCANALL:
			DoActionsScanAll();
			break;
		case IDM_ACTIONS_SCANSEL:
			DoActionsScanSel();
			break;
		case IDM_ACTIONS_FIXSEL:
			DoActionsFixSel();
			break;
		case IDM_ACTIONS_OPTIONS:
			DoActionsOptions();
			break;
		
		case IDM_HELP_ABOUT:
			DoHelpAbout();
			break;
		}
		return 0;
	case WM_CLOSE:
		DoFileQuit();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	
	return DefWindowProc(hWnd,message,wParam,lParam);
}

int ArrangeWindows() {
	RECT r;
	int w,h;
	
	GetClientRect(hWnd,&r);
	
	w=r.right;
	h=r.bottom;

	MoveWindow(hListView,4,4,w-8-200,h-12-100,TRUE);
	MoveWindow(hEdit,4,h-4-92,w-8,92,TRUE);
	
	return 0;
}

int InitListView() {
	LV_COLUMN col;
	char szFileHeading[]="File";
	char szStateHeading[]="State";
	
	col.mask=LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	
	col.fmt=LVCFMT_LEFT;
	col.cx=300;
	col.pszText=szFileHeading;
	col.iSubItem=0;
	
	SendMessage(hListView,LVM_INSERTCOLUMN,0,(LPARAM)&col);
	
	col.fmt=LVCFMT_LEFT;
	col.cx=200;
	col.pszText=szStateHeading;
	col.iSubItem=1;
	
	SendMessage(hListView,LVM_INSERTCOLUMN,1,(LPARAM)&col);
	
	return 0;
}
