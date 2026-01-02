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

#ifndef TIMELINE_RECORDER_H
#define TIMELINE_RECORDER_H

#include "core/sdlapp.h"
#include "core/vectors.h"
#include <vector>
#include <string>
#include <ctime>

class Gource;

struct FrameSnapshot {
    float timestamp;        // Simulation time (currtime)
    time_t display_time;    // Display timestamp for UI
    int frame_number;       // Sequential frame number

    // Camera state
    vec3 camera_pos;
    vec3 camera_target;
    vec3 camera_dest;

    // Directory nodes
    struct DirNodeState {
        std::string path;
        vec2 pos;
        vec2 vel;
        vec4 col;
        float radius;
        bool visible;
    };
    std::vector<DirNodeState> dirnodes;

    // Files
    struct FileState {
        std::string path;
        vec2 pos;
        vec3 colour;
        float alpha;
        bool visible;
        bool removing;
    };
    std::vector<FileState> files;

    // Users
    struct UserState {
        std::string name;
        vec2 pos;
        vec3 colour;
        float alpha;
        bool visible;
        int active_action_count;
    };
    std::vector<UserState> users;

    // Actions
    struct ActionState {
        std::string source_user;
        std::string target_file;
        float progress;
        vec3 colour;
    };
    std::vector<ActionState> actions;

    FrameSnapshot() : timestamp(0.0f), display_time(0), frame_number(0) {
        camera_pos = vec3(0,0,0);
        camera_target = vec3(0,0,0);
        camera_dest = vec3(0,0,0);
    }
};

class TimelineRecorder {
private:
    std::vector<FrameSnapshot> snapshots;
    float snapshot_interval;     // e.g., 1/60.0 (60 FPS)
    float next_snapshot_time;
    bool recording;
    float total_duration;
    float first_capture_time;    // Time of first capture (for relative timestamps)

    Gource* gource;              // Reference to capture state

public:
    TimelineRecorder(float fps = 60.0f);
    ~TimelineRecorder();

    void startRecording(Gource* g);
    void stopRecording();
    bool isRecording() const;

    // Called each frame during pre-simulation
    void captureFrame(float currtime);

    // Playback access
    const FrameSnapshot* getSnapshotAt(float time) const;
    const FrameSnapshot* getSnapshotByIndex(size_t idx) const;
    size_t getSnapshotCount() const;
    float getTotalDuration() const;

    // Memory management
    void clear();
    size_t getMemoryUsage() const;
};

#endif
