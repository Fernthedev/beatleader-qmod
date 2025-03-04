#pragma once

#include <string>
#include "include/Utils/ReplayManager.hpp"

using namespace std;

namespace LeaderboardUI {
    extern function<void()> retryCallback; 

    void setup();
    void reset();

    void updateStatus(ReplayUploadStatus status, string description, float progress);
}