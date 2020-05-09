#pragma once

#include "math.h"
#include "vehicle_data.h"

class VehicleSceneQueryData
{
public:
    static VehicleSceneQueryData* allocate(
        const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, const PxU32 maxNumHitPointsPerWheel, const PxU32 numVehiclesInBatch,
        PxBatchQueryPreFilterShader preFilterShader, PxBatchQueryPostFilterShader postFilterShader,
        PxAllocatorCallback& allocator);
    void free(PxAllocatorCallback& allocator) { allocator.deallocate(this); }
    static PxBatchQuery* setUpBatchedSceneQuery(const PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, PxScene* scene);
    PxRaycastQueryResult* getRaycastQueryResultBuffer(const PxU32 batchId) { return (mRaycastResults + batchId * mNumQueriesPerBatch); }
    PxSweepQueryResult* getSweepQueryResultBuffer(const PxU32 batchId) { return (mSweepResults + batchId * mNumQueriesPerBatch); }
    PxU32 getQueryResultBufferSize() const { return mNumQueriesPerBatch; }

private:
    PxU32 mNumQueriesPerBatch;
    PxU32 mNumHitResultsPerQuery;
    PxRaycastQueryResult* mRaycastResults;
    PxSweepQueryResult* mSweepResults;
    PxRaycastHit* mRaycastHitBuffer;
    PxSweepHit* mSweepHitBuffer;
    PxBatchQueryPreFilterShader mPreFilterShader;
    PxBatchQueryPostFilterShader mPostFilterShader;
};

struct WheelInfo
{
    glm::mat4 transform;
    glm::vec3 position;
    glm::vec3 contactNormal;
    glm::vec3 contactPosition;
    f32 rotationSpeed;
    f32 lateralSlip;
    f32 longitudinalSlip;
    f32 oilCoverage;
    f32 dustAmount;
    bool isTouchingTrack;
    bool isOffroad;
    bool isTouchingGlue;
    bool isInAir;
};

struct GroundSpot
{
    enum GroundType
    {
        DUST,
        OIL,
        GLUE,
    };
    u32 groundType;
    glm::vec3 p;
    f32 radius;
};

struct IgnoredGroundSpot
{
    Entity* e;
    f32 t;
};

class VehiclePhysics
{
    PxVehicleDrive4W* vehicle4W;
    VehicleSceneQueryData* sceneQueryData;
    PxBatchQuery* batchQuery;
    PxVehicleDrivableSurfaceToTireFrictionPairs* frictionPairs;
	f32 engineThrottle = 0.f;
	VehicleTuning* tuning;
	PxWheelQueryResult wheelQueryResults[NUM_WHEELS];

	bool isInAir = true;

    SmallVec<GroundSpot, 16> groundSpots;
    SmallVec<IgnoredGroundSpot> ignoredGroundSpots;

    void checkGroundSpots(PxScene* physicsScene, f32 deltaTime);
    void updateWheelInfo(f32 deltaTime);

public:
    WheelInfo wheelInfo[NUM_WHEELS];

    void setup(void* userData, PxScene* scene, glm::mat4 const& transform, VehicleTuning* tuning);
    ~VehiclePhysics();

    void update(PxScene* scene, f32 timestep, bool digital, f32 accel, f32 brake, f32 steer,
            bool handbrake, bool canGo, bool onlyBrake);
    void reset(glm::mat4 const& transform);
    f32 getEngineRPM() const { return vehicle4W->mDriveDynData.getEngineRotationSpeed() * 9.5493f + 900.f; }
    f32 getForwardSpeed() const { return vehicle4W->computeForwardSpeed(); }
    f32 getSidewaysSpeed() const { return vehicle4W->computeSidewaysSpeed(); }
    PxRigidDynamic* getRigidBody() const { return vehicle4W->getRigidDynamicActor(); }
    glm::mat4 getTransform() const { return convert(PxMat44(getRigidBody()->getGlobalPose())); }
    glm::vec3 getPosition() const { return convert(getRigidBody()->getGlobalPose().p); }
    glm::vec3 getForwardVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector0()); }
    glm::vec3 getRightVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector1()); }
    glm::vec3 getUpVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector2()); }
    u32 getCurrentGear()  const{ return vehicle4W->mDriveDynData.mCurrentGear; }
    f32 getCurrentGearRatio()  const{ return tuning->gearRatios[getCurrentGear()]; }
    f32 getAverageWheelRotationSpeed() const;
    f32 getEngineThrottle() const { return engineThrottle; }
    void addIgnoredGroundSpot(Entity* e) { ignoredGroundSpots.push_back({ e, 1.f }); }
};

