#pragma once
#include <cstdint>

namespace Offsets {

    enum General : std::uintptr_t {
        DataModel = 0x1C0,
        Game = 0x1C0,
        Workspace = 0x178,
        GameID = 0x190,
        ScriptContext = 0x3F0,
        GameLoaded = 0x600,
        ForceNewAFKDuration = 0x1F8,
        VisualEnginePointer = 0x7a69470,
        FakeDataModelPointer = 0x7d03628
    };

    enum World : std::uintptr_t {
        SkyBoxBack = 0x110,
        SkyBoxBottom = 0x140,
        SkyBoxFront = 0x170,
        SkyBoxLeft = 0x1A0,
        SkyBoxRight = 0x1D0,
        SkyBoxTop = 0x200,
        StarCount = 0x260,
        SunTextureId = 0x230,
        MoonTextureId = 0xE0,
        ClickDistance = 0x100
    };

    enum Rendering : std::uintptr_t {
        Visual_Engine = 0x10,
        Dimensions = 0x720,
        View_Matrix = 0x4B0
    };

    enum Hierarchy : std::uintptr_t {
        Size = 0x8,
        Children = 0x70,
        Parent = 0x68,
        ChildrenEnd = 0x8,
        HierarchyInterPolation = 0x10
    };

    enum LightingEnum : std::uintptr_t {
        Ambient = 0xD8,
        Brightness = 0x120,
        ColorShift_Bottom = 0xF0,
        ColorShift_Top = 0xE4,
        OutdoorAmbient = 0x108,
        FogColor = 0xFC,
        FogStart = 0x138,
        FogEnd = 0x134
    };

    // Lighting struct for lowercase access (code uses Offsets::Lighting::brightness)
    struct Lighting {
        static constexpr std::uintptr_t ambient = 0xD8;
        static constexpr std::uintptr_t brightness = 0x120;
        static constexpr std::uintptr_t colorshift_bottom = 0xF0;
        static constexpr std::uintptr_t colorshift_top = 0xE4;
        static constexpr std::uintptr_t fogstart = 0x138;
        static constexpr std::uintptr_t fogend = 0x134;
        static constexpr std::uintptr_t intensity = 0x14C;
        static constexpr std::uintptr_t size = 0x150;
        static constexpr std::uintptr_t threshold = 0x154;
        static constexpr std::uintptr_t spread = 0x158;
        static constexpr std::uintptr_t color = 0x194;
    };

    enum Entity : std::uintptr_t {
        Local_Entity = 0x130,
        Model_Instance = 0x360,
        Humanoid_RigType = 0x1C8,
        Primary_Part = 0x268,
        Move_Direction = 0x158,
        WalkSpeed = 0x1D4,
        WalkSpeed1 = 0x1D4,
        WalkSpeed2 = 0x3C0,
        WalkSpeedCheck = 0x3C0,
        JumpPower = 0x1B0,
        JumpPower1 = 0x1B0,
        HipHeight = 0x1A0,
        AutoJump = 0x1DB,
        Sit = 0x1DC,
        AssemblyLinearVelocity = 0xF0,
        AssemblyAngularVelocity = 0xFC,
        Primitive = 0x148,
        Humanoid_CurrentData = 0x8D8,
        UseJumpPower = 0x1BC
    };

    enum utils : std::uintptr_t {
        AnimationID = 0xD0,
        MoveDirection = 0x158,
        ClickDetectorMaxActivationDistance = 0x100,
        MeshPartColor3 = 0x194,
        MaterialType = 0x228,
        CameraSubject = 0xE8,
        Transparency = 0xF0
    };

    enum TaskScheduler : std::uintptr_t {
        Pointer = 0x7E1BC88,
        JobName = 0x18,
        JobStart = 0x1D0,
        JobEnd = 0x1D8,
        CurrentFps = 0x1B0,
        JobId = 0x138,
        JobPriority = 0x1C,
        JobInterpation = 0x8,
        JobBuisnessPlaying = 0x20
    };

    enum Identity : std::uintptr_t {
        Name = 0xB0,
        DisplayName = 0x130,
        Class_Name = 0x8,
        Team = 0x270,
        UserId = 0x298,
        Value = 0xD0,
        Ping = 0xC8,
        TeamColor = 0xD0
    };

    enum Input : std::uintptr_t {
        MousePosition = 0xA0,
        InputObject = 0xA0,
        MouseSensitivity = 0x7DAE210
    };

    enum Camera : std::uintptr_t {
        Camera_Position = 0x11C,
        Camera_Rotation = 0xF8,
        CameraMaxZoomOut = 0x2F0,
        CameraMinZoomOut = 0x2F4,
        FieldOfView = 0x160,
        Camera_Offset = 0xE8
    };

    enum Part : std::uintptr_t {
        Part_Primitive = 0x148,
        Part_Material = 0x228,
        Part_Position = 0x12C,
        Part_Velocity = 0xF0,
        Part_Rotation = 0xC0,
        Part_Size = 0x1B0,
        Part_Color = 0x194,
        CFrame = 0xC0
    };

    enum Frame : std::uintptr_t {
        Xoffset = 0x520,
        Yoffset = 0x528,
        Rotation = 0x188,
        sizeX = 0x540,
        sizeY = 0x544,
        posX = 0x520,
        posY = 0x528
    };

    enum Health : std::uintptr_t {
        Current = 0x194,
        Max = 0x1B4
    };

    enum PrimitiveFlags : std::uintptr_t {
        Anchored = 0x2,
        CanCollide = 0x8,
        CanTouch = 0x10
    };

    //// Global offset aliases for backward compatibility
    //inline constexpr std::uintptr_t WalkSpeed1 = Entity::WalkSpeed1;
    //inline constexpr std::uintptr_t WalkSpeed2 = Entity::WalkSpeed2;
    //inline constexpr std::uintptr_t JumpPower1 = Entity::JumpPower1;
    //inline constexpr std::uintptr_t CFrame = Part::CFrame;
    //inline constexpr std::uintptr_t CanCollide = PrimitiveFlags::CanCollide;
    //inline constexpr std::uintptr_t Humanoid_CurrentData = Entity::Humanoid_CurrentData;
    //inline constexpr std::uintptr_t JobPriority = TaskScheduler::JobPriority;
    //inline constexpr std::uintptr_t JobInterpation = TaskScheduler::JobInterpation;
    //inline constexpr std::uintptr_t JobBuisnessPlaying = TaskScheduler::JobBuisnessPlaying;
}