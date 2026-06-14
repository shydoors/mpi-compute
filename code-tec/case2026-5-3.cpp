#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

int main() {
  // 1. 读取图像
  // 请确保当前目录下有 "test.jpg" 或者修改为您自己的图片路径
  String imagePath = "C:/teaching/test1.jpg";
  Mat src = imread(imagePath);

  if (src.empty()) {
    cout << "无法加载图像，请检查路径：" << imagePath << endl;
    return -1;
  }

  // 2. 转换为灰度图
  // 边缘检测通常在单通道灰度图上进行，计算量更小且效果更清晰
  Mat gray;
  cvtColor(src, gray, COLOR_BGR2GRAY);

  // 3. 定义卷积核 (这里使用 Sobel 算子)
  // 水平方向边缘检测核 (检测垂直边缘)
  Mat kernelX = (Mat_<int>(3, 3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);

  // 垂直方向边缘检测核 (检测水平边缘)
  Mat kernelY = (Mat_<int>(3, 3) << -1, -2, -1, 0, 0, 0, 1, 2, 1);

  // 4. 执行卷积操作 (filter2D)
  // 输出深度设为 CV_16S，因为卷积结果可能包含负数或超过 255 的值
  Mat gradX, gradY;
  filter2D(gray, gradX, CV_16S, kernelX, Point(-1, -1), 0, BORDER_DEFAULT);
  filter2D(gray, gradY, CV_16S, kernelY, Point(-1, -1), 0, BORDER_DEFAULT);

  // 5. 取绝对值并转换为 8 位图像以便显示
  Mat absX, absY;
  convertScaleAbs(gradX, absX);
  convertScaleAbs(gradY, absY);

  // 6. 合并两个方向的边缘 (加权平均)
  Mat dst;
  addWeighted(absX, 0.5, absY, 0.5, 0, dst);

  // 7. 显示结果
  imshow("Original Image", src);
  imshow("Edge Detection (Convolution)", dst);

  // 等待按键，按任意键退出
  waitKey(0);

  return 0;
}
