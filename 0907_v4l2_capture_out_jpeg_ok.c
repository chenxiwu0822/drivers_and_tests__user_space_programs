/*
Step 1: Open the Capture Device.
Step 2: Query the Capture
Step 3: Image Format
Step 4: Request Buffers
Step 5: Query Buffer
Step 6: Capture Image
Step 7: Store data in OpenCV datatype


在进行V4L2开发中，常用的命令标志符如下(some are optional)：
•    VIDIOC_REQBUFS：
			请求V4L2驱动分配视频缓冲区（申请V4L2视频驱动分配内存），V4L2是视频设备的驱动层，位于内核空间，
			所以通过VIDIOC_REQBUFS控制命令字申请的内存位于内核空间，应用程序不能直接访问，需要通过调
			用mmap内存映射函数把内核空间内存映射到用户空间后，应用程序通过访问用户空间地址来访问内核空间。
•    VIDIOC_QUERYBUF：
			查询已经分配的V4L2的视频缓冲区的相关信息，包括视频缓冲区的使用状态、在内核空间的偏移地址、
			缓冲区长度等。在应用程序设计中通过调VIDIOC_QUERYBUF来获取内核空间的视频缓冲区信息，然后调用
			函数mmap把内核空间地址映射到用户空间，这样应用程序才能够访问位于内核空间的视频缓冲区.
•    VIDIOC_QUERYCAP：	查询视频设备的功能
•    VIDIOC_ENUM_FMT：	获取当前视频设备支持的视频格式
•    VIDIOC_S_FMT：	设置视频设备的视频数据格式，例如设置视频图像数据的长、宽，图像格式（JPEG、YUYV格式）
•    VIDIOC_G_FMT：	读取当前驱动的视频捕获格式 
•    VIDIOC_TRY_FMT：	验证当前驱动的显示格式 
•    VIDIOC_CROPCAP：	查询驱动的修剪能力 
•    VIDIOC_S_CROP：	设置视频信号的边框 
•    VIDIOC_G_CROP：	读取视频信号的边框 

•    VIDIOC_QBUF： 	投放一个空的视频缓冲区到视频缓冲区输入队列中
•    VIDIOC_DQBUF：	第二个D是删除的意思
		   	从视频缓冲区的输出队列中取得一个已经保存有一帧视频数据的视频缓冲区

•    VIDIOC_STREAMON：	启动视频采集命令，应用程序调用VIDIOC_STREAMON启动视频采集命令后，
			视频设备驱动程序开始采集视频数据，并把采集到的视频数据保存到视频驱动的视频缓冲区中 
•    VIDIOC_STREAMOFF：
			停止视频采集命令，应用程序调用VIDIOC_ STREAMOFF停止视频采集命令后，视频设备驱动程序
			不再采集视频数据。
•    VIDIOC_QUERYSTD：	检查当前视频设备支持的标准，例如PAL或NTSC。
*/

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define	MJPEG_FILE	"0907_1_1440x1440.jpg"
#define IMAGE_WIDTH	1440 //ok: 640,700,800,1024,1920
#define IMAGE_HEIGHT	1440 //ok: 480,500,600,768,1080
uint8_t *buffer;
int	size;

static int xioctl(int fd, int request, void *arg)
{
	int	r;

	do {
		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

static int capture_image(int fd)
{
	int	r;
	struct v4l2_buffer	buf = {0};
	fd_set	fds;
	struct timeval	tv = {0};
	FILE * fp;

	//投放一个空的视频缓冲区到视频缓冲区输入队列中	
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;
	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
		printf("%s: query buffer failure. exit.\n", __func__);
		return 1;
	}
	
	//启动视频采集命令，应用程序调用VIDIOC_STREAMON启动视频采集命令后，驱动程序开始采集视频数据，
	//并把采集到的视频数据保存到视频驱动的视频缓冲区中 (也就是说，用mmap出来的buffer也能看到了)
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type)) {
		printf("%s: start capture failure. exit.\n", __func__);
        	return 1;
	}


	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = 2;

	r = select(fd+1, &fds, NULL, NULL, &tv);
	if (-1 == r) {
		printf("%s: waiting for frame failure. exit.\n", __func__);
        	return 1;
	}

	//从视频缓冲区的输出队列中,取得一个已经保存有一帧视频数据的视频缓冲区
	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
		printf("%s: retrieving frame failure. exit.\n", __func__);	//Retriev: 取回
		return 1;
	}

	printf("%s: saving image and output to stdout...\n", __func__);

	fp = fopen(MJPEG_FILE, "wb");
	if(!fp){
		printf("open "MJPEG_FILE" error\n");
		return;
	}

	fwrite(buffer, size, 1, fp);

        if (-1 == munmap(buffer, size))
                printf("%s: munmap error.\n", __func__);
	return 0;
}

static int init_mmap(int fd)
{
	struct v4l2_requestbuffers	req = {0};
	struct v4l2_buffer		buf = {0};

	//请求V4L2驱动分配视频缓冲区（申请V4L2视频驱动分配内存），V4L2是视频设备的驱动层，位于内核空间。
	//所以申请的内存位于内核空间，应用程序不能直接访问，需要通过调用mmap内存映射
	//函数把内核空间内存映射到用户空间后，应用程序通过访问用户空间地址来访问内核空间。
	req.count = 1;	//
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		printf("%s: requesting buffer failure. exit.\n", __func__);
		return 1;
	}

	//查询已经分配的V4L2的视频缓冲区的相关信息，包括视频缓冲区的使用状态、在内核空间的偏移地址、缓冲区长度等。
	//获取内核空间的视频缓冲区信息，然后调mmap把内核空间地址映射到用户空间，这样app才能够访问内核空间的视频缓冲区.
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;
	if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
		printf("%s: querying buffer failure. exit.\n", __func__);
		return 1;
	}

	size = buf.length; 

	buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

	printf("%s: Length: %d	Address: %p\n", __func__, buf.length, buffer);
	printf("%s: Image Length: %d\n", __func__, buf.bytesused);

	return 0;
}

static int init_device(int fd)
{	
	struct v4l2_fmtdesc	fmtdesc = {0};	//
	struct v4l2_format	fmt = {0};
	char	fourcc[5] = {0};
	char	c,e;

	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	//获取当前视频设备支持的视频格式
        printf("  FMT : CE Desc\n--------------------\n");
        while (0 == xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))
        {
                strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
                c = fmtdesc.flags & 1? 'C' : ' ';
                e = fmtdesc.flags & 2? 'E' : ' ';
                printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
                fmtdesc.index++;
        }

	printf("--------------------\n");

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = IMAGE_WIDTH;
        fmt.fmt.pix.height = IMAGE_HEIGHT;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;	//
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;	//
          fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;	//
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	//设置视频设备的视频数据格式，例如设置视频图像数据的长、宽，图像格式（JPEG、YUYV格式）
	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
		printf("%s: setting pixel format failure. exit.\n", __func__);
		return 1;
	}
        strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
        printf( "Selected Camera Mode:\n"
                "  Width: %d\n"
                "  Height: %d\n"
                "  PixFmt: %s\n"
                "  Field: %d\n",
                fmt.fmt.pix.width,
                fmt.fmt.pix.height,
                fourcc,
                fmt.fmt.pix.field);

        return 0;
}

static int print_caps(int fd)
{
	struct v4l2_capability	caps = {0};
	struct v4l2_cropcap	cropcap = {0};
	
	//查询视频设备的功能
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps)) {
		printf("%s: querying capabilities failure. exit.\n", __func__);
		return 1;
	}
	printf( "Driver Caps:\n"
	       	"Driver:	%s\n"
		"Card:		%s\n"
		"Bus:		%s\n"
		"Version:	%d.%d\n"
		"Capabilities:	%08x\n",
		caps.driver,
		caps.card,
		caps.bus_info,
		(caps.version>>16)&&0xff,
		(caps.version>>24)&&0xff,
		caps.capabilities);

	//查询驱动的修剪能力
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		printf("%s: querying cropping capabilities ok.\n", __func__);
	} else {
		printf("%s: querying cropping capabilities failure...\n", __func__);
		//return 1;
	}
        printf( "Camera Cropping:\n"
                "  Bounds: %dx%d+%d+%d\n"	/*Bounds: 边界,范围 */
                "  Default: %dx%d+%d+%d\n"
                "  Aspect: %d/%d\n",		/*Aspect: 面,面貌,体,点,样子 */
						/*Defrect: 打歪 */
                cropcap.bounds.width, cropcap.bounds.height, cropcap.bounds.left, cropcap.bounds.top,
                cropcap.defrect.width, cropcap.defrect.height, cropcap.defrect.left, cropcap.defrect.top,
                cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);	

	return 0;
}

int main(int argc, char** argv)
{
	int	fd;
	int	i;

	fd = open("/dev/video0", O_RDWR);
	if (fd == -1) {
		printf("%s: open video device failure. exit.\n", __func__);
		return 1;
	}
	
	if (print_caps(fd))
		return 1;

	if (init_device(fd))
		return 1;

	if (init_mmap(fd))
		return 1;
	
	//for (i = 0; i < 5; i++) {
		if (capture_image(fd))
			return 1;
	//}

	close(fd);
	return 0;
}
