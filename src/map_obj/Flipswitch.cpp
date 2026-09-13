// TODO: handle variable length (1-3 blocks)
// TODO: add single block flipswitch block model
// TODO: fix flipswitch block shading
// TODO: add flipswitch *panel*

#include <sap/Sap.h>
#include <red/util/SpriteUtil.h>

#include <actor/ActorMgr.h>
#include <audio/GameAudio.h>
#include <collision/ActorBgCollisionMgr.h>
#include <graphics/AnimModel.h>
#include <map/SwitchFlagMgr.h>
#include <map_obj/ActorBlockBase.h>

namespace sap {

// ===== FLIPSWITCH MANAGER =====
class FlipswitchManager : public Actor {
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    FlipswitchManager(const ActorCreateParam& param);
    ~FlipswitchManager() override = default;

    Result create() override;
    bool execute() override;

    u8 getManagerID() const {
        return mID;
    }

    void incrementSwitchCount() {
        mNumSwitches++;
    }

    void incrementActivatedSwitchCount() {
        mNumActivatedSwitches++;
    }

    void decrementActivatedSwitchCount() {
        mNumActivatedSwitches--;
    }

    bool isCompleted() const {
        return mNumSwitches != 0 && mNumSwitches == mNumActivatedSwitches;
    }
private:
    u8 mID;
    u8 mTargetEventID;
    u32 mNumSwitches;
    u32 mNumActivatedSwitches;
    bool mShouldActivateEvent;
};


const ActorCreateInfo FlipswitchManager::cCreateInfo = {
    .flag = ActorCreateInfo::cFlag_IgnoreSpawnRange
};

Profile* FlipswitchManager::cProfile = sap::getRegistrar()->newProfile<FlipswitchManager>("flipswitch_manager")
    .createInfo(&FlipswitchManager::cCreateInfo)
    .build();

FlipswitchManager::FlipswitchManager(const ActorCreateParam& param)
    : Actor(param)
{ }

ActorBase::Result FlipswitchManager::create() {
    mID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 1, 2));
    mTargetEventID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 3, 4));
    mShouldActivateEvent = red::SpriteUtil::getNybble5(this) == 0;

    mNumSwitches = 0;
    mNumActivatedSwitches = 0;

    return cResult_Success;
}

bool FlipswitchManager::execute() {
    ActorMgr* actors = ActorMgr::instance();

    if (isCompleted()) {
        SwitchFlagMgr::instance()->set(mTargetEventID - 1, 0, mShouldActivateEvent);
    }

    return true;
}

class FlipswitchBase {
public:
    FlipswitchBase()
        : mManager(nullptr)
        , mManagerID(0)
        , mIsActivated(false)
        , mIsCompleted(false)
    { }

    virtual void setCompleted() = 0;

    void searchForManager() {
        ActorMgr* actors = ActorMgr::instance();
        for (ActorMgr::iterator it = actors->getActorBegin(); it != actors->getActorEnd(); it++) {
            Actor* actor = static_cast<Actor*>(*it);
            if (actor == nullptr || actor->getProfile() != FlipswitchManager::cProfile) {
                continue;
            }

            FlipswitchManager* mgr = static_cast<FlipswitchManager*>(actor);

            if (mgr->getManagerID() == mManagerID) {
                mManager = mgr;
                mManager->incrementSwitchCount();
                onManagerFound();
                break;
            }
        }
    }

    virtual void onManagerFound() = 0;

    bool hasManager() const {
        return mManager != nullptr;
    }
protected:
    bool mManagerFound;
    FlipswitchManager* mManager;
    u8 mManagerID;
    bool mIsActivated;
    bool mIsCompleted;
};

// ===== FLIPSWITCH BLOCK =====
class FlipswitchBlock : public ActorBlockBase, public FlipswitchBase {
public:
    enum Length : u32 {
        One,
        Two,
        Three
    };

    enum Frame : u32 {
        Untouched,
        Touched,
        Completed
    };

    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    FlipswitchBlock(const ActorCreateParam& param);
    ~FlipswitchBlock() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override;

    void spawnItemUp() override;
    void spawnItemDown() override;

    void setCompleted() override {
        mIsCompleted = true;
        setFrame(Frame::Completed);
        changeState(StateID_HitWait);
    }

    void onManagerFound() override {
        ActorBgCollisionMgr::instance()->entry(mBoxBgCollision);
    }

    void updateModel() const {
        mModel->update(mPos, mAngle, mScale);
    }

    void doFlip() {
        mIsActivated = !mIsActivated;
        setFrame(mIsActivated ? Frame::Touched : Frame::Untouched);

        if (mIsActivated) {
            mManager->incrementActivatedSwitchCount();
        } else {
            mManager->decrementActivatedSwitchCount();
        }

        GameAudio::getAudioObjMap()->startSound("SE_OBJ_STEP_ON_SWITCH", mPos);
        
        // check if we were the last one
        if (mManager->isCompleted()) {
            GameAudio::getAudioObjMap()->startSound("SE_OBJ_GET_DRAGON_COIN_COMPLETE", mPos);
        }
    }

    bool isActivated() const {
        return mIsActivated;
    }

    void setFrame(Frame frame) const {
        mModel->getTexAnim(0)->getFrameCtrl().setFrame(frame);
    }
private:
    AnimModel* mModel;
    Length mLength;
};

const ActorCreateInfo FlipswitchBlock::cCreateInfo = {
    .offset_x = 8,
    .offset_y = -8,
    .flag = ActorCreateInfo::cFlag_MapObj
};

Profile* FlipswitchBlock::cProfile = sap::getRegistrar()->newProfile<FlipswitchBlock>("flipswitch_block")
    .resources<"block_flipstch">(ProfileInfo::cResType_Course)
    .createInfo(&FlipswitchBlock::cCreateInfo)
    .build();

FlipswitchBlock::FlipswitchBlock(const ActorCreateParam& param)
    : ActorBlockBase(param)
{ }

ActorBase::Result FlipswitchBlock::create() {
    mIsActivated = false;
    mIsCompleted = false;

    mLength = static_cast<Length>(red::SpriteUtil::getNybble1(this));
    mManagerID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 2, 3));

    // block
    if (!ActorBlockBase::init(false, true)) {
        return cResult_Failed;
    }

    registerColliderActiveInfo();
    ActorBgCollisionMgr::instance()->release(mBoxBgCollision);

    changeState(StateID_Wait);
    mBoxBgCollision.setType(BgCollision::cType_QuestionBlock);
    mContent = cContent_Empty;

    sead::Buffer<sead::Vector2f>& points = mBoxBgCollision.getPoints();

    f32 dist;

    switch (mLength) {
        case One: dist = 8.0f; break;
        case Two: dist = 16.0f; break;
        case Three: dist = 32.0f; break;
    };

    points[0].x -= dist;
    points[1].x += dist;
    points[2].x += dist;
    points[3].x -= dist;

    for (u32 i = 0; i < 4; i++) {
        constexpr f32 Y_OFFSET = 8.0f;
        points[i].y -= Y_OFFSET;
    }

    // model
    mScale = sead::Vector3f(1.0f, 1.0f, 1.0f);
    mModel = AnimModel::create("block_flipstch", "block_flipstch", 0, 2);
    mModel->playTexAnim("flip_two_wide");

    FrameCtrl& frameCtrl = mModel->getTexAnim(0)->getFrameCtrl();

    frameCtrl.setPlayMode(FrameCtrl::cMode_NoRepeat);
    frameCtrl.setRate(0.0f);
    frameCtrl.setFrame(Frame::Untouched);

    // misc

    if (mType == cType_Hit) {
        setCompleted();
    }
    
    searchForManager();

    if (hasManager()) {
        execute();
        updateModel();
    }
    
    return cResult_Success;
}

bool FlipswitchBlock::execute() {
    if (!hasManager()) {
        searchForManager();
        if (!hasManager()) {
            return true;
        }
    }
    
    if (mManager->isCompleted()) {
        setCompleted();
    }

    if (!ActorBlockBase::execute()) {
        return false;
    }

    if (mType == cType_Hit && mIsCompleted) {
        changeState(StateID_HitWait);
    }

    updateModel();
    return true;
}

bool FlipswitchBlock::draw() {
    mModel->draw();
    return true;
}

void FlipswitchBlock::spawnItemUp() {
    changeState(StateID_Wait);

    if (mIsCompleted) {
        return;
    }

    doFlip();
}

void FlipswitchBlock::spawnItemDown() {
    changeState(StateID_Wait);

    if (mIsCompleted) {
        return;
    }
    
    doFlip();
}

}
