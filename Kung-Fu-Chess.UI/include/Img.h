#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <filesystem>

class Img {
public:
    Img();

    Img& read(const std::string& path,
              const std::pair<int, int>& size = {},
              bool keep_aspect = false,
              int interpolation = cv::INTER_AREA);

    void draw_on(Img& other_img, int x, int y);

    void put_text(const std::string& txt, int x, int y, double font_size,
                  const cv::Scalar& color = cv::Scalar(255, 255, 255, 255),
                  int thickness = 1);

    void draw_rectangle(int x, int y, int width, int height,
                         const cv::Scalar& color = cv::Scalar(255, 255, 255, 255),
                         int thickness = 1);

    void show();

    void resize(int width, int height, int interpolation = cv::INTER_LINEAR);

    // Deep copy (cv::Mat's own copy is shallow/shared).
    Img clone() const;

    const cv::Mat& get_mat() const { return img_; }

    bool is_loaded() const { return !img_.empty(); }

private:
    cv::Mat img_;
};
