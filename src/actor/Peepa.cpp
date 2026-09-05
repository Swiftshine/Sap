// TODO: randomise wait animation frame on init
// TODO: behaviour sans platform
// NOTE: need to wait until redcore has an official symbol for `linkPivotal`

#include <sap/Sap.h>
#include <red/util/SpriteUtil.h>

#include <actor/ActorMgr.h>
#include <actor/Profile.h>
#include <collision/ActorBgCollisionMgr.h>
#include <collision/ActorPolylineBgCollision.h>
#include <enemy/Enemy.h>
#include <graphics/AnimModel.h>
#include <map_obj/LiftNormalModelDraw.h>
#include <map_obj/ParentMovementMgr.h>

namespace sap {

class Peepa;
const LiftNormalModelDraw::ModelType PLATFORM_MODEL_TYPES[6] = {
    LiftNormalModelDraw::cModelType_Wood,
    LiftNormalModelDraw::cModelType_WoodSnow,
    LiftNormalModelDraw::cModelType_FerrisWheel,
    LiftNormalModelDraw::cModelType_Stone,
    LiftNormalModelDraw::cModelType_BoltPlatform,
    LiftNormalModelDraw::cModelType_Cloud,
};

// ===== PLATFORM =====
class Platform : public Actor {
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    Platform(const ActorCreateParam& param);

    ~Platform() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override;

    bool hasParent() const {
        return mParent != nullptr;
    }
private:
    // platform
    u8 mLengthInTiles;
    LiftNormalModelDraw mModel;
    ActorPolylineBgCollision<1> mSemisolidCollider;

    // movement
    u8 mMovementID;
    ParentMovementMgr mMovementManager;
};

const ActorCreateInfo Platform::cCreateInfo = {
    .offset_x = 8,
    .offset_y = -8
};

Profile* Platform::cProfile = sap::getRegistrar()->newProfile<Platform>("generic_platform")
    .resources<"lift_han_wood", "lift_han_stone", "lift_han_spin", "lift_han_sky", "lift_kanransha">(ProfileInfo::cResType_Course)
    .createInfo(&Platform::cCreateInfo)
    .build();

Platform::Platform(const ActorCreateParam& param)
    : Actor(param)
{ }

ActorBase::Result Platform::create() {
    // model
    mPos.z -= 1000.0f;
    mLengthInTiles = red::SpriteUtil::getNybble6(this) + 2;
    mModel.setModelScale(1.0f);
    mModel.setLength(mLengthInTiles);
    mModel.getModelOffset() = sead::Vector3f(-(mLengthInTiles * 16.0f * 0.5f), 0.0f, 0.0f);
    mModel.getAngle().z() = mAngle.z();

    LiftNormalModelDraw::ModelType modelType = PLATFORM_MODEL_TYPES[red::SpriteUtil::getNybble5(this)];

    mModel.setModelType(modelType);
    mModel.init();

    // collision
    const sead::Vector2f points[2] = {
        { mLengthInTiles * -8.0f, 0.0f },
        { mLengthInTiles * 8.0f, 0.0f }
    };

    mSemisolidCollider.set(this, {
        .pos_offset { 0.0f, 8.0f },
        .rot_pivot_offset { 0.0f, -8.0f },
        .points = points
    });

    mSemisolidCollider.setType(BgCollision::cType_Normal);

    BgUnitCode::Attr surfaceAttribute = BgUnitCode::cNone;

    if (modelType == LiftNormalModelDraw::cModelType_WoodSnow) {
        surfaceAttribute = BgUnitCode::cSnow;
    } else if (modelType == LiftNormalModelDraw::cModelType_Cloud) {
        surfaceAttribute = BgUnitCode::cCloud;
    }

    mSemisolidCollider.setAttr(surfaceAttribute);
    ActorBgCollisionMgr::instance()->entry(mSemisolidCollider);

    // movement
    if (!hasParent()) {
        mMovementID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 7, 8));
        u8 movementType = red::SpriteUtil::getNybble9(this);
        u32 movementMask = mMovementManager.getTypeMask(static_cast<ParentMovementType>(movementType));

        if (movementType == ParentMovementType::cPos_CenterRotation) {
            mMovementManager.linkPivotal(mPos, movementMask, mMovementID);
        } else {
            mMovementManager.link(mPos, movementMask, mMovementID);
        }

        mMovementManager.execute();
    }
    
    return cResult_Success;
}

bool Platform::draw() {
    mModel.draw();
    return true;
}

// ====== PEEPA ======

class Peepa : public Enemy {
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    Peepa(const ActorCreateParam& param);
    ~Peepa() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override;

    // void vsPlayerHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;
    // void vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;

    void updateModel() {
        constexpr f32 SCALE_FACTOR = 0.16f;
        constexpr sead::Vector3f MODEL_SCALE(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR);
        mModel->update(mPos, mAngle, MODEL_SCALE);
    }
    
    bool hasPlatform() const {
        return mPlatform != nullptr;
    }

    bool isMovementControlled() const {
        return mMovementID != 0;
    }

    sead::Vector3f getPlatformPos() const {
        const f32 PLATFORM_OFFSET = 12.0f;
        sead::Vector3f pos = mPos;
        pos.y += PLATFORM_OFFSET;
        pos.z -= 1000.0f;
        return pos;
    }

    bool shouldRotateSelf() const {
        return mShouldRotate && !hasPlatform();
    }

    bool shouldRotatePlatform() const {
        return mShouldRotate && hasPlatform();
    }

    Angle getPlatformAngle() const {
        return mPlatformAngle;
    }

private:
    // peepa model
    AnimModel* mModel;

    // platform
    Platform* mPlatform;
    Angle mPlatformAngle;

    // movement
    u8 mMovementID;
    ParentMovementMgr mMovementManager;
    bool mShouldRotate;
};

const ActorCreateInfo Peepa::cCreateInfo = {
    .offset_x = 8,
    .offset_y = -16  
};

Peepa::Peepa(const ActorCreateParam& param)
    : Enemy(param)
    , mPlatform(nullptr)
{ }

Profile* Peepa::cProfile = sap::getRegistrar()->newProfile<Peepa>("peepa")
    .resources<"teren", "lift_han_wood", "lift_han_stone", "lift_han_spin", "lift_han_sky", "lift_kanransha">(ProfileInfo::cResType_Course)
    .createInfo(&cCreateInfo)
    .build();

ActorBase::Result Peepa::create() {
    // model setup
    mModel = AnimModel::create("teren", "teren", 3);
    mModel->playSklAnim("Wait");

    // platform
    u8 platformSetting = red::SpriteUtil::getNybble1(this);
    u8 platformLength = red::SpriteUtil::getNybble2(this);
    
    if (platformSetting != 0) {
        ActorCreateParam param;
        param.profile = Platform::cProfile;
        param.parent_id = mActorUniqueID;

        // nybble 5
        param.param_0 |= static_cast<u32>(platformSetting - 1) << 0x1C;
        // nybble 6
        param.param_0 |= static_cast<u32>(platformLength) << 0x18;

        LiftNormalModelDraw::ModelType modelType = PLATFORM_MODEL_TYPES[platformSetting + 1];
        
        mPlatform = static_cast<Platform*>(ActorMgr::instance()->createImmediately(param));
        mPlatform->getPos() = mPos;
        mPlatform->getPos().y += 8.0f;
    }

    // movement
    mMovementID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 3, 4));

    if (isMovementControlled()) {
        u8 movementType = red::SpriteUtil::getNybble5(this);
        u32 movementMask = mMovementManager.getTypeMask(static_cast<ParentMovementType>(movementType));
        if (movementType == ParentMovementType::cPos_CenterRotation) {
            mMovementManager.linkPivotal(mPos, movementMask, mMovementID);
        } else {
            mMovementManager.link(mPos, movementMask, mMovementID);
        }
        mMovementManager.execute();
    }

    mShouldRotate = static_cast<bool>(red::SpriteUtil::getNybble6(this));

    return cResult_Success;
}

bool Peepa::execute() {
    if (isMovementControlled()) {
        mMovementManager.execute();
        mPos = mMovementManager.getPosition();

        if (shouldRotateSelf() && !hasPlatform()) {
            mAngle.z() = mMovementManager.getAngle();
        }

        if (shouldRotatePlatform()) {
            mPlatformAngle = mMovementManager.getAngle();
        }
    }
    
    updateModel();

    return true;
}

bool Platform::execute() {
    f32 lengthInUnits = mLengthInTiles * 16.0f;
    
    mSemisolidCollider.setAngle(mAngle.z());
    mSemisolidCollider.execute();
    
    if (hasParent()) {
        // move according to parent
        Peepa* parent = static_cast<Peepa*>(mParent);
        mPos = parent->getPlatformPos();

        if (parent->shouldRotatePlatform()) {
            mAngle.z() = parent->getPlatformAngle();
        }
    } else {
        // move according to movement mgr
        mMovementManager.execute();
        mPos = mMovementManager.getPosition();
        mAngle.z() = mMovementManager.getAngle();
    }

    mModel.getAngle().z() = mAngle.z();
    mModel.move(mPos, lengthInUnits);

    return true;
}

bool Peepa::draw() {
    mModel->draw();
    return true;
}

}
