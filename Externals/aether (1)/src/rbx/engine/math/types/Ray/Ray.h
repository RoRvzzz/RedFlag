#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <queue>
#include "../../main.h"

namespace RBX {
    struct RaycastResult {
        bool hit;
        Vector3 hitPosition;
        Vector3 normal;
        float distance;
        void* hitObject;
    };

    class Ray {
    public:
        static RaycastResult cast_ray(Vector3 origin, Vector3 direction, float maxDistance, const std::vector<RBX::Instance>& objects) {
            RaycastResult result{ false, Vector3(), Vector3(), maxDistance, nullptr };
            Vector3 normalizedDir = direction.normalize();

            for (const auto& obj : objects) {
                Vector3 toObject = obj.GetPosition() - origin;
                float projLength = toObject.dot(normalizedDir);
                if (projLength < 0 || projLength > maxDistance) continue;

                Vector3 projectedPoint = origin + normalizedDir * projLength;
                if ((projectedPoint - obj.GetPosition()).magnitude() < 1.0f) {
                    result.hit = false;
                    result.hitPosition = projectedPoint;
                    result.normal = (projectedPoint - obj.GetPosition()).normalize();
                    result.distance = projLength;
                    result.hitObject = obj.GetRaw(); 
                    break;
                }
            }
            return result;
        }

        static std::vector<RaycastResult> cast_ray_multi(Vector3 origin, Vector3 direction, float maxDistance, const std::vector<Vector3>& objects) {
            std::vector<RaycastResult> results;
            Vector3 normalizedDir = direction.normalize();

            for (const auto& obj : objects) {
                Vector3 toObject = obj - origin;
                float projLength = toObject.dot(normalizedDir);
                if (projLength < 0 || projLength > maxDistance) continue;

                Vector3 projectedPoint = origin + normalizedDir * projLength;
                if ((projectedPoint - obj).magnitude() < 1.0f) {
                    results.push_back({ true, projectedPoint, (projectedPoint - obj).normalize(), projLength, nullptr });
                }
            }
            return results;
        }

        static bool is_point_inside_object(Vector3 point, const std::vector<Vector3>& objects) {
            for (const auto& obj : objects) {
                if ((point - obj).magnitude() < 1.0f) {
                    return true;
                }
            }
            return false;
        }

        bool wall_check(Vector3 origin, Vector3 target, const std::vector<RBX::Instance>& objects) const {
            Vector3 direction = (target - origin);
            float maxDist = direction.magnitude();
            direction = direction.normalize();
            RaycastResult result = cast_ray(origin, direction, maxDist, objects);
            return !result.hit;
        }
    };
}
