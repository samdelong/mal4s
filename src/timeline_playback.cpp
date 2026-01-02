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

#include "timeline_playback.h"
#include <algorithm>

TimelinePlayback::TimelinePlayback(TimelineRecorder* rec)
    : recorder(rec), current_time(0.0f), playing(false), playback_speed(1.0f) {
}

void TimelinePlayback::setTime(float time) {
    current_time = std::max(0.0f, std::min(time, recorder->getTotalDuration()));
}

void TimelinePlayback::setPlaybackSpeed(float speed) {
    playback_speed = speed;
}

void TimelinePlayback::play() {
    playing = true;
}

void TimelinePlayback::pause() {
    playing = false;
}

void TimelinePlayback::seekTo(float percent) {
    float duration = recorder->getTotalDuration();
    current_time = percent * duration;
    current_time = std::max(0.0f, std::min(current_time, duration));
}

vec2 TimelinePlayback::interpolateVec2(const vec2& a, const vec2& b, float t) const {
    return vec2(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    );
}

vec3 TimelinePlayback::interpolateVec3(const vec3& a, const vec3& b, float t) const {
    return vec3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

void TimelinePlayback::getAdjacentSnapshots(const FrameSnapshot** before,
                                             const FrameSnapshot** after,
                                             float* blend) const {
    size_t count = recorder->getSnapshotCount();
    if (count == 0) {
        *before = nullptr;
        *after = nullptr;
        *blend = 0.0f;
        return;
    }

    // Find snapshots bracketing current_time
    *before = recorder->getSnapshotAt(current_time);
    if (!*before) {
        *after = recorder->getSnapshotByIndex(0);
        *blend = 0.0f;
        return;
    }

    // Find the next snapshot
    size_t before_idx = 0;
    for (size_t i = 0; i < count; ++i) {
        const FrameSnapshot* snap = recorder->getSnapshotByIndex(i);
        if (snap == *before) {
            before_idx = i;
            break;
        }
    }

    if (before_idx + 1 < count) {
        *after = recorder->getSnapshotByIndex(before_idx + 1);

        // Calculate blend factor
        float time_delta = (*after)->timestamp - (*before)->timestamp;
        if (time_delta > 0.0f) {
            *blend = (current_time - (*before)->timestamp) / time_delta;
            *blend = std::max(0.0f, std::min(*blend, 1.0f));
        } else {
            *blend = 0.0f;
        }
    } else {
        *after = *before;
        *blend = 0.0f;
    }
}

FrameSnapshot TimelinePlayback::getCurrentState() const {
    const FrameSnapshot* before;
    const FrameSnapshot* after;
    float blend;

    getAdjacentSnapshots(&before, &after, &blend);

    if (!before || !after) {
        return FrameSnapshot();
    }

    // If blend is 0 or they're the same, just return the before snapshot
    if (blend == 0.0f || before == after) {
        return *before;
    }

    // Create interpolated snapshot
    FrameSnapshot result;
    result.timestamp = current_time;
    result.display_time = before->display_time; // Use discrete time
    result.frame_number = before->frame_number;

    // Interpolate camera
    result.camera_pos = interpolateVec3(before->camera_pos, after->camera_pos, blend);
    result.camera_target = interpolateVec3(before->camera_target, after->camera_target, blend);
    result.camera_dest = interpolateVec3(before->camera_dest, after->camera_dest, blend);

    // Interpolate directory nodes
    // Match by path
    for (const auto& before_dn : before->dirnodes) {
        // Find corresponding node in after snapshot
        const FrameSnapshot::DirNodeState* after_dn = nullptr;
        for (const auto& adn : after->dirnodes) {
            if (adn.path == before_dn.path) {
                after_dn = &adn;
                break;
            }
        }

        FrameSnapshot::DirNodeState state;
        state.path = before_dn.path;

        if (after_dn) {
            state.pos = interpolateVec2(before_dn.pos, after_dn->pos, blend);
            state.vel = interpolateVec2(before_dn.vel, after_dn->vel, blend);
            state.radius = before_dn.radius + (after_dn->radius - before_dn.radius) * blend;
            // Use before snapshot for discrete states
            state.col = before_dn.col;
            state.visible = before_dn.visible;
        } else {
            state = before_dn;
        }

        result.dirnodes.push_back(state);
    }

    // Interpolate files
    for (const auto& before_f : before->files) {
        const FrameSnapshot::FileState* after_f = nullptr;
        for (const auto& af : after->files) {
            if (af.path == before_f.path) {
                after_f = &af;
                break;
            }
        }

        FrameSnapshot::FileState state;
        state.path = before_f.path;

        if (after_f) {
            state.pos = interpolateVec2(before_f.pos, after_f->pos, blend);
            state.alpha = before_f.alpha + (after_f->alpha - before_f.alpha) * blend;
            // Use before snapshot for discrete states
            state.colour = before_f.colour;
            state.visible = before_f.visible;
            state.removing = before_f.removing;
        } else {
            state = before_f;
        }

        result.files.push_back(state);
    }

    // Interpolate users
    for (const auto& before_u : before->users) {
        const FrameSnapshot::UserState* after_u = nullptr;
        for (const auto& au : after->users) {
            if (au.name == before_u.name) {
                after_u = &au;
                break;
            }
        }

        FrameSnapshot::UserState state;
        state.name = before_u.name;

        if (after_u) {
            state.pos = interpolateVec2(before_u.pos, after_u->pos, blend);
            state.alpha = before_u.alpha + (after_u->alpha - before_u.alpha) * blend;
            // Use before snapshot for discrete states
            state.colour = before_u.colour;
            state.visible = before_u.visible;
            state.active_action_count = before_u.active_action_count;
        } else {
            state = before_u;
        }

        result.users.push_back(state);
    }

    // Actions - use before snapshot (discrete)
    result.actions = before->actions;

    return result;
}

float TimelinePlayback::getCurrentTime() const {
    return current_time;
}

float TimelinePlayback::getProgress() const {
    float duration = recorder->getTotalDuration();
    if (duration == 0.0f) return 0.0f;
    return current_time / duration;
}

bool TimelinePlayback::isPlaying() const {
    return playing;
}
