/*
  Raspberry Pi camera capture library using V4L2.

  2020-02-01  T. Nakagawa
*/

#ifndef CAMERA_H_
#define CAMERA_H_

#define CHECK(cond, msg) do {if (!(cond)) {fprintf(stderr, "ERROR(%s:%d): %s\n", __FILE__, __LINE__, (msg)); exit(1);}} while (false);

#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

class Camera {
 public:
  Camera(const std::string &device, int width, int height) : width_(width), height_(height) {
    fd_ = open(device.c_str(), O_RDWR);
    CHECK(fd_ >= 0, "Cannot open the video device.");

    struct v4l2_capability cap = {0};
    CHECK(ioctl(fd_, VIDIOC_QUERYCAP, &cap) >= 0, "No capability.");

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width_;
    fmt.fmt.pix.height = height_;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    CHECK(ioctl(fd_, VIDIOC_S_FMT, &fmt) >= 0, "Failed to set a format.");

    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    CHECK(ioctl(fd_, VIDIOC_REQBUFS, &req) >= 0, "Failed to request buffers.");

    struct v4l2_buffer buf = {0};
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = 0;
    CHECK(ioctl(fd_, VIDIOC_QUERYBUF, &buf) >= 0, "Failed to query a buffer.");
    buffer_ = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, fd_, buf.m.offset);
    length_ = buf.length;

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    CHECK(ioctl(fd_, VIDIOC_STREAMON, &type) >= 0, "Failed to stream.");
  }

  ~Camera() {
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd_, VIDIOC_STREAMOFF, &type);

    munmap(buffer_, length_);
    close(fd_);
  }

  void Configure(const std::string &config) {  // config="<shutter_speed_in_100us>"
    const int shutter_speed = std::stoi(config);
    struct v4l2_control ctrl = {0};
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ctrl.value = V4L2_EXPOSURE_MANUAL;
    CHECK(ioctl(fd_, VIDIOC_S_CTRL, &ctrl) >= 0, "Failed to set control.");
    ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    ctrl.value = shutter_speed;
    CHECK(ioctl(fd_, VIDIOC_S_CTRL, &ctrl) >= 0, "Failed to set control.");
  }

  cv::Mat Capture() {
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    CHECK(ioctl(fd_, VIDIOC_QBUF, &buf) >= 0, "Failed to query a buffer.");

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd_, &fds);
    CHECK(select(fd_ + 1, &fds, NULL, NULL, NULL) >= 0, "Failed to select.");

    CHECK(ioctl(fd_, VIDIOC_DQBUF, &buf) >= 0, "Failed to dequeue a buffer.");
    const int width = (int)buf.bytesused / 3 / height_;  // Effective piexels.
    CHECK((int)buf.bytesused == 3 * width * height_, "Invalid buffer size.");
    const cv::Mat image(height_, width, CV_8UC3, buffer_);
    return image(cv::Rect(0, 0, width_, height_));  // Clipping.
  }

 private:
  const int width_;
  const int height_;
  int fd_;
  void *buffer_;
  size_t length_;
};

#undef CHECK

#endif
