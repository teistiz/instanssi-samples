#ifndef CUBES_TRANSFORM_H
#define CUBES_TRANSFORM_H

typedef struct Transform { float m[16]; } Transform;

Transform tfIdentity();
Transform tfTranslate(float x, float y, float z);
Transform tfScale(float s);
Transform tfScale3(float x, float y, float z);
Transform tfRotate(float angle, float x, float y, float z);
Transform tfMultiply(Transform *a, Transform *b);
Transform tfPerspective(float near, float far, float aspect, float fov_y);

typedef struct TransformStack {
    unsigned size;
    unsigned pos;
    Transform t[];
} TransformStack;

void tfsCreate(TransformStack **p, unsigned maxSize);
void tfsClear(TransformStack *tfs);
Transform tfsGet(TransformStack *tfs);
void tfsPop(TransformStack *tfs);
void tfsPush(TransformStack *tfs, Transform tf);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif
