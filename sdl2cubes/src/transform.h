#ifndef CUBES_TRANSFORM_H
#define CUBES_TRANSFORM_H

// Single linear transformation in 3D space, aka a 4x4 matrix.
typedef struct Transform { float m[16]; } Transform;

// identity transform
Transform tfIdentity();
// translation
Transform tfTranslate(float x, float y, float z);
// uniform scaling
Transform tfScale(float s);
// non-uniform scaling
Transform tfScale3(float x, float y, float z);
// rotation around unit vector
Transform tfRotate(float angle, float x, float y, float z);
// chain transformations
Transform tfMultiply(Transform *a, Transform *b);
// perspective projection
Transform tfPerspective(float near, float far, float aspect, float fov_y);
// orthogonal projection
Transform tfOrtho(float near, float far, float w, float h);

// OpenGL-like transform stack.
typedef struct TransformStack {
    unsigned size;
    unsigned pos;
    Transform t[];
} TransformStack;

// allocate a new transform stack
void tfsCreate(TransformStack **p, unsigned maxSize);
// reset the stack to a single identity matrix
void tfsClear(TransformStack *tfs);
// get the current transform
Transform tfsGet(TransformStack *tfs);
// pop the transform stack's top
void tfsPop(TransformStack *tfs);
// push a copy of the current stack top
void tfsPush(TransformStack *tfs);
// replace stack top with multiple of another and current transform
void tfsApply(TransformStack *tfs, Transform tf);
// replace stack top with multiple of current and another transform
void tfsApplyR(TransformStack *tfs, Transform tf);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif
