#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/font.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/v4l2-dv-timings.h>
#include <media/videobuf2-vmalloc.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>
#include <media/videobuf-core.h>
#include <media/videobuf-vmalloc.h>

static struct video_device *my_vivid_device;
struct v4l2_device v4l2_dev;
struct v4l2_format my_vivid_format;

//应用程序通过ioctl()查询性能？看它是不是摄像头设备
static int my_vivid_vidioc_querycap(struct file *file, void  *priv,
					                        struct v4l2_capability *cap)
{
    printk("enter %s\n", __func__);
	strcpy(cap->driver, "myvivid");
	strcpy(cap->card, "myvivid");
    cap->version = 0x0001;
	//capabilities表明是视频的捕捉设备、V4L2_CAP_STREAMING表明可以通过ioctl读取数据
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	return 0;
}
//列举支持哪些格式（只支持一种格式YUYV）
int my_vivid_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	printk("enter %s\n", __func__);

	if (f->index >= 1)
        return -EINVAL;

	strcpy(f->description, "4:2:2, packed, YUYV");
    f->pixelformat = V4L2_PIX_FMT_YUYV;
    return 0;
}
//返回当前所使用的格式--》填充v4l2_format结构体成员
int my_vivid_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
    printk("enter %s\n", __func__);

    memcpy(f, &my_vivid_format, sizeof(my_vivid_format));
    return 0;
}
//测试驱动程序是否支持某种格式！
int my_vivid_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
    unsigned int maxw, maxh;
    enum v4l2_field field;
    printk("enter %s\n", __func__);
    //my_vivid_vidioc_enum_fmt_vid_cap里设置了只支持YUYV格式
    if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
        return -EINVAL;
    
    //不理解field干嘛的。。参考vivid
    field = f->fmt.pix.field;
    if (field == V4L2_FIELD_ANY)
    {
        field = V4L2_FIELD_INTERLACED;
    }
    else if (V4L2_FIELD_INTERLACED != field)
    {
        return -EINVAL;
    }
    maxw  = 1024; //参考vivid虚拟设备支持的分辨率
    maxh  = 768;
    /* 调整format的width, height,
     * 计算bytesperline, sizeimage
     */
    v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2,
                          &f->fmt.pix.height, 32, maxh, 0, 0);
    //16是颜色深度（一个颜色用16位）、 bytesperline每行占据字节数
    f->fmt.pix.bytesperline =
        (f->fmt.pix.width * 16) >> 3; 
    f->fmt.pix.sizeimage =
        f->fmt.pix.height * f->fmt.pix.bytesperline;
    return 0;
}
//设置设备的格式
int my_vivid_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
    int ret;
    printk("enter %s\n", __func__);
    //先测试vivid是否支持该格式！支持再设置
    ret = my_vivid_vidioc_try_fmt_vid_cap(file, NULL, f);
    if (ret < 0)
        return ret;
    //没有硬件、设置的格式保存在my_vivid_format
    memcpy(&my_vivid_format, f, sizeof(my_vivid_format));
    return ret;
}
static const struct v4l2_file_operations my_vivid_fops = {
	.owner = THIS_MODULE,
    .unlocked_ioctl = video_ioctl2,/* V4L2 ioctl handler */
};


static void my_vivid_release(struct video_device *vdev)
{

}

static const struct v4l2_ioctl_ops my_vivid_ioctl_ops =
{
    // 表示它是一个摄像头设备
    .vidioc_querycap          = my_vivid_vidioc_querycap,

    /* 用于列举、获得、测试、设置摄像头的数据的格式 */
    .vidioc_enum_fmt_vid_cap  = my_vivid_vidioc_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap     = my_vivid_vidioc_g_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap   = my_vivid_vidioc_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap     = my_vivid_vidioc_s_fmt_vid_cap,
#if 0
    /* 缓冲区操作: 申请/查询/放入队列/取出队列 */
    .vidioc_reqbufs       = my_vivid_vidioc_reqbufs,
    .vidioc_querybuf      = my_vivid_vidioc_querybuf,
    .vidioc_qbuf          = my_vivid_vidioc_qbuf,
    .vidioc_dqbuf         = my_vivid_vidioc_dqbuf,
    // 启动/停止
    .vidioc_streamon      = my_vivid_vidioc_streamon,
    .vidioc_streamoff     = my_vivid_vidioc_streamoff,
#endif
};

static int __init my_vivid_init(void)
{
    int error;
    /*1  分配一个video_device结构体*/
    my_vivid_device = video_device_alloc(); //kzalloc

    /*2  设置 */
    my_vivid_device->release = my_vivid_release;
    my_vivid_device->v4l2_dev  = &v4l2_dev;
    my_vivid_device->fops = &my_vivid_fops;
    my_vivid_device->ioctl_ops = &my_vivid_ioctl_ops;
    /*3  注册*/ 
    //type 参考vivid-core.c  -nr 参考函数说明 -1 表示第一个可用设备
    error = video_register_device(my_vivid_device,VFL_TYPE_GRABBER, -1);


    return error;
}

static void __exit my_vivid_exit(void)
{
    //注册--对应销毁
    video_unregister_device(my_vivid_device);
    //分配对应释放
    video_device_release(my_vivid_device);

}

module_init(my_vivid_init);
module_exit(my_vivid_exit);
MODULE_LICENSE("GPL");