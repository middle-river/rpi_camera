#include "camera_mmal.h"
#include <opencv2/imgcodecs.hpp>

int main(void) {
  Camera cam(3280, 2464);

  cam.Configure("100000 1.0 1.0");
  const cv::Mat m = cam.Capture();
  cv::imwrite("output.jpg", m);
  return 0;
}
