#include "Img.h"

#include <stdexcept>
#include <vector>

Img::Img() {
}

Img& Img::read(const std::string& path,
               const std::pair<int, int>& size,
               bool keep_aspect,
               int interpolation) {
    img_ = cv::imread(path, cv::IMREAD_UNCHANGED);
    if (img_.empty()) {
        throw std::runtime_error("Cannot load image: " + path);
    }

    if (size.first != 0 && size.second != 0) {
        int target_w = size.first;
        int target_h = size.second;
        int h = img_.rows;
        int w = img_.cols;

        if (keep_aspect) {
            double scale = std::min(static_cast<double>(target_w) / w,
                                     static_cast<double>(target_h) / h);
            int new_w = static_cast<int>(w * scale);
            int new_h = static_cast<int>(h * scale);
            cv::resize(img_, img_, cv::Size(new_w, new_h), 0, 0, interpolation);
        } else {
            cv::resize(img_, img_, cv::Size(target_w, target_h), 0, 0, interpolation);
        }
    }

    return *this;
}

void Img::draw_on(Img& other_img, int x, int y) {
    if (img_.empty() || other_img.img_.empty()) {
        throw std::runtime_error("Both images must be loaded before drawing.");
    }

    cv::Mat source_img = img_;
    cv::Mat target_img = other_img.img_;

    if (source_img.channels() != target_img.channels()) {
        if (source_img.channels() == 3 && target_img.channels() == 4) {
            cv::cvtColor(source_img, source_img, cv::COLOR_BGR2BGRA);
        } else if (source_img.channels() == 4 && target_img.channels() == 3) {
            cv::cvtColor(source_img, source_img, cv::COLOR_BGRA2BGR);
        }
    }

    int h = source_img.rows;
    int w = source_img.cols;
    int H = target_img.rows;
    int W = target_img.cols;

    if (y + h > H || x + w > W) {
        throw std::runtime_error("Image does not fit at the specified position.");
    }

    cv::Mat roi = target_img(cv::Rect(x, y, w, h));

    if (source_img.channels() == 4) {
        std::vector<cv::Mat> src_channels;
        cv::split(source_img, src_channels);

        cv::Mat alpha;
        src_channels[3].convertTo(alpha, CV_32F, 1.0 / 255.0);

        std::vector<cv::Mat> roi_channels;
        cv::split(roi, roi_channels);

        for (int c = 0; c < 3; ++c) {
            cv::Mat src_f;
            cv::Mat dst_f;
            src_channels[c].convertTo(src_f, CV_32F);
            roi_channels[c].convertTo(dst_f, CV_32F);
            cv::Mat blended_f = dst_f.mul(1.0 - alpha) + src_f.mul(alpha);
            blended_f.convertTo(roi_channels[c], roi_channels[c].type());
        }

        cv::merge(roi_channels, roi);
    } else {
        source_img.copyTo(roi);
    }
}

void Img::put_text(const std::string& txt, int x, int y, double font_size,
                    const cv::Scalar& color, int thickness) {
    if (img_.empty()) {
        throw std::runtime_error("Image not loaded.");
    }

    cv::putText(img_, txt, cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, font_size,
                color, thickness, cv::LINE_AA);
}

void Img::draw_rectangle(int x, int y, int width, int height, const cv::Scalar& color, int thickness) {
    if (img_.empty()) {
        throw std::runtime_error("Image not loaded.");
    }

    cv::rectangle(img_, cv::Rect(x, y, width, height), color, thickness);
}

void Img::resize(int width, int height, int interpolation) {
    if (img_.empty()) {
        throw std::runtime_error("Image not loaded.");
    }

    cv::resize(img_, img_, cv::Size(width, height), 0, 0, interpolation);
}

Img Img::clone() const {
    Img copy;
    copy.img_ = img_.clone();
    return copy;
}

void Img::show() {
    if (img_.empty()) {
        throw std::runtime_error("Image not loaded.");
    }

    cv::imshow("Image", img_);
    cv::waitKey(0);
    cv::destroyAllWindows();
}
