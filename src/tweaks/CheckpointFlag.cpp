// TODO: effects

#include <actor/Actor.h>
#include <actor/ActorCollision.h>
#include <player/PlayerObject.h>
#include <red/util/SpriteUtil.h>

#include <container/seadSafeArray.h>
#include <math/seadMathCalcCommon.h>

#define TELKIN_REGISTERS
#include <telkin/Assembly.h>
#include <telkin/Hooks.h>

namespace sap {
    enum CheckpointBehavior : u32 {
        Default,
        NoPowerup,
        PowerupConditionally,
        PowerupConditionallyWithOverride,
        PowerupAlways
    };

    void changePlayerPowerup(Actor* checkpoint, PlayerObject* player) {
        CheckpointBehavior behavior = static_cast<CheckpointBehavior>(red::SpriteUtil::getNybble21(checkpoint));
        PlayerMode powerup = static_cast<PlayerMode>(red::SpriteUtil::getNybble22(checkpoint));

        switch (behavior) {
            case CheckpointBehavior::Default: {
                if (player->getPlayerMode() == PlayerMode::cPlayerMode_Small) {
                    player->setItem(PlayerMode::cPlayerMode_Normal);
                    player->setItemGetEffect();
                }
                break;
            }

            case CheckpointBehavior::NoPowerup: break;

            case CheckpointBehavior::PowerupConditionally: {
                PlayerMode current = player->getPlayerMode();

                bool hasPowerupAlready = current >= PlayerMode::cPlayerMode_Fire;
                if (hasPowerupAlready) {
                    break;
                }

                // intentional fallthrough
            }

            case CheckpointBehavior::PowerupConditionallyWithOverride: {
                if (!player->canChangeTo(powerup)) {
                    break;
                }

                // intentional fallthrough
            }
            
            case CheckpointBehavior::PowerupAlways: {
                player->setItem(powerup);
                player->setItemGetEffect();
                break;
            }
        }
    }

    void changePlayerPowerupHook() tAssembly(
        tSaveLR;
        
        mr r3, r30; // checkpoint
        mr r4, r31; // player

        bl _ZN3sap19changePlayerPowerupEP5ActorP12PlayerObject;
    
        // resume
        tRestoreLR;
        addi r0, r2, 0x1C;
        mtctr r0;
        bctr;
    )

    tBranch(0x02728F14, sap::changePlayerPowerupHook, tk::BranchType::bl);
    tBranch(0x02728F98, sap::changePlayerPowerupHook, tk::BranchType::bl);

    void setModelAngle(Actor* checkpoint) {
        const u32 ORIGINAL_Y_ANGLE = -0x40000000;
        bool flip = static_cast<bool>(red::SpriteUtil::getNybble5(checkpoint));

        checkpoint->getAngle().y() = ORIGINAL_Y_ANGLE * (flip ? -1 : 1);
        
        bool affectedByGravity = (red::SpriteUtil::getNybble11(checkpoint) & 2) != 0;
        
        if (affectedByGravity) {
            return;
        }
        
        u8 givenAngle = red::SpriteUtil::getNybbleRange(checkpoint, 23, 24);
        const f32 ANGLE_DEG = 11.25;
        f32 angleDeg = ANGLE_DEG * givenAngle;

        u32 angleBAM;
        if (angleDeg < 180.0f) {
            angleBAM = sead::Mathf::deg2idx(angleDeg);
        } else {
            angleBAM = (u32)sead::Mathf::deg2idx(angleDeg - 180.0f) + 0x80000000;
        }

        checkpoint->getAngle().z() = angleBAM;
    }
    
    void changeModelAngleHook() tAssembly(
        tSaveLR;

        mr r3, r29;
        bl _ZN3sap13setModelAngleEP5Actor;

        tRestoreLR;
        blr;
    );

    tBranch(0x02727F90, sap::changeModelAngleHook, tk::BranchType::bl);

    using CC = ActorCollisionCheck;
    void setCheckpointCollision(CC* collider, Actor* checkpoint, const CC::CollisionData& colData) {
        const f32 ANGLE_DEG = 11.25f;
        const u8 givenAngle =
            red::SpriteUtil::getNybbleRange(checkpoint, 23, 24);

        const f32 angleDeg = ANGLE_DEG * givenAngle;
        const f32 angleRad = sead::Mathf::deg2rad(angleDeg);

        const f32 s = sead::Mathf::sin(angleRad);
        const f32 c = sead::Mathf::cos(angleRad);

        const f32 halfWidth = 4.0f;
        const f32 halfHeight = 24.0f;

        CC::CollisionData cd = colData;

        // tweak the size
        cd.half_size = CC::Vec2(
            sead::Mathf::abs(halfWidth * c) +
            sead::Mathf::abs(halfHeight * s),

            sead::Mathf::abs(halfWidth * s) +
            sead::Mathf::abs(halfHeight * c)
        );

        // rotate the offset
        cd.center_offset = CC::Vec2(
            -24.0f * s,
            24.0f * c
        );

        collider->set(checkpoint, cd);
    }

    tBranch(0x02728068, sap::setCheckpointCollision, tk::BranchType::bl);
}

