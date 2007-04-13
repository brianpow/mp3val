/*
 * MP3val-frontend - a frontend for MP3val program
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

#include "base.h"

#include <windows.h>
#include <commctrl.h>
#include <cstdio>

#include "commands.h"
#include "listman.h"
#include "resource.h"
#include "strings.h"
#include "buttons.h"

HWND hWnd,hListView,hEdit,hProgress,hToolbar;
HMENU hViewMenu,hPopup;

bool bClicked=false;

WNDPROC StdListViewProc;

int InitListView();
int InitEdit();
int InitProgressBar();
int InitToolBar();
int ArrangeWindows();
int ArrangeListView();

LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK ListViewSubclassingProc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
	char szAppName[]="MP3Val-frontend";
	char szMainWindowCaption[]="MP3val-frontend 0.1.0+ (alpha)";
	MSG msg;
	WNDCLASS wndclass;
	BOOL msgstatus;
	HACCEL hAccel;
	
	InitCommonControls();
	
	hPopup=LoadMenu(hInstance,MAKEINTRESOURCE(IDM_POPUP));
	hPopup=GetSubMenu(hPopup,0);

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON1));
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
	
	hAccel=LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_APPACCEL));
	ShowWindow(hWnd,iCmdShow);
	UpdateWindow(hWnd);

	msgstatus=FALSE;

	while((msgstatus=GetMessage(&msg, NULL, 0, 0))&&msgstatus!=-1) {
		if(!TranslateAccelerator(hWnd,hAccel,&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if(msgstatus==-1) return -1;

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	PAINTSTRUCT ps;
	LPNMHDR pnmhdr;
	
	switch(message) {
	case WM_CREATE:
		::hWnd=hWnd;
		hListView=CreateWindowEx(WS_EX_CLIENTEDGE,WC_LISTVIEW,"",WS_CHILD|WS_VISIBLE|LVS_REPORT|WS_VSCROLL|WS_HSCROLL,0,0,10,10,hWnd,NULL,GetModuleHandle(NULL),NULL);
		hEdit=CreateWindowEx(WS_EX_CLIENTEDGE,"Edit","",WS_CHILD|WS_VISIBLE|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY|WS_VSCROLL,0,0,100,100,hWnd,NULL,GetModuleHandle(NULL),NULL);
		hProgress=CreateWindow(PROGRESS_CLASS,"",WS_CHILD|WS_VISIBLE,0,0,100,100,hWnd,NULL,GetModuleHandle(NULL),NULL);
		hToolbar=CreateWindow(TOOLBARCLASSNAME,"",TBSTYLE_TOOLTIPS|WS_CHILD|WS_VISIBLE,0,0,100,100,hWnd,NULL,GetModuleHandle(NULL),NULL);

		ArrangeWindows();

		InitListView();
		InitEdit();
		InitProgressBar();
		InitToolBar();

		InitCommands();

		hViewMenu=GetSubMenu(GetMenu(hWnd),2);
		StdListViewProc=(WNDPROC)SetWindowLongPtr(hListView,GWLP_WNDPROC,(LONG_PTR)ListViewSubclassingProc);
		DragAcceptFiles(hWnd,TRUE);
		return 0;
	case WM_PAINT:
		hDC=BeginPaint(hWnd,&ps);
		EndPaint(hWnd,&ps);
		return 0;
	case WM_SIZE:
		ArrangeWindows();
		ArrangeListView();
		return 0;
	case WM_DROPFILES:
		DoDropFiles((HDROP)wParam);
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDM_FILE_ADDFILE:
			DoFileAddFile();
			break;
		case IDM_FILE_ADDDIR:
			DoFileAddDir();
			break;
		case IDM_FILE_OPTIONS:
			DoActionsOptions();
			break;
		case IDM_FILE_QUIT:
			DoFileQuit();
			break;
		
		case IDM_ACTIONS_REMOVE:
			DoActionsRemove(true);
			break;
		case IDM_ACTIONS_CLEAR:
			DoActionsRemove(false);
			break;
		case IDM_ACTIONS_SCANALL:
			DoActionsScan(false,false);
			break;
		case IDM_ACTIONS_SCANSEL:
			DoActionsScan(true,false);
			break;
		case IDM_ACTIONS_FIX_PROBLEMS:
			DoActionsScan(false,true);
			break;
		case IDM_ACTIONS_FIXSEL:
			DoActionsScan(true,true);
			break;
		case IDM_ACTIONS_STOPSCAN:
			DoActionsStopScan();
			break;
		
		case IDM_VIEW_ALL:
			DoViewSetMode(VM_EVERYTHING);
			break;
		case IDM_VIEW_NOT_SCANNED:
			DoViewSetMode(VM_NOT_SCANNED);
			break;
		case IDM_VIEW_NORMAL:
			DoViewSetMode(VM_NORMAL);
			break;
		case IDM_VIEW_PROBLEMS:
			DoViewSetMode(VM_PROBLEMS);
			break;
		case IDM_VIEW_REPAIRED:
			DoViewSetMode(VM_FIXED);
			break;
		
		case IDM_HELP_ABOUT:
			DoHelpAbout();
			break;
		}
		return 0;
	case WM_NOTIFY:
		pnmhdr=(LPNMHDR)lParam;
		if(pnmhdr->hwndFrom==hListView) {
			switch(pnmhdr->code) {
			case NM_RCLICK:
				HandleListViewRClick((LPNMITEMACTIVATE)pnmhdr);
				return 0;
			case LVN_ITEMCHANGED:
				return 0;
			}
		}
		else if(pnmhdr->code==TTN_GETDISPINFO) {
			LPNMTTDISPINFO(pnmhdr)->hinst=NULL;
			switch(pnmhdr->idFrom) {
			case IDM_FILE_ADDFILE:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_ADDFILE;
				break;
			case IDM_FILE_ADDDIR:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_ADDDIR;
				break;
			
			case IDM_ACTIONS_REMOVE:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_DELETE;
				break;
			case IDM_ACTIONS_CLEAR:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_CLEAR;
				break;

			case IDM_ACTIONS_SCANALL:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_SCANALL;
				break;
			case IDM_ACTIONS_FIX_PROBLEMS:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_FIXALL;
				break;
			case IDM_ACTIONS_STOPSCAN:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_STOP;
				break;
			
			default:
				LPNMTTDISPINFO(pnmhdr)->lpszText=STR_TOOLTIP_HELP;
				break;
			}
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

	MoveWindow(hListView,4,28,w-8,h-12-170,TRUE);
	MoveWindow(hEdit,4,h-4-146,w-8,125,TRUE);
	MoveWindow(hProgress,4,h-19,w-8,15,TRUE);
	
	return 0;
}

int InitListView() {
	LV_COLUMN col;
	char szFileHeading[]="File";
	char szStateHeading[]="State";
	RECT r;
	
	GetClientRect(hWnd,&r);
	
	col.mask=LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	
	col.fmt=LVCFMT_LEFT;
	col.cx=r.right-r.left-8-130-4;
	col.pszText=szFileHeading;
	col.iSubItem=0;
	
	SendMessage(hListView,LVM_INSERTCOLUMN,0,(LPARAM)&col);
	
	col.fmt=LVCFMT_LEFT;
	col.cx=100;
	col.pszText=szStateHeading;
	col.iSubItem=1;
	
	SendMessage(hListView,LVM_INSERTCOLUMN,1,(LPARAM)&col);
	
	ListView_SetExtendedListViewStyle(hListView,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	
	return 0;
}

int ArrangeListView() {
	RECT r;
	
	GetClientRect(hWnd,&r);
	
	ListView_SetColumnWidth(hListView,0,r.right-r.left-8-130);
	ListView_SetColumnWidth(hListView,1,100);
	
	return 0;
}

int InitEdit() {
	HFONT hFont;
	HDC hDC;
	LOGFONT lf=
	{
			0, //lfHeight will be set later
			0, //lfWidth
			0, //lfEscapement
			0, //lfOrienation
			FW_NORMAL, //lfWeight
			FALSE, //lfItalic
			FALSE, //lfUnderline
			FALSE, //lfStrikeOut
			DEFAULT_CHARSET, //lfCharSet
			OUT_DEFAULT_PRECIS, //lfOutPrecision
			CLIP_DEFAULT_PRECIS, //lfClipPrecision
			DEFAULT_QUALITY, //lfQuality
			FF_MODERN, //lfPitchAndFamily
			"\0" //lfFaceName
	};
	
	hDC=GetDC(hEdit);
	lf.lfHeight=-MulDiv(8,GetDeviceCaps(hDC,LOGPIXELSY),72);
	ReleaseDC(hEdit,hDC);
	
	hFont=CreateFontIndirect(&lf);
	SendMessage(hEdit,WM_SETFONT,(WPARAM)hFont,(LPARAM)FALSE);
	
	return 0;
}

int InitProgressBar() {
	SendMessage(hProgress,PBM_SETSTEP,(WPARAM)1,0);

	return 0;
}

int InitToolBar() {
	TBADDBITMAP tbab;
	TBBUTTON tbb[11];
	int i=0;
	
	SendMessage(hToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),(LPARAM)0);
	
	tbab.hInst=GetModuleHandle(NULL);
	tbab.nID=IDB_TOOLBAR;
	SendMessage(hToolbar,TB_ADDBITMAP,(WPARAM)8,(LPARAM)&tbab);
	
	tbb[i].iBitmap=TOOLBAR_IMAGE_ADDFILE;
	tbb[i].idCommand=IDM_FILE_ADDFILE;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;

	tbb[i].iBitmap=TOOLBAR_IMAGE_ADDDIR;
	tbb[i].idCommand=IDM_FILE_ADDDIR;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;
	
	tbb[i].iBitmap=0;
	tbb[i].idCommand=0;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=BTNS_SEP;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;
	
	tbb[i].iBitmap=TOOLBAR_IMAGE_DELSEL;
	tbb[i].idCommand=IDM_ACTIONS_REMOVE;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;
	
	tbb[i].iBitmap=TOOLBAR_IMAGE_CLEAR;
	tbb[i].idCommand=IDM_ACTIONS_CLEAR;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;

	tbb[i].iBitmap=0;
	tbb[i].idCommand=0;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=BTNS_SEP;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;
	
	tbb[i].iBitmap=TOOLBAR_IMAGE_SCANALL;
	tbb[i].idCommand=IDM_ACTIONS_SCANALL;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;

	tbb[i].iBitmap=TOOLBAR_IMAGE_FIXALL;
	tbb[i].idCommand=IDM_ACTIONS_FIX_PROBLEMS;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;
	
	tbb[i].iBitmap=TOOLBAR_IMAGE_STOP;
	tbb[i].idCommand=IDM_ACTIONS_STOPSCAN;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;
	
	tbb[i].iBitmap=0;
	tbb[i].idCommand=0;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=BTNS_SEP;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	i++;

	tbb[i].iBitmap=TOOLBAR_IMAGE_HELP;
	tbb[i].idCommand=IDM_HELP_ABOUT;
	tbb[i].fsState=TBSTATE_ENABLED;
	tbb[i].fsStyle=TBSTYLE_BUTTON;
	tbb[i].dwData=0;
	tbb[i].iString=0;
	
	SendMessage(hToolbar,TB_ADDBUTTONS,(WPARAM)11,(LPARAM)&tbb);
	SendMessage(hToolbar,TB_SETINDENT,(WPARAM)4,(LPARAM)0);
	SendMessage(hToolbar,TB_AUTOSIZE,(WPARAM)0,(LPARAM)0);
	
	return 0;
}

LRESULT CALLBACK ListViewSubclassingProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	LRESULT res;
	
	res=CallWindowProc(StdListViewProc,hWnd,message,wParam,lParam);
	
	if(message==WM_LBUTTONDOWN||message==WM_KEYDOWN) {
		bClicked=true;
		HandleSelectionChange(-1);
	}
	return res;
}
