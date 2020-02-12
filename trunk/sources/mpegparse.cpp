/*
 * MP3val - a program for MPEG audio file validation
 * Copyright (C) 2005-2009 Alexey Kuznetsov (ring0) and Eugen Tikhonov (jetsys)
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

#include "crossapi.h"
#include "mpegparse.h"
#include "report.h"
#include "out.h"
#include "crc.h"
#include <cstring>
#include <iostream>
#include <fstream>

using namespace std;

int mpeg1layer1_bitrates[]={-1,32,64,96,128,160,192,224,256,288,320,352,384,416,448,-1};
int mpeg1layer2_bitrates[]={-1,32,48,56,64,80,96,112,128,160,192,224,256,320,384,-1};
int mpeg1layer3_bitrates[]={-1,32,40,48,56,64,80,96,112,128,160,192,224,256,320,-1};
int mpeg2layer1_bitrates[]={-1,32,48,56,64,80,96,112,128,144,160,176,192,224,256,-1};
int mpeg2layers23_bitrates[]={-1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,-1};

int ValidateMPEGFrame(unsigned char *baseptr,int index, MPEGINFO *mpginfo);
int CheckMP3CRC(unsigned char *baseptr,int index,MPEGINFO *mpginfo,bool fix);
int ValidateID3v1Tag(unsigned char *baseptr,int index,int size, MPEGINFO *mpginfo);
int ValidateID3v2Tag(unsigned char *baseptr,int index,int size, MPEGINFO *mpginfo);
int ValidateAPEv2Tag(unsigned char *baseptr,int index,int size, MPEGINFO *mpginfo);

int ParseXingHeader(unsigned char *baseptr,int index,MPEGINFO *mpginfo);
int ParseVBRIHeader(unsigned char *baseptr,int index,MPEGINFO *mpginfo);

int ParseRIFFHeader(unsigned char *baseptr,int index,int iFileSize,int *iNewFileSize);

int MPEGResync(unsigned char *baseptr,int index,int iFileSize,int frames);

DWORD rotate_dword(DWORD x);

int ValidateEnhancedTag(unsigned char *baseptr,int index,int size, MPEGINFO *mpginfo){
	if(size>=4 && !memcmp(&baseptr[index],"TAG+",4)){
		if(size >= 227){
		mpginfo->enhancedTag++;
		return 227;
		}
		else
		{
			//TODO: Handle truncated tag
		}

	}
	return 0;
}

int ValidateID3v1Tag(unsigned char *baseptr,int index,int size, MPEGINFO *mpginfo) {
	if(size>=3&&!memcmp(&baseptr[index],"TAG",3) && memcmp(&baseptr[index],"TAG+",4))
	{
		if(size >= 128)
		{
		mpginfo->id3v1++;
		return 128;
		}
		else
		{
			//TODO: Handle truncated tag
		}

	}
	return 0;
}

int CheckTags(unsigned char *baseptr,int pos,int end,ostream *out,char *filename,MPEGINFO *mpginfo){
	int totalBytes=0, tmp, index;
	bool tagFound;
	do{
		tagFound=false;
		index=pos+totalBytes;
		if((tmp=ValidateEnhancedTag(baseptr,index,end-index,mpginfo))){
			if(!ValidateID3v1Tag(baseptr,index+tmp,end-index,mpginfo))
				PrintMessage(out,"WARNING",filename,"Enhanced Tag found but no ID3 v1 Tag followed!\n",index,tmp);
			totalBytes+=tmp;
			tagFound=true;
			index=pos+totalBytes;
		}
		if((tmp=ValidateID3v1Tag(baseptr,index,end-index,mpginfo))){
			PrintMessage(out,"WARNING",filename,"ID3 v1 tag found, it should be at the end of file!\n",index,tmp);
			totalBytes+=tmp;
			tagFound=true;
			index=pos+totalBytes;
		}
		if((tmp=ValidateID3v2Tag(baseptr,index,end-index,mpginfo))){
			PrintMessage(out,"WARNING",filename,"ID3 v2 tag found, it should be at the beginning or the end of file!\n",index,tmp);
			totalBytes+=tmp;
			tagFound=true;
			index=pos+totalBytes;
		}
		if((tmp=ValidateAPEv2Tag(baseptr,index,end-index,mpginfo))){
			PrintMessage(out,"INFO",filename,"APE v2 tag found.\n",index,tmp);
			totalBytes+=tmp;
			tagFound=true;
			index=pos+totalBytes;
		}
	}while(tagFound==true);
	return totalBytes;
}

int ValidateFile(unsigned char *baseptr,int iFileSize,MPEGINFO *mpginfo,ostream *out,char *filename,bool fix,int hFile, bool bSplitFile) {
	int iFrame;
	int iFrameSize=0;
	int iLastMPEGFrame=0,iNewFrame;
	bool WasFirstFrame=false;
	int iXingOffset=0;
	int iID3v1Offset=0;
	int iEnhancedTagOffset=0;
	int iLastConsecutiveFrameBegin=0;
	int iFirstMPEGFrameOffset=0;
	DWORD dwTemp;
	int mpeg_total;
	bool LastFrameWasMPEG=false;
	char *prefix;
	prefix=getFilename(filename);
	iFrame=0;

	mpginfo->clear();

	if(int tmp=ValidateID3v1Tag(baseptr,iFileSize-128,128,mpginfo)) {
		iFileSize-=tmp;
		iID3v1Offset=iFileSize;
		if(int tmp=ValidateEnhancedTag(baseptr,iFileSize-227,227,mpginfo)){
			iFileSize-=tmp;
			iEnhancedTagOffset=iFileSize;
		}
	}

	if((iFileSize>=4)&&!memcmp(&baseptr[iFrame],"ID3",3)) {
		if(iFrame+10>iFileSize) {
			mpginfo->truncated=iFrame;
		}
		else {
			iFrame+=ValidateID3v2Tag(baseptr,iFrame,iFileSize-iFrame,mpginfo);
			if(fix) {
				if(WriteToFile(hFile,(char *)baseptr,0,iFrame,iFileSize)==-1) return -1;
				LastFrameWasMPEG=false;
			}
		}
	}

	while(iFrame!=iFileSize) {
		if(iFrame+4>iFileSize) {
//Bad (unknown) frame
			mpginfo->truncated=iFrame;
			break;
		}
		if(!memcmp(&baseptr[iFrame],"RIFF",4)) {
			if(!WasFirstFrame) {
//This is actually a WAV file, not MPEG. Parsing RIFF header
				mpginfo->riff=iFrame;
				iNewFrame=ParseRIFFHeader(baseptr,iFrame,iFileSize,&iFileSize);
				if(iNewFrame!=-1) {
					iFrame=iNewFrame;
					continue;
				}
			}
		}

		if((baseptr[iFrame]==0xFF)&&((baseptr[iFrame+1]&0xE0)==0xE0)) {
//MPEG frame
			iFrameSize=ValidateMPEGFrame(baseptr,iFrame,mpginfo);
			if(iFrameSize!=-1) {
				if(iFrameSize+iFrame<=iFileSize&&mpginfo->iLastMPEGLayer==3&&mpginfo->bLastFrameCRC) CheckMP3CRC(baseptr,iFrame,mpginfo,fix);
				if(fix&&!WasFirstFrame) iFirstMPEGFrameOffset=CrossAPI_SetFilePointer(hFile,0,true);
				if(fix) {
					if(WriteToFile(hFile,(char *)baseptr,iFrame,iFrameSize,iFileSize)==-1) return -1;
					LastFrameWasMPEG=true;
				}
				if(!WasFirstFrame) {
					WasFirstFrame=true;
					mpginfo->iFirstMPEGFrameSize=iFrameSize;
					if(mpginfo->iLastMPEGVersion==1) {
						if(mpginfo->LastFrameStereo) {
							iXingOffset=32;
						}
						else {
							iXingOffset=17;
						}
					}
					else {
						if(mpginfo->LastFrameStereo) {
							iXingOffset=17;
						}
						else {
							iXingOffset=9;
						}
					}
					ParseXingHeader(baseptr,iFrame+4+iXingOffset,mpginfo);
					if(!mpginfo->VBRHeaderPresent) ParseVBRIHeader(baseptr,iFrame+4+32,mpginfo);
				}
				iLastMPEGFrame=iFrame;
				iFrame+=iFrameSize;
				continue;
			}
		}
//APEv2 tag
		if(!memcmp(&baseptr[iFrame],"APET",4)) {
			if(iFrame+16>iFileSize) {
				mpginfo->truncated=iFrame;
				break;
			}
			iFrameSize=ValidateAPEv2Tag(baseptr,iFrame,iFileSize-iFrame,mpginfo);
			if(fix) {
				if(WriteToFile(hFile,(char *)baseptr,iFrame,iFrameSize,iFileSize)==-1) return -1;
				LastFrameWasMPEG=false;
			}
			iFrame+=iFrameSize;
			continue;
		}

		if(fix) {
			if(LastFrameWasMPEG) {
				mpginfo->iDeletedFrames++;
				mpginfo->iTotalMPEGBytes-=WriteToFile(hFile,NULL,0,-1,-1);
			}
/*			else {
				if(WriteToFile(hFile,NULL,0,-1,-1)==-1) return -1;
			}*/
		}
		else if(LastFrameWasMPEG) {
			mpginfo->iDeletedFrames++;
			mpginfo->iTotalMPEGBytes-=GetLastFrameSize();
		}

		if(!iFrame) {
			iNewFrame=MPEGResync(baseptr,iFrame,iFileSize,8);
			if(iNewFrame==-1) {
				mpginfo->unknown_format=0;
				break;
			}
			mpginfo->garbage_at_the_begin=0;
			if(!fix)
				PrintMessage(out,"WARNING",filename,"Garbage at the beginning of the file",mpginfo->garbage_at_the_begin, iNewFrame);
			mpginfo->iErrors++;
			iFrame=iNewFrame;
			iLastConsecutiveFrameBegin=iNewFrame;
		}
		else {
			iNewFrame=MPEGResync(baseptr,iLastMPEGFrame?(iLastMPEGFrame+1):iFrame,iFileSize,6);
			if(iNewFrame==-1) {
				mpginfo->garbage_at_the_end=iFrame;
				if(!fix) PrintMessage(out,"WARNING",filename,"Garbage at the end of the file",mpginfo->garbage_at_the_end, -1, iFileSize);
				iFrame+=CheckTags(baseptr,iFrame,iFileSize,out,filename,mpginfo);
				PrintMessage(out,"INFO",filename,"Last good consecutive range", iLastConsecutiveFrameBegin, -1, iFrame);
				if(bSplitFile) writeFile(prefix, "mp3", baseptr, iLastConsecutiveFrameBegin, iFrame-iLastConsecutiveFrameBegin);
				break;
			}
			mpginfo->mpeg_stream_error=iFrame;
			if(!fix) {
				PrintMessage(out,"WARNING",filename,"MPEG stream error, resynchronized successfully",mpginfo->mpeg_stream_error, -1, iNewFrame);
				iFrame+=CheckTags(baseptr,iFrame,iFileSize,out,filename,mpginfo);
				PrintMessage(out,"INFO",filename,"Last good consecutive range", iLastConsecutiveFrameBegin, -1, iFrame);
				if(bSplitFile) writeFile(prefix, "mp3", baseptr, iLastConsecutiveFrameBegin, iFrame-iLastConsecutiveFrameBegin);

			}
			mpginfo->iErrors++;
			iFrame=iNewFrame;
			iLastConsecutiveFrameBegin=iNewFrame;
		}

	}

	mpeg_total=
		mpginfo->mpeg1layer1+
		mpginfo->mpeg1layer2+
		mpginfo->mpeg1layer3+
		mpginfo->mpeg2layer1+
		mpginfo->mpeg2layer2+
		mpginfo->mpeg2layer3+
		mpginfo->mpeg25layer1+
		mpginfo->mpeg25layer2+
		mpginfo->mpeg25layer3;

	if(mpginfo->truncated>=0) {
		if(fix) {
			if(WriteToFile(hFile,NULL,0,-1,-1)==-1) return -1;
			if(LastFrameWasMPEG) {
				mpginfo->iTotalMPEGBytes-=iFrameSize;
				mpginfo->iDeletedFrames++;
			}
		}
		else if(LastFrameWasMPEG) {
			mpginfo->iTotalMPEGBytes-=iFrameSize;
			mpginfo->iDeletedFrames++;
		}
	}

	if(fix&&mpginfo->id3v1) {
		if(mpginfo->enhancedTag)
			if(WriteToFile(hFile,(char *)baseptr,iEnhancedTagOffset,227,-1)==-1) return -1;
		if(WriteToFile(hFile,(char *)baseptr,iID3v1Offset,128,-1)==-1) return -1;
	}

	if(fix) CrossAPI_SetEndOfFile(hFile);

	if(fix&&mpginfo->VBRHeaderPresent) {
		if(mpginfo->IsXingHeader) {
			if(mpginfo->BytesPresent&&mpginfo->FramesPresent) {
				CrossAPI_SetFilePointer(hFile,iFirstMPEGFrameOffset+iXingOffset+12,false);
				dwTemp=rotate_dword(mpeg_total-mpginfo->iDeletedFrames);
				if(WriteToFile(hFile,(char *)&dwTemp,0,4,-1)==-1) return -1;
				dwTemp=rotate_dword(mpginfo->iTotalMPEGBytes);
				if(WriteToFile(hFile,(char *)&dwTemp,0,4,-1)==-1) return -1;
			}
			else if(mpginfo->BytesPresent) {
				CrossAPI_SetFilePointer(hFile,iFirstMPEGFrameOffset+iXingOffset+12,false);
				dwTemp=rotate_dword(mpginfo->iTotalMPEGBytes);
				if(WriteToFile(hFile,(char *)&dwTemp,0,4,-1)==-1) return -1;
			}
			else if(mpginfo->FramesPresent) {
				CrossAPI_SetFilePointer(hFile,iFirstMPEGFrameOffset+iXingOffset+12,false);
				dwTemp=rotate_dword(mpeg_total-mpginfo->iDeletedFrames);
				if(WriteToFile(hFile,(char *)&dwTemp,0,4,-1)==-1) return -1;
			}
		}
		else {
			CrossAPI_SetFilePointer(hFile,iFirstMPEGFrameOffset+46,false);
			dwTemp=rotate_dword(mpginfo->iTotalMPEGBytes);
			if(WriteToFile(hFile,(char *)&dwTemp,0,4,-1)==-1) return -1;
			dwTemp=rotate_dword(mpeg_total-mpginfo->iDeletedFrames);
			if(WriteToFile(hFile,(char *)&dwTemp,0,4,-1)==-1) return -1;
		}
	}

	if(fix) mpginfo->iErrors=1;

	return 0;
}

int ValidateMPEGFrame(unsigned char *baseptr,int index, MPEGINFO *mpginfo) {
	int mpeg_version, mpeg_layer;
	int mpeg_bitrate, mpeg_sampling_rate, mpeg_padding;

	int bitrate_index=0;
	int iFrameSize;

//Check if the frame contains CRC
	if(baseptr[index+1]&0x01) {
		mpginfo->bLastFrameCRC=false;
	}
	else {
		mpginfo->bLastFrameCRC=true;
		mpginfo->bCRC=true;
	}

// Determine MPEG version and layer
	switch((baseptr[index+1]>>1)&0x0F) {
	case 0x0F:
		mpginfo->mpeg1layer1++;
		mpeg_version=1;
		mpeg_layer=1;
		break;
	case 0x0E:
		mpginfo->mpeg1layer2++;
		mpeg_version=1;
		mpeg_layer=2;
		break;
	case 0x0D:
		mpginfo->mpeg1layer3++;
		mpeg_version=1;
		mpeg_layer=3;
		break;
	case 0x0B:
		mpginfo->mpeg2layer1++;
		mpeg_version=2;
		mpeg_layer=1;
		break;
	case 0x0A:
		mpginfo->mpeg2layer2++;
		mpeg_version=2;
		mpeg_layer=2;
		break;
	case 0x09:
		mpginfo->mpeg2layer3++;
		mpeg_version=2;
		mpeg_layer=3;
		break;
	case 0x03:
		mpginfo->mpeg25layer1++;
		mpeg_version=25;
		mpeg_layer=1;
		break;
	case 0x02:
		mpginfo->mpeg25layer2++;
		mpeg_version=25;
		mpeg_layer=2;
		break;
	case 0x01:
		mpginfo->mpeg25layer3++;
		mpeg_version=25;
		mpeg_layer=3;
		break;
	default:
		mpginfo->mpeg_stream_error=index;
		return -1;
	}
// Calculate bit rate
	*((unsigned char *)&bitrate_index)=((baseptr[index+2])>>4)&0x0F;
	if(mpeg_version==1) {
		if(mpeg_layer==1) {
			mpeg_bitrate=mpeg1layer1_bitrates[bitrate_index];
		}
		else if(mpeg_layer==2) {
			mpeg_bitrate=mpeg1layer2_bitrates[bitrate_index];
		}
		else {
			mpeg_bitrate=mpeg1layer3_bitrates[bitrate_index];
		}
	}
	else {
		if(mpeg_layer==1) {
			mpeg_bitrate=mpeg2layer1_bitrates[bitrate_index];
		}
		else {
			mpeg_bitrate=mpeg2layers23_bitrates[bitrate_index];
		}
	}

	if(mpeg_bitrate==-1) {
		mpginfo->mpeg_stream_error=index;
		return -1;
	}

	if(mpginfo->iLastBitrate>0&&mpginfo->iLastBitrate!=mpeg_bitrate) mpginfo->bVariableBitrate=true;
	mpginfo->iLastBitrate=mpeg_bitrate;

//Determine sampling rate
	switch((baseptr[index+2]>>2)&0x03) {
	case 0x00:
		if(mpeg_version==1) mpeg_sampling_rate=44100;
		else if(mpeg_version==2) mpeg_sampling_rate=22050;
		else mpeg_sampling_rate=11025;
		break;
	case 0x01:
		if(mpeg_version==1) mpeg_sampling_rate=48000;
		else if(mpeg_version==2) mpeg_sampling_rate=24000;
		else mpeg_sampling_rate=12000;
		break;
	case 0x02:
		if(mpeg_version==1) mpeg_sampling_rate=32000;
		else if(mpeg_version==2) mpeg_sampling_rate=16000;
		else mpeg_sampling_rate=8000;
		break;
	default:
		mpginfo->mpeg_stream_error=index;
		return -1;
	}

//Check if padding is being used
	if((baseptr[index+2]>>1)&0x01) mpeg_padding=1;
	else mpeg_padding=0;

//Check if frame is stereo
	if((baseptr[index+3]&0xC0)==0xC0) mpginfo->LastFrameStereo=false;
	else mpginfo->LastFrameStereo=true;

	mpginfo->iLastMPEGVersion=mpeg_version;
	mpginfo->iLastMPEGLayer=mpeg_layer;

	if(mpeg_layer==1) iFrameSize=(12*mpeg_bitrate*1000/mpeg_sampling_rate+mpeg_padding)*4;
	else if(mpeg_layer==2) iFrameSize=144*mpeg_bitrate*1000/mpeg_sampling_rate+mpeg_padding;
	else if(mpeg_layer==3&&mpeg_version==1)	iFrameSize=144*mpeg_bitrate*1000/mpeg_sampling_rate+mpeg_padding;
	else iFrameSize=72*mpeg_bitrate*1000/mpeg_sampling_rate+mpeg_padding;

	mpginfo->iTotalMPEGBytes+=iFrameSize;

	return iFrameSize;
}

int CheckMP3CRC(unsigned char *baseptr,int index,MPEGINFO *mpginfo,bool fix) {
	int crc=0xFFFF;
	int storedcrc=0;
	int iSideInfoSize;
	crc=CalculateCRC16(crc,0x8005,(char *)&baseptr[index+2],2);

	if(mpginfo->LastFrameStereo) {
		if(mpginfo->iLastMPEGVersion==1) {
			iSideInfoSize=32;
		}
		else {
			iSideInfoSize=17;
		}
	}
	else {
		if(mpginfo->iLastMPEGVersion==1) {
			iSideInfoSize=17;
		}
		else {
			iSideInfoSize=9;
		}
	}

	crc=CalculateCRC16(crc,0x8005,(char *)&baseptr[index+6],iSideInfoSize);

	((char *)&storedcrc)[1]=baseptr[index+4];
	((char *)&storedcrc)[0]=baseptr[index+5];

	if(storedcrc!=crc) {
		mpginfo->bCRCError=true;
		mpginfo->iCRCErrors++;
		if(fix) {
			baseptr[index+4]=((char *)&crc)[1];
			baseptr[index+5]=((char *)&crc)[0];
		}
	}

	return 0;
}

int ValidateID3v2Tag(unsigned char *baseptr,int index,int size, MPEGINFO *mpginfo) {
	int iDataSize=0;
	if(size>=3 && !memcmp(&baseptr[index],"ID3",3)) {
		if(size<9)
			return 0; //TODO: Handle Truncateed Tag
		iDataSize=baseptr[index+9];
		iDataSize+=128*baseptr[index+8];
		iDataSize+=16384*baseptr[index+7];
		iDataSize+=2097152*baseptr[index+6];

		iDataSize+=(baseptr[index+5]&0x10)?20:10;
		if(size>=iDataSize){
			mpginfo->id3v2++;
			return iDataSize;
		}
		else
		{
			//TODO: Handle Truncateed Tag
		}

	}
	return 0;
}

int ValidateAPEv2Tag(unsigned char *baseptr,int index,int size,MPEGINFO *mpginfo) {
	if(size>=4 && !memcmp(&baseptr[index],"APET",4)){
		if(size < 12) // TODO: Handle truncated tag
			return 0;
		int tagSize= *((int *)&baseptr[index+12])+32;
		if(tagSize >= size)
		{
			mpginfo->apev2++;
			return tagSize;
		}
		else
		{
			// TODO: Handle truncated tag
		}

	}
	return 0;
}

int MPEGResync(unsigned char *baseptr,int index,int iFileSize,int frames) {
	unsigned char *p=&baseptr[index];
	int sync_frames=0;
	int new_frame=0;
	int iFrameSize;
	MPEGINFO tmp_mpginfo;
	int iMPEGVersion=0,iMPEGLayer=0;

	do {
		if(iFileSize-(p-baseptr)-3<=0) return -1;
		p=(unsigned char *)memchr(p,'\xFF',iFileSize-(p-baseptr)-3);
		if(!p) break;
		if((p[1]&0xE0)!=0xE0) {
			p++;
			sync_frames=0;
			iMPEGVersion=0;
			iMPEGLayer=0;
			continue;
		}
		iFrameSize=ValidateMPEGFrame(baseptr,p-baseptr,&tmp_mpginfo);
		if(iFrameSize==-1) {
			p++;
			sync_frames=0;
			iMPEGVersion=0;
			iMPEGLayer=0;
			continue;
		}
		new_frame=iFrameSize+(p-baseptr);
		sync_frames++;
		iMPEGVersion=tmp_mpginfo.iLastMPEGVersion;
		iMPEGLayer=tmp_mpginfo.iLastMPEGLayer;
		while(sync_frames<frames) {
			if(new_frame+4>iFileSize) {
				sync_frames=0;
				iMPEGVersion=0;
				iMPEGLayer=0;
				p++;
				break;
			}
			iFrameSize=ValidateMPEGFrame(baseptr,new_frame,&tmp_mpginfo);
			if(iFrameSize==-1) {
				sync_frames=0;
				iMPEGVersion=0;
				iMPEGLayer=0;
				p++;
				break;
			}
			new_frame+=iFrameSize;
			sync_frames++;
			if(iMPEGVersion&&iMPEGVersion!=tmp_mpginfo.iLastMPEGVersion) {
				sync_frames=0;
				iMPEGVersion=0;
				iMPEGLayer=0;
				p++;
				break;
			}
			if(iMPEGLayer&&iMPEGLayer!=tmp_mpginfo.iLastMPEGLayer) {
				sync_frames=0;
				iMPEGVersion=0;
				iMPEGLayer=0;
				p++;
				break;
			}
		}
	} while(sync_frames<frames);

	if(sync_frames>=frames) return (p-baseptr);
	return -1;
}

int ParseXingHeader(unsigned char *baseptr,int index,MPEGINFO *mpginfo) {
	if(memcmp(&baseptr[index],"Xing",4)&&memcmp(&baseptr[index],"Info",4)) return 0;
	mpginfo->VBRHeaderPresent=true;
	mpginfo->IsXingHeader=true;
	switch(baseptr[index+7]&0x03) {
	case 0x00:
		return 0;
	case 0x01:
		mpginfo->iFrames=16777216*baseptr[index+8]+65536*baseptr[index+9]+256*baseptr[index+10]+baseptr[index+11];
		mpginfo->FramesPresent=true;
		mpginfo->BytesPresent=false;
		return 0;
	case 0x02:
		mpginfo->iBytes=16777216*baseptr[index+8]+65536*baseptr[index+9]+256*baseptr[index+10]+baseptr[index+11];
		mpginfo->FramesPresent=false;
		mpginfo->BytesPresent=true;
		return 0;
	case 0x03:
		mpginfo->iFrames=16777216*baseptr[index+8]+65536*baseptr[index+9]+256*baseptr[index+10]+baseptr[index+11];
		mpginfo->iBytes=16777216*baseptr[index+12]+65536*baseptr[index+13]+256*baseptr[index+14]+baseptr[index+15];
		mpginfo->FramesPresent=true;
		mpginfo->BytesPresent=true;
		return 0;
	}

	return 0;
}

int ParseVBRIHeader(unsigned char *baseptr,int index,MPEGINFO *mpginfo) {
	if(memcmp(&baseptr[index],"VBRI",4)) return 0;
	mpginfo->VBRHeaderPresent=true;
	mpginfo->IsXingHeader=false;
	mpginfo->iBytes=16777216*baseptr[index+10]+65536*baseptr[index+11]+256*baseptr[index+12]+baseptr[index+13];
	mpginfo->iFrames=16777216*baseptr[index+14]+65536*baseptr[index+15]+256*baseptr[index+16]+baseptr[index+17];
	mpginfo->FramesPresent=true;
	mpginfo->BytesPresent=true;
	return 0;
}

int ParseRIFFHeader(unsigned char *baseptr,int index,int iFileSize,int *iNewFileSize) {
	int iDataLength;

	if(index+11>iFileSize) return -1;
	if(memcmp(&baseptr[index],"RIFF",4)) return -1;

	if(memcmp(&baseptr[index+8],"WAVE",4)) return -1;

	index+=12;

	do {
		if(index+7>iFileSize) return -1;
		iDataLength=*(int *)&baseptr[index+4];
		if(memcmp(&baseptr[index],"data",4)) {
			index+=iDataLength+8;
			continue;
		}

		*iNewFileSize=index+8+iDataLength;
		if(*iNewFileSize>iFileSize) *iNewFileSize=iFileSize;
		return index+8;
	}while(true);
}

DWORD rotate_dword(DWORD x) {
	DWORD res;

	((unsigned char *)&res)[0]=((unsigned char *)&x)[3];
	((unsigned char *)&res)[1]=((unsigned char *)&x)[2];
	((unsigned char *)&res)[2]=((unsigned char *)&x)[1];
	((unsigned char *)&res)[3]=((unsigned char *)&x)[0];

	return res;
}
