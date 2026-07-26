#pragma once

#include <opencv2/opencv.hpp>

// Every UI layout/size/color knob lives here (kCellSizePx in Constants.h is the one exception).
namespace ui_constants {

inline constexpr int kHistoryPanelWidthPx = 260;
inline constexpr int kHistoryRowHeightPx = 22;
inline constexpr int kHistoryColumnX[] = { 10, 80, 130, 190 };
inline constexpr double kHistoryFontSize = 0.5;

inline constexpr int kUsernameY = 20;
inline constexpr double kUsernameFontSize = 0.5;

inline constexpr int kScoreY = kUsernameY + 22;
inline constexpr int kScoreRowHeightPx = 22;
inline constexpr double kScoreFontSize = 0.5;

inline constexpr int kHistoryHeaderY = kScoreY + kScoreRowHeightPx + 28;

inline constexpr int kRankLabelMarginPx = 24; // strip left of the board, for 1-8
inline constexpr int kFileLabelMarginPx = 24; // strip below the board, for a-h
inline constexpr double kAxisLabelFontSize = 0.4;

// Where the board itself starts, horizontally. Single source of truth for
// both rendering (UIManager) and click mapping (InputHandler).
inline constexpr int kBoardOffsetX = kHistoryPanelWidthPx + kRankLabelMarginPx;

inline constexpr int kSelectionThickness = 3;
inline constexpr double kGameOverFontSize = 1.5;
inline constexpr int kGameOverThickness = 3;

inline const cv::Scalar kPanelBackgroundColor(255, 255, 255, 255);
inline const cv::Scalar kTextColor(0, 0, 0, 255);
inline const cv::Scalar kCooldownFillColor(0, 0, 255, 255);
inline const cv::Scalar kSelectionColor(0, 255, 255, 255);
inline const cv::Scalar kGameOverColor(0, 0, 255, 255);

} // namespace ui_constants
