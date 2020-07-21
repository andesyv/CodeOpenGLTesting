// Shader maths

/** Quaternions:
 * These are some quaternion math functions
 * that enables using vec4 as a quaternion.
 * All quaternion math functions are prefixed by a q.
 *
 * Quaternion logic:
 * vec4.x = i, vec4.y = j, vec4.z = k
 * vec4.w = s
 * vec4.xyz = v
 */


// struct quat
// {
//     vec4 v;
// };

vec4 qadd(vec4 a, vec4 b)
{
    // return quat(vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w));
    return a + b;
}

vec4 qsub(vec4 a, vec4 b)
{
    return a - b;
}

vec4 qconj(vec4 a)
{
    return vec4(-a.xyz, a.w);
}

float qlength(vec4 a)
{
    return length(a);
}

float qnorm(vec4 a)
{
    return length(a);
}

float qnormSqrd(vec4 a)
{
    return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

vec4 qmul(vec4 a, float b)
{
    return a * b;
}

// quat div(float a, quat b)
// {
//     return quat(a / b.v);
// }

vec4 qdiv(vec4 a, float b)
{
    return a * (1 / b);
}

vec4 qmul(vec4 a, vec4 b)
{
    return vec4(
        a.w * b.x + b.w * a.x + a.y * b.z - b.y * a.z,
        a.w * b.y + b.w * a.y + a.z * b.x - b.z * a.x,
        a.w * b.z + b.w * a.z + a.x * b.y - b.x * a.y,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    );
}

vec4 qnormalize(vec4 a)
{
    return a * (1 / length(a));
}

// Note inverse of unit quaternion = conjugate
vec4 qinverse(vec4 a)
{
    return qmul(qconj(a), 1 / qnormSqrd(a));
}

// Using the unit version of quaternion calculations
vec3 qrotate(vec3 p, vec4 quat)
{
    // return qmul(qmul(quat, vec4(p, 0.0)), qinverse(quat)).xyz;
    return qmul(qmul(quat, vec4(p, 0.0)), qconj(quat)).xyz;
}