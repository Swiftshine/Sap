
#include <actor/Actor.h>
#include <player/PlayerObject.h>
#include <red/util/SpriteUtil.h>

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
}

