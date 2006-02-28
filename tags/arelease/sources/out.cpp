/*
 * MP3val - a program for MPEG audio file validation
 * Copyright (C) 2005-2006 Alexey Kuznetsov (ring0) and Eugen Tikhonov (jetsys)
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

#include "out.h"

static int iPrevSize=0;

int WriteToFile(HANDLE hFile,char *baseptr,int index,int bytes,int iFileSize) {
	int tmp;
	DWORD dwBytesWritten,dwBytesWrittenTotal=0;

	if(bytes<0) {
		if(iPrevSize) SetFilePointer(hFile,-iPrevSize,NULL,FILE_CURRENT);
		tmp=iPrevSize;
		iPrevSize=0;
		return tmp;
	}

	if(iFileSize>=0) {
		if(index+bytes>iFileSize) bytes=iFileSize-index;
	}

	while(dwBytesWrittenTotal<(DWORD)bytes) {
		if(!WriteFile(hFile,&baseptr[index+dwBytesWrittenTotal],bytes-dwBytesWrittenTotal,&dwBytesWritten,NULL)) {
			return -1;
		}
		dwBytesWrittenTotal+=dwBytesWritten;
	}

	iPrevSize=bytes;

	return bytes;
}
