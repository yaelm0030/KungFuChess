#pragma once

#include <opencv2/opencv.hpp>

// Every UI layout/size/color knob lives here so tweaking the screen doesn't
// require hunting through UIManager/InputHandler. constants::kCellSizePx
// (Kung-Fu-Chess/include/Constants.h) is the one exception: it's a shared
// engine/UI coordinate contract, not a purely presentational value.
namespace ui_constants {

// ---- History panel (left of the rank labels) ----
inline constexpr int kHistoryPanelWidthPx = 260;
inline constexpr int kHistoryHeaderY = 20;
inline constexpr int kHistoryRowHeightPx = 22;
inline constexpr int kHistoryColumnX[] = { 10, 80, 130, 190 };
inline constexpr double kHistoryFontSize = 0.5;

// ---- Board axis labels ----
inline constexpr int kRankLabelMarginPx = 24; // strip left of the board, for 1-8
inline constexpr int kFileLabelMarginPx = 24; // strip below the board, for a-h
inline constexpr double kAxisLabelFontSize = 0.4;

// Where the board itself starts, horizontally. Single source of truth for
// both rendering (UIManager) and click mapping (InputHandler).
inline constexpr int kBoardOffsetX = kHistoryPanelWidthPx + kRankLabelMarginPx;

// ---- Overlay appearance ----
inline constexpr int kSelectionThickness = 3;
inline constexpr double kGameOverFontSize = 1.5;
inline constexpr int kGameOverThickness = 3;

// ---- Colors (BGRA) ----
inline const cv::Scalar kPanelBackgroundColor(255, 255, 255, 255);
inline const cv::Scalar kTextColor(0, 0, 0, 255);
inline const cv::Scalar kCooldownFillColor(0, 0, 255, 255);
inline const cv::Scalar kSelectionColor(0, 255, 255, 255);
inline const cv::Scalar kGameOverColor(0, 0, 255, 255);

} // namespace ui_constants
