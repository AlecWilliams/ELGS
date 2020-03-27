#pragma once
#include "line.h"
#include <iostream>
bool find_path(vector<vec3>& bezier, vec3 plane_pos, vec3 plane_dir, vec3 landing_pos, vec3 landing_dir, float currentHeight, float  glidingRatio);
float FindDistanceToSegment(vec3 pt, vec3 p1, vec3 p2, vec3& direction, vec3& closest);