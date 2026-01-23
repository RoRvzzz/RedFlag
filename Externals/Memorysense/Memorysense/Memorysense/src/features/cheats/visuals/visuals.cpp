#include <iostream>
#include "visuals.h"
#include <features/cache/cache.h>
#include <sdk/sdk.h>
#include <mutex>
#include <cfloat>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <iostream>
#include <main.h>
#include "renderer/renderer.h"
#include <clipper2/clipper.h>
#include <clipper2/clipper.core.h>

namespace visuals {

    void render() {
        if (!g_visualengine || !g_datamodel) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->Flags &= ~ImDrawListFlags_AntiAliasedLines;
        std::vector<cache::cached_entity> snapshot;

        {
            std::scoped_lock lock(g_players_mutex);
            snapshot = g_player_cache;
        }

        rbx::player_t local = g_datamodel->get_local_player();
        for (auto& ent : snapshot) {
            if (memorysense::visuals::clientcheck) {
                if (!ent.character.address || ent.player.address == local.address) continue;
            }
            if (memorysense::visuals::teamcheck) {
                if (ent.team.address == local.get_team()) continue;
            }

            rbx::part_t head, left_lower_leg, right_lower_leg, left_upper_leg, right_upper_leg;
            rbx::part_t left_lower_arm, right_lower_arm, left_upper_arm, right_upper_arm;
            rbx::part_t left_hand, right_hand, left_foot, right_foot, upper_torso, lower_torso;

            rbx::part_t left_arm, right_arm, left_leg, right_leg, torso;

            rbx::humanoid_t humanoid;

            std::vector<rbx::part_t> parts;

            if (!ent.r15.r15parts.empty()) {
                humanoid = rbx::humanoid_t(ent.r15.r15parts["Humanoid"].address);

                head = ent.r15.r15parts["Head"];
                upper_torso = ent.r15.r15parts["UpperTorso"];
                lower_torso = ent.r15.r15parts["LowerTorso"];
                left_upper_leg = ent.r15.r15parts["LeftUpperLeg"];
                right_upper_leg = ent.r15.r15parts["RightUpperLeg"];
                left_lower_leg = ent.r15.r15parts["LeftLowerLeg"];
                right_lower_leg = ent.r15.r15parts["RightLowerLeg"];
                left_foot = ent.r15.r15parts["LeftFoot"];
                right_foot = ent.r15.r15parts["RightFoot"];
                left_upper_arm = ent.r15.r15parts["LeftUpperArm"];
                right_upper_arm = ent.r15.r15parts["RightUpperArm"];
                left_lower_arm = ent.r15.r15parts["LeftLowerArm"];
                right_lower_arm = ent.r15.r15parts["RightLowerArm"];
                left_hand = ent.r15.r15parts["LeftHand"];
                right_hand = ent.r15.r15parts["RightHand"];

                parts = {
                    head, upper_torso, lower_torso,
                    left_upper_leg, left_lower_leg, left_foot,
                    right_upper_leg, right_lower_leg, right_foot,
                    left_upper_arm, left_lower_arm, left_hand,
                    right_upper_arm, right_lower_arm, right_hand
                };
            }
            else if (!ent.r6.r6parts.empty()) {
                humanoid = rbx::humanoid_t(ent.r6.r6parts["Humanoid"].address);

                head = ent.r6.r6parts["Head"];
                torso = ent.r6.r6parts["Torso"];
                left_arm = ent.r6.r6parts["Left Arm"];
                right_arm = ent.r6.r6parts["Right Arm"];
                left_leg = ent.r6.r6parts["Left Leg"];
                right_leg = ent.r6.r6parts["Right Leg"];

                parts = { head, torso, left_arm, right_arm, left_leg, right_leg };
            }

            if (!head.address) continue;

            auto head_prim = head.get_primitive();
            auto head_pos = head_prim.get_position();
            auto head_screen = g_visualengine->world_to_screen(head_pos);

            if (head_screen.x <= 0.f || head_screen.y <= 0.f)
                continue;

            float left = FLT_MAX, top = FLT_MAX;
            float right = -FLT_MAX, bottom = -FLT_MAX;
            bool valid = false;

            static const math::vector3_t local_corners[8] = {
                {-1,-1,-1}, {1,-1,-1}, {-1,1,-1}, {1,1,-1},
                {-1,-1, 1}, {1,-1, 1}, {-1,1, 1}, {1,1, 1}
            };

            for (auto& part : parts) {
                if (!part.address) continue;

                auto prim = part.get_primitive();
                auto size = prim.get_size();
                auto pos = prim.get_position();
                auto rot = prim.get_rotation();

                for (const auto& lc : local_corners) {
                    math::vector3_t world = pos + rot * math::vector3_t{lc.x * size.x * 0.5f, lc.y * size.y * 0.5f, lc.z * size.z * 0.5f};
                    auto screen = g_visualengine->world_to_screen(world);
                    if (screen.x < 0.f || screen.y < 0.f) continue;

                    valid = true;
                    left = min(left, screen.x);
                    top = min(top, screen.y);
                    right = max(right, screen.x);
                    bottom = max(bottom, screen.y);
                }
            }

            if (!valid || left >= right || top >= bottom) continue;

            ImVec2 pos(left - 1.f, top - 1.f);
            ImVec2 size((right - left) + 2.f, (bottom - top) + 2.f);

            if (memorysense::visuals::box) {
                visualize::outlined(pos, size, memorysense::visuals::colors::visible::box, 0.f, memorysense::visuals::outline_elements[0], memorysense::visuals::outline_color);
            }

            struct ElementInfo {
                bool enabled;
                memorysense::visuals::ESP_SIDE side;
                int priority;
                std::string text;
                ImU32 color;
                bool use_outline;
                int element_type;
            };
            
            std::vector<ElementInfo> elements;

            if (memorysense::visuals::name) {
                ElementInfo name_element;
                name_element.enabled = true;
                name_element.side = memorysense::visuals::element_sides[0];
                name_element.priority = memorysense::visuals::element_priorities[0];
                name_element.text = ent.name;
                name_element.color = memorysense::visuals::colors::visible::name;
                name_element.use_outline = memorysense::visuals::outline_elements[2];
                name_element.element_type = 0;
                elements.push_back(name_element);
            }

            if (memorysense::visuals::distance) {
                uint64_t local_hrp_addr = g_datamodel->get_local_player().find_first_child("Head");
                rbx::part_t local_hrp(local_hrp_addr);
                math::vector3_t local_pos = local_hrp.get_primitive().get_position();
                float dist = (head_pos - local_pos).magnitude();
                char text_buffer[64];
                snprintf(text_buffer, sizeof(text_buffer), "[%.1fm]", dist);
                
                ElementInfo distance_element;
                distance_element.enabled = true;
                distance_element.side = memorysense::visuals::element_sides[1];
                distance_element.priority = memorysense::visuals::element_priorities[1];
                distance_element.text = text_buffer;
                distance_element.color = memorysense::visuals::colors::visible::distance;
                distance_element.use_outline = memorysense::visuals::outline_elements[3];
                distance_element.element_type = 1;
                elements.push_back(distance_element);
            }

            if (memorysense::visuals::flags) {
                int humanoid_state = humanoid.get_humanoid_state();
                const char* state = "Unknown";
                if (humanoid_state == 8 && head.get_primitive().get_velocity().magnitude() < 0.1f) {
                    state = "Idle";
                }
                else {
                    static const char* states[] = { "FallingDown", "Ragdoll", "GettingUp", "Jumping", "Swimming", "Freefall", "Flying", "Landed", "Running", "Unknown", "RunningNoPhysics", "StrafingNoPhysics", "Climbing", "Seated", "PlatformStanding", "Dead", "Physics", "Unknown", "None" };
                    state = states[humanoid_state];
                }
                
                ElementInfo flags_element;
                flags_element.enabled = true;
                flags_element.side = memorysense::visuals::element_sides[2];
                flags_element.priority = memorysense::visuals::element_priorities[2];
                flags_element.text = state;
                flags_element.color = memorysense::visuals::colors::visible::flags;
                flags_element.use_outline = memorysense::visuals::outline_elements[4];
                flags_element.element_type = 2;
                elements.push_back(flags_element);
            }
            
            if (memorysense::visuals::healthbar) {
                ElementInfo healthbar_element;
                healthbar_element.enabled = true;
                healthbar_element.side = memorysense::visuals::element_sides[4];
                healthbar_element.priority = memorysense::visuals::element_priorities[4];
                healthbar_element.text = "";
                healthbar_element.color = memorysense::visuals::colors::visible::healthbar;
                healthbar_element.use_outline = memorysense::visuals::outline_elements[1];
                healthbar_element.element_type = 3;
                elements.push_back(healthbar_element);
            }

            std::sort(elements.begin(), elements.end(), [](const ElementInfo& a, const ElementInfo& b) {
                return a.priority > b.priority;
            });

            float left_offset = 0.0f, right_offset = 0.0f, top_offset = 0.0f, bottom_offset = 0.0f;

            bool has_healthbar_left = false, has_healthbar_right = false, has_healthbar_top = false, has_healthbar_bottom = false;
            float healthbar_space = 0.0f;

            for (const auto& element : elements) {
                if (element.element_type == 3) {
                    float healthbar_thickness = memorysense::visuals::healthbar_size + 2.0f;
                    float healthbar_gap = 4.0f;
                    float element_spacing = 1.0f;
                    healthbar_space = healthbar_thickness + healthbar_gap + element_spacing;
                    
                    if (element.side == memorysense::visuals::ESP_SIDE::LEFT) has_healthbar_left = true;
                    else if (element.side == memorysense::visuals::ESP_SIDE::RIGHT) has_healthbar_right = true;
                    else if (element.side == memorysense::visuals::ESP_SIDE::TOP) has_healthbar_top = true;
                    else if (element.side == memorysense::visuals::ESP_SIDE::BOTTOM) has_healthbar_bottom = true;
                    break;
                }
            }

            for (const auto& element : elements) {
                if (!element.enabled) continue;
                
                float current_offset = 0.0f;
                if (element.side == memorysense::visuals::ESP_SIDE::LEFT) {
                    current_offset = left_offset;
                } else if (element.side == memorysense::visuals::ESP_SIDE::RIGHT) {
                    current_offset = right_offset;
                } else if (element.side == memorysense::visuals::ESP_SIDE::TOP) {
                    current_offset = top_offset;
                } else if (element.side == memorysense::visuals::ESP_SIDE::BOTTOM) {
                    current_offset = bottom_offset;
                }
                
                if (element.element_type == 3) {
                    // Automatically determine vertical/horizontal based on side
                    bool is_vertical = (element.side == memorysense::visuals::ESP_SIDE::LEFT || 
                                       element.side == memorysense::visuals::ESP_SIDE::RIGHT);
                    
                    visualize::healthbar_positioned(
                        pos, size, humanoid.get_health(), humanoid.get_max_health(),
                        element.color, element.side, is_vertical,
                        memorysense::visuals::healthbar_padding, memorysense::visuals::healthbar_size,
                        element.use_outline, memorysense::visuals::outline_color
                    );
                } else {
                    ImGui::PushFont(var->font.tahoma);
                    ImVec2 text_size = ImGui::CalcTextSize(element.text.c_str());
                    ImGui::PopFont();

                    float extra_offset = 0.0f;
                    if (element.side == memorysense::visuals::ESP_SIDE::LEFT && has_healthbar_left) {
                        extra_offset = healthbar_space;
                    } else if (element.side == memorysense::visuals::ESP_SIDE::RIGHT && has_healthbar_right) {
                        extra_offset = healthbar_space;
                    } else if (element.side == memorysense::visuals::ESP_SIDE::TOP && has_healthbar_top) {
                        extra_offset = healthbar_space;
                    } else if (element.side == memorysense::visuals::ESP_SIDE::BOTTOM && has_healthbar_bottom) {
                        extra_offset = healthbar_space;
                    }
                    
                    ImVec2 text_pos = visualize::get_element_position(pos, size, element.side, current_offset, text_size, extra_offset);

                    int text_align = middle;
                    if (element.side == memorysense::visuals::ESP_SIDE::LEFT) {
                        text_align = left;
                    } else if (element.side == memorysense::visuals::ESP_SIDE::RIGHT) {
                        text_align = left;
                    } else if (element.side == memorysense::visuals::ESP_SIDE::TOP) {
                        text_align = middle;
                    } else if (element.side == memorysense::visuals::ESP_SIDE::BOTTOM) {
                        text_align = middle;
                    }
                    
                    visualize::outlined_text(text_pos, size, element.text.c_str(), element.color, 0.f, text_align, element.use_outline, memorysense::visuals::outline_color);

                    float text_height = text_size.y + 3.0f;
                    if (element.side == memorysense::visuals::ESP_SIDE::LEFT) left_offset += text_height;
                    else if (element.side == memorysense::visuals::ESP_SIDE::RIGHT) right_offset += text_height;
                    else if (element.side == memorysense::visuals::ESP_SIDE::TOP) top_offset += text_height;
                    else if (element.side == memorysense::visuals::ESP_SIDE::BOTTOM) bottom_offset += text_height;
                }
            }

            if (memorysense::visuals::cham) {
                ImDrawList* draw = ImGui::GetForegroundDrawList();
                draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;

                auto project_part = [&](auto& part) -> std::vector<ImVec2> {
                    std::vector<ImVec2> projected;
                    if (!part.address) return projected;

                    auto prim = part.get_primitive();
                    auto size = prim.get_size();
                    auto pos = prim.get_position();
                    auto rot = prim.get_rotation();

                    for (const auto& lc : local_corners) {
                        math::vector3_t world = pos + rot * math::vector3_t{ lc.x * size.x * 0.5f, lc.y * size.y * 0.5f, lc.z * size.z * 0.5f };
                        auto screen = g_visualengine->world_to_screen(world);
                        if (screen.x >= 0.f && screen.y >= 0.f)
                            projected.push_back(ImVec2(screen.x, screen.y));
                    }

                    if (projected.size() < 3) return {};

                    std::sort(projected.begin(), projected.end(), [](const ImVec2& a, const ImVec2& b) {
                        return a.x < b.x || (a.x == b.x && a.y < b.y);
                        });

                    std::vector<ImVec2> hull;
                    auto cross = [](const ImVec2& O, const ImVec2& A, const ImVec2& B) {
                        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
                        };

                    for (auto& p : projected) {
                        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
                            hull.pop_back();
                        hull.push_back(p);
                    }

                    size_t t = hull.size() + 1;
                    for (int i = (int)projected.size() - 1; i >= 0; --i) {
                        auto& p = projected[i];
                        while (hull.size() >= t && cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
                            hull.pop_back();
                        hull.push_back(p);
                    }

                    hull.pop_back();
                    return hull;
                    };

                if (memorysense::visuals::cham_type == 0) {
                    for (auto& part : parts) {
                        auto hull = project_part(part);
                        if (hull.empty()) continue;
                        draw->AddConvexPolyFilled(hull.data(), hull.size(), memorysense::visuals::colors::visible::cham_fill);
                        draw->AddPolyline(hull.data(), hull.size(), memorysense::visuals::colors::visible::cham_outline, true, 2.f);
                    }
                }
                else if (memorysense::visuals::cham_type == 1) {
                    Clipper2Lib::Paths64 all_parts;
                    for (auto& part : parts) {
                        auto hull = project_part(part);
                        if (hull.size() < 3) continue;

                        Clipper2Lib::Path64 path;
                        for (auto& pt : hull)
                            path.push_back({ static_cast<int64_t>(pt.x * 1000.0), static_cast<int64_t>(pt.y * 1000.0) });
                        all_parts.push_back(path);
                    }

                    if (all_parts.empty()) return;

                    auto unified_solution = Clipper2Lib::Union(all_parts, Clipper2Lib::FillRule::NonZero);
                    for (auto& sp : unified_solution) {
                        std::vector<ImVec2> poly;
                        for (auto& pt : sp) poly.push_back(ImVec2(pt.x / 1000.0f, pt.y / 1000.0f));
                        if (poly.size() < 3) continue;

                        draw->AddConcavePolyFilled(poly.data(), poly.size(), memorysense::visuals::colors::visible::cham_fill);
                        draw->AddPolyline(poly.data(), poly.size(), memorysense::visuals::colors::visible::cham_outline, true, 2.f);
                    }
                }
            }

            if (memorysense::visuals::tracer) {
                ImVec2 screen_center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y);
                ImVec2 target(head_screen.x, head_screen.y);
                visualize::outlined_line(screen_center, target, memorysense::visuals::colors::visible::tracer, 1.0f, memorysense::visuals::outline_color, 2.0f, memorysense::visuals::outline_elements[5]);
            }

        }
    }
}