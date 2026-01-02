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

#include "timeline_recorder.h"
#include "gource.h"
#include "dirnode.h"
#include "file.h"
#include "user.h"
#include "action.h"
#include <algorithm>

TimelineRecorder::TimelineRecorder(float fps) {
    snapshot_interval = 1.0f / fps;
    next_snapshot_time = 0.0f;
    recording = false;
    total_duration = 0.0f;
    gource = nullptr;
}

TimelineRecorder::~TimelineRecorder() {
    clear();
}

void TimelineRecorder::startRecording(Gource* g) {
    gource = g;
    recording = true;
    next_snapshot_time = 0.0f;
    total_duration = 0.0f;
    snapshots.clear();
    first_capture_time = -1.0f;  // Will be set on first capture
}

void TimelineRecorder::stopRecording() {
    recording = false;
    if (!snapshots.empty()) {
        total_duration = snapshots.back().timestamp;
    }
}

bool TimelineRecorder::isRecording() const {
    return recording;
}

void TimelineRecorder::captureFrame(float currtime) {
    if (!recording || !gource) return;

    // Set first capture time on first frame
    if (first_capture_time < 0.0f) {
        first_capture_time = currtime;
        next_snapshot_time = currtime;
    }

    // Only capture at the specified interval
    if (currtime < next_snapshot_time) return;

    FrameSnapshot snapshot;
    // Use RELATIVE time from first capture
    snapshot.timestamp = currtime - first_capture_time;
    snapshot.display_time = gource->getCurrentTime();
    snapshot.frame_number = static_cast<int>(snapshots.size());

    // Capture camera state
    const ZoomCamera& camera = gource->getCamera();
    snapshot.camera_pos = camera.getPos();
    snapshot.camera_target = camera.getTarget();
    snapshot.camera_dest = camera.getDest();

    // Capture directory nodes
    for (auto it = gGourceDirMap.begin(); it != gGourceDirMap.end(); ++it) {
        RDirNode* node = it->second;
        if (!node) continue;

        FrameSnapshot::DirNodeState state;
        state.path = node->getPath();
        state.pos = node->pos;
        state.vel = node->vel;
        state.col = node->getColour();
        state.radius = node->dir_radius;
        state.visible = node->isVisible();

        snapshot.dirnodes.push_back(state);
    }

    // Capture files
    const std::map<std::string, RFile*>& files = gource->getFiles();
    for (auto it = files.begin(); it != files.end(); ++it) {
        RFile* file = it->second;
        if (!file) continue;

        FrameSnapshot::FileState state;
        state.path = file->path;
        state.pos = file->getPos();
        state.colour = file->getColour();
        state.alpha = file->getAlpha();
        state.visible = !file->isHidden();
        state.removing = file->removing;

        snapshot.files.push_back(state);
    }

    // Capture users
    const std::map<std::string, RUser*>& users = gource->getUsers();
    for (auto it = users.begin(); it != users.end(); ++it) {
        RUser* user = it->second;
        if (!user) continue;

        FrameSnapshot::UserState state;
        state.name = user->getName();
        state.pos = user->getPos();
        state.colour = user->getColour();
        state.alpha = user->getAlpha();
        state.visible = !user->isHidden();
        state.active_action_count = user->getActionCount();

        snapshot.users.push_back(state);
    }

    // Capture actions - iterate through all users
    for (auto uit = users.begin(); uit != users.end(); ++uit) {
        RUser* user = uit->second;
        if (!user) continue;

        // Note: activeActions is private, so we can't access it directly
        // For now, we'll store action count; in playback we'll reconstruct actions
        // based on the simulation state
    }

    snapshots.push_back(snapshot);
    next_snapshot_time += snapshot_interval;
}

const FrameSnapshot* TimelineRecorder::getSnapshotAt(float time) const {
    if (snapshots.empty()) return nullptr;

    // Binary search for the snapshot at or just before the given time
    size_t left = 0;
    size_t right = snapshots.size();

    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (snapshots[mid].timestamp < time) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    if (left == 0) return &snapshots[0];
    if (left >= snapshots.size()) return &snapshots[snapshots.size() - 1];

    // Return the snapshot at or just before the time
    return &snapshots[left > 0 ? left - 1 : 0];
}

const FrameSnapshot* TimelineRecorder::getSnapshotByIndex(size_t idx) const {
    if (idx >= snapshots.size()) return nullptr;
    return &snapshots[idx];
}

size_t TimelineRecorder::getSnapshotCount() const {
    return snapshots.size();
}

float TimelineRecorder::getTotalDuration() const {
    return total_duration;
}

void TimelineRecorder::clear() {
    snapshots.clear();
    recording = false;
    total_duration = 0.0f;
}

size_t TimelineRecorder::getMemoryUsage() const {
    size_t total = 0;

    for (const auto& snapshot : snapshots) {
        total += sizeof(FrameSnapshot);
        total += snapshot.dirnodes.size() * sizeof(FrameSnapshot::DirNodeState);
        total += snapshot.files.size() * sizeof(FrameSnapshot::FileState);
        total += snapshot.users.size() * sizeof(FrameSnapshot::UserState);
        total += snapshot.actions.size() * sizeof(FrameSnapshot::ActionState);

        // Add string storage
        for (const auto& dn : snapshot.dirnodes) {
            total += dn.path.capacity();
        }
        for (const auto& f : snapshot.files) {
            total += f.path.capacity();
        }
        for (const auto& u : snapshot.users) {
            total += u.name.capacity();
        }
        for (const auto& a : snapshot.actions) {
            total += a.source_user.capacity();
            total += a.target_file.capacity();
        }
    }

    return total;
}
