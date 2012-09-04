import sys, os, pdb, shutil, pickle, time, argparse, multiprocessing
import math, logging
from os.path import join as pathjoin

import straightener

"""
A script to handle straightening multiple images.
"""

def straighten_images_process(imgpaths, imgsdir, outdir, queue, imgsize):
    """
    A function (intended to be called from another process) that
    straightens all images in imgpaths.
    Input:
        list imgpaths
        str imgsdir: The root directory of the original image directory
        str outdir: The root directory of the output images
        obj queue: A Queue instance used for IPC. If any images fail to
                   be straightened, then a tuple will be put:
                       (1, IMGPATH, OUTPATH)
        tuple imgsize: If given, the size of the output images. Should
                       be (WIDTH, HEIGHT)
    """
    imgsdir = os.path.abspath(imgsdir)
    for imgpath in imgpaths:
        # do straighten
        imgpath = os.path.abspath(imgpath)
        prefix = os.path.normpath(os.path.commonprefix((imgsdir, imgpath)))
        if '/' != prefix[-1]:
            # commonprefix won't include the trailing '/' for directories
            prefix = prefix + '/'
        rel = os.path.normpath(imgpath[len(prefix):])
        outpath = pathjoin(outdir, rel)
        create_dirs(os.path.split(outpath)[0])
        outpath_png = os.path.splitext(outpath)[0] + '.png'
        try:
            straightener.straighten_image(imgpath, outpath_png, imgsize=imgsize)
        except Exception as e:
            queue.put((1, imgpath, outpath))
    queue.put(0)
    return 0

def create_dirs(*dirs):
    for dir in dirs:
        try:
            os.makedirs(dir)
        except:
            pass
    
def get_images_gen(imgsdir):
    for dirpath, dirnames, filenames in os.walk(imgsdir):
        for imgname in [f for f in filenames if is_image_ext(f)]:
            yield imgname
    raise StopIteration

def divy_images(imgsdir, num):
    count = 0
    result = []
    for dirpath, dirnames, filenames in os.walk(imgsdir):
        if count >= num:
            yield result
            result = []
            count = 0
        for imgname in [f for f in filenames if is_image_ext(f)]:
            if count >= num:
                yield result
                result = []
                count = 0
            result.append(pathjoin(dirpath, imgname))
            count +=1
    if result:
        yield result
    raise StopIteration
                
def spawn_jobs(imgsdir, outdir, num_imgs, queue, imgsize=None):
    n_procs = float(multiprocessing.cpu_count())
    print 'cpu count:', n_procs
    imgs_per_proc = int(math.ceil(num_imgs / n_procs))
    print 'cpu count: {0} total number of imgs: {1} imgs_per_proc: {2}'.format(n_procs, num_imgs, imgs_per_proc)
    pool = multiprocessing.Pool()
    num_subprocs = 0
    for i, imgpaths in enumerate(divy_images(imgsdir, imgs_per_proc)):
        if imgpaths:
            print 'Process {0} got {1} imgs'.format(i, len(imgpaths))
            foo = pool.apply_async(straighten_images_process, args=(imgpaths, imgsdir, outdir, queue, imgsize))
            num_subprocs += 1
    pool.close()
    pool.join()

    # Handle any errors that might have happened.
    num_done = 0
    num_errs = 0
    errfile = open('_straighten_errors.log', 'w')
    while num_done < num_subprocs:
        thing = queue.get()
        if thing == 0:
            num_done += 1
        else:
            # Failed to straighten this image.
            errcode, imgpath, outpath = thing
            print >>errfile, "{0}.) Failed to straighten: {1}.".format(num_errs, imgpath)
            num_errs += 1
    errfile.close()
    if num_errs:
        print "Number of fatal errors:", num_errs
        print "    More information can be found in: _straighten_errors.log"

def start_straightening(imgsdir, outdir, num_imgs, imgsize=None):
    """
    Kicks off the straightening by spawning a 'master' process which
    spawns child worker processes.
    """
    manager = multiprocessing.Manager()
    queue = manager.Queue()

    print "Spawning master process to start straightening images in", imgsdir

    p = multiprocessing.Process(target=spawn_jobs, args=(imgsdir, outdir, num_imgs, queue, imgsize))
    p.start()
    p.join()

    print "Finished straightening."

def is_there_image(dir):
    """
    Return True if there exists at least one image in this directory
    (will recursively search).
    """
    for dirpath, dirnames, filenames in os.walk(dir):
        if [f for f in filenames if is_image_ext(f)]:
            return True
    return False

def count_images(imgsdir):
    ct = 0
    for dirpath, dirnames, filenames in os.walk(imgsdir):
        ct += len([f for f in filenames if is_image_ext(f)])
    return ct

def is_image_ext(path):
    return os.path.splitext(path)[1].lower() in ('.png', '.jpg', '.jpeg',
                                                 '.tiff', '.tif', '.bmp')

def do_main():
    usage="python batch_straightener.py [-o OUTDIR] [-r RESIZE] [--size WIDTH HEIGHT] \
[-m MAXANGLE] [-g] [-d] IMGDIR"
    parser = argparse.ArgumentParser(usage=usage,
                                     description='Straightens a set of images.')

    parser.add_argument("-o", "--outdir",
                        dest="outdir", default="",
                        help="Output Directory")
    parser.add_argument("-r", "--resize-factor",
                        dest="resize", default=2.0, type=float,
                        help="Shrinking factor")
    parser.add_argument("--size", dest="imgsize", default=None,
                        nargs=2,
                        help="Make output images be of a given size \
by padding/cropping the output images appropriately.")
    parser.add_argument("-m", "--max-angle",
                        dest="maxAngle", default=4.0, type=float,
                        help="Maximum expected angle from the vertical/horizontal (in degrees)")
    parser.add_argument("-g", "--graph", action="store_true", dest="graph",
                      default=False, help="Graph the discovered lines")
    parser.add_argument("-d", "--debug", action="store_true", dest="debug",
                      default=False, help="Print debugging info")
    parser.add_argument("imgsdir", help="Input directory")

    args = parser.parse_args()
    
    imgsdir = args.imgsdir
    outdir = args.outdir
    resize = args.resize
    maxAngle = args.maxAngle
    imgsize = args.imgsize
    GRAPH = args.graph
    DEBUG = args.debug

    print "Counting images in {0}...".format(imgsdir)
    num_imgs = count_images(imgsdir)
    print "...Finished Counting images: there are {0} images.".format(num_imgs)

    if imgsize != None:
        imgsize = (int(imgsize[1]), int(imgsize[0]))

    print "Calling the start_straightening job..."
    start_straightening(imgsdir, outdir, num_imgs, imgsize=imgsize)

if __name__ == '__main__':
    do_main()

# 652 1480 madera
# 1968 3530 napa
# 1715 2847 orange
# 1280, 2104 marin   (58,242 ballots)  [this was for the voted ballots, which
#                                       we'll use for straightening the blank
#                                       ballots too]
# 1700 2200 slo11
# 2400 3840 santacruz
# 1744 2878 yolo
