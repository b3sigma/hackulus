#include <assert.h>
#include "Player.h"
#include <Kernel/OVR_Alg.h>

Player::Player(void)
    : UserEyeHeight(1.8f), EyePos(7.7f, 1.8f, -1.0f, 0.0f), EyeYaw(YawInitial),
      EyePitch(0), EyeRoll(0), LastSensorYaw(0),
      camera_(fd::Camera::LOOK) {
  GamepadMove = Vector3f(0);
  GamepadRotate = Vector3f(0);
  memset(&Inputs, 0, sizeof(Inputs));
}

Player::~Player(void) {
}

void Player::SetInput(InputType type, bool isDown, int mask) {
  assert((int)type >= 0 && (int)type < NumInputTypes);
  Inputs[type] = (isDown) ? Inputs[type] | mask : Inputs[type] & ~mask;
}

void Player::HandleCollision(double dt,
    Array<Ptr<CollisionModel> >* collisionModels,
    Array<Ptr<CollisionModel> >* groundCollisionModels, bool shiftDown) {
  if (Inputs[MoveForward] || Inputs[MoveBackward] || Inputs[MoveLeft] || Inputs[MoveRight]
      || GamepadMove.LengthSq() > 0) {
    Vector3f orientationVector;
    // Handle keyboard movement.
    // This translates EyePos based on Yaw vector direction and keys pressed.
    // Note that Pitch and Roll do not affect movement (they only affect view).
    if (Inputs[MoveForward] || Inputs[MoveBackward] || Inputs[MoveLeft] || Inputs[MoveRight]) {
      Vector3f localMoveVector(0, 0, 0);
      Matrix4f yawRotate = Matrix4f::RotationY(EyeYaw);

      if (Inputs[MoveForward]) {
        localMoveVector = ForwardVector;
      } else if (Inputs[MoveBackward]) {
        localMoveVector = -ForwardVector;
      }

      if (Inputs[MoveRight]) {
        localMoveVector += RightVector;
      } else if (Inputs[MoveLeft]) {
        localMoveVector -= RightVector;
      }

      // Normalize vector so we don't move faster diagonally.
      localMoveVector.Normalize();
      orientationVector = yawRotate.Transform(localMoveVector);
    } else if (GamepadMove.LengthSq() > 0) {
      Matrix4f yawRotate = Matrix4f::RotationY(EyeYaw);
      GamepadMove.Normalize();
      orientationVector = yawRotate.Transform(GamepadMove);
    }

    float moveLength = OVR::Alg::Min<float>(
        MoveSpeed * (float) dt * (shiftDown ? 3.0f : 1.0f), 1.0f);

    float checkLengthForward = moveLength;
    Planef collisionPlaneForward;
    float checkLengthLeft = moveLength;
    Planef collisionPlaneLeft;
    float checkLengthRight = moveLength;
    Planef collisionPlaneRight;
    bool gotCollision = false;
    bool gotCollisionLeft = false;
    bool gotCollisionRight = false;
    Vector3f& v3EyePos = EyePos.asV3();

    for (unsigned int i = 0; i < collisionModels->GetSize(); ++i) {
      // Checks for collisions at eye level, which should prevent us from
      // slipping under walls
      if (collisionModels->At(i)->TestRay(v3EyePos, orientationVector,
          checkLengthForward, &collisionPlaneForward)) {
        gotCollision = true;
      }

      Matrix4f leftRotation = Matrix4f::RotationY(
          45 * (Math<float>::Pi / 180.0f));
      Vector3f leftVector = leftRotation.Transform(orientationVector);
      if (collisionModels->At(i)->TestRay(v3EyePos, leftVector, checkLengthLeft,
          &collisionPlaneLeft)) {
        gotCollisionLeft = true;
      }
      Matrix4f rightRotation = Matrix4f::RotationY(
          -45 * (Math<float>::Pi / 180.0f));
      Vector3f rightVector = rightRotation.Transform(orientationVector);
      if (collisionModels->At(i)->TestRay(v3EyePos, rightVector, checkLengthRight,
          &collisionPlaneRight)) {
        gotCollisionRight = true;
      }
    }

    if (gotCollision) {
      // Project orientationVector onto the plane
      Vector3f slideVector = orientationVector
          - collisionPlaneForward.N
              * (orientationVector.Dot(collisionPlaneForward.N));

      // Make sure we aren't in a corner
      for (unsigned int j = 0; j < collisionModels->GetSize(); ++j) {
        if (collisionModels->At(j)->TestPoint(
            EyePos.asV3() - Vector3f(0.0f, RailHeight, 0.0f)
                + (slideVector * (moveLength)))) {
          moveLength = 0;
        }
      }
      if (moveLength != 0) {
        orientationVector = slideVector;
      }
    }
    // Checks for collisions at foot level, which allows us to follow terrain
    orientationVector *= moveLength;
    v3EyePos += orientationVector;

    Planef collisionPlaneDown;
    float finalDistanceDown = 10;

    for (unsigned int i = 0; i < groundCollisionModels->GetSize(); ++i) {
      float checkLengthDown = 10;
      if (groundCollisionModels->At(i)->TestRay(v3EyePos,
          Vector3f(0.0f, -1.0f, 0.0f), checkLengthDown, &collisionPlaneDown)) {
        finalDistanceDown = Alg::Min(finalDistanceDown, checkLengthDown);
      }
    }

    // Maintain the minimum camera height
    if (UserEyeHeight - finalDistanceDown < 1.0f) {
      v3EyePos.y += UserEyeHeight - finalDistanceDown;
    }
  }
}

/************************************************************************************
 Modified from:
 Copyright   :   Copyright 2012 Oculus VR, Inc. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 *************************************************************************************/
