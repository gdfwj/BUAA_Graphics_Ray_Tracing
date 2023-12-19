//
// Created by gdfwj on 2022/11/28.
//

#ifndef TCODE_OBJECTS_H
#define TCODE_OBJECTS_H

#include "my_math.h"

enum MaterialType {
    ROUGH, REFLECTIVE, REFRACTIVE
};

struct Material {
    Vector3f ka, kd, ks;    // 环境光照，漫反射，镜面反射系数，用于phong模型计算 0.1 1.0 0.5
    float shininess;    // 表面平整程度，用于镜面反射计算
    Vector3f F0;            // 垂直入射时，反射光的占比：[(n-1)^2+k^2]/[(n+1)^2+k^2]
    float ior;            // 折射率
    MaterialType type;

    Material(MaterialType t) { type = t; }
};

struct RoughMaterial :Material
{
    RoughMaterial(const Vector3f _kd, const Vector3f _ks, float _shininess) : Material(ROUGH)
    {
        MultiplyVector3andFloat(ka, _kd, M_PI);
        CopyVector3(kd, _kd);
        CopyVector3(ks, _ks);
        shininess = _shininess;
    }
};

struct ReflectiveMaterial :Material
{
    // n: 折射率；kappa：消光系数
    ReflectiveMaterial(Vector3f n, Vector3f kappa) :Material(REFLECTIVE)
    {
        Vector3f one{1, 1, 1};
        Vector3f nSubOne, nAddOne, kappa2, div1, div2;
        SubVector3(nSubOne, n, one);
        MultiplyVector3ByElement(nSubOne, nSubOne, nSubOne); // nSubOne = (n - one) * (n - one)
        AddVector3(nAddOne, n, one);
        MultiplyVector3ByElement(nAddOne, nAddOne, nAddOne); // nAddOne = (n + one) * (n + one)
        MultiplyVector3ByElement(kappa2, kappa, kappa); // kappa2 = kappa*kappa
        AddVector3(div1, nSubOne, kappa2);
        AddVector3(div2, nAddOne, kappa2);
        DivVector3ByElement(F0, div1, div2);
        //F0 = ((n - one) * (n - one) + kappa * kappa) / ((n + one) * (n + one) + kappa * kappa); Fresnel公式
    }
};

struct RefractiveMaterial :Material
{
    // n，折射率；一个物体透明，光显然不会在其内部消逝，因此第二项忽略
    RefractiveMaterial(Vector3f n) :Material(REFRACTIVE)
    {
        Vector3f one{1, 1, 1};
        Vector3f nSubOne, nAddOne;
        SubVector3(nSubOne, n, one);
        MultiplyVector3ByElement(nSubOne, nSubOne, nSubOne);
        AddVector3(nAddOne, n, one);
        MultiplyVector3ByElement(nAddOne, nAddOne, nAddOne);
        DivVector3ByElement(F0, nSubOne, nAddOne);
        //  F0 = ((n - one) * (n - one)) / ((n + one) * (n + one));
        ior = (n[0]+n[1]+n[2])/3;		// 该物体的折射率取单色光或取均值都可以
    }
};

struct Ray        // 光照
{
    Vector3f start, dir;                // 起点，方向
    Ray(Vector3f _start, Vector3f _dir) {
        CopyVector3(start, _start);
        CopyVector3(dir, _dir);        // 方向 归一化
        NormalizeVector3(dir);
    }
};

struct Hit        // 光线和物体表面交点
{
    float t;                    // 交点距光线起点距离，当t大于0时表示相交，默认取-1表示无交点
    Vector3f position, normal;        // 交点坐标，法线
    Material *material;            // 交点处表面的材质
    Hit() { t = -1; }
};


class MyObject            // 定义一个基类(接口)，可交
{
public:
    virtual Hit intersect(const Ray &ray) = 0;        // 需要根据表面类型实现

protected:
    Material *material;
};

class Sphere : public MyObject        // 定义球体
{
    Vector3f center;
    float radius;
public:
    Sphere(const Vector3f _center, float _radius, Material *_material) {
        CopyVector3(center, _center);
        radius = _radius;
        material = _material;
    }

    ~Sphere() {}

    Hit intersect(const Ray &ray) {
        Hit hit;
        Vector3f dist;
        SubVector3(dist, ray.start, center);            // 距离
        float a = DotVector3(ray.dir, ray.dir);        // dot表示点乘，这里是联立光线与球面方程
        float b = DotVector3(dist, ray.dir) * 2.0f;
        float c = DotVector3(dist, dist) - radius * radius;
        float delta = b * b - 4.0f * a * c;        // b^2-4ac
        if (delta < 0)        // 无交点
            return hit;
        float sqrt_delta = sqrtf(delta);
        float t1 = (-b + sqrt_delta) / 2.0f / a;    // 求得两个交点，t1 >= t2
        float t2 = (-b - sqrt_delta) / 2.0f / a;
        if (t1 <= 0)
            return hit;
        hit.t = (t2 > 0) ? t2 : t1;        // 取近的那个交点
        Vector3f hit_dist;
        MultiplyVector3andFloat(hit_dist, ray.dir, hit.t);
        AddVector3(hit.position, ray.start, hit_dist); // hit.position = ray.start + ray.dir * hit.t;
        Vector3f sub_ans;
        SubVector3(sub_ans, hit.position, center);
        DivVector3andFloat(hit.normal, sub_ans, radius);//hit.normal = (hit.position - center) / radius;
        hit.material = material;
        return hit;
    }

};

class Plane : public MyObject {    // 点法式方程表示平面
    Vector3f normal;        // 法线
    Vector3f p0;            // 面上一点坐标，N(p-p0)=0
public:
    Plane(Vector3f _p0, Vector3f _normal, Material *_material) {
        CopyVector3(normal, _normal);
        NormalizeVector3(_normal);
        CopyVector3(p0, _p0);
        material = _material;
    }

    Hit intersect(const Ray &ray) {
        Hit hit;
        float nD = DotVector3(ray.dir, normal);    // 射线方向与法向量点乘，为0表示平行
        if (nD == 0)
            return hit;

        float t1 = (DotVector3(normal, p0) - DotVector3(normal, ray.start)) / nD;
        if (t1 < 0)
            return hit;
        hit.t = t1;
        Vector3f multipleRes;
        MultiplyVector3andFloat(multipleRes, ray.dir, hit.t);
        AddVector3(hit.position, ray.start, multipleRes); // hit.position = ray.start + ray.dir * hit.t;
        CopyVector3(hit.normal, normal);
        hit.material = material;
        return hit;
    }
};

class Cube : public MyObject {
    Vector3f center;
    float a;
public:
    Cube(Vector3f _center, float _a) {
        CopyVector3(center, _center);
        a = _a;
    }

    Hit intersect(const Ray &ray) {
        Hit hit;
        return hit;
    }
};


#endif //TCODE_OBJECTS_H
