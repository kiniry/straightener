# Dependencies (versions tentative) #
  * Python >= 2.7
> > Note: Python 2.6 is supported, but you'll have to install the argparse module. See the argparse module page on installation instructions: http://pypi.python.org/pypi/argparse
  * NumPy >= 1.4.1
  * OpenCV (with Python wrapper) >= 2.1 (?)

# Installation #

Grab a copy of the source via git.

Run
```
python setup.py install --user
```
to install the lineDetect module (Hough transform implementation).

Or, if you want to install it sitewide, run the following as root:
```
python setup.py install
```

# Usage #

To straighten a rotated image `foo.png`, invoke the program with
```
python straightener.py foo.png
```
The result will be written to `foo-unrotated.png`.

For help on all of the syntax and command-line arguments, run
```
python straightener.py -h
```

# Applicability #

There are some requirements on the input image:
  * Supported image formats: BMP, DIB, JPEG, JPG, JPE, PNG, PBM, PGM, PPM, SR, RAS, TIFF, TIF (anything openCV supports)

  * The image must contain at least one near-vertical or near-horizontal line whose length is >= 2/3 of the width of the image.  (The tool currently assumes width <= height; TODO: make the requirement to be 2/3 x min(width,height) or something like that.)

The tool is intended to be well-suited for straightening, for example, scans of ballots, forms, and other documents with several vertical or horizontal lines.  The tool is not likely to be useful for processing random photographs and other arbitrary images.