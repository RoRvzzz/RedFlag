#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <windows.h>
#include "engine/math/types/Math/Math.h"

namespace RBX
{
    bool Initializer();
    bool GetGlobalVariables();
    bool CheckIfGlobalVariablesInvalid();
    bool checkUmodule();
    void getrobloxId();


    class Instance final
    {
    public:

        std::uint64_t address = 0;
        std::string GetName() const;
        std::string GetClass() const;
        std::uint64_t GetRenderView();
        uintptr_t GetHumanoidEnumState();
        std::uint64_t GetCameraMatrix();
        std::pair<int, int> GetViewportSize();
        bool IsRenderViewValid();
        void SetRenderFeature(std::string featureName, bool enable);
        RBX::Vector2 GetDimensions();
        RBX::Matrix4x4 GetViewMatrix();
        std::vector<RBX::Instance> GetChildren() const;
        RBX::Instance GetParent();
        RBX::Instance FindFirstChild(std::string child) const;
        bool IsA(const std::string& className) const;
        bool IsDescendantOf(const RBX::Instance& ancestor) const;
        std::vector<RBX::Instance> GetDescendants() const;
        RBX::Instance FindFirstDescendant(const std::string& name) const;
        Instance WaitForChild(std::string childName, std::chrono::milliseconds timeout) const;
        RBX::Instance FindFirstChildOfClass(std::string child);
        Instance GetService(std::string serviceName);
        bool Destroy();
        bool SetProperty(const std::string& propertyName, std::uint64_t value);
        std::uint64_t GetProperty(const std::string& propertyName) const;
        bool Clone(RBX::Instance& newInstance) const;
        bool MoveTo(RBX::Instance& parent);
        std::vector<RBX::Instance> FindAllChildrenByName(const std::string& childName) const;
        std::vector<RBX::Instance> FindAllDescendantsByName(const std::string& name) const;
        int_fast64_t GetGameId();
        RBX::Instance GetDataModelPTR();
        RBX::Instance GetDataModel();
        int_fast64_t GetCurrentPlaceId();
        std::vector<RBX::Instance> GetPlayerList();
        bool IsInGame();
        int GetPlayerCount();
        std::string GetJobId();
        int SetAfkTime(int time);
        RBX::Instance GetLocalPlayer() const;
        RBX::Instance GetModelInstance() const;
        RBX::Instance GetModelPrimaryPart();
        std::int32_t GetRigType();
        RBX::Instance GetTeam() const;
        void SetPartPos(RBX::Vector3 argument);
        RBX::Vector3 GetMoveDirection();
        void SetPartCframe(RBX::CFrame Cframe);
        RBX::CFrame GetPartCframe();
        void writePositionMultipleTimes(std::uint64_t primitive, RBX::Vector3 position);
        uint64_t getPlayerUserId();
        void SetPartVelocity(RBX::Vector3 velocity);
        std::uint64_t SetFramePositionX(uint64_t position);
        std::uint64_t SetFramePositionY(uint64_t position);
        uint64_t SetFrameRotation(uint64_t rotation);
        float GetHealth() const;
        std::string getPlayerDisplayName();
        void SetHumanoidWalkSpeed(float walkspeed);
        void write_double(double value);
        void write_health(float health);
        void write_velocity(Vector3 velo);
        void write_extra(Vector3 velo);
        void write_cframe(RBX::CFrame newCFrame);
        void write_jumppower(float jumppower);
        float read_jumppower();
        void SetHumanoidJumpPower(float JumpPower);
        float GetMaxHealth() const;
        int getHumanoidState() const;
        RBX::CFrame GetCframe();
        static std::string ReadString(std::uint64_t address);
        RBX::Instance GetCameraInstance();
        RBX::Vector3 GetCameraPosition();
        RBX::Instance GetLighting() const;
        void Setbrightness(float brightness);
        void SetFogEnd(float FogEnd);
        void SetFogStart(float FogStart);
        void SetAmbience(RBX::Vector3 Color);
        void SetColorShiftTop(RBX::Vector3 Color);
        void* GetRaw() const {
            return (void*)this;
        }
        void SetColorShiftBottom(RBX::Vector3 Color);
        void SetBloomIntensity(float Power);
        void SetBloomSize(float Size);
        void SetBloomThreshHold(float ThreshHold);
        void SetSunRayIntensity(float Intensity);
        void SetSunRaySpread(float Spread);
        int GetColor3();
        void setColor3(RBX::Vector3 color);
        RBX::Instance Spectate(RBX::Instance stringhere);
        RBX::Instance UnSpectate();
        void SetCameraMaxZoom(int zoom);
        void SetCameraMinZoom(int zoom);
        RBX::Matrix3x3 GetCameraRotation();
        float GetFov();
        void SetCameraRotation(RBX::Matrix3x3 Rotation);
        static void CallCachedMouseService(std::uint64_t address);
        void WriteMousePosition(std::uint64_t address, float x, float y);
        void SetColor(const std::string& part_name, RBX::Vector3 color);
        Instance SetAllColor(RBX::Vector3 color);
        void SetMaterial(const std::string& part_name);
        Instance SetAllMaterial();
        void SetSize2(float size);
        std::uint64_t GetPart() const;
        int GetPartMaterial() const;
        bool SetCanCollide(bool value);
        RBX::Vector3 GetPosition() const;
        RBX::Vector3 GetVelocity() const;
        RBX::Matrix3x3 GetRotation() const;
        RBX::Vector3 GetSize() const;
        Vector3 SetSize(Vector3 size) const;
        void SetBoolFromValue(bool value) const;
        bool getBoolFromValue() const;
        void SetIntValue(int value);
        int getIntFromValue() const;
        RBX::Vector3 getVec3FromValue() const;
        void SetVec3FromValue(RBX::Vector3 Value);
        void SetFloatValue(float value) const;
        float GetFloatFromValue() const;
        void SetStringValue(const std::string& value);
        std::string getStringFromValue() const;
        int getPlayerArmor();
        RBX::Instance GetPlayersService();
        int getPlayerTeamColor();
        bool isPlayerVisible(Matrix4x4 viewMatrix);
        RBX::Instance GetVisualEngine();
        static void GetMouseService();
        int fetchPlayer(std::uint64_t address) const;
        static void updatePlayers();
        RBX::Instance GetWorkspace();
        void SetWalkSpeed(float walkspeed);
        void SetJumpPower(float jumppower);
        void SetHipHeight(float HipHeight);
        void SetWalkSpeedCompensator(float walkspeed);
        float GetWalkSpeed();
        float GetJumpPower();
        float GetHipHeight();
        void SetUseJumpPower(bool usejumppower);
        float GetPing();
        RBX::Instance GetWorkspaceByOffset();
        RBX::Instance GetCurrentCamera();
        uint64_t GetScriptContext();
        uint64_t GetDataModelViaScriptContext();
        bool isValid() const noexcept {
            return address != 0;
        }
        template<typename T>
        void WriteValue(const T& value);
        template<typename T>
        T ReadValue();
        bool operator==(const RBX::Instance& other) const {
            return this->address == other.address;
        }
        static  void updateWorkspace();
    private:
        static std::uint64_t GetInputObject(std::uint64_t base_address);
        static std::uint64_t cached_input_object;
        static std::uint64_t consistent_input_object_pointer;








    };
    class TaskScheduler final {
    public:


        std::uint64_t address = 0;
        uintptr_t GetTargetFPS();

        uintptr_t SetTargetFPS(double value);

        void PauseTask(uintptr_t jobAddress);

        uintptr_t GetScheduler();

        std::vector<RBX::Instance> GetJobs();

        std::string GetJobName(RBX::Instance instance);

        void ResumeTask(uintptr_t jobAddress);

        bool RemoveTaskByName(const std::string& tarGetName);


        void UpdateJobPriority(uintptr_t jobAddress, unsigned int newPriority);



        uint64_t GetJobByName(const std::string& tarGetName);


    private:
        // jobs structure
       // job.name
       //job.address



    };

    class PlayerInstance final
    {
    public:
        bool operator==(const PlayerInstance& other) const {
            return address == other.address;
        }

        //  RBX::Vector3 pos;
        bool isSwimming;
        bool isJumping;
        bool isRunning;
        bool isWalking;

        std::uint64_t address;
        std::vector<RBX::Instance> children;

        // Informations
        std::string name;
        RBX::Instance team;
        RBX::Instance character;
        Vector3 Position;
        Matrix3x3 Rotation;
        Vector3 Size;
        int r15;
        int shield;
        // Da Hood Exclusive
        RBX::Instance bodyEffects;
        RBX::Instance knockedOut;
        float health;
        float maxhealth;
        RBX::Instance ifGrabbed;
        RBX::Instance reloadcheckaimbot;
        RBX::Instance mousePosition;
        RBX::Instance mousePos;
        RBX::Instance currentTool;
        RBX::Instance aim;
        RBX::Instance  flame_obj;
        RBX::Instance tool;
        std::string currentToolName;
        RBX::Instance hc_aim;
        RBX::Instance Hood_Game;
        RBX::Instance weapon_aim;
        RBX::Instance Crosshair2;
        RBX::Instance jail_aim;
        RBX::Instance armor_obj;
        bool hasGunEquipped = false;

        // Parts
        RBX::Instance humanoid;
        RBX::Instance head;
        RBX::Instance  rootJoint;
        RBX::Instance  neck;
        RBX::Instance rootPart;
        RBX::Instance pf_localplr;
        RBX::Instance upperTorso;
        RBX::Instance lowerTorso;
        RBX::Instance leftUpperLeg;
        RBX::Instance leftFoot;
        RBX::Instance rightUpperLeg;
        RBX::Instance rightFoot;
        RBX::Instance leftUpperArm;
        RBX::Instance leftHand;
        RBX::Instance rightUpperArm;
        RBX::Instance leftLowerLeg;
        RBX::Instance leftLowerArm;
        RBX::Instance rightLowerArm;
        RBX::Instance rightLowerLeg;
        RBX::Instance rightHand;
        RBX::Instance leftLeg;
        RBX::Instance rightLeg;
        RBX::Instance leftArm;
        RBX::Instance rightArm;
        RBX::Instance Vehicles;
    };

    class WorkSpaceInstance final
    {
    public:
        bool operator==(const PlayerInstance& other) const {
            return address == other.address;
        }

        //      RBX::Vector3 pos;

        std::uint64_t address;
        std::vector<RBX::Instance> children;

        std::string name;

        RBX::Instance mousePosition;
        RBX::Instance mousePos;
        RBX::Instance aim;
        RBX::Instance hc_aim;
        RBX::Instance Hood_Game;
        RBX::Instance weapon_aim;
        RBX::Instance Crosshair2;
        RBX::Instance jail_aim;

        RBX::Instance Vehicles;
    };
    class WorkSpace final
    {
    public:
        bool operator==(const PlayerInstance& other) const {
            return address == other.address;
        }

        RBX::Instance lighting;
        RBX::Instance workspace;
        RBX::Instance datamodel;
        RBX::Instance players;

        std::uint64_t address;
        std::vector<RBX::Instance> children;

        // Informations
        std::string name;

    };

    void write_walkspeed(float value);

    void walkspeedloop(float value);

    float read_walkspeed();

    RBX::Vector2 WorldToScreen(RBX::Vector3 world, RBX::Vector2 dimensions, RBX::Matrix4x4 viewmatrix);

}
