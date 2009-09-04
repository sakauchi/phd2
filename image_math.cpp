/*
 *  image_math.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include "image_math.h"

int dbl_sort_func (double *first, double *second) {
	if (*first < *second)
		return -1;
	else if (*first > *second)
		return 1;
	return 0;
}

int us_sort_func (const void *first, const void *second) {
	if (*(unsigned short *)first < *(unsigned short *)second)
		return -1;
	else if (*(unsigned short *)first > *(unsigned short *)second)
		return 1;
	return 0;
}

float CalcSlope(ArrayOfDbl& y) {
// Does a linear regression to calculate the slope
	int x, size;
	double s_xy, s_x, s_y, s_xx, nvalid;
	double retval;
	size = (int) y.GetCount();
	nvalid = 0.0;
	s_xy = 0.0;
	s_xx = 0.0;
	s_x = 0.0;
	s_y = 0.0;

	for (x=1; x<=size; x++) {
		if (1) {
			nvalid = nvalid + 1;
			s_xy = s_xy + (double) x * y[x-1];
			s_x = s_x + (double) x;
			s_y = s_y + y[x-1];
			s_xx = s_xx + (double) x * (double) x;
		}
	}

	retval = (nvalid * s_xy - (s_x * s_y)) / (nvalid * s_xx - (s_x * s_x));
	return (float) retval;
}

//
bool QuickLRecon(usImage& img) {
	// Does a simple debayer of luminance data only -- sliding 2x2 window
	usImage Limg;
	int x, y;
	int xsize, ysize;
	unsigned short *ptr0, *ptr1;

	xsize = img.Size.GetWidth();
	ysize = img.Size.GetHeight();
	if (Limg.Init(xsize,ysize)) {
		(void) wxMessageBox(wxT("Memory allocation error"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	for (y=0; y<ysize-1; y++) {
		for (x=0; x<xsize-1; x++) {
			Limg.ImageData[x+y*xsize] = (img.ImageData[x+y*xsize] + img.ImageData[x+1+y*xsize] + img.ImageData[x+(y+1)*xsize] + img.ImageData[x+1+(y+1)*xsize]) / 4;
		}
		Limg.ImageData[x+y*xsize]=Limg.ImageData[(x-1)+y*xsize];  // Last one in this row -- just duplicate
	}
	for (x=0; x<xsize; x++)
		Limg.ImageData[x+(ysize-1)*xsize]=Limg.ImageData[x+(ysize-2)*xsize];  // Last row -- just duplicate
	ptr0=img.ImageData;
	ptr1=Limg.ImageData;
	for (x=0; x<img.NPixels; x++, ptr0++, ptr1++)
		*ptr0=(*ptr1);
	//delete Limg;
	Limg.Init(0,0);

	return false;
}

bool Median3(usImage& img) {
	usImage Limg;
	int x, y;
	int xsize, ysize;
	unsigned short *ptr0, *ptr1;
	unsigned short array[9];

	xsize = img.Size.GetWidth();
	ysize = img.Size.GetHeight();
	if (Limg.Init(xsize,ysize)) {
		(void) wxMessageBox(wxT("Memory allocation error"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	for (y=1; y<ysize-1; y++) {
		for (x=1; x<xsize-1; x++) {
			array[0] = img.ImageData[(x-1)+(y-1)*xsize];
			array[1] = img.ImageData[(x)+(y-1)*xsize];
			array[2] = img.ImageData[(x+1)+(y-1)*xsize];
			array[3] = img.ImageData[(x-1)+(y)*xsize];
			array[4] = img.ImageData[(x)+(y)*xsize];
			array[5] = img.ImageData[(x+1)+(y)*xsize];
			array[6] = img.ImageData[(x-1)+(y+1)*xsize];
			array[7] = img.ImageData[(x)+(y+1)*xsize];
			array[8] = img.ImageData[(x+1)+(y+1)*xsize];
			qsort(array,9,sizeof(unsigned short),us_sort_func);
			Limg.ImageData[x+y*xsize] = array[4];
		}
		Limg.ImageData[(xsize-1)+y*xsize]=img.ImageData[(xsize-1)+y*xsize];  // 1st & Last one in this row -- just grab from orig
		Limg.ImageData[y*xsize]=img.ImageData[y*xsize];
	}
	for (x=0; x<xsize; x++) {
		Limg.ImageData[x+(ysize-1)*xsize]=img.ImageData[x+(ysize-1)*xsize];  // Last row -- just duplicate
		Limg.ImageData[x]=img.ImageData[x];  // First row
	}
	ptr0=img.ImageData;
	ptr1=Limg.ImageData;
	for (x=0; x<img.NPixels; x++, ptr0++, ptr1++)
		*ptr0=(*ptr1);
	//delete Limg;
	Limg.Init(0,0);

	return false;
}

void FindStar(usImage& img) {

	// ADD BIT ABOUT USING PREVIOUS DELTA TO FIGURE NEW STARTING SPOT TO LOOK - -maybe have this be an option
	unsigned short *dataptr;
	int x, y, searchsize;
	int base_x, base_y;  // expected position in image (potentially cropped) coordinates
	double mass, mx, my, val;
	int start_x, start_y, rowsize;
	unsigned long lval, maxlval, mean;
	unsigned short max, nearmax1, nearmax2, sval, localmin;

	if ((StarX <= SearchRegion) || (StarY <= SearchRegion) ||
		(StarX >= (img.Size.GetWidth() - SearchRegion)) || (StarY >= (img.Size.GetHeight() - SearchRegion))) {
		FoundStar = false;
		return;
	}

	LastdX = dX; // Save the previous motion
	LastdY = dY;

	base_x = (int) StarX ;
	base_y = (int) StarY;

	dataptr = img.ImageData;
	rowsize = img.Size.GetWidth();
	searchsize = SearchRegion * 2 + 1;
	maxlval = nearmax1 = nearmax2 = max = 0;
	start_x = base_x - SearchRegion; // u-left corner of local area
	start_y = base_y - SearchRegion;
	mean=0;

	// figure the local offset
	localmin = 65535;
//	localmin = 0;
	double localmean = 0.0;
	for (y=0; y<searchsize; y++) {
		for (x=0; x<searchsize; x++) {
			if (*(dataptr + (start_x + x) + rowsize * (start_y + y-1)) < localmin)
				localmin = *(dataptr + (start_x + x) + rowsize * (start_y + y-1));
//			localmin += *(dataptr + (start_x + x) + rowsize * (start_y + y-1));
			localmean = localmean + (double)  *(dataptr + (start_x + x) + rowsize * (start_y + y-1));

		}
	}
//	localmin = localmin / (searchsize*searchsize);
	localmean = localmean / (double) (searchsize * searchsize);
	// get rough guess on star's location
	for (y=0; y<searchsize; y++) {
		for (x=0; x<searchsize; x++) {
			lval = *(dataptr + (start_x + x) + rowsize * (start_y + y)) +  // combine adjacent pixels to smooth image
				*(dataptr + (start_x + x+1) + rowsize * (start_y + y)) +		// find max of this smoothed area and set
				*(dataptr + (start_x + x-1) + rowsize * (start_y + y)) +		// base_x and y to be this spot
				*(dataptr + (start_x + x) + rowsize * (start_y + y+1)) +
				*(dataptr + (start_x + x) + rowsize * (start_y + y-1)) +
				*(dataptr + (start_x + x) + rowsize * (start_y + y));  // weigh current pixel by 2x
			if (lval >= maxlval) {
				base_x = start_x + x;
				base_y = start_y + y;
				maxlval = lval;
			}
			sval = *(dataptr + (start_x + x) + rowsize * (start_y + y)) -localmin;
			if ( sval > max) {
				nearmax2 = nearmax1;
				nearmax1 = max;
				max = sval;
			}
			mean = mean + sval;
		}
	}
	mean = mean / (searchsize * searchsize);
	frame->SetStatusText(_T(""),1);
	if ((frame->canvas->State == STATE_SELECTED) && (nearmax1 == nearmax2) && (nearmax1 == maxlval)) { // alert user that this is not the best star
//		wxMessageBox(wxString::Format("This star appears to be saturated and will lead to sub-optimal\nguiding results.  You may wish to select another star, decrease\nexposure duration or decrease camera gain"));
		frame->SetStatusText(_T("SATURATED STAR"),1);
	}
	// should be close now, hone in
	//start_x = base_x - 5; // u-left corner of local area
	//start_y = base_y - 5;
	int ft_range = 15; // must be odd
	StarMass = mass;
	if ((mass < 50.0) || // this is ~1 at each pixel -- likely dropped frame
//		(max > (5*nearmax1)) || // likely hot pixel
		(SNR < 3.0) ) { // maxlval is already the sum of 6 pixels around hottest so this is the
		dX = 0.0;
		dY = 0.0;
		FoundStar=false;
//		frame->SetStatusText(_T("NO STAR"),1);
		if (mass < 50) frame->SetStatusText(wxString::Format(_T("NO STAR: %.1f"),mass),1);
//		else if (max > (5*nearmax1)) frame->SetStatusText(wxString::Format("HOT PIXEL: %u, %u",max, nearmax1),1);
		else if (SNR < 3.0) frame->SetStatusText(wxString::Format(_T("LOW SNR: %.1f"),SNR),1);
	}
	else {
		StarX = mx / mass;
		StarY = my / mass;
		dX = StarX - LockX;
		dY = StarY - LockY;
		FoundStar=true;
		if (max == nearmax2)
			frame->SetStatusText(_T("Star saturated"));
		else
			frame->SetStatusText(_T(""),1);
	}
//	frame->SetStatusText(wxString::Format("%dx%d: %.1f, %ld(%.1f), %ld, %ld",base_x,base_y,mass,mean,val,maxlval, nearmax2),1);
	CropX = StarX - (CROPXSIZE/2);
	CropY = StarY - (CROPYSIZE/2);
	if (CropX < 0) CropX = 0;
	else if ((CropX + CROPXSIZE) >= CurrentFullFrame.Size.GetWidth()) CropX = CurrentFullFrame.Size.GetWidth() - (CROPXSIZE + 1);
	if (CropY < 0) CropY = 0;
	else if ((CropY + CROPYSIZE) >= CurrentFullFrame.Size.GetHeight()) CropY = CurrentFullFrame.Size.GetHeight() - (CROPYSIZE + 1);

}

bool SquarePixels(usImage& img, float xsize, float ysize) {
	// Stretches one dimension to square up pixels
	int x,y;
	int newsize;
	usImage tempimg;
	unsigned short *ptr;
	unsigned short *optr;
	double ratio, oldposition;

	if (!img.ImageData)
		return true;
	if (xsize == ysize) return false;  // nothing to do

	// Copy the existing data
	if (tempimg.Init(img.Size.GetWidth(), img.Size.GetHeight())) {
		wxMessageBox(_T("Could not allocate enough memory"),_T("Error"),wxOK);
		return true;
	}
	ptr = tempimg.ImageData;
	optr = img.ImageData;
	for (x=0; x<img.NPixels; x++, ptr++, optr++) {
		*ptr=*optr;
	}

	float weight;
	int ind1, ind2, linesize;
	// if X > Y, when viewing stock, Y is unnaturally stretched, so stretch X to match
	if (xsize > ysize) {
		ratio = ysize / xsize;
		newsize = ROUND((float) tempimg.Size.GetWidth() * (1.0/ratio));  // make new image correct size
		img.Init(newsize,tempimg.Size.GetHeight());
		optr=img.ImageData;
		linesize = tempimg.Size.GetWidth();  // size of an original line
		for (y=0; y<img.Size.GetHeight(); y++) {
			for (x=0; x<newsize; x++, optr++) {
				oldposition = x * ratio;
				ind1 = (unsigned int) floor(oldposition);
				ind2 = (unsigned int) ceil(oldposition);
				if (ind2 > (tempimg.Size.GetWidth() - 1)) ind2 = tempimg.Size.GetWidth() - 1;
				weight = ceil(oldposition) - oldposition;
				*optr = (unsigned short) (((float) *(tempimg.ImageData + y*linesize + ind1) * weight) + ((float) *(tempimg.ImageData + y*linesize + ind1) * (1.0 - weight)));
			}
		}

	}
	return false;
}

bool Subtract(usImage& light, usImage& dark) {
	unsigned short *lptr;
	unsigned short *dptr;
	int i;

	if ((!light.ImageData) || (!dark.ImageData))
		return true;
	if (light.NPixels != dark.NPixels)
		return true;

	lptr = light.ImageData;
	dptr = dark.ImageData;

	for (i=0; i<light.NPixels; i++, lptr++, dptr++)
		if ((*lptr + 100) > *dptr)
			*lptr = *lptr + 100 - *dptr;
		else
			*lptr = 0;

	return false;
}

void AutoFindStar(usImage& img, int& xpos, int& ypos) {
	// returns x and y of best star or 0 in each if nothing good found
	float A, B1, B2, C1, C2, C3, D1, D2, D3;
//	int score, *scores;
	int x, y, i, linesize;
	unsigned short *uptr;

//	scores = new int[img.NPixels];
	linesize = img.Size.GetWidth();
	//	double PSF[6] = { 0.69, 0.37, 0.15, -0.1, -0.17, -0.26 };
	// A, B1, B2, C1, C2, C3, D1, D2, D3
	double PSF[14] = { 0.906, 0.584, 0.365, .117, .049, -0.05, -.064, -.074, -.094 };
	double mean;
	double PSF_fit;
	double BestPSF_fit = 0.0;
//	for (x=0; x<img.NPixels; x++)
//		scores[x] = 0;

	// OK, do seem to need to run 3x3 median first
	Median3(img);

	/* PSF Grid is:
		D3 D3 D3 D3 D3 D3 D3 D3 D3
		D3 D3 D3 D2 D1 D2 D3 D3 D3
		D3 D3 C3 C2 C1 C2 C3 D3 D3
		D3 D2 C2 B2 B1 B2 C2 D3 D3
		D3 D1 C1 B1 A  B1 C1 D1 D3
		D3 D2 C2 B2 B1 B2 C2 D3 D3
		D3 D3 C3 C2 C1 C2 C3 D3 D3
		D3 D3 D3 D2 D1 D2 D3 D3 D3
		D3 D3 D3 D3 D3 D3 D3 D3 D3

		1@A
		4@B1, B2, C1, and C3
		8@C2, D2
		48 * D3
		*/
	for (y=40; y<(img.Size.GetHeight()-40); y++) {
		for (x=40; x<(linesize-40); x++) {
//			score = 0;
			A =  (float) *(img.ImageData + linesize * y + x);
			B1 = (float) *(img.ImageData + linesize * (y-1) + x) + (float) *(img.ImageData + linesize * (y+1) + x) + (float) *(img.ImageData + linesize * y + (x + 1)) + (float) *(img.ImageData + linesize * y + (x-1));
			B2 = (float) *(img.ImageData + linesize * (y-1) + (x-1)) + (float) *(img.ImageData + linesize * (y+1) + (x+1)) + (float) *(img.ImageData + linesize * (y+1) + (x + 1)) + (float) *(img.ImageData + linesize * (y+1) + (x-1));
			C1 = (float) *(img.ImageData + linesize * (y-2) + x) + (float) *(img.ImageData + linesize * (y+2) + x) + (float) *(img.ImageData + linesize * y + (x + 2)) + (float) *(img.ImageData + linesize * y + (x-2));
			C2 = (float) *(img.ImageData + linesize * (y-2) + (x-1)) + (float) *(img.ImageData + linesize * (y-2) + (x+1)) + (float) *(img.ImageData + linesize * (y+2) + (x + 1)) + (float) *(img.ImageData + linesize * (y+2) + (x-1)) +
				(float) *(img.ImageData + linesize * (y-1) + (x-2)) + (float) *(img.ImageData + linesize * (y-1) + (x+2)) + (float) *(img.ImageData + linesize * (y+1) + (x + 2)) + (float) *(img.ImageData + linesize * (y+1) + (x-2));
			C3 = (float) *(img.ImageData + linesize * (y-2) + (x-2)) + (float) *(img.ImageData + linesize * (y+2) + (x+2)) + (float) *(img.ImageData + linesize * (y+2) + (x + 2)) + (float) *(img.ImageData + linesize * (y+2) + (x-2));
			D1 = (float) *(img.ImageData + linesize * (y-3) + x) + (float) *(img.ImageData + linesize * (y+3) + x) + (float) *(img.ImageData + linesize * y + (x + 3)) + (float) *(img.ImageData + linesize * y + (x-3));
			D2 = (float) *(img.ImageData + linesize * (y-3) + (x-1)) + (float) *(img.ImageData + linesize * (y-3) + (x+1)) + (float) *(img.ImageData + linesize * (y+3) + (x + 1)) + (float) *(img.ImageData + linesize * (y+3) + (x-1)) +
				(float) *(img.ImageData + linesize * (y-1) + (x-3)) + (float) *(img.ImageData + linesize * (y-1) + (x+3)) + (float) *(img.ImageData + linesize * (y+1) + (x + 3)) + (float) *(img.ImageData + linesize * (y+1) + (x-3));
			D3 = 0.0;
			uptr = img.ImageData + linesize * (y-4) + (x-4);
			for (i=0; i<9; i++, uptr++)
				D3 = D3 + *uptr;
			uptr = img.ImageData + linesize * (y-3) + (x-4);
			for (i=0; i<3; i++, uptr++)
				D3 = D3 + *uptr;
			uptr = uptr + 2;
			for (i=0; i<3; i++, uptr++)
				D3 = D3 + *uptr;
			D3 = D3 + (float) *(img.ImageData + linesize * (y-2) + (x-4)) + (float) *(img.ImageData + linesize * (y-2) + (x+4)) + (float) *(img.ImageData + linesize * (y-2) + (x-3)) + (float) *(img.ImageData + linesize * (y-2) + (x-3)) +
				(float) *(img.ImageData + linesize * (y+2) + (x-4)) + (float) *(img.ImageData + linesize * (y+2) + (x+4)) + (float) *(img.ImageData + linesize * (y+2) + (x - 3)) + (float) *(img.ImageData + linesize * (y+2) + (x-3)) +
				(float) *(img.ImageData + linesize * y + (x + 4)) + (float) *(img.ImageData + linesize * y + (x-4));

			uptr = img.ImageData + linesize * (y+4) + (x-4);
			for (i=0; i<9; i++, uptr++)
				D3 = D3 + *uptr;
			uptr = img.ImageData + linesize * (y+3) + (x-4);
			for (i=0; i<3; i++, uptr++)
				D3 = D3 + *uptr;
			uptr = uptr + 2;
			for (i=0; i<3; i++, uptr++)
				D3 = D3 + *uptr;

			mean = (A+B1+B2+C1+C2+C3+D1+D2+D3)/85.0;
			PSF_fit = PSF[0] * (A-mean) + PSF[1] * (B1 - 4.0*mean) + PSF[2] * (B2 - 4.0 * mean) +
				PSF[3] * (C1 - 4.0*mean) + PSF[4] * (C2 - 8.0*mean) + PSF[5] * (C3 - 4.0 * mean) +
				PSF[6] * (D1 - 4.0*mean) + PSF[7] * (D2 - 8.0*mean) + PSF[8] * (D3 - 48.0 * mean);


			if (PSF_fit > BestPSF_fit) {
				BestPSF_fit = PSF_fit;
				xpos = x;
				ypos = y;
			}

			/*			mean = (A + B1 + B2 + C1 + C2 + C3) / 25.0;
			PSF_fit = PSF[0] * (A-mean) + PSF[1] * (B1 - 4.0*mean) + PSF[2] * (B2 - 4.0 * mean) +
				PSF[3] * (C1 - 4.0*mean) + PSF[4] * (C2 - 8.0*mean) + PSF[5] * (C3 - 4.0 * mean);*/

	/*		score = (int) (100.0 * PSF_fit);

			if (PSF_fit > 0.0)
				scores[x+y*linesize] = (int) PSF_fit;
			else
				scores[x+y*linesize] = 0;
	*/
			//			if ( ((B1 + B2) / 8.0) < 0.3 * A) // Filter hot pixels
			//				scores[x+y*linesize] = -1.0;



		}
	}
/*	score = 0;
	for (x=0; x<img.NPixels; x++) {
//		img.ImageData[x] = (unsigned short) scores[x];
		if (scores[x] > score) {
			score = scores[x];
			ypos = x / linesize;
			xpos = x - (ypos * linesize);
		}
	}
*/


}