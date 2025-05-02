#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Vector {
public:
	explicit Vector(double x = 0, double y = 0, double z = 0) {
		data[0] = x;
		data[1] = y;
		data[2] = z;
	}
	double norm2() const {
		return data[0] * data[0] + data[1] * data[1] + data[2] * data[2];
	}
	double norm() const {
		return sqrt(norm2());
	}
	void normalize() {
		double n = norm();
		data[0] /= n;
		data[1] /= n;
		data[2] /= n;
	}
	double operator[](int i) const { return data[i]; };
	double& operator[](int i) { return data[i]; };
	double data[3];
};

Vector operator+(const Vector& a, const Vector& b) {
	return Vector(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}
Vector operator-(const Vector& a, const Vector& b) {
	return Vector(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}
Vector operator*(const double a, const Vector& b) {
	return Vector(a*b[0], a*b[1], a*b[2]);
}
Vector operator*(const Vector& a, const double b) {
	return Vector(a[0]*b, a[1]*b, a[2]*b);
}
Vector operator/(const Vector& a, const double b) {
	return Vector(a[0] / b, a[1] / b, a[2] / b);
}
double dot(const Vector& a, const Vector& b) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vector cross(const Vector& a, const Vector& b) {
	return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
}

class Ray {
public:
	Vector o, d;
	Ray(const Vector& o, const Vector& d) :o(o), d(d){};
};

class Sphere {
public:
	double R;
	Vector C;
	Vector albedo;
	Sphere(double R, const Vector& C, const Vector& albedo) :R(R), C(C), albedo(albedo){
	};
	bool intersect(const Ray& ray, Vector& P, Vector& N) const {
		double delta = dot(ray.d, ray.o - C) * dot(ray.d, ray.o - C) - (dot(ray.o - C, ray.o - C)) + R * R;
		if (delta < 0) return false;
        double t1 = -dot(ray.d, ray.o - C) - sqrt(delta); 
        double t2 = -dot(ray.d, ray.o - C) + sqrt(delta);
		double t;
		if (t2 < 0) return false; 
		else if (t1 > 0) t = t1;
		else t = t2;
		P = ray.o + ray.d * t;
        N = (P - C);
        N.normalize();
		return true;
    };
};


bool pointInShadow(const Vector& P, const Vector& lightPos, const std::vector<Sphere>& spheres) {
    Vector shadowDir = lightPos - P;
    double maxDist = shadowDir.norm();  
    shadowDir.normalize();

    Ray shadowRay(P + shadowDir * 1e-4, shadowDir);  

    for (const auto& sphere : spheres) {
        Vector tempP, tempN;
        if (sphere.intersect(shadowRay, tempP, tempN)) {
            double hitDist = (tempP - P).norm();
            if (hitDist < maxDist) {
                return true;  
            }
        }
    }
    return false;  
}

class Scene {
public:
    std::vector<Sphere> spheres;
    Vector lightPos;
    double lightIntensity;
    
    Scene(const Vector& lightPos, double lightIntensity) : lightPos(lightPos), lightIntensity(lightIntensity) {}
    void addSphere(const Sphere& sphere) { spheres.push_back(sphere); }
    bool intersect(const Ray& ray, Vector& P, Vector& N, Vector& albedo) const {
        bool hit = false;
        double minDist = 1e9;
        for (const auto& sphere : spheres) {
            Vector tempP, tempN;
            if (sphere.intersect(ray, tempP, tempN)) {
                double dist = (tempP - ray.o).norm();
                if (dist < minDist) {
                    minDist = dist;
                    P = tempP;
                    N = tempN;
                    albedo = sphere.albedo;
                    hit = true;
                }
            }
        }
        return hit;
    }
};


int main() {
	int W = 512;
	int H = 512;
    Vector Camera(0, 0, 55);
    double fov = 60 * M_PI / 180.;
    Vector lightPos(-10, 20, 40);
    double lightIntensity = 1e5;
    Scene scene(lightPos, lightIntensity);
    scene.addSphere(Sphere(8, Vector(0, 0, 0), Vector(0.8, 0.8, 0.8)));
    scene.addSphere(Sphere(8, Vector(20, 0, 0), Vector(0.8, 0.8, 0.8)));
    scene.addSphere(Sphere(8, Vector(-20, 0, 0), Vector(0.8, 0.8, 0.8)));
    scene.addSphere(Sphere(940, Vector(0, 1000, 0), Vector(0.2, 0.5, 0.9)));
    scene.addSphere(Sphere(990, Vector(0, -1000, 0), Vector(0.3, 0.4, 0.7)));
    scene.addSphere(Sphere(940, Vector(0, 0, -1000), Vector(0.4, 0.8, 0.7)));
    scene.addSphere(Sphere(940, Vector(-1000, 0, 0), Vector(0.9, 0.2, 0.9)));
    scene.addSphere(Sphere(940, Vector(1000, 0, 0), Vector(0.6, 0.5, 0.1)));
    scene.addSphere(Sphere(940, Vector(0, 0, 1000), Vector(0.9, 0.4, 0.3)));


    std::vector<unsigned char> image(W * H * 3, 0);
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            double d = -W / (2. * tan(fov / 2));
            Vector direction(j + 0.5 - W / 2, H - i - 1 + 0.5 - H / 2, d);
            direction.normalize();
            Ray ray(Camera, direction);
            Vector P, N, albedo;
            if (scene.intersect(ray, P, N, albedo)) {
                Vector lightDir = scene.lightPos - P;
                double d2 = lightDir.norm2();
                lightDir.normalize();
                int vp = 1;
                bool inshad = pointInShadow(P, scene.lightPos, scene.spheres);
                if (inshad) vp = 0;
                double intensity = (scene.lightIntensity / (4 * M_PI * d2)) * vp * std::max(0.0, dot(N, lightDir));
                Vector color = (albedo / M_PI) * (255 * intensity);
                image[(i * W + j) * 3 + 0] = static_cast<unsigned char>(std::min(255.0, std::max(0.0, color[0])));
                image[(i * W + j) * 3 + 1] = static_cast<unsigned char>(std::min(255.0, std::max(0.0, color[1])));
                image[(i * W + j) * 3 + 2] = static_cast<unsigned char>(std::min(255.0, std::max(0.0, color[2])));
            }
        }
    }
	stbi_write_png("image.png", W, H, 3, &image[0], 0);

	return 0;
}