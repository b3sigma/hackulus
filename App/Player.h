#ifndef OVR_WorldDemo_Player_h
#define OVR_WorldDemo_Player_h

#include "../fourd/common/camera.h"

#include "OVR.h"
#include "../CommonRender/Render/Render_Device.h"

using namespace OVR;
using namespace OVR::Render;

//-------------------------------------------------------------------------------------
// The RHS coordinate system is defines as follows (as seen in perspective view):
//  Y - Up
//  Z - Back
//  X - Right
const Vector3f UpVector(0.0f, 1.0f, 0.0f);
const Vector3f ForwardVector(0.0f, 0.0f, -1.0f);
const Vector3f RightVector(1.0f, 0.0f, 0.0f);

// We start out looking in the positive Z (180 degree rotation).
const float YawInitial = 3.141592f;
const float Sensitivity = 1.0f;
const float MoveSpeed = 3.0f; // m/s

// These are used for collision detection
const float RailHeight = 0.8f;

//-------------------------------------------------------------------------------------
// ***** Player

// Player class describes position and movement state of the player in the 3D world.
class Player {
public:

  enum InputType {
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    MoveIn,
    MoveOut,
    RollRightUp,
    RollInsideRight,
    RollUpInside,
    NumInputTypes,
  };
  void SetInput(InputType type, bool isDown, int mask = 1);


  fd::Camera camera_;

  float UserEyeHeight;
  Vector4f EyePos;

  // This is the current approach to eye positions
  // Leave it for now, but the right approach would seem to be to store a full 4d matrix for each eye.
  // Then, for rotations, pitch changes, or roll changes, we apply the appropriate matrix transform
  // in the approach combination of spaces. It seems like the eyes can be mapped to a 3d subset of'
  // coordinates at any given time. Consequently, this would give 4 modes that would be toggled
  // through in order to select motion. This is probably going to be a vomit comet.
  // Then again, are 4 modes enough, because for each mode there are 6 different arrangements
  // of coordinates that make sense. Limits are probably good.

  // The other approach is to simply keep the head movements for 3d only and then do rotations
  // with the other dimension with some other kind of input. The hard part of this is that
  // aside from strict translation in w, all the other movements will incorporate some kind
  // of movement in one of the other dimensions. To constrain it too heavily seems likely
  // to remove the very purpose of the project.

  float EyeYaw; // Rotation around Y, CCW positive when looking at RHS (X,Z) plane.
  float EyePitch; // Pitch. If sensor is plugged in, only read from sensor.
  float EyeRoll; // Roll, only accessible from Sensor.
  float LastSensorYaw; // Stores previous Yaw value from to support computing delta.

  // Movement state; different bits may be set based on the state of keys.
  UByte Inputs[NumInputTypes];
  Vector3f GamepadMove, GamepadRotate;

  Player();
  ~Player();

  // At least collision is ray tracing based, which is probably tractable in 4d.
  // Although it is going to be interesting trying to figure out what everything means in terms
  // of being inside stuff or not.
  void HandleCollision(double dt, Array<Ptr<CollisionModel> >* collisionModels,
      Array<Ptr<CollisionModel> >* groundCollisionModels, bool shiftDown);
};

#endif

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
