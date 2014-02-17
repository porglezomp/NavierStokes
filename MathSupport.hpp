#ifndef MATH_SUPPORT_H
#define MATH_SUPPORT_H

template <typename T>
T sgn(T t) { return (t < 0) ? T(-1) : T(1); }

// Thanks to Iñigo Quilez
float almostIdentity( float x, float m, float n ) {
    if( x>m ) return x;
    const float a = 2.0f*n - m;
    const float b = 2.0f*m - 3.0f*n;
    const float t = x/m;
    return (a*t + b)*t*t + n;
}

float saturate(float x) {
	return std::max(0.0f, std::min(1.0f, x));
}

#endif