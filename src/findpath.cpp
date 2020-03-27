#include "findpath.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define PI 3.1415926
#define BEZIER_SEARCH_STEPS 20
#define MIN_RADIUS_MIDDLE 0.25
#define MIN_RADIUS_START_END 0.55
#define FINDPATH_CHANCES 12


void bezierCurve(vec3 * controlpoints, vector<vec3> & result_path, int steps)
	{
	double xu = 0.0, yu = 0.0, u = 0.0;
	int i = 0;
	float fsteps = 1. / (float)steps;
	result_path.push_back(controlpoints[0]);
	for (u = fsteps; u < 1.0; u += fsteps)
		{
		xu = pow(1 - u, 3) * controlpoints[0].x + 3 * u * pow(1 - u, 2) * controlpoints[1].x + 3 * pow(u, 2) * (1 - u) * controlpoints[2].x + pow(u, 3) * controlpoints[3].x;
		yu = pow(1 - u, 3) * controlpoints[0].y + 3 * u * pow(1 - u, 2) * controlpoints[1].y + 3 * pow(u, 2) * (1 - u) * controlpoints[2].y + pow(u, 3) * controlpoints[3].y;

		result_path.push_back(vec3(xu, yu, 0));
		}
	result_path.push_back(controlpoints[3]);
	}

double anglevec(vec3 v0, vec3 v1)
	{
	// Unitize the input vectors
	v0 = normalize(v0);
	v1 = normalize(v1);
	double dotp = dot(v0, v1);
	dotp = (dotp < -1.0 ? -1.0 : (dotp > 1.0 ? 1.0 : dotp));
	return  acos(dotp);
	}
bool intersection(vec2 line_A_1, vec2 line_A_2, vec2 line_B_1, vec2 line_B_2, vec3& erg)
	{
	// Store the values for fast access and easy
	// equations-to-code conversion
	float x1 = line_A_1.x, x2 = line_A_2.x, x3 = line_B_1.x, x4 = line_B_2.x;
	float y1 = line_A_1.y, y2 = line_A_2.y, y3 = line_B_1.y, y4 = line_B_2.y;

	float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	// If d is zero, there is no intersection
	if (d == 0) return NULL;

	// Get the x and y
	float pre = (x1 * y2 - y1 * x2), post = (x3 * y4 - y3 * x4);
	float x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
	float y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

	// Check if the x and y coordinates are within both lines
	/*if (x < MIN(x1, x2) || x > MAX(x1, x2) ||
		x < MIN(x3, x4) || x > MAX(x3, x4)) return false;
	if (y < MIN(y1, y2) || y > MAX(y1, y2) ||
		y < MIN(y3, y4) || y > MAX(y3, y4)) return false;
*/
// Return the point of intersection
	erg.x = x;
	erg.y = y;
	erg.z = 0.0;
	return true;
	}
void cubic_properties2(vec3* l_controlpoints, vec3 plane_dir, vec3 land_dir, float& min_radius, float& min_radius_first_30perc, float& min_radius_last_30perc, float& length)
	{
	length = 0;
	min_radius_first_30perc = min_radius_last_30perc = min_radius = 1e10;

	plane_dir.z = 0;
	land_dir.z = 0;
	vector<vec3> bezier;
	bezier.push_back(vec3(0, 0, 0));
	bezierCurve(l_controlpoints, bezier, BEZIER_SEARCH_STEPS);
	bezier[0] = bezier[1] - normalize(plane_dir) * glm::length(bezier[2] - bezier[1]);
	int lasts = bezier.size() - 1;
	vec3 last = bezier[lasts] - normalize(land_dir) * glm::length(bezier[lasts] - bezier[lasts - 1]);
	bezier.push_back(last);

	for (int i = 1; i <= lasts; i++)
		{
		float length_a = glm::length(bezier[i - 1] - bezier[i]);
		float length_b = glm::length(bezier[i] - bezier[i + 1]);

		length += length_b;

		float radiuslen = MIN(length_a, length_b);
		float rad = 1e10;

		vec2 A, B, C;
		A = bezier[i - 1];
		B = bezier[i];
		C = bezier[i + 1];
		A = B + normalize(vec2(bezier[i - 1]) - B) * radiuslen;
		C = B + normalize(vec2(bezier[i + 1]) - B) * radiuslen;

		vec3 erg;
		if (intersection(
			A,
			vec3(A.x + B.y - A.y, A.y + A.x - B.x, 0),
			C,
			vec3(C.x + B.y - C.y, C.y + C.x - B.x, 0),
			erg
			))
			rad = glm::length(erg - vec3(A, 0));



		if (i < 4)
			min_radius_first_30perc = MIN(rad, min_radius_first_30perc);
		else if (i > BEZIER_SEARCH_STEPS - 2)
			min_radius_last_30perc = MIN(rad, min_radius_last_30perc);
		else
			min_radius = MIN(rad, min_radius);
		}
	}

//extern vector<vec3> controlpoints;
extern bool curve_unvalid;
class evaluating_curve_
	{
	public:
		bool first_attempt = true;
		bool getting_A_mul_bigger = false;
		bool getting_B_mul_bigger = false;
		bool getting_length_longer = false;
		bool getting_length_shorter = false;
		float margin_rad_first = 0.0;
		float margin_rad_last = 0.0;
		float margin_rad_middle = 0.0;
		float last_margin_rad_first = 0.0;
		float last_margin_rad_last = 0.0;
		float last_margin_rad_middle = 0.0;
		float curve_len = 0;
		float last_curve_len = 0;
		void set_values(float rad, float radfirst, float radlast, float len)
			{
			if (!first_attempt)
				{
				last_margin_rad_first = margin_rad_first;
				last_margin_rad_last = margin_rad_last;
				last_margin_rad_middle = margin_rad_middle;
				last_curve_len = curve_len;
				}
			margin_rad_first = radfirst - MIN_RADIUS_START_END;
			margin_rad_last = radlast - MIN_RADIUS_START_END;
			margin_rad_middle = rad - MIN_RADIUS_MIDDLE;
			if (first_attempt)
				{
				last_margin_rad_first = margin_rad_first;
				last_margin_rad_last = margin_rad_last;
				last_margin_rad_middle = margin_rad_middle;
				last_curve_len = curve_len;
				}
			}
		/*	bool check_is_impossible()
				{
				if (first_attempt) return false;
				if (getting_length_longer)
					{
					if (last_margin_rad_first > margin_rad_first && margin_rad_first < 0) return true;
					if (last_margin_rad_last > margin_rad_last && margin_rad_last < 0) return true;
					if (last_margin_rad_middle > margin_rad_middle && margin_rad_middle < 0) return true;
					}
				if (getting_length_shorter)
					{
					if (margin_rad_first < 0) return true;
					if (margin_rad_last < 0) return true;
					if (margin_rad_middle < 0) return true;
					}
				return false;
				}*/
				/*	void recommend_mul_factors(float &mul_A,float &mul_B)
						{
						mul_A = mul_B = 1;
						if (first_attempt) return;
						if (getting_A_mul_bigger && getting_B_mul_bigger && getting_length_shorter)return;

						if (getting_A_mul_bigger && margin_rad_first<0)
							if (last_margin_rad_first < margin_rad_first)
								{
								float diff_margin = margin_rad_first - last_margin_rad_first;
								if (margin_rad_first + diff_margin < 0)
									mul_A = margin_rad_first / diff_margin;
								}

						if (getting_B_mul_bigger && margin_rad_last < 0)
							if (last_margin_rad_last < margin_rad_last)
								{
								float diff_margin = margin_rad_last - last_margin_rad_last;
								if (margin_rad_last + diff_margin < 0)
									mul_B = margin_rad_last / diff_margin;
								}

						if (getting_length_longer)
							{
							float diff_margin = curve_len - last_curve_len;
							if (curve_len + diff_margin < 0)
								mul_B = mul_A = curve_len / diff_margin;
							}
						}*/
		evaluating_curve_() {}
		bool check_ok()
			{
			if (getting_A_mul_bigger || getting_B_mul_bigger || getting_length_longer || getting_length_shorter)return false;
			return true;
			}
		void reset_attempts()
			{
			getting_A_mul_bigger = false;
			getting_B_mul_bigger = false;
			getting_length_longer = false;
			getting_length_shorter = false;
			}
	};
//*******************************************
evaluating_curve_ evaluating_curve_fct(vec3* l_controlpoints, float maxDistance, float control_A_mul, float control_B_mul, vec3 plane_pos, vec3 plane_dir, vec3 landing_pos, vec3 landing_dir, int steps)
	{
	float min_radius, len, minfirst, minlast;
	float add_fact = maxDistance / (float)steps;
	evaluating_curve_ evaluating_curve;
	vec3 a, b, c, d;
	a = plane_pos;
	d = landing_pos;
	for (int i = 0; i < steps; i++)
		{
		b = plane_pos + normalize(plane_dir) * control_A_mul;
		c = d + normalize(landing_dir) * control_B_mul;

		l_controlpoints[0] = vec3(a.x, a.y, 0.0);
		l_controlpoints[1] = vec3(b.x, b.y, 0.0);
		l_controlpoints[2] = vec3(c.x, c.y, 0.0);
		l_controlpoints[3] = vec3(d.x, d.y, 0.0);
		cubic_properties2(l_controlpoints, plane_dir, landing_dir, min_radius, minfirst, minlast, len);

		//float mul_A, mul_B;
		//evaluating_curve.recommend_mul_factors(mul_A, mul_B);


		evaluating_curve.set_values(min_radius, minfirst, minlast, len);

		evaluating_curve.reset_attempts();
		//evaluating:
		if (evaluating_curve.margin_rad_first < 0 || evaluating_curve.margin_rad_middle < 0)evaluating_curve.getting_A_mul_bigger = true;
		if (evaluating_curve.margin_rad_last < 0 || evaluating_curve.margin_rad_middle < 0)evaluating_curve.getting_B_mul_bigger = true;
		if (len < maxDistance * 0.8) evaluating_curve.getting_length_longer = true;
		if (len > maxDistance * 1.4) evaluating_curve.getting_length_shorter = true;

		bool impossible = false;
		if (evaluating_curve.first_attempt)
			{
			evaluating_curve.first_attempt = false;
			}

		if (evaluating_curve.getting_A_mul_bigger)
			control_A_mul += add_fact;
		if (evaluating_curve.getting_B_mul_bigger)
			control_B_mul += add_fact;
		if (evaluating_curve.getting_length_longer)
			{
			control_A_mul += add_fact;
			control_B_mul += add_fact;
			}
		if (evaluating_curve.getting_length_shorter)
			{
			if (evaluating_curve.margin_rad_first > MIN_RADIUS_START_END * 2 && !evaluating_curve.getting_A_mul_bigger)
				control_A_mul -= add_fact;
			if (evaluating_curve.margin_rad_last > MIN_RADIUS_START_END * 2 && !evaluating_curve.getting_B_mul_bigger)
				control_B_mul -= add_fact;
			}

		if (evaluating_curve.check_ok())break;
		}
	return evaluating_curve;
	}
//***********************************
bool find_path(vector<vec3>& bezier, vec3 plane_pos, vec3 plane_dir, vec3 landing_pos, vec3 landing_dir, float currentHeight, float  glidingRatio)
	{
	vec3 l_controlpoints[4];
	float maxDistance = (currentHeight * glidingRatio);

	float direct_distance = distance(landing_pos, plane_pos);

	if (direct_distance > maxDistance)
		return false;

	//temporary testing radius
	float radius = 1.0;
	float speedMultiplier = 1.0f;

	float control_A_mul = maxDistance / 10.;
	float control_B_mul = maxDistance / 10.;


	float min_radius, len, minfirst, minlast;
	evaluating_curve_ evaluating_curve = evaluating_curve_fct(l_controlpoints, maxDistance, control_A_mul, control_B_mul, plane_pos, plane_dir, landing_pos, landing_dir, FINDPATH_CHANCES);

	//cout << evaluating_curve.getting_A_mul_bigger << " , " << evaluating_curve.getting_B_mul_bigger << " , " << evaluating_curve.getting_length_longer << " , " << evaluating_curve.getting_length_shorter << endl;

	if (evaluating_curve.check_ok())
		{
		curve_unvalid = false;
		bezierCurve(l_controlpoints, bezier, 100);
		//	cout << minfirst << " , " << minlast << " , " << min_radius << " , " << len << endl;
		return true;
		}
	//try with 2 curves points
	//find "air landing" spot
	/*if (!evaluating_curve.getting_length_shorter)
		{
		curve_unvalid = true;
		bezierCurve(controlpoints, bezier, 100);
		return true;
		}*/
		//split in two:
	float maxDistance_half = maxDistance / 2.;
	float target_length_hyp = maxDistance_half * 0.85;
	float direct_distance_half = direct_distance * 0.5;

	if (direct_distance > (MIN_RADIUS_START_END * 2) && target_length_hyp > direct_distance_half)
		{
		vec3 direct_vec = landing_pos - plane_pos;
		vec3 halfpoint = plane_pos + direct_vec / 2.0f;
		vec3 perpent_dir = normalize(vec3(-halfpoint.y, halfpoint.x, 0));
		if (dot(perpent_dir, landing_dir) < 0) perpent_dir *= -1;



		float air_landing_length = sqrt(target_length_hyp * target_length_hyp - direct_distance_half * direct_distance_half);
		vec3 air_landing_pos = halfpoint + perpent_dir * air_landing_length;
		vec3 air_landing_dir = -normalize(direct_vec);


		//curve_unvalid = true;
		int both_ok = 0;
		evaluating_curve = evaluating_curve_fct(l_controlpoints, maxDistance_half, control_A_mul, control_B_mul, plane_pos, plane_dir, air_landing_pos, air_landing_dir, 12);
		both_ok += evaluating_curve.check_ok();
		bezierCurve(l_controlpoints, bezier, 50);
		evaluating_curve = evaluating_curve_fct(l_controlpoints, maxDistance_half, control_A_mul, control_B_mul, air_landing_pos, -air_landing_dir, landing_pos, landing_dir, 12);
		both_ok += evaluating_curve.check_ok();
		bezierCurve(l_controlpoints, bezier, 50);
		if (both_ok == 2)
			{
			curve_unvalid = false;
			return true;
			}

		}
	else
		{
		//	curve_unvalid = true;
			//bezierCurve(controlpoints, bezier, 100);
		}

	//curve_unvalid = true;
	//bezierCurve(controlpoints, bezier, 100);
	//	cout << minfirst << " , " << minlast << " , " << min_radius << " , " << len << endl;
	return false;

	return true;
	}
//##############################################################
// Calculate the distance between
// point pt and the segment p1 --> p2.
float FindDistanceToSegment(vec3 pt, vec3 p1, vec3 p2,vec3 &direction,vec3 &closest)
	{
	float dx = p2.x - p1.x;
	float dy = p2.y - p1.y;
	if ((dx == 0) && (dy == 0))
		{
		// It's a point not a line segment.
		closest = p1;
		dx = pt.x - p1.x;
		dy = pt.y - p1.y;
		return sqrt(dx * dx + dy * dy);
		}

	// Calculate the t that minimizes the distance.
	float t = ((pt.x - p1.x) * dx + (pt.y - p1.y) * dy) /
		(dx * dx + dy * dy);

	// See if this represents one of the segment's
	// end points or a point in the middle.
	if (t < 0)
		{
		closest = vec3(p1.x, p1.y,0);
		dx = pt.x - p1.x;
		dy = pt.y - p1.y;
		}
	else if (t > 1)
		{
		closest = vec3(p2.x, p2.y, 0);
		dx = pt.x - p2.x;
		dy = pt.y - p2.y;
		}
	else
		{
		closest = vec3(p1.x + t * dx, p1.y + t * dy,0.0);
		dx = pt.x - closest.x;
		dy = pt.y - closest.y;
		}
	direction = vec3(dx, dy, 0);
	return sqrt(dx * dx + dy * dy);
	}