// TODO: randomise wait animation frame on init
// TODO: behaviour sans platform
// TODO: platform rotation
// TODO: different platform types
// TODO: movement control

#include <sap/Sap.h>

#include <actor/Profile.h>
#include <collision/ActorBgCollisionMgr.h>
#include <collision/ActorPolylineBgCollision.h>
#include <enemy/Enemy.h>
#include <graphics/AnimModel.h>
#include <map_obj/LiftNormalModelDraw.h>

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

    void configurePlatformModel(LiftNormalModelDraw::ModelType modelType, u32 lengthInTiles) {
        mPlatformModel.setModelScale(1.0f);
        mPlatformModel.setLength(lengthInTiles);
        mPlatformModel.getModelOffset() = sead::Vector3f(-(lengthInTiles * 16.0f * 0.5f), 16.0f, mPos.z - 100.0f);
        mPlatformModel.setModelType(modelType);
        mPlatformModel.init();
    }

    void updatePlatformModel() {
        const sead::Vector3f pos(mPos.x, mPos.y, mPos.z - 10000.0f);
        mPlatformModel.move(pos, getPlatformLengthInUnits());
    }

    f32 getPlatformLengthInUnits() {
        return mPlatformLengthInTiles * 16.0f;
    }
private:
    // peepa model
    AnimModel* mModel;

    // platform
    bool mHasPlatform;
    LiftNormalModelDraw mPlatformModel;
    ActorPolylineBgCollision<1> mSemisolidCollider;
    u32 mPlatformLengthInTiles;
};

const ActorCreateInfo Peepa::cCreateInfo = {
    .offset_x = 8,
    .offset_y = -16  
};

Peepa::Peepa(const ActorCreateParam& param)
    : Enemy(param)
{ }

Profile* Peepa::cProfile = sap::getRegistrar()->newProfile<Peepa>("peepa")
    .resources<"teren", "lift_han_wood"
    // , "lift_han_wood", "lift_han_stone", "lift_han_spin", "lift_han_sky"
    >(ProfileInfo::cResType_Course)
    .createInfo(&cCreateInfo)
    .build();

ActorBase::Result Peepa::create() {
    // model setup
    mModel = AnimModel::create("teren", "teren", 3);
    mModel->playSklAnim("Wait");

    mPlatformLengthInTiles = 4;
    configurePlatformModel(LiftNormalModelDraw::cModelType_Wood, mPlatformLengthInTiles);
    mHasPlatform = true;

    // collision
    const sead::Vector2f points[2] = {
        { mPlatformLengthInTiles * -8.0f, 0.0f },
        { mPlatformLengthInTiles * 8.0f, 0.0f },
    };

    mSemisolidCollider.set(this, {
        .pos_offset = { 0.0f, 24.0f },
        .rot_pivot_offset = { 0.0f, 0.0f },
        .points = points,
    });

    mSemisolidCollider.setType(BgCollision::cType_Normal);
    mSemisolidCollider.setAttr(BgUnitCode::cNone);

    ActorBgCollisionMgr::instance()->entry(mSemisolidCollider);

    return cResult_Success;
}

bool Peepa::execute() {
    if (mHasPlatform) {
        updatePlatformModel();
        mSemisolidCollider.setAngle(mAngle.z());
        mSemisolidCollider.execute();
    }

    updateModel();

    return true;
}

bool Peepa::draw() {
    mModel->draw();

    if (mHasPlatform) {
        mPlatformModel.draw();
    }

    return true;
}

