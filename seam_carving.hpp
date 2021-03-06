#ifndef _SEAM_CARVING_HPP_
#define _SEAM_CARVING_HPP_

#include <vector>
#include <functional>
#include <algorithm>
#include <opencv2/opencv.hpp>

#include "weighted_value.h"

struct path_result
{
    std::vector<int> path;
    int total_energy;
};

typedef weighted_value<int, int> weighted_int_t;
typedef short energy_t;
constexpr energy_t energy_inf = 30000;
constexpr int energy_cv_type = CV_16S;

// find a horizontal seam whose energy is minimum
template <typename COMPARE_FUNC = std::less<int> >
path_result find_hori_seam(const cv::Mat &energy_image,
                           weighted_int_t *buffer = nullptr,
                           COMPARE_FUNC cmp = std::less<int>())
{
    weighted_int_t *dp;
    if (!buffer)
    {
        dp = new weighted_int_t[(energy_image.cols + 1) * energy_image.rows];
    }
    else
    {
        dp = buffer;
    }

    // first level: init
    for (int y = 1; y < energy_image.rows - 1; ++y)
    {
        energy_t e = energy_image.at<energy_t>(y, 0);
        dp[y] = weighted_int_t { e, -1 };
    }

    // dp
    weighted_int_t *last_level = dp, *current_level = dp + energy_image.rows;
    for (int x = 1; x < energy_image.cols; ++x)
    {
        for (int y = 1; y < energy_image.rows - 1; ++y)
        {
            weighted_int_t &e_min = current_level[y];

            // (x - 1, y)
            e_min.weight = last_level[y].weight;
            e_min.value = y;

            // (x - 1, y - 1)
            if (y - 1 >= 1 && cmp(last_level[y - 1].weight, e_min.weight))
            {
                e_min.weight = last_level[y - 1].weight;
                e_min.value = y - 1;
            }

            // (x - 1, y + 1)
            if (y + 1 < energy_image.rows - 1 && cmp(last_level[y + 1].weight, e_min.weight))
            {
                e_min.weight = last_level[y + 1].weight;
                e_min.value = y + 1;
            }

            energy_t e = energy_image.at<energy_t>(y, x);
            e_min.weight += e;
        }

        last_level += energy_image.rows;
        current_level += energy_image.rows;
    }

    for (int y = 1; y < energy_image.rows - 1; ++y)
    {
        weighted_int_t &e_min = current_level[y];

        // (x - 1, y)
        e_min.weight = last_level[y].weight;
        e_min.value = y;
    }

    // last level: find minimum.
    weighted_int_t *e_min;
    e_min = std::min_element(current_level + 1, current_level + (energy_image.rows - 1));

    // backtrace
    std::vector<int> energy_min_path(energy_image.cols);
    energy_min_path[energy_image.cols - 1] = e_min->value;
    for (int x = energy_image.cols - 2; x >= 0; --x)
    {
        energy_min_path[x] = last_level[energy_min_path[x + 1]].value;
        last_level -= energy_image.rows;
    }

    if (!buffer)
    {
        delete [] dp;
    }

    return path_result { std::move(energy_min_path), e_min->weight };
}

// find a vertical seam whose energy is minimum
template <typename COMPARE_FUNC = std::less<int> >
path_result find_vert_seam(const cv::Mat &energy_image,
                           weighted_int_t *buffer = nullptr,
                           COMPARE_FUNC cmp = std::less<int>())
{
    weighted_int_t *dp;
    if (!buffer)
    {
        dp = new weighted_int_t[(energy_image.rows + 1) * energy_image.cols];
    }
    else
    {
        dp = buffer;
    }

    // first level: init
    for (int x = 1; x < energy_image.cols - 1; ++x)
    {
        energy_t e = energy_image.at<energy_t>(0, x);
        dp[x] = weighted_int_t{ e, -1 };
    }

    // dp
    weighted_int_t *last_level = dp, *current_level = dp + energy_image.cols;
    for (int y = 1; y < energy_image.rows; ++y)
    {
        for (int x = 1; x < energy_image.cols - 1; ++x)
        {
            weighted_int_t &e_min = current_level[x];

            // (x, y - 1)
            e_min.weight = last_level[x].weight;
            e_min.value = x;

            // (x - 1, y - 1)
            if (x - 1 >= 1 && cmp(last_level[x - 1].weight, e_min.weight))
            {
                e_min.weight = last_level[x - 1].weight;
                e_min.value = x - 1;
            }

            // (x + 1, y - 1)
            if (x + 1 < energy_image.cols - 1 && cmp(last_level[x + 1].weight, e_min.weight))
            {
                e_min.weight = last_level[x + 1].weight;
                e_min.value = x + 1;
            }

            energy_t e = energy_image.at<energy_t>(y, x);
            e_min.weight += e;
        }

        last_level += energy_image.cols;
        current_level += energy_image.cols;
    }

    for (int x = 1; x < energy_image.cols - 1; ++x)
    {
        weighted_int_t &e_min = current_level[x];

        // (x, y - 1)
        e_min.weight = last_level[x].weight;
        e_min.value = x;
    }

    // last level: find minimum.
    weighted_int_t *e_min;
    e_min = std::min_element(current_level + 1, current_level + (energy_image.cols - 1));

    // backtrace
    std::vector<int> energy_min_path(energy_image.rows);
    energy_min_path[energy_image.rows - 1] = e_min->value;
    for (int y = energy_image.rows - 2; y >= 0; --y)
    {
        energy_min_path[y] = last_level[energy_min_path[y + 1]].value;
        last_level -= energy_image.cols;
    }

    if (!buffer)
    {
        delete[] dp;
    }

    return path_result{ std::move(energy_min_path), e_min->weight };
}

template <std::size_t CHANNELS, typename TCHANNEL>
cv::Mat remove_path_hori(const cv::Mat &image, const std::vector<int> &path)
{
    cv::Mat result(image.rows - 1, image.cols, image.type());
    for (int y = 0; y < result.rows; ++y)
    {
        for (int x = 0; x < result.cols; ++x)
        {
            int img_y = y;
            if (img_y >= path[x])
            {
                ++img_y;
            }
            for (int c = 0; c < CHANNELS; ++c)
            {
                result.at<TCHANNEL>(y, CHANNELS * x + c) =
                    image.at<TCHANNEL>(img_y, CHANNELS * x + c);
            }
        }
    }
    return result;
}

template <std::size_t CHANNELS, typename TCHANNEL>
cv::Mat remove_path_vert(const cv::Mat &image, const std::vector<int> &path)
{
    cv::Mat result(image.rows, image.cols - 1, image.type());
    for (int y = 0; y < result.rows; ++y)
    {
        for (int x = 0; x < result.cols; ++x)
        {
            int img_x = x;
            if (img_x >= path[y])
            {
                ++img_x;
            }
            for (int c = 0; c < CHANNELS; ++c)
            {
                result.at<TCHANNEL>(y, CHANNELS * x + c) =
                    image.at<TCHANNEL>(y, CHANNELS * img_x + c);
            }
        }
    }
    return result;
}

template <std::size_t CHANNELS, typename TCHANNEL>
cv::Mat insert_path_hori(const cv::Mat &image, const std::vector<int> &path)
{
    cv::Mat result(image.rows + 1, image.cols, image.type());
    for (int y = 0; y < result.rows; ++y)
    {
        for (int x = 0; x < result.cols; ++x)
        {
            int img_y = y;
            if (img_y == path[x] && img_y < image.rows)
            {
                for (int c = 0; c < CHANNELS; ++c)
                {
                    result.at<TCHANNEL>(y, CHANNELS * x + c) =
                        static_cast<TCHANNEL>(
                            (static_cast<int>(image.at<TCHANNEL>(img_y - 1, CHANNELS * x + c)) +
                                static_cast<int>(image.at<TCHANNEL>(img_y, CHANNELS * x + c))) / 2
                            );
                }
            }
            else
            {
                if (img_y > path[x])
                {
                    --img_y;
                }
                for (int c = 0; c < CHANNELS; ++c)
                {
                    result.at<TCHANNEL>(y, CHANNELS * x + c) =
                        image.at<TCHANNEL>(img_y, CHANNELS * x + c);
                }
            }
        }
    }
    return result;
}

template <std::size_t CHANNELS, typename TCHANNEL>
cv::Mat insert_path_vert(const cv::Mat &image, const std::vector<int> &path)
{
    cv::Mat result(image.rows, image.cols + 1, image.type());
    for (int y = 0; y < result.rows; ++y)
    {
        for (int x = 0; x < result.cols; ++x)
        {
            int img_x = x;
            if (img_x == path[y] && img_x < image.cols)
            {
                for (int c = 0; c < CHANNELS; ++c)
                {
                    result.at<TCHANNEL>(y, CHANNELS * x + c) =
                        static_cast<TCHANNEL>(
                            (static_cast<int>(image.at<TCHANNEL>(y, CHANNELS * (img_x - 1) + c)) +
                                static_cast<int>(image.at<TCHANNEL>(y, CHANNELS * img_x + c))) / 2
                            );
                }
            }
            else
            {
                if (img_x > path[y])
                {
                    --img_x;
                }
                for (int c = 0; c < CHANNELS; ++c)
                {
                    result.at<TCHANNEL>(y, CHANNELS * x + c) =
                        image.at<TCHANNEL>(y, CHANNELS * img_x + c);
                }
            }
        }
    }
    return result;
}

template <std::size_t CHANNELS, typename TCHANNEL>
void set_path_hori(cv::Mat &image, const std::vector<int> &path,
                   const std::vector<TCHANNEL> &color)
{
    for (int x = 0; x < image.cols; ++x)
    {
        for (int c = 0; c < CHANNELS; ++c)
        {
            image.at<TCHANNEL>(path[x], CHANNELS * x + c) = color[c];
        }
    }
}

template <std::size_t CHANNELS, typename TCHANNEL>
void set_path_vert(cv::Mat &image, const std::vector<int> &path,
                   const std::vector<TCHANNEL> &color)
{
    for (int y = 0; y < image.rows; ++y)
    {
        for (int c = 0; c < CHANNELS; ++c)
        {
            image.at<TCHANNEL>(y, CHANNELS * path[y] + c) = color[c];
        }
    }
}

typedef cv::Mat (*energy_func_t)(const cv::Mat &gray_image);

cv::Mat sobel_energy(const cv::Mat &gray_image);
cv::Mat scharr_energy(const cv::Mat &gray_image);
cv::Mat laplacian_energy(const cv::Mat &gray_image);
void draw_seam(cv::Mat &image, int nc, int nr,
               const std::vector<unsigned char> &color, energy_func_t energy_func);

#endif // _SEAM_CARVING_HPP_