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



static const struct v4l2_file_operations my_vivid_fops = {
	.owner = THIS_MODULE,
};


static void my_vivid_release(struct video_device *vdev)
{

}

static const struct v4l2_ioctl_ops my_vivid_ioctl_ops =
{
    // // 表示它是一个摄像头设备
    // .vidioc_querycap          = my_vivid_vidioc_querycap,
    // /* 用于列举、获得、测试、设置摄像头的数据的格式 */
    // .vidioc_enum_fmt_vid_cap  = my_vivid_vidioc_enum_fmt_vid_cap,
    // .vidioc_g_fmt_vid_cap     = my_vivid_vidioc_g_fmt_vid_cap,
    // .vidioc_try_fmt_vid_cap   = my_vivid_vidioc_try_fmt_vid_cap,
    // .vidioc_s_fmt_vid_cap     = my_vivid_vidioc_s_fmt_vid_cap,
    // /* 缓冲区操作: 申请/查询/放入队列/取出队列 */
    // .vidioc_reqbufs       = my_vivid_vidioc_reqbufs,
    // .vidioc_querybuf      = my_vivid_vidioc_querybuf,
    // .vidioc_qbuf          = my_vivid_vidioc_qbuf,
    // .vidioc_dqbuf         = my_vivid_vidioc_dqbuf,
    // // 启动/停止
    // .vidioc_streamon      = my_vivid_vidioc_streamon,
    // .vidioc_streamoff     = my_vivid_vidioc_streamoff,
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
    //myvivid_device->ioctl_ops = &my_vivid_ioctl_ops;
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