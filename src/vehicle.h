#pragma once

#include "math.h"
#include "vehicle_physics.h"
#include "track_graph.h"
#include "ribbon.h"
#include "driver.h"
#include "scene.h"

struct VehicleInput
{
    f32 accel = 0.f;
    f32 brake = 0.f;
    f32 steer = 0.f;
    bool digital = false;
    bool handbrake = false;
    bool beginShoot = false;
    bool holdShoot = false;
    bool beginShootRear = false;
    bool holdShootRear = false;
    bool switchFrontWeapon = false;
    bool switchRearWeapon = false;
    bool reset = false;
};

class Vehicle
{
// TODO: should be private
public:
    VehiclePhysics vehiclePhysics;
    VehicleTuning tuning;
    ActorUserData actorUserData;
    Controller* controller = nullptr;

	Driver* driver;
	Scene* scene;
	u32 vehicleIndex;
	Decal testDecal;
	i32 cameraIndex = -1;

    // states
    bool isInAir = true;
    bool isOnTrack = false; //
    bool isBraking = false; // TODO: this is never set
	bool isBackingUp = false;
	bool isBlocked = false;
	bool isFollowed = false;
	bool isNearHazard = false;

    // gameplay data
    VehicleInput input;
	bool finishedRace = false;
	bool useResetTransform = false;
	Vec3 cameraTargetMovePoint;
    Vec3 cameraTarget;
    Vec3 cameraFrom;
    f32 hitPoints = 0.f;
    i32 currentLap = 0;
	i32 placement = 1;
    TrackGraph::QueryResult graphResult;
    u32 preferredFollowPathIndex = 0;
    u32 currentFollowPathIndex = 0;
    f32 distanceAlongPath = 2.f;
    Vec3 previousTargetPosition;
    Vec3 startOffset = Vec3(0);
    Mat4 startTransform;
	f32 flipTimer = 0.f;
	f32 deadTimer = 0.f;
	f32 controlledBrakingTimer = 0.f;
	u32 lastDamagedBy;
	u32 lastOpponentDamagedBy = UINT32_MAX;
	f64 lastTimeDamagedByOpponent = 0.f;
	f32 smokeTimer = 0.f;
	f32 smokeTimerDamage = 0.f;
	f32 offsetChangeTimer = 0.f;
	f32 offsetChangeInterval = 5.f;
    u32 engineSound = 0;
    u32 tireSound = 0;
    Vec3 lastValidPosition;
    Vec3 previousVelocity;
    f32 engineRPM = 0.f;
    f32 lappingOffset[16] = { 0 };
    f32 engineThrottleLevel = 0.f;
	i32 currentFrontWeaponIndex = 0;
	i32 currentRearWeaponIndex = 0;
	f32 airTime = 0.f;
	f32 savedAirTime = 0.f;
	f32 airBonusGracePeriod = 0.f;
	u32 totalAirBonuses = 0;
    Vec3 shieldColor = Vec3(0, 0, 0);
    f32 shieldStrength = 0.f;

    // ai
    Vec3 targetOffset = Vec3(0);
	f32 backupTimer = 0.f;
    f32 attackTimer = 0.f;
    f32 targetTimer = 0.f;
    f32 resetTimer = 0.f;
    Vehicle* target = nullptr;
    f32 fearTimer = 0.f;
    f32 rearWeaponTimer = 0.f;

    // weapons
    SmallArray<OwnedPtr<Weapon>, ARRAY_SIZE(VehicleConfiguration::weaponIndices)>
        frontWeapons;
    SmallArray<OwnedPtr<Weapon>, ARRAY_SIZE(VehicleConfiguration::weaponIndices)>
        rearWeapons;
    SmallArray<OwnedPtr<Weapon>, 2> specialAbilities;

    Vec3 screenShakeVelocity = Vec3(0);
    Vec3 screenShakeOffset = Vec3(0);
    f32 screenShakeTimer = 0.f;
    f32 screenShakeDirChangeTimer = 0.f;

    bool isWheelSlipping[NUM_WHEELS] = {};
	Ribbon tireMarkRibbons[NUM_WHEELS];
	f32 glueSoundTimer = 0.f;

	f32 targetMotionBlurStrength = 0.f;
	f32 motionBlurStrength = 0.f;
	f32 motionBlurResetTimer = 0.f;

    Array<VehicleDebris> vehicleDebris;
    void createVehicleDebris(VehicleDebris const& debris) { vehicleDebris.push(debris); }

	struct Notification
	{
        const char* text;
        f32 secondsToKeepOnScreen;
        f32 time;
        Vec3 color;
	};
	SmallArray<Notification> notifications;

	RaceStatistics raceStatistics;

public:
	Vehicle(class Scene* scene, Mat4 const& transform, Vec3 const& startOffset,
	        Driver* driver, VehicleTuning&& tuning, u32 vehicleIndex, i32 cameraIndex);
	~Vehicle();

    Driver* getDriver() const { return driver; }
    AIDriverData* getAI() const { return (AIDriverData*)g_res.getResource(driver->aiDriverGUID); }

    bool hasAbility(const char* name)
    {
        return specialAbilities.findIf([name](auto& a){ return a->info.name == name; });
    }
    bool isDead() const { return deadTimer > 0.f; }

    void blowUp(f32 respawnTime=0.7f, bool giveAttackCredit=true);
    void reset(Mat4 const& transform);
    void applyDamage(f32 amount, u32 instigator);
    void repair(f32 amount) { hitPoints = min(hitPoints + amount, this->tuning.maxHitPoints); }
    void addNotification(const char* text, f32 time=2.f, Vec3 const& color=Vec3(1.f))
    {
        if (notifications.size() == notifications.maximumSize())
        {
            notifications.erase(notifications.begin());
        }
        notifications.push({ text, time, 0.f, color });
    }
    void addBonus(const char* name, u32 amount, Vec3 const& color = Vec3(0.9f, 0.9f, 0.01f))
    {
        raceStatistics.bonuses.push({ name, amount });
        addNotification(name, 2.f, color);
    }
    void fixup()
    {
        hitPoints = this->tuning.maxHitPoints;
        addNotification("FIXUP!", 2.f, Vec3(1.f));
    }
    void showDebugInfo();
    void restoreHitPoints()
    {
        hitPoints = this->tuning.maxHitPoints;
    }

    void updateAiInput(f32 deltaTime, RenderWorld* rw);
    void updatePlayerInput(f32 deltaTime, RenderWorld* rw);

    void onUpdate(RenderWorld* rw, f32 deltaTime);
    void onRender(RenderWorld* rw, f32 deltaTime);
    void drawWeaponAmmo(Renderer* renderer, Vec2 pos, Weapon* weapon,
            bool showAmmo, bool selected);
    void drawHUD(class Renderer* rw, f32 deltaTime);
    void shakeScreen(f32 intensity);
    void updateCamera(RenderWorld* rw, f32 deltaTime);
    void resetAmmo();
    void onTrigger(ActorUserData* userData);

    VehiclePhysics* getVehiclePhysics() { return &vehiclePhysics; }
    VehicleTuning* getTuning() { return &tuning; }
    Mat4 getTransform() { return vehiclePhysics.getTransform(); }
    f32 getForwardSpeed() { return vehiclePhysics.getForwardSpeed(); }
    PxRigidBody* getRigidBody() { return vehiclePhysics.getRigidBody(); }
    Vec3 getPosition() { return vehiclePhysics.getPosition(); }
    Vec3 getForwardVector() { return vehiclePhysics.getForwardVector(); }
    Vec3 getRightVector() { return vehiclePhysics.getRightVector(); }
    Vec3 getUpVector() { return vehiclePhysics.getUpVector(); }
    f32 getTraversedDistance() const;

    void setMotionBlur(f32 strength, f32 resetTimer)
    {
        targetMotionBlurStrength = strength;
        motionBlurResetTimer = resetTimer;
    }

    void setShield(Vec3 const& shieldColor, f32 strength)
    {
        this->shieldColor = shieldColor;
        this->shieldStrength = strength;
    }
};
