/*
找到了L4V2 api官方测试程序,弄了半天总算有点明白,稍微做了改动。

这个程序是使用YUV格式保存,并且通过stdio输出,这样做的目的是方便重定向到下一个转换程序。
但是对于着急需要测试和入门学习的人来说...

所以,我将程序稍微修改了一下: 
	当只要求拍摄一幅图片时，就自动将其保存为bmp文件，这样就可以快速对摄像头进行测试。

此程序没有库的依赖，是最简单和轻量级的。
我的board上摄像头是video4，你也可以单独指定。
*/ 


/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 * 
 *  This program is provided with the V4L2 API
 *  see http://linuxtv.org/docs.php for more information
 *      http://www.linuxtv.org/downloads/v4l-dvb-apis/capture-example.html
 *  
 *  modify by shigh1005@gmail.com
 *
 *  example:
 *	./video_capture_example_output_bmp -d /dev/video0 -m -f -c 1
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include "v4l2grab.h"


#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define	BMP_FILE	"test.bmp"
//#define CONNECT(x,y)	x##y
#define	IMAGE_WIDTH	640 //640	//you can change this.
#define	IMAGE_HEIGHT	480 //480	//you can change this.

enum io_method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

struct buffer {
	void   *start;
	size_t  length;
};

static char            *dev_name;
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              force_format;
static int              frame_count = 70;


static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r;
        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

int yuyv_2_rgb888(unsigned char *inp, int size, unsigned char *outp)
{
	int	i,j;
	unsigned char y1,y2,u,v;
	int r1,g1,b1,r2,g2,b2;
	 
	for(i=0;i<480;i++) {
		for(j=0;j<320;j++) {
		y1 = *( inp + (i*320+j)*4);
		u  = *( inp + (i*320+j)*4 + 1);
		y2 = *( inp + (i*320+j)*4 + 2);
		v  = *( inp + (i*320+j)*4 + 3);
		 
		r1 = y1 + 1.042*(v-128);
		g1 = y1 - 0.34414*(u-128) - 0.71414*(v-128);
		b1 = y1 + 1.772*(u-128);
		 
		r2 = y2 + 1.042*(v-128);
		g2 = y2 - 0.34414*(u-128) - 0.71414*(v-128);
		b2 = y2 + 1.772*(u-128);
		 
		if(r1>255)
		r1 = 255;
		else if(r1<0)
		r1 = 0;
		 
		if(b1>255)
		b1 = 255;
		else if(b1<0)
		b1 = 0;
		 
		if(g1>255)
		g1 = 255;
		else if(g1<0)
		g1 = 0;
		 
		if(r2>255)
		r2 = 255;
		else if(r2<0)
		r2 = 0;
		 
		if(b2>255)
		b2 = 255;
		else if(b2<0)
		b2 = 0;
		 
		if(g2>255)
		g2 = 255;
		else if(g2<0)
		g2 = 0;
		 
		*(outp + ((480-1-i)*320+j)*6    ) = (unsigned char)b1;
		*(outp + ((480-1-i)*320+j)*6 + 1) = (unsigned char)g1;
		*(outp + ((480-1-i)*320+j)*6 + 2) = (unsigned char)r1;
		*(outp + ((480-1-i)*320+j)*6 + 3) = (unsigned char)b2;
		*(outp + ((480-1-i)*320+j)*6 + 4) = (unsigned char)g2;
		*(outp + ((480-1-i)*320+j)*6 + 5) = (unsigned char)r2;
		}
	}
	printf("\nChange YUV to RGB OK！\n");
}

void save_as_bmp(char *bufp, int size)
{
	FILE * fp1,* fp2;

	bit_map_file_header   bf;
	bit_map_info_header   bi;
	unsigned char output_buffer[IMAGE_WIDTH*IMAGE_HEIGHT*3];

	fp1 = fopen(BMP_FILE, "wb");
	if(!fp1){
		printf("open "BMP_FILE" error\n");
		return;
	}

	//Set bit_map_info_header
	bi.biSize = 40;
	bi.biWidth = IMAGE_WIDTH;
	bi.biHeight = IMAGE_HEIGHT;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = 0;
	bi.biSizeImage = IMAGE_WIDTH*IMAGE_HEIGHT*3;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	//Set bit_map_file_header
	bf.bfType = 0x4d42;
	bf.bfSize = 54 + bi.biSizeImage;    
	bf.bfReserved = 0;
	bf.bfOffBits = 54;


	// fwrite(buffers[0].start, IMAGE_WIDTH * IMAGE_HEIGHT * 2, 1, fp2);
	//printf("save "YUV"OK\n");

	yuyv_2_rgb888(bufp, size,output_buffer);
	fwrite(&bf, 14, 1, fp1);
	fwrite(&bi, 40, 1, fp1);   
	fwrite(output_buffer, bi.biSizeImage, 1, fp1);

	printf("Save to file "BMP_FILE" OK!\n");

	fclose(fp1);
}

static void process_image(const void *p, int size)
{
	if (out_buf)
		fwrite(p, size, 1, stdout);
		//save_as_bmp((char *)p, size);
        fflush(stderr);
        fprintf(stderr, "x");
        fflush(stdout);
}

static int read_frame(void)
{
        struct v4l2_buffer buf;
        unsigned int i;

        switch (io) {
		case IO_METHOD_READ:	//暂不使用
		        if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
				switch (errno) {
		                case EAGAIN:
		                        return 0;
		                case EIO:
		                        /* Could ignore EIO, see spec. */
		                        /* fall through */
		                default:
		                        errno_exit("read");
		                }
		        }
		        process_image(buffers[0].start, buffers[0].length);
		        break;

		case IO_METHOD_MMAP:	//this.
		        CLEAR(buf);

		        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		        buf.memory = V4L2_MEMORY_MMAP;

		        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
		                switch (errno) {
		                case EAGAIN:
		                        return 0;
		                case EIO:
		                        /* Could ignore EIO, see spec. */
		                        /* fall through */
		                default:
		                        errno_exit("VIDIOC_DQBUF");
		                }
		        }

		        assert(buf.index < n_buffers);

		        process_image(buffers[buf.index].start, buf.bytesused);

		        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		                errno_exit("VIDIOC_QBUF");
		        break;

		case IO_METHOD_USERPTR:	//暂不使用
		        CLEAR(buf);

		        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		        buf.memory = V4L2_MEMORY_USERPTR;

		        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
		                switch (errno) {
		                case EAGAIN:
		                        return 0;
		                case EIO:
		                        /* Could ignore EIO, see spec. */
		                        /* fall through */
		                default:
		                        errno_exit("VIDIOC_DQBUF");
		                }
		        }

		        for (i = 0; i < n_buffers; ++i)
		                if (buf.m.userptr == (unsigned long)buffers[i].start
		                    && buf.length == buffers[i].length)
		                        break;

		        assert(i < n_buffers);

		        process_image((void *)buf.m.userptr, buf.bytesused);

		        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		                errno_exit("VIDIOC_QBUF");
		        break;
        }

        return 1;	//
}

static void mainloop(void)
{
        unsigned int count;
	int photo=0;

        count = frame_count;

        while (count-- > 0) {
		for (;;) {
			fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;
                                errno_exit("select");
                        }

                        if (0 == r) {
                                fprintf(stderr, "select timeout\n");
                                exit(EXIT_FAILURE);
                        }

                        if (read_frame())
                                break;
                        /* EAGAIN - continue select loop. */
                }
		
		//其实每一帧都可以转
		//save_as_bmp(buffers[0].start, buffers[0].length);
	}

	if(frame_count == 1){	//change to bmp
		save_as_bmp(buffers[0].start, buffers[0].length);
	}
}

static void stop_capturing(void)
{
        enum v4l2_buf_type type;

        switch (io) {
		case IO_METHOD_READ:
		        /* Nothing to do. */
		        break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
		        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
		                errno_exit("VIDIOC_STREAMOFF");
		        break;
        }
}

static void start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) {
		case IO_METHOD_READ:	//暂不使用
		        /* Nothing to do. */
		        break;

		case IO_METHOD_MMAP:	//this
		        for (i = 0; i < n_buffers; ++i) {
		                struct v4l2_buffer buf;

		                CLEAR(buf);
		                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		                buf.memory = V4L2_MEMORY_MMAP;
		                buf.index = i;

		                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		                        errno_exit("VIDIOC_QBUF");
		        }

		        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		                errno_exit("VIDIOC_STREAMON");
		        break;

		case IO_METHOD_USERPTR:	//暂不使用
		        for (i = 0; i < n_buffers; ++i) {
		                struct v4l2_buffer buf;

		                CLEAR(buf);
		                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		                buf.memory = V4L2_MEMORY_USERPTR;
		                buf.index = i;
		                buf.m.userptr = (unsigned long)buffers[i].start;
		                buf.length = buffers[i].length;

		                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		                        errno_exit("VIDIOC_QBUF");
		        }
		        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		                errno_exit("VIDIOC_STREAMON");
		        break;
        }
}

static void uninit_device(void)
{
        unsigned int i;

        switch (io) {
		case IO_METHOD_READ:
		        free(buffers[0].start);
		        break;

		case IO_METHOD_MMAP:
		        for (i = 0; i < n_buffers; ++i)
		                if (-1 == munmap(buffers[i].start, buffers[i].length))
		                        errno_exit("munmap");
		        break;

		case IO_METHOD_USERPTR:
		        for (i = 0; i < n_buffers; ++i)
		                free(buffers[i].start);
		        break;
        }

        free(buffers);
}

static void init_read(unsigned int buffer_size)
{
        buffers = calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}

static void init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support memory mapping\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
                exit(EXIT_FAILURE);
        }

        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start = mmap(NULL /* start anywhere */,
					      buf.length,
					      PROT_READ | PROT_WRITE /* required */,
					      MAP_SHARED /* recommended */,
					      fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

static void init_userp(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support user pointer i/o\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        buffers = calloc(4, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = malloc(buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);
                }
        }
}

static void init_device(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n", dev_name);
                exit(EXIT_FAILURE);
        }

        switch (io) {
        case IO_METHOD_READ:
                if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                        fprintf(stderr, "%s does not support read i/o\n", dev_name);
                        exit(EXIT_FAILURE);
                }
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                        fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
                        exit(EXIT_FAILURE);
                }
                break;
        }


        /* Select video input, video standard and tune here. */

        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */


                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }

        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (force_format) {
                fmt.fmt.pix.width       = IMAGE_WIDTH;
                fmt.fmt.pix.height      = IMAGE_HEIGHT;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

                if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                /* Note VIDIOC_S_FMT may change width and height. */
        } else {
                /* Preserve original settings as set by v4l2-ctl for example */
                if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                        errno_exit("VIDIOC_G_FMT");
        }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        switch (io) {
        case IO_METHOD_READ:
                init_read(fmt.fmt.pix.sizeimage);
                break;
        case IO_METHOD_MMAP:
                init_mmap();
                break;
        case IO_METHOD_USERPTR:
                init_userp(fmt.fmt.pix.sizeimage);
                break;
        }
}

static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");
        fd = -1;
}

static void open_device(void)
{
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}
/*
example:
./video_capture_example_output_bmp -d /dev/video0 -r -f -c 1
./video_capture_example_output_bmp -d /dev/video0 -r -o -f -c 1
*/
static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\n\n"
                 "Version 1.3\n"
                 "Options:\n"
                 "-d | --device name   Video device name [%s]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers [default]\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
                 "-o | --output        Outputs stream to stdout\n"
                 "-f | --format        Force format to IMAGE_WIDTH x IMAGE_HEIGHT YUYV\n"
                 "-c | --count         Number of frames to grab [%i]\n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruofc:";

static const struct option long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};


int main(int argc, char **argv)
{
        dev_name = "/dev/video0";

        for (;;) {
                int idx;
                int c;

                c = getopt_long(argc, argv, short_options, long_options, &idx);

                if (-1 == c)
                        break;

                switch (c) {
		        case 0: /* getopt_long() flag */
		                break;
		        case 'd':
		                dev_name = optarg;
		                break;
		        case 'h':
		                usage(stdout, argc, argv);
		                exit(EXIT_SUCCESS);
		        case 'm':
		                io = IO_METHOD_MMAP;
		                break;
		        case 'r':
		                io = IO_METHOD_READ;
		                break;
		        case 'u':
		                io = IO_METHOD_USERPTR;
		                break;
		        case 'o':
		                out_buf++;
		                break;
		        case 'f':
		                force_format++;
		                break;
		        case 'c':
		                errno = 0;
		                frame_count = strtol(optarg, NULL, 0);
		                if (errno)
		                        errno_exit(optarg);
		                break;
		        default:
		                usage(stderr, argc, argv);
		                exit(EXIT_FAILURE);
                }
        }

        open_device();
        init_device();
        start_capturing();
        mainloop();
        stop_capturing();
        uninit_device();
        close_device();
        fprintf(stderr, "\n");
        return 0;
}

