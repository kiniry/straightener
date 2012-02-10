/*
 * A basic Hough transform line detection Python module that operates on
 * an inverted binary map represented in a NumPy 2d matrix.
 *
 * Adapted from the openCV implementation of a Hough transform:
 */

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef LINEDETECTMODULE_H_
#define LINEDETECTMODULE_H_

#define PI 3.1415926535897932384626433832795
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct CvMat
{
    int type;
    int step;

    unsigned char *data;

    int rows;
    int cols;
} CvMat;

static PyObject* lineDetect_findLines(PyObject *self, PyObject *args);
static PyObject* houghTransform(const CvMat* img, float rho, float theta,
                       int threshold, float window, float adjustment);

int quickSortPartition(int* toSort, int* values, int left, int right, int pivotIndex);
void quickSort(int* toSort, int* values, int left, int right);
double round(double num);

#endif /* LINEDETECTMODULE_H_ */
