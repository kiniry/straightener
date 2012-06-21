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

#include <Python.h>
#include <numpy/arrayobject.h>
#include "lineDetectmodule.h"
#include <math.h>

static PyObject *LineDetectError;

static PyMethodDef LineDetectMethods[] = {
    {"findLines", (PyCFunction)lineDetect_findLines,  METH_VARARGS,
        PyDoc_STR("findLines(binaryMap, rho, theta, threshold, window, adjustment) -> [(rho, theta),...]")},
    {NULL,              NULL}           /* sentinel */
};

static PyObject* lineDetect_findLines(PyObject *self, PyObject *args) {
    float rho, theta, window, adjustment;
    int threshold;
    PyArrayObject *imgArray;
    PyObject *result;
    CvMat *imgMat;

    if (!PyArg_ParseTuple(args, "O!ffiff", &PyArray_Type, &imgArray, &rho, &theta,
                          &threshold, &window, &adjustment)) {
        return NULL;
    }

    imgArray = (PyArrayObject*) PyArray_Cast(imgArray, NPY_UBYTE);
    imgMat = (CvMat*) malloc(sizeof(CvMat));
    imgMat->rows = imgArray->dimensions[0];
    imgMat->cols = imgMat->step = imgArray->dimensions[1];
    imgMat->data = (unsigned char*) imgArray->data;

    result = houghTransform(imgMat, rho, theta, threshold, window, adjustment);
    free(imgMat);
    /* According to this link, PyArray_Cast creates a new object:
     *    http://docs.scipy.org/doc/numpy/reference/c-api.array.html 
     * So, remember to decrement the refcount for imgArray so that it
     * can be garbage collected. */
    Py_DECREF(imgArray);
    
    return result;
}

/*
 * Parameters:
 *   - img: an inverted binary map
 *   - rho: Distance resolution in pixel-related units
 *   - theta: Angle resolution in radians
 *   - threshold: Minimum accumulator value
 *   - window: Maximum angle from the vertical/horizontal to search for
 *   - adjustment: Shift from the vertical/horizontal
 */

static PyObject* houghTransform(const CvMat* img, float rho, float theta,
                       int threshold, float window, float adjustment) {
    int *accum, *sort_buf;
    float *tabSin, *tabCos;

    const unsigned char* image;
    int step, width, height;
    int numangle, numrho;
    int total = 0;
    float ang, linerho, lineangle;
    int r, n, idx, base;
    int i, j;
    float irho = 1 / rho;
    double scale;
    float *angles;
    float bottomPoint, topPoint, midPoint;
    float bottomStart, bottomEnd, topStart, topEnd;
    float bottomWindow, topWindow;
    PyObject* tuple = NULL;
    PyObject* lines = PyList_New(0);

    if (lines == NULL) {
      PyErr_SetString(LineDetectError, "Cannot create new list.");
      return NULL;
    }

    adjustment = -adjustment;

    if (adjustment < 0) {
      bottomPoint = 0.0;
      topPoint = PI + adjustment;
      topStart = topPoint - window;
      topEnd = MIN(PI, topPoint + window);
      bottomStart = 0;
      bottomEnd = MAX(0.0, topPoint + window - PI);
    } else {
      bottomPoint = adjustment;
      topPoint = PI;
      bottomStart = MAX(0, bottomPoint - window);
      bottomEnd = bottomPoint + window;
      topEnd = PI;
      topStart = MIN(PI, PI + (bottomPoint - window));
    }

    topWindow = (topEnd - topStart);
    bottomWindow = (bottomEnd - bottomStart);

    midPoint = PI/2 + adjustment;

    image = img->data;
    step = img->step;
    width = img->cols;
    height = img->rows;

    numangle = 2 * ceil(window / theta) + ceil(bottomWindow / theta) + ceil(topWindow / theta);

    angles = (float*) malloc (sizeof(float) * numangle);

    i = 0;
    for (ang = bottomStart, n = 0; n < ceil(bottomWindow / theta); ang += theta, n++, i++) {
        angles[i] = ang;
    }

    for (ang = midPoint, n = 0; n < ceil(window / theta); ang -= theta, n++, i++) {
        angles[i] = ang;
    }

    for (ang = midPoint, n = 0; n < ceil(window / theta); ang += theta, n++, i++) {
      angles[i] = ang;
    }

    for (ang = topEnd, n = 0; n < ceil(topWindow / theta); ang -= theta, n++, i++) {
        angles[i] = ang;
    }

    numrho = ceil(((width + height) * 2 + 1) / rho);

    accum = (int*) malloc(sizeof(int) * ((numangle+2) * (numrho+2)));
    sort_buf = (int*) malloc(sizeof(int) * (numangle * numrho));
    tabSin = (float*) malloc(sizeof(float) * numangle);
    tabCos = (float*) malloc(sizeof(float) * numangle);

    memset(accum, 0, sizeof(accum[0]) * (numangle+2) * (numrho+2));

    for(n = 0; n < numangle; n++) {
        ang = angles[n];
        tabSin[n] = (float) (sin(ang) * irho);
        tabCos[n] = (float) (cos(ang) * irho);
    }

    // stage 1. fill accumulator
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (image[i * step + j] != 0)
                for (n = 0; n < numangle; n++) {
                    r = round(j * tabCos[n] + i * tabSin[n]);
                    r += (numrho - 1) / 2;
                    accum[(n+1) * (numrho+2) + r+1]++;
                }
        }
    }

    // stage 2. find local maximums
    for (r = 0; r < numrho; r++) {
        for (n = 0; n < numangle; n++) {
            base = (n+1) * (numrho+2) + r+1;

            if (accum[base] > threshold &&
                accum[base] > accum[base - 1] && accum[base] >= accum[base + 1] &&
                accum[base] > accum[base - numrho - 2] && accum[base] >= accum[base + numrho + 2]) {
              sort_buf[total++] = base;
            }
        }
    }

    // stage 3. sort the detected lines by accumulator value
    quickSort(sort_buf, accum, 0, total-1);

    // stage 4. build a python data structure containing the discovered lines
    scale = 1./(numrho+2);
    for (i = 0; i < total; i++) {
        idx = sort_buf[i];
        n = floor(idx * scale) - 1;
        r = idx - (n+1) * (numrho+2) - 1;
        linerho = (r - (numrho - 1)*0.5f) * rho;
        lineangle = angles[n];
        tuple = Py_BuildValue("(ff)", linerho, lineangle);
        if (PyList_Append(lines, tuple) == -1) {
          Py_DECREF(tuple);
          free(accum);
          free(sort_buf);
          free(tabSin);
          free(tabCos);
          free(angles);          
          return NULL;
        }
        Py_DECREF(tuple);
    }

    free(accum);
    free(sort_buf);
    free(tabSin);
    free(tabCos);
    free(angles);

    return lines;
}

PyMODINIT_FUNC initlineDetect(void) {
    PyObject *m;

    m = Py_InitModule("lineDetect", LineDetectMethods);
    if (m == NULL)
        return;
    import_array();

    LineDetectError = PyErr_NewException("lineDetect.error", NULL, NULL);
    //Py_INCREF(LineDetectError);
    PyModule_AddObject(m, "error", LineDetectError);
}

/*
 * Utils
 */

/*
 * An in-place quicksort implementation that sorts toSort using the corresponding
 * values in values in decreasing order:
 *    => values[toSort[n]] >= values[toSort[n+1]])
 * Taken straight from http://en.wikipedia.org/wiki/Quicksort
 */

int quickSortPartition(int* toSort, int* values, int left, int right, int pivotIndex) {
    int pivotValue, storeIndex;
    int i, tmp;

    pivotValue = values[toSort[pivotIndex]];
    // move pivot to the end
    tmp = toSort[pivotIndex];
    toSort[pivotIndex] = toSort[right];
    toSort[right] = tmp;

    storeIndex = left;
    for (i = left; i < right; i++) {
        if (values[toSort[i]] > pivotValue) {
            tmp = toSort[i];
            toSort[i] = toSort[storeIndex];
            toSort[storeIndex] = tmp;
            storeIndex++;
        }
    }
    // Move pivot to its final place
    tmp = toSort[storeIndex];
    toSort[storeIndex] = toSort[right];
    toSort[right] = tmp;

    return storeIndex;
}

void quickSort(int* toSort, int* values, int left, int right) {
    int pivotIndex;

    if (left < right) {
        pivotIndex = left + (right-left)/2;
        pivotIndex = quickSortPartition(toSort, values, left, right, pivotIndex);
        quickSort(toSort, values, left, pivotIndex - 1);
        quickSort(toSort, values, pivotIndex + 1, right);
    }
}

double round(double num) {
    double c = ceil(num);
    double f = floor(num);
    return c ? ((c - num) < (num - f)) : f;
}
