/*
 * MP3val - a program for MPEG audio file validation
 * Copyright (C) 2005-2007 Alexey Kuznetsov (ring0) and Eugen Tikhonov (jetsys)
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

#ifndef __REPORT_H__
#define __REPORT_H__

#include <iostream>

using namespace std;

#include "mpegparse.h"

int PrintReport(ostream *out,char *filename,MPEGINFO *mpginfo);
int PrintMessage(ostream *out,char *caption,char *filename,char *message,int iErrorFrame);

#endif
