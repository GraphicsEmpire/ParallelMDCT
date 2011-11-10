#ifndef PS_VECTORMATH_H
#define PS_VECTORMATH_H

#include "PS_MathBase.h"

//Compact Vector Library
template <typename T>
struct vec2
{
	vec2() {}
	explicit vec2(T x_, T y_):x(x_), y(y_) {}
	T x;
	T y;
};

template <typename T>
struct vec3
{
	vec3() {}
	explicit vec3(T x_, T y_, T z_):x(x_), y(y_), z(z_) {}
	T x;
	T y;
	T z;

	/*
	vec3& operator =(const vec3& a)
	{
		x = a.x;
		y = a.y;
		z = a.z;
		return (*this);
	}
	*/
};

template <typename T>
struct vec4
{
	vec4() {}
	explicit vec4(T x_, T y_, T z_, T w_):x(x_), y(y_), z(z_), w(w_) {}
	T x;
	T y;
	T z;
	T w;
};

///////////////////////////////////////////////////////////
typedef vec2<int> vec2i;
typedef vec2<float> vec2f;

typedef vec3<int> vec3i;
typedef vec3<float> vec3f;

typedef vec4<int> vec4i;
typedef vec4<float> vec4f;
///////////////////////////////////////////////////////////
inline bool isEqual(const vec3i& a, const vec3i& b)
{
	return ((a.x == b.x)&&(a.y == b.y)&&(a.z == b.z));
}
///////////////////////////////////////////////////////////
inline vec3f vadd3f(const vec3f& a, const vec3f& b)
{
	vec3f res;
	res.x = a.x + b.x;
	res.y = a.y + b.y;
	res.z = a.z + b.z;
	return res;
}

inline vec3f vsub3f(const vec3f& a, const vec3f& b)
{
	vec3f res;
	res.x = a.x - b.x;
	res.y = a.y - b.y;
	res.z = a.z - b.z;
	return res;
}

inline float vdot3f(const vec3f& a, const vec3f& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;	
}

inline vec3f vscale3f(float a, const vec3f& b)
{
	vec3f res(a * b.x, a * b.y, a * b.z);	
	return res;
}

inline float vdist3f(const vec3f& a, const vec3f& b)
{
	vec3f d = vsub3f(a, b);
	return FastSqrt(vdot3f(d, d));
}

inline float vdistSquared3f(const vec3f& a, const vec3f& b)
{
	vec3f d = vsub3f(a, b);
	return vdot3f(d, d);
}

inline int vmaxExtent3f(const vec3f& hi, const vec3f& lo)
{
	vec3f d = vsub3f(hi, lo);
	if(d.x > d.y && d.x > d.z)
		return 0;
	else if(d.y > d.z)
		return 1;
	else
		return 2;
}

inline float vsurfaceArea3f(const vec3f& hi, const vec3f& lo)
{
	vec3f d = vsub3f(hi, lo);
	return 2.0f * (d.x * d.y + d.y * d.z + d.x * d.z);
}

inline float velement3f(const vec3f& a, int dim)
{
	if(dim == 0)
		return a.x;
	else if(dim == 1)
		return a.y;
	else
		return a.z;
}

inline void vsetElement3f(vec3f& a, int dim, float val)
{
	if(dim == 0)
		a.x = val;
	else if(dim == 1)
		a.y = val;
	else
		a.z = val;
}


inline float vlength3f(const vec3f& a)
{
	float d = a.x * a.x + a.y * a.y + a.z * a.z;
	return FastSqrt(d);
}

inline float vlengthSquared3f(const vec3f& a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;	
}

inline float vnormalize3f(vec3f& a)
{	
	float d = FastSqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	if ( d > 0 )
	{
		float r = 1.0f / d;
		a.x *= r;
		a.y *= r;
		a.z *= r;
	}
	else
	{
		a.x = 0.0f;
		a.y = 0.0f;
		a.z = 0.0f;
	}

	return d;
}

inline vec3f vcross3f(const vec3f& a, const vec3f& b)
{
	vec3f res;
	res.x = a.y*b.z - a.z*b.y;
	res.y = a.z*b.x - a.x*b.z;
	res.z = a.x*b.y - a.y*b.x;
	return res;
}

inline vec3f vmin3f(const vec3f& a, const vec3f& b)
{
	vec3f res;
	res.x = MATHMIN(a.x, b.x);
	res.y = MATHMIN(a.y, b.y);
	res.z = MATHMIN(a.z, b.z);
	return res;
}

inline vec3f vmax3f(const vec3f& a, const vec3f& b)
{
	vec3f res;
	res.x = MATHMAX(a.x, b.x);
	res.y = MATHMAX(a.y, b.y);
	res.z = MATHMAX(a.z, b.z);
	return res;
}

/////////////////////////////////////////////////////////////////|
inline vec4f vadd4f(const vec4f& a, const vec4f& b)
{
	vec4f res;
	res.x = a.x + b.x;
	res.y = a.y + b.y;
	res.z = a.z + b.z;
	res.w = a.w + b.w;
	return res;
}

inline vec4f vsub4f(const vec4f& a, const vec4f& b)
{
	vec4f res;
	res.x = a.x - b.x;
	res.y = a.y - b.y;
	res.z = a.z - b.z;
	res.w = a.w - b.w;
	return res;
}

inline float vdot4f(const vec4f& a, const vec4f& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;	
}

inline vec4f vscale4f(float a, const vec4f& b)
{
	vec4f res(a * b.x, a * b.y, a * b.z, a * b.w);	
	return res;
}
///////////////////////////////////////////////////////////
#endif
