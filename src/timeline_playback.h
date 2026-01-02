/*
    Copyright (C) 2025 Timeline Feature Contributors

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TIMELINE_PLAYBACK_H
#define TIMELINE_PLAYBACK_H

#include "timeline_recorder.h"

class TimelinePlayback {
private:
    TimelineRecorder* recorder;
    float current_time;
    bool playing;
    float playback_speed;

    // Interpolate between two snapshots
    vec2 interpolateVec2(const vec2& a, const vec2& b, float t) const;
    vec3 interpolateVec3(const vec3& a, const vec3& b, float t) const;

public:
    TimelinePlayback(TimelineRecorder* rec);

    void setTime(float time);
    void setPlaybackSpeed(float speed);
    void play();
    void pause();
    void seekTo(float percent);  // 0.0 to 1.0

    // Returns interpolated snapshot for current time
    FrameSnapshot getCurrentState() const;

    // Get two adjacent snapshots for manual interpolation
    void getAdjacentSnapshots(const FrameSnapshot** before, const FrameSnapshot** after, float* blend) const;

    float getCurrentTime() const;
    float getProgress() const;  // 0.0 to 1.0
    bool isPlaying() const;
};

#endif
