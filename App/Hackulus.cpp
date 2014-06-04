#include <vector>

#include "../fourd/common/fourmath.h"
#include "../fourd/common/mesh.h"

#include "OVR.h"

#include "../CommonRender/Platform/Platform_Default.h"
#include "../CommonRender/Render/Render_Device.h"
#include "../CommonRender/Render/Render_XmlSceneLoader.h"
#include "../CommonRender/Render/Render_FontEmbed_DejaVu48.h"
#include "../CommonRender/Platform/Gamepad.h"



#include <Kernel/OVR_SysFile.h>
#include <Kernel/OVR_Log.h>
#include <Kernel/OVR_Timer.h>

#include "Player.h"

// Filename to be loaded by default, searching specified paths.
//#define WORLDDEMO_ASSET_FILE  "Tuscany.xml"
//#define WORLDDEMO_ASSET_FILE  "Tiny.xml"
#define WORLDDEMO_ASSET_FILE  "Trivial.xml"
#define WORLDDEMO_ASSET_PATH1 "Assets/Tuscany/"
#define WORLDDEMO_ASSET_PATH2 "../Assets/Tuscany/"
// This path allows the shortcut to work.
#define WORLDDEMO_ASSET_PATH3 "Samples/OculusWorldDemo/Assets/Tuscany/"

using namespace OVR;
using namespace OVR::Platform;
using namespace OVR::Render;
//  HackulusApp::OnStartup - This function will initialize OVR::DeviceManager and HMD,
//                                    creating SensorDevice and attaching it to SensorFusion.
//                                    This needs to be done before obtaining sensor data.
//
//  HackulusApp::OnIdle    - Here we poll SensorFusion for orientation, apply it
//                                    to the scene and handle movement.
//                                    Stereo rendering is also done here, by delegating to
//                                    to Render function for each eye.

// An instance of this class is created on application startup (main/WinMain).
// It then works as follows:
//  - Graphics and HMD setup is done HackulusApp::OnStartup(). This function
//    also creates the room model from Slab declarations.
//  - Per-frame processing is done in OnIdle(). This function processes
//    sensor and movement input and then renders the frame.
//  - Additional input processing is done in OnMouse, OnKey.

class HackulusApp: public Application, public MessageHandler {
public:
  HackulusApp();
  ~HackulusApp();

  virtual int OnStartup(int argc, const char** argv);
  virtual void OnIdle();

  virtual void OnMouseMove(int x, int y, int modifiers);
  virtual void OnKey(OVR::KeyCode key, int chr, bool down, int modifiers);
  virtual void OnResize(int width, int height);

  virtual void OnMessage(const Message& msg);

  void Render(const StereoEyeParams& stereo);

  // Sets temporarily displayed message for adjustments
  void SetAdjustMessage(const char* format, ...);
  // Overrides current timeout, in seconds (not the future default value);
  // intended to be called right after SetAdjustMessage.
  void SetAdjustMessageTimeout(float timeout);

  // Stereo setting adjustment functions.
  // Called with deltaTime when relevant key is held.
  void AdjustFov(float dt);
  void AdjustAspect(float dt);
  void AdjustIPD(float dt);
  void AdjustEyeHeight(float dt);

  void AdjustMotionPrediction(float dt);

  void AdjustDistortion(float dt, int kIndex, const char* label);
  void AdjustDistortionK0(float dt) {
    AdjustDistortion(dt, 0, "K0");
  }
  void AdjustDistortionK1(float dt) {
    AdjustDistortion(dt, 1, "K1");
  }
  void AdjustDistortionK2(float dt) {
    AdjustDistortion(dt, 2, "K2");
  }
  void AdjustDistortionK3(float dt) {
    AdjustDistortion(dt, 3, "K3");
  }

  void AdjustDistortion(float val, int kIndex);
  void AdjustEsd(float val);

  // Adds room model to scene.
  void PopulateScene(const char* fileName);
  void PopulatePreloadScene();
  void ClearScene();

  // Magnetometer calibration procedure
  void UpdateManualMagCalibration();

protected:
  RenderDevice* pRender;
  RendererParams RenderParams;
  int Width, Height;
  int Screen;
  int FirstScreenInCycle;

  // *** Oculus HMD Variables
  Ptr<DeviceManager> pManager;
  Ptr<SensorDevice> pSensor;
  Ptr<HMDDevice> pHMD;
  Ptr<Profile> pUserProfile;
  SensorFusion SFusion;
  HMDInfo TheHMDInfo;

  Ptr<LatencyTestDevice> pLatencyTester;
  Util::LatencyTest LatencyUtil;

  double LastUpdate;
  int FPS;
  int FrameCounter;
  double NextFPSUpdate;

  Array<Ptr<CollisionModel> > CollisionModels;
  Array<Ptr<CollisionModel> > GroundCollisionModels;

  // Loading process displays screenshot in first frame
  // and then proceeds to load until finished.
  enum LoadingStateType {
    LoadingState_Frame0, LoadingState_DoLoad, LoadingState_Finished
  };

  // Player
  Player ThePlayer;
  Matrix4f View;
  ViewMatrices FullView;
  Scene MainScene;
  Scene LoadingScene;
  Scene GridScene;
  Scene YawMarkGreenScene;
  Scene YawMarkRedScene;
  Scene YawLinesScene;

  LoadingStateType LoadingState;

  Ptr<ShaderFill> LitSolid, LitTextures[4];

  // Stereo view parameters.
  StereoConfig SConfig;
  PostProcessType PostProcess;

  // LOD
  String MainFilePath;
  Array<String> LODFilePaths;
  int ConsecutiveLowFPSFrames;
  int CurrentLODFileIndex;

  float DistortionK0;
  float DistortionK1;
  float DistortionK2;
  float DistortionK3;

  String AdjustMessage;
  double AdjustMessageTimeout;

  // Saved distortion state.
  float SavedK0, SavedK1, SavedK2, SavedK3;
  float SavedESD, SavedAspect, SavedEyeDistance;

  // Allows toggling color around distortion.
  Color DistortionClearColor;

  // Stereo settings adjustment state.
  typedef void (HackulusApp::*AdjustFuncType)(float);
  bool ShiftDown;
  AdjustFuncType pAdjustFunc;
  float AdjustDirection;

  enum SceneRenderMode {
    Scene_World, Scene_Grid, Scene_Both, Scene_YawView
  };
  SceneRenderMode SceneMode;

  enum TextScreen {
    Text_None, Text_Orientation, Text_Config, Text_Help, Text_Count
  };
  TextScreen TextScreen;

  struct DeviceStatusNotificationDesc {
    DeviceHandle Handle;
    MessageType Action;

    DeviceStatusNotificationDesc()
        : Action(Message_None) {
    }
    DeviceStatusNotificationDesc(MessageType mt, const DeviceHandle& dev)
        : Handle(dev), Action(mt) {
    }
  };
  Array<DeviceStatusNotificationDesc> DeviceStatusNotificationsQueue;

  Model* CreateModel(Vector3f pos, struct SlabModel* sm);
  Model* CreateBoundingModel(CollisionModel &cm);
  void PopulateLODFileNames();
  void DropLOD();
  void RaiseLOD();
  void CycleDisplay();
  void GamepadStateChanged(const GamepadState& pad);
};

//-------------------------------------------------------------------------------------

HackulusApp::HackulusApp()
    : pRender(0), LastUpdate(0), LoadingState(LoadingState_Frame0),
    // Initial location
    SConfig(), PostProcess(PostProcess_Distortion),
    DistortionClearColor(0, 0, 0),
    ShiftDown(false), pAdjustFunc(0), AdjustDirection(1.0f),
    SceneMode(Scene_World), TextScreen(Text_None) {
  Width = 1280;
  Height = 800;
  Screen = 0;
  FirstScreenInCycle = 0;

  FPS = 0;
  FrameCounter = 0;
  NextFPSUpdate = 0;

  ConsecutiveLowFPSFrames = 0;
  CurrentLODFileIndex = 0;

  AdjustMessageTimeout = 0;
}

HackulusApp::~HackulusApp() {
  RemoveHandlerFromDevices();

  if (DejaVu.fill) {
    DejaVu.fill->Release();
  }
  pLatencyTester.Clear();
  pSensor.Clear();
  pHMD.Clear();

  CollisionModels.ClearAndRelease();
  GroundCollisionModels.ClearAndRelease();
}

int HackulusApp::OnStartup(int argc, const char** argv) {

  // *** Oculus HMD & Sensor Initialization

  // Create DeviceManager and first available HMDDevice from it.
  // Sensor object is created from the HMD, to ensure that it is on the
  // correct device.

  pManager = *DeviceManager::Create();

  // We'll handle it's messages in this case.
  pManager->SetMessageHandler(this);

  pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
  if (pHMD) {
    pSensor = *pHMD->GetSensor();

    // This will initialize HMDInfo with information about configured IPD,
    // screen size and other variables needed for correct projection.
    // We pass HMD DisplayDeviceName into the renderer to select the
    // correct monitor in full-screen mode.
    if (pHMD->GetDeviceInfo(&TheHMDInfo)) {
      //RenderParams.MonitorName = hmd.DisplayDeviceName;
      SConfig.SetHMDInfo(TheHMDInfo);
    }

    // Retrieve relevant profile settings.
    pUserProfile = pHMD->GetProfile();
    if (pUserProfile) {
      ThePlayer.UserEyeHeight = pUserProfile->GetEyeHeight();
      ThePlayer.EyePos.y = ThePlayer.UserEyeHeight;
    }
  } else {
    // If we didn't detect an HMD, try to create the sensor directly.
    // This is useful for debugging sensor interaction; it is not needed in
    // a shipping app.
    pSensor = *pManager->EnumerateDevices<SensorDevice>().CreateDevice();
  }

  // Create the Latency Tester device and assign it to the LatencyTesterUtil object.
  pLatencyTester =
      *pManager->EnumerateDevices<LatencyTestDevice>().CreateDevice();
  if (pLatencyTester) {
    LatencyUtil.SetDevice(pLatencyTester);
  }
  // Make the user aware which devices are present.
  if (pHMD == NULL && pSensor == NULL) {
    SetAdjustMessage(
        "---------------------------------\nNO HMD DETECTED\nNO SENSOR DETECTED\n---------------------------------");
  } else if (pHMD == NULL) {
    SetAdjustMessage(
        "----------------------------\nNO HMD DETECTED\n----------------------------");
  } else if (pSensor == NULL) {
    SetAdjustMessage(
        "---------------------------------\nNO SENSOR DETECTED\n---------------------------------");
  } else {
    SetAdjustMessage("--------------------------------------------\n"
        "Press F9 for Full-Screen on Rift\n"
        "--------------------------------------------");
  }

  // First message should be extra-long.
  SetAdjustMessageTimeout(10.0f);

  if (TheHMDInfo.HResolution > 0) {
    Width = TheHMDInfo.HResolution;
    Height = TheHMDInfo.VResolution;
  }

  if (!pPlatform->SetupWindow(Width, Height)) {
    return 1;
  }

  String Title = "Oculus World Demo";
  if (TheHMDInfo.ProductName[0]) {
    Title += " : ";
    Title += TheHMDInfo.ProductName;
  }
  pPlatform->SetWindowTitle(Title);

  // Report relative mouse motion in OnMouseMove
  pPlatform->SetMouseMode(Mouse_Relative);

  if (pSensor) {
    // We need to attach sensor to SensorFusion object for it to receive
    // body frame messages and update orientation. SFusion.GetOrientation()
    // is used in OnIdle() to orient the view.
    SFusion.AttachToSensor(pSensor);

    SFusion.SetDelegateMessageHandler(this);

    SFusion.SetPredictionEnabled(true);
  }

  // *** Initialize Rendering

  const char* graphics = "d3d11";

  // Select renderer based on command line arguments.
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-r") && i < argc - 1) {
      graphics = argv[i + 1];
    } else if (!strcmp(argv[i], "-fs")) {
      RenderParams.Fullscreen = true;
    }
  }

  // Enable multi-sampling by default.
  RenderParams.Multisample = 4;
  pRender = pPlatform->SetupGraphics(OVR_DEFAULT_RENDER_DEVICE_SET, graphics,
      RenderParams);

  // *** Configure Stereo settings.

  SConfig.SetFullViewport(Viewport(0, 0, Width, Height));
  SConfig.SetStereoMode(Stereo_LeftRight_Multipass);

  // Configure proper Distortion Fit.
  // For 7" screen, fit to touch left side of the view, leaving a bit of
  // invisible screen on the top (saves on rendering cost).
  // For smaller screens (5.5"), fit to the top.
  if (TheHMDInfo.HScreenSize > 0.0f) {
    if (TheHMDInfo.HScreenSize > 0.140f) // 7"
      SConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
    else
      SConfig.SetDistortionFitPointVP(0.0f, 1.0f);
  }

  pRender->SetSceneRenderScale(SConfig.GetDistortionScale());
  //pRender->SetSceneRenderScale(1.0f);

  SConfig.Set2DAreaFov(DegreeToRad(85.0f));

  // *** Identify Scene File & Prepare for Loading

  // This creates lights and models.
  if (argc == 2) {
    MainFilePath = argv[1];
    PopulateLODFileNames();
  } else {
    fprintf(stderr, "Usage: OculusWorldDemo [input XML]\n");
    MainFilePath = WORLDDEMO_ASSET_FILE;
  }

  // Try to modify path for correctness in case specified file is not found.
  if (!SysFile(MainFilePath).IsValid()) {
    String prefixPath1(
        pPlatform->GetContentDirectory() + "/" + WORLDDEMO_ASSET_PATH1),
        prefixPath2(WORLDDEMO_ASSET_PATH2), prefixPath3(WORLDDEMO_ASSET_PATH3);
    if (SysFile(prefixPath1 + MainFilePath).IsValid())
      MainFilePath = prefixPath1 + MainFilePath;
    else if (SysFile(prefixPath2 + MainFilePath).IsValid())
      MainFilePath = prefixPath2 + MainFilePath;
    else if (SysFile(prefixPath3 + MainFilePath).IsValid())
      MainFilePath = prefixPath3 + MainFilePath;
  }

  PopulatePreloadScene();

  LastUpdate = pPlatform->GetAppTime();
  //pPlatform->PlayMusicFile(L"Loop.wav");

  return 0;
}

void HackulusApp::OnMessage(const Message& msg) {
  if (msg.Type == Message_DeviceAdded || msg.Type == Message_DeviceRemoved) {
    if (msg.pDevice == pManager) {
      const MessageDeviceStatus& statusMsg =
          static_cast<const MessageDeviceStatus&>(msg);

      { // limit the scope of the lock
        Lock::Locker lock(pManager->GetHandlerLock());
        DeviceStatusNotificationsQueue.PushBack(
            DeviceStatusNotificationDesc(statusMsg.Type, statusMsg.Handle));
      }

      switch (statusMsg.Type) {
        case OVR::Message_DeviceAdded:
          LogText("DeviceManager reported device added.\n");
          break;

        case OVR::Message_DeviceRemoved:
          LogText("DeviceManager reported device removed.\n");
          break;

        default:
          OVR_ASSERT(0);
          break;
          // unexpected type
      }
    }
  }
}

void HackulusApp::OnResize(int width, int height) {
  Width = width;
  Height = height;
  SConfig.SetFullViewport(Viewport(0, 0, Width, Height));
}

void HackulusApp::OnMouseMove(int x, int y, int modifiers) {
  if (modifiers & Mod_MouseRelative) {
    // Get Delta
    int dx = x, dy = y;

    const float maxPitch = ((3.1415f / 2) * 0.98f);

    // Apply to rotation. Subtract for right body frame rotation,
    // since yaw rotation is positive CCW when looking down on XZ plane.
    ThePlayer.EyeYaw -= (Sensitivity * dx) / 360.0f;

    if (!pSensor) {
      ThePlayer.EyePitch -= (Sensitivity * dy) / 360.0f;

      if (ThePlayer.EyePitch > maxPitch) {
        ThePlayer.EyePitch = maxPitch;
      }
      if (ThePlayer.EyePitch < -maxPitch) {
        ThePlayer.EyePitch = -maxPitch;
      }
    }
  }
}

void HackulusApp::OnKey(OVR::KeyCode key, int chr, bool down, int modifiers) {
  OVR_UNUSED(chr);

  if (modifiers & Mod_Control) {
    switch (key) {
      case Key_Q:
        if (down) {
          pPlatform->Exit(0);
        } break;
      default:
        break;
    }
    return;
  }

  if (modifiers & Mod_Alt) {
    switch (key) {
      case Key_B:
        if (down) {
          if (SConfig.GetDistortionScale() == 1.0f) {
            if (SConfig.GetHMDInfo().HScreenSize > 0.140f) // 7"
                {
              SConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
            } else {
              SConfig.SetDistortionFitPointVP(0.0f, 1.0f);
            }
          } else {
            // No fitting; scale == 1.0.
            SConfig.SetDistortionFitPointVP(0, 0);
          }
        }
        break;

        // Support toggling background color for distortion so that we can see
        // the effect on the periphery.
      case Key_V:
        if (down) {
          if (DistortionClearColor.B == 0) {
            DistortionClearColor = Color(0, 128, 255);
          } else {
            DistortionClearColor = Color(0, 0, 0);
          }

          pRender->SetDistortionClearColor(DistortionClearColor);
        }
        break;

      case Key_R:
        SFusion.Reset();
        SetAdjustMessage("Sensor Fusion Reset");
        break;

      // Distortion correction adjustments
      case Key_H:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK0 : NULL;
        AdjustDirection = -1;
        break;
      case Key_Y:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK0 : NULL;
        AdjustDirection = 1;
        break;
      case Key_J:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK1 : NULL;
        AdjustDirection = -1;
        break;
      case Key_U:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK1 : NULL;
        AdjustDirection = 1;
        break;
      case Key_K:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK2 : NULL;
        AdjustDirection = -1;
        break;
      case Key_I:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK2 : NULL;
        AdjustDirection = 1;
        break;
      case Key_L:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK3 : NULL;
        AdjustDirection = -1;
        break;
      case Key_O:
        pAdjustFunc = down ? &HackulusApp::AdjustDistortionK3 : NULL;
        AdjustDirection = 1;
        break;

      case Key_G:
        if (down) {
          if (SceneMode == Scene_World) {
            SceneMode = Scene_Grid;
            SetAdjustMessage("Grid Only");
          } else if (SceneMode == Scene_Grid) {
            SceneMode = Scene_Both;
            SetAdjustMessage("Grid Overlay");
          } else if (SceneMode == Scene_Both) {
            SceneMode = Scene_World;
            SetAdjustMessage("Grid Off");
          }
        }
        break;

        // Reset the camera position in case we get stuck
      case Key_T:
        ThePlayer.EyePos = Vector3f(10.0f, ThePlayer.UserEyeHeight, 10.0f);
        break;

      case Key_N:
        pAdjustFunc = down ? &HackulusApp::AdjustMotionPrediction : NULL;
        AdjustDirection = -1;
        break;

      case Key_M:
        pAdjustFunc = down ? &HackulusApp::AdjustMotionPrediction : NULL;
        AdjustDirection = 1;
        break;

        /*
         case Key_N:
         RaiseLOD();
         break;
         case Key_M:
         DropLOD();
         break;
         */

      // Cycle through drift correction options
      case Key_Z:
        if (down) {
          if (SFusion.IsYawCorrectionEnabled()) {
            SFusion.SetGravityEnabled(false);
            SFusion.SetYawCorrectionEnabled(false);
          } else if (SFusion.IsGravityEnabled()) {
            SFusion.SetYawCorrectionEnabled(true);
          } else {
            SFusion.SetGravityEnabled(true);
          }
          SetAdjustMessage("Tilt Correction %s\nYaw Correction %s",
              SFusion.IsGravityEnabled() ? "On" : "Off",
              SFusion.IsYawCorrectionEnabled() ? "On" : "Off");
        }
        break;


      case Key_C:
        if (down) {
          // Toggle chromatic aberration correction on/off.
          RenderDevice::PostProcessShader shader =
              pRender->GetPostProcessShader();

          if (shader == RenderDevice::PostProcessShader_Distortion) {
            pRender->SetPostProcessShader(
                RenderDevice::PostProcessShader_DistortionAndChromAb);
            SetAdjustMessage("Chromatic Aberration Correction On");
          } else if (shader
              == RenderDevice::PostProcessShader_DistortionAndChromAb) {
            pRender->SetPostProcessShader(
                RenderDevice::PostProcessShader_Distortion);
            SetAdjustMessage("Chromatic Aberration Correction Off");
          } else
            OVR_ASSERT(false);
        }
        break;

      case Key_P:
        if (down) {
          // Toggle motion prediction.
          if (SFusion.IsPredictionEnabled()) {
            SFusion.SetPredictionEnabled(false);
            SetAdjustMessage("Motion Prediction Off");
          } else {
            SFusion.SetPredictionEnabled(true);
            SetAdjustMessage("Motion Prediction On");
          }
        }
        break;
    }
    return;
  }

  switch (key) {
      // Handle player movement keys.
      // We just update movement state here, while the actual translation is done in OnIdle()
      // based on time.
    case Key_W:
      ThePlayer.SetInput(Player::MoveForward, down, 1);
      break;
    case Key_S:
      ThePlayer.SetInput(Player::MoveBackward, down, 1);
      break;
    case Key_A:
      ThePlayer.SetInput(Player::MoveLeft, down, 1);
      break;
    case Key_D:
      ThePlayer.SetInput(Player::MoveRight, down, 1);
      break;
    case Key_Up:
      ThePlayer.SetInput(Player::MoveForward, down, 2);
      break;
    case Key_Down:
      ThePlayer.SetInput(Player::MoveBackward, down, 2);
      break;
    case Key_Left:
      ThePlayer.SetInput(Player::MoveLeft, down, 2);
      break;
    case Key_Right:
      ThePlayer.SetInput(Player::MoveRight, down, 2);
      break;
    case Key_Q : {
      ThePlayer.SetInput(Player::MoveUp, down, 1);
    } break;
    case Key_E : {
      ThePlayer.SetInput(Player::MoveDown, down, 1);
    } break;
    case Key_R : {
      ThePlayer.SetInput(Player::MoveIn, down, 1);
    } break;
    case Key_F : {
      ThePlayer.SetInput(Player::MoveOut, down, 1);
    } break;
//    case 't' : {
//      _camera.ApplyRollInput(-rollAmount, Camera::RIGHT, Camera::UP);
//    } break;
//    case 'g' : {
//      _camera.ApplyRollInput(rollAmount, Camera::RIGHT, Camera::UP);
//    } break;
//    case 'y' : {
//      _camera.ApplyRollInput(-rollAmount, Camera::INSIDE, Camera::RIGHT);
//    } break;
//    case 'h' : {
//      _camera.ApplyRollInput(rollAmount, Camera::INSIDE, Camera::RIGHT);
//    } break;
//    case 'u' : {
//      _camera.ApplyRollInput(-rollAmount, Camera::UP, Camera::INSIDE);
//    } break;
//    case 'j' : {
//      _camera.ApplyRollInput(rollAmount, Camera::UP, Camera::INSIDE);
//    } break;

    case Key_Minus:
      pAdjustFunc = down ? &HackulusApp::AdjustEyeHeight : 0;
      AdjustDirection = -1;
      break;
    case Key_Equal:
      pAdjustFunc = down ? &HackulusApp::AdjustEyeHeight : 0;
      AdjustDirection = 1;
      break;

    case Key_F1:
      SConfig.SetStereoMode(Stereo_None);
      PostProcess = PostProcess_None;
      SetAdjustMessage("StereoMode: None");
      break;
    case Key_F2:
      SConfig.SetStereoMode(Stereo_LeftRight_Multipass);
      PostProcess = PostProcess_None;
      SetAdjustMessage("StereoMode: Stereo + No Distortion");
      break;
    case Key_F3:
      SConfig.SetStereoMode(Stereo_LeftRight_Multipass);
      PostProcess = PostProcess_Distortion;
      SetAdjustMessage("StereoMode: Stereo + Distortion");
      break;

    case Key_Space:
      if (!down) {
        TextScreen = (enum TextScreen) ((TextScreen + 1) % Text_Count);
      }
      break;

    case Key_F4:
      if (!down) {
        RenderParams = pRender->GetParams();
        RenderParams.Multisample = RenderParams.Multisample > 1 ? 1 : 4;
        pRender->SetParams(RenderParams);
        if (RenderParams.Multisample > 1) {
          SetAdjustMessage("Multisampling On");
        } else {
          SetAdjustMessage("Multisampling Off");
        }
      }
      break;
    case Key_F9:
#ifndef OVR_OS_LINUX    // On Linux F9 does the same as F11.
      if (!down)
      {
        CycleDisplay();
      }
      break;
#endif
#ifdef OVR_OS_MAC
      case Key_F10: // F11 is reserved on Mac
#else
    case Key_F11:
#endif
      if (!down) {
        RenderParams = pRender->GetParams();
        RenderParams.Display = DisplayId(SConfig.GetHMDInfo().DisplayDeviceName,
            SConfig.GetHMDInfo().DisplayId);
        pRender->SetParams(RenderParams);

        pPlatform->SetMouseMode(Mouse_Normal);
        pPlatform->SetFullscreen(RenderParams,
            pRender->IsFullscreen() ? Display_Window : Display_FakeFullscreen);
        pPlatform->SetMouseMode(Mouse_Relative); // Avoid mode world rotation jump.
        // If using an HMD, enable post-process (for distortion) and stereo.
        if (RenderParams.IsDisplaySet() && pRender->IsFullscreen()) {
          SConfig.SetStereoMode(Stereo_LeftRight_Multipass);
          PostProcess = PostProcess_Distortion;
        }
      }
      break;

    case Key_Escape:
      if (!down) {
        // switch to primary screen windowed mode
        pPlatform->SetFullscreen(RenderParams, Display_Window);
        RenderParams.Display = pPlatform->GetDisplay(0);
        pRender->SetParams(RenderParams);
        Screen = 0;
      }
      break;

      // Stereo adjustments.
    case Key_BracketLeft:
      pAdjustFunc = down ? &HackulusApp::AdjustFov : 0;
      AdjustDirection = 1;
      break;
    case Key_BracketRight:
      pAdjustFunc = down ? &HackulusApp::AdjustFov : 0;
      AdjustDirection = -1;
      break;

    case Key_Insert:
    case Key_Num0:
      pAdjustFunc = down ? &HackulusApp::AdjustIPD : 0;
      AdjustDirection = 1;
      break;
    case Key_Delete:
    case Key_Num9:
      pAdjustFunc = down ? &HackulusApp::AdjustIPD : 0;
      AdjustDirection = -1;
      break;

    case Key_PageUp:
      pAdjustFunc = down ? &HackulusApp::AdjustAspect : 0;
      AdjustDirection = 1;
      break;
    case Key_PageDown:
      pAdjustFunc = down ? &HackulusApp::AdjustAspect : 0;
      AdjustDirection = -1;
      break;

    case Key_Tab:
      if (down) {
        float t0 = SConfig.GetDistortionK(0), t1 = SConfig.GetDistortionK(1),
            t2 = SConfig.GetDistortionK(2), t3 = SConfig.GetDistortionK(3);
        float tESD = SConfig.GetEyeToScreenDistance(), taspect =
            SConfig.GetAspectMultiplier(), tipd = SConfig.GetIPD();

        if (SavedK0 > 0.0f) {
          SConfig.SetDistortionK(0, SavedK0);
          SConfig.SetDistortionK(1, SavedK1);
          SConfig.SetDistortionK(2, SavedK2);
          SConfig.SetDistortionK(3, SavedK3);
          SConfig.SetEyeToScreenDistance(SavedESD);
          SConfig.SetAspectMultiplier(SavedAspect);
          SConfig.SetIPD(SavedEyeDistance);

          if (ShiftDown) {
            // Swap saved and current values. Good for doing direct comparisons.
            SetAdjustMessage("Swapped current and saved. New settings:\n"
                "ESD:\t120 %.3f\t350 Eye:\t490 %.3f\n"
                "K0: \t120 %.4f\t350 K2: \t490 %.4f\n"
                "K1: \t120 %.4f\t350 K3: \t490 %.4f\n", SavedESD,
                SavedEyeDistance, SavedK0, SavedK2, SavedK1, SavedK3);
            SavedK0 = t0;
            SavedK1 = t1;
            SavedK2 = t2;
            SavedK3 = t3;
            SavedESD = tESD;
            SavedAspect = taspect;
            SavedEyeDistance = tipd;
          } else {
            SetAdjustMessage("Restored:\n"
                "ESD:\t120 %.3f\t350 Eye:\t490 %.3f\n"
                "K0: \t120 %.4f\t350 K2: \t490 %.4f\n"
                "K1: \t120 %.4f\t350 K3: \t490 %.4f\n", SavedESD,
                SavedEyeDistance, SavedK0, SavedK2, SavedK1, SavedK3);
          }
        } else {
          SetAdjustMessage("Setting Saved");
          SavedK0 = t0;
          SavedK1 = t1;
          SavedK2 = t2;
          SavedK3 = t3;
          SavedESD = tESD;
          SavedAspect = taspect;
          SavedEyeDistance = tipd;
        }

      }
      break;

    case Key_Slash:
      printf("\nView 4:\n");
      FullView.CameraView.printIt();
      printf("\nPos 4:\n");
      FullView.CameraPos.printIt();
      break;

      // Holding down Shift key accelerates adjustment velocity.
    case Key_Shift:
      ShiftDown = down;
      break;
    case Key_F5:
      if (!down) {
        UPInt numNodes = MainScene.Models.GetSize();
        for (UPInt i = 0; i < numNodes; i++) {
          Ptr<OVR::Render::Model> nodePtr = MainScene.Models[i];
          Render::Model* pNode = nodePtr.GetPtr();
          if (pNode->IsCollisionModel) {
            pNode->Visible = !pNode->Visible;
          }
        }
      }
      break;

      // Show view of yaw angles (for mag calibration/analysis)
    case Key_F6:
      if (down) {
        if (SceneMode != Scene_YawView) {
          SceneMode = Scene_YawView;
          SetAdjustMessage("Magnetometer Yaw Angle Marks");
        } else {
          SceneMode = Scene_World;
          SetAdjustMessage("Magnetometer Marks Off");
        }
      }
      break;
    default:
      break;
  }
}

static const char* HelpText =
    "F1         \t100 NoStereo\n"
        "F2         \t100 Stereo                     \t420 Z    \t520 Drift Correction\n"
        "F3         \t100 StereoHMD                  \t420 F6   \t520 Yaw Drift Info\n"
        "F4         \t100 MSAA                       \t420 R    \t520 Reset SensorFusion\n"
        "F9         \t100 FullScreen                 \t420\n"
        "F11        \t100 Fast FullScreen                   \t500 - +       \t660 Adj EyeHeight\n"
        "C          \t100 Chromatic Ab                      \t500 [ ]       \t660 Adj FOV\n"
        "P          \t100 Motion Pred                       \t500 Shift     \t660 Adj Faster\n"
        "N/M        \t180 Adj Motion Pred\n"
        "( / )      \t180 Adj EyeDistance";

enum DrawTextCenterType {
  DrawText_NoCenter = 0,
  DrawText_VCenter = 0x1,
  DrawText_HCenter = 0x2,
  DrawText_Center = DrawText_VCenter | DrawText_HCenter
};

static void DrawTextBox(RenderDevice* prender, float x, float y, float textSize,
    const char* text, DrawTextCenterType centerType = DrawText_NoCenter) {
  float ssize[2] = { 0.0f, 0.0f };

  prender->MeasureText(&DejaVu, text, textSize, ssize);

  // Treat 0 a VCenter.
  if (centerType & DrawText_HCenter) {
    x = -ssize[0] / 2;
  }
  if (centerType & DrawText_VCenter) {
    y = -ssize[1] / 2;
  }

  prender->FillRect(x - 0.02f, y - 0.02f, x + ssize[0] + 0.02f,
      y + ssize[1] + 0.02f, Color(40, 40, 100, 210));
  prender->RenderText(&DejaVu, text, x, y, textSize, Color(255, 255, 0, 210));
}

void PrintMat(const Matrix4f& m, const char* desc) {
  if (desc) {
    printf("%s\n", desc);
  }
  const float* raw = &m.M[0][0];
  for (int i = 0; i < 16; i++) {
    printf("%f\t", raw[i]);
    if ((i + 1) % 4 == 0) {
      printf("\n");
    }
  }
}

void HackulusApp::OnIdle() {

  double curtime = pPlatform->GetAppTime();
  float dt = float(curtime - LastUpdate);
  LastUpdate = curtime;

  // Update gamepad.
  GamepadState gamepadState;
  if (GetPlatformCore()->GetGamepadManager()->GetGamepadState(0,
      &gamepadState)) {
    GamepadStateChanged(gamepadState);
  }

  if (LoadingState == LoadingState_DoLoad) {
    PopulateScene(MainFilePath.ToCStr());
    LoadingState = LoadingState_Finished;
    return;
  }

  // Check if any new devices were connected.
  {
    bool queueIsEmpty = false;
    while (!queueIsEmpty) {
      DeviceStatusNotificationDesc desc;

      {
        Lock::Locker lock(pManager->GetHandlerLock());
        if (DeviceStatusNotificationsQueue.GetSize() == 0)
          break;
        desc = DeviceStatusNotificationsQueue.Front();

        // We can't call Clear under the lock since this may introduce a dead lock:
        // this thread is locked by HandlerLock and the Clear might cause
        // call of Device->Release, which will use Manager->DeviceLock. The bkg
        // thread is most likely locked by opposite way:
        // Manager->DeviceLock ==> HandlerLock, therefore - a dead lock.
        // So, just grab the first element, save a copy of it and remove
        // the element (Device->Release won't be called since we made a copy).

        DeviceStatusNotificationsQueue.RemoveAt(0);
        queueIsEmpty = (DeviceStatusNotificationsQueue.GetSize() == 0);
      }

      bool wasAlreadyCreated = desc.Handle.IsCreated();

      if (desc.Action == Message_DeviceAdded) {
        switch (desc.Handle.GetType()) {
          case Device_Sensor:
            if (desc.Handle.IsAvailable() && !desc.Handle.IsCreated()) {
              if (!pSensor) {
                pSensor = *desc.Handle.CreateDeviceTyped<SensorDevice>();
                if (pSensor) {
                  SFusion.AttachToSensor(pSensor);

                  SetAdjustMessage("---------------------------\n"
                      "SENSOR connected\n"
                      "---------------------------");
                } else {
                  SetAdjustMessage("----------------------------\n"
                      "SENSOR connect failed\n"
                      "Unplug and reconnect it.\n"
                      "----------------------------");
                }
              } else if (!wasAlreadyCreated) {
                LogText(
                    "A new SENSOR has been detected, but it is not currently used.");
              }
            }
            break;
          case Device_LatencyTester:
            if (desc.Handle.IsAvailable() && !desc.Handle.IsCreated()) {
              if (!pLatencyTester) {
                pLatencyTester = *desc.Handle.CreateDeviceTyped<
                    LatencyTestDevice>();
                LatencyUtil.SetDevice(pLatencyTester);
                if (!wasAlreadyCreated)
                  SetAdjustMessage("----------------------------------------\n"
                      "LATENCY TESTER connected\n"
                      "----------------------------------------");
              }
            }
            break;
          case Device_HMD: {
            OVR::HMDInfo info;
            desc.Handle.GetDeviceInfo(&info);
            // if strlen(info.DisplayDeviceName) == 0 then
            // this HMD is 'fake' (created using sensor).
            if (strlen(info.DisplayDeviceName) > 0
                && (!pHMD || !info.IsSameDisplay(TheHMDInfo))) {
              SetAdjustMessage("------------------------\n"
                  "HMD connected\n"
                  "------------------------");
              if (!pHMD || !desc.Handle.IsDevice(pHMD))
                pHMD = *desc.Handle.CreateDeviceTyped<HMDDevice>();
              // update stereo config with new HMDInfo
              if (pHMD && pHMD->GetDeviceInfo(&TheHMDInfo)) {
                //RenderParams.MonitorName = hmd.DisplayDeviceName;
                SConfig.SetHMDInfo(TheHMDInfo);
              }
              LogText("HMD device added.\n");
            }
            break;
          }
          default:
            break;
        }
      } else if (desc.Action == Message_DeviceRemoved) {
        if (desc.Handle.IsDevice(pSensor)) {
          LogText("Sensor reported device removed.\n");
          SFusion.AttachToSensor(NULL);
          pSensor.Clear();
          SetAdjustMessage("-------------------------------\n"
              "SENSOR disconnected.\n"
              "-------------------------------");
        } else if (desc.Handle.IsDevice(pLatencyTester)) {
          LogText("Latency Tester reported device removed.\n");
          LatencyUtil.SetDevice(NULL);
          pLatencyTester.Clear();
          SetAdjustMessage("---------------------------------------------\n"
              "LATENCY SENSOR disconnected.\n"
              "---------------------------------------------");
        } else if (desc.Handle.IsDevice(pHMD)) {
          if (pHMD && !pHMD->IsDisconnected()) {
            SetAdjustMessage("---------------------------\n"
                "HMD disconnected\n"
                "---------------------------");
            // Disconnect HMD. pSensor is used to restore 'fake' HMD device
            // (can be NULL).
            pHMD = pHMD->Disconnect(pSensor);

            // This will initialize TheHMDInfo with information about configured IPD,
            // screen size and other variables needed for correct projection.
            // We pass HMD DisplayDeviceName into the renderer to select the
            // correct monitor in full-screen mode.
            if (pHMD && pHMD->GetDeviceInfo(&TheHMDInfo)) {
              //RenderParams.MonitorName = hmd.DisplayDeviceName;
              SConfig.SetHMDInfo(TheHMDInfo);
            }
            LogText("HMD device removed.\n");
          }
        }
      } else {
        OVR_ASSERT(0);
        // unexpected action
      }
    }
  }

  // If one of Stereo setting adjustment keys is pressed, adjust related state.
  if (pAdjustFunc) {
    (this->*pAdjustFunc)(dt * AdjustDirection * (ShiftDown ? 5.0f : 1.0f));
  }

  // Process latency tester results.
  const char* results = LatencyUtil.GetResultsString();
  if (results != NULL) {
    LogText("LATENCY TESTER: %s\n", results);
  }

  // >>> THIS MUST BE PLACED AS CLOSE AS POSSIBLE TO WHERE THE HMD ORIENTATION IS READ <<<
  LatencyUtil.ProcessInputs();

  // Handle Sensor motion.
  // We extract Yaw, Pitch, Roll instead of directly using the orientation
  // to allow "additional" yaw manipulation with mouse/controller.
  if (pSensor) {
    Quatf hmdOrient = SFusion.GetPredictedOrientation();

    float yaw = 0.0f;
    hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &ThePlayer.EyePitch,
        &ThePlayer.EyeRoll);

    ThePlayer.EyeYaw += (yaw - ThePlayer.LastSensorYaw);
    ThePlayer.LastSensorYaw = yaw;

    // NOTE: We can get a matrix from orientation as follows:
    // Matrix4f hmdMat(hmdOrient);

    // Test logic - assign quaternion result directly to view:
    // Quatf hmdOrient = SFusion.GetOrientation();
    // View = Matrix4f(hmdOrient.Inverted()) * Matrix4f::Translation(-EyePos);
  }

  if (curtime >= NextFPSUpdate) {
    NextFPSUpdate = curtime + 1.0;
    FPS = FrameCounter;
    FrameCounter = 0;
  }
  FrameCounter++;

  if (FPS < 40) {
    ConsecutiveLowFPSFrames++;
  } else {
    ConsecutiveLowFPSFrames = 0;
  }

  if (ConsecutiveLowFPSFrames > 200) {
    DropLOD();
    ConsecutiveLowFPSFrames = 0;
  }

  ThePlayer.EyeYaw -= ThePlayer.GamepadRotate.x * dt;
  ThePlayer.HandleCollision(dt, &CollisionModels, &GroundCollisionModels,
      ShiftDown);

  if (!pSensor) {
    ThePlayer.EyePitch -= ThePlayer.GamepadRotate.y * dt;

    const float maxPitch = ((3.1415f / 2) * 0.98f);
    if (ThePlayer.EyePitch > maxPitch) {
      ThePlayer.EyePitch = maxPitch;
    }
    if (ThePlayer.EyePitch < -maxPitch) {
      ThePlayer.EyePitch = -maxPitch;
    }
  }

  // Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
  //
  Matrix4 rollPitchYaw = Matrix4f::RotationY(ThePlayer.EyeYaw)
      * Matrix4f::RotationX(ThePlayer.EyePitch)
      * Matrix4f::RotationZ(ThePlayer.EyeRoll);
  Vector4f up = rollPitchYaw.transform(Player::UpVector);
  Vector4f forward = rollPitchYaw.transform(Player::ForwardVector);
  Vector4f in = rollPitchYaw.transform(Player::InVector);

  // Minimal head modeling; should be moved as an option to SensorFusion.
  float headBaseToEyeHeight = 0.15f; // Vertical height of eye from base of head
  float headBaseToEyeProtrusion = 0.09f; // Distance forward of eye from base of head

  Vector4f eyeCenterInHeadFrame(0.0f, headBaseToEyeHeight,
      -headBaseToEyeProtrusion, 0.0f);
  Vector4f shiftedEyePos = ThePlayer.EyePos
      + rollPitchYaw.transform(eyeCenterInHeadFrame);
  shiftedEyePos.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height
  View = Matrix4f::LookAtRH(shiftedEyePos.asV3(), shiftedEyePos.asV3() + forward.asV3(), up.asV3());
  fd::Mat4f::lookAtRH(shiftedEyePos, shiftedEyePos + forward, up, in,
      static_cast<fd::Mat4f&>(FullView.CameraView),
      static_cast<fd::Vec4f&>(FullView.CameraPos));
  // You still bastard, fix this now while there is still time
  FullView.CameraView = FullView.CameraView.transpose();

  //  Transformation without head modeling.
  // View = Matrix4f::LookAtRH(EyePos, EyePos + forward, up);

  // This is an alternative to LookAtRH:
  // Here we transpose the rotation matrix to get its inverse.
  //  View = (Matrix4f::RotationY(EyeYaw) * Matrix4f::RotationX(EyePitch) *
  //                                        Matrix4f::RotationZ(EyeRoll)).Transposed() *
  //         Matrix4f::Translation(-EyePos);

  switch (SConfig.GetStereoMode()) {
    case Stereo_None:
      Render(SConfig.GetEyeRenderParams(StereoEye_Center));
      break;

    case Stereo_LeftRight_Multipass:
      //case Stereo_LeftDouble_Multipass:
      Render(SConfig.GetEyeRenderParams(StereoEye_Left));
      Render(SConfig.GetEyeRenderParams(StereoEye_Right));
      break;

  }

  pRender->Present();
  // Force GPU to flush the scene, resulting in the lowest possible latency.
  pRender->ForceFlushGPU();
}

void HackulusApp::Render(const StereoEyeParams& stereo) {
  pRender->BeginScene(PostProcess);

  // *** 3D - Configures Viewport/Projection and Render
  pRender->ApplyStereoParams(stereo);
  pRender->Clear();

  //pRender->SetDepthMode(true, true);
  pRender->SetDepthMode(false, false);
  if (SceneMode != Scene_Grid) {
    FullView.View = stereo.ViewAdjust * View;
    FullView.CameraPos.x += stereo.ViewAdjust.M[0][3];
    FullView.CameraPos.y += stereo.ViewAdjust.M[1][3];
    FullView.CameraPos.z += stereo.ViewAdjust.M[2][3];
    //FullView.CameraView.transpose().splice3dInto4d(FullView.CameraView, FullView.CameraPos);
    FullView.FourToThree.storeIdentity();
    MainScene.Render(pRender, stereo.ViewAdjust * View, &FullView);
  }

  if (SceneMode == Scene_YawView) {
    Matrix4f trackerOnlyOrient = Matrix4f::RotationY(ThePlayer.LastSensorYaw)
        * Matrix4f::RotationX(ThePlayer.EyePitch)
        * Matrix4f::RotationZ(ThePlayer.EyeRoll);
    YawLinesScene.Render(pRender,
        stereo.ViewAdjust * trackerOnlyOrient.Inverted(), NULL /* fullView */);
    //YawMarkRedScene.Render(pRender, stereo.ViewAdjust);
  }

  // *** 2D Text & Grid - Configure Orthographic rendering.

  // Render UI in 2D orthographic coordinate system that maps [-1,1] range
  // to a readable FOV area centered at your eye and properly adjusted.
  pRender->ApplyStereoParams2D(stereo);
  pRender->SetDepthMode(false, false);

  float unitPixel = SConfig.Get2DUnitPixel();
  float textHeight = unitPixel * 22;

  if ((SceneMode == Scene_Grid) || (SceneMode == Scene_Both)) { // Draw grid two pixels thick.
    GridScene.Render(pRender, Matrix4f(), NULL /* fullView */);
    GridScene.Render(pRender, Matrix4f::Translation(unitPixel, unitPixel, 0), NULL /* fullView */);
  }

  // Display Loading screen-shot in frame 0.
  if (LoadingState != LoadingState_Finished) {
    LoadingScene.Render(pRender, Matrix4f(), NULL /* fullView */);
    String loadMessage = String("Loading ") + MainFilePath;
    DrawTextBox(pRender, 0.0f, 0.0f, textHeight, loadMessage.ToCStr(),
        DrawText_HCenter);
    LoadingState = LoadingState_DoLoad;
  }

  if (!AdjustMessage.IsEmpty()
      && AdjustMessageTimeout > pPlatform->GetAppTime()) {
    DrawTextBox(pRender, 0.0f, 0.4f, textHeight, AdjustMessage.ToCStr(),
        DrawText_HCenter);
  }

  switch (TextScreen) {
    case Text_Orientation: {
      char buf[256], gpustat[256], worldInfo[256];
      MainScene.GetDebugString(worldInfo, sizeof(worldInfo));
      OVR_sprintf(buf, sizeof(buf), " Yaw:%4.0f  Pitch:%4.0f  Roll:%4.0f \n"
          " FPS: %d  Frame: %d \n Pos: %3.2f, %3.2f, %3.2f \n"
          " EyeHeight: %3.2f\n"
          " %s\n",
          RadToDegree(ThePlayer.EyeYaw),
          RadToDegree(ThePlayer.EyePitch), RadToDegree(ThePlayer.EyeRoll), FPS,
          FrameCounter, ThePlayer.EyePos.x, ThePlayer.EyePos.y,
          ThePlayer.EyePos.z, ThePlayer.UserEyeHeight,
          worldInfo
          );
      size_t texMemInMB = pRender->GetTotalTextureMemoryUsage() / 1058576;
      if (texMemInMB) {
        OVR_sprintf(gpustat, sizeof(gpustat), "\n GPU Tex: %u MB", texMemInMB);
        OVR_strcat(buf, sizeof(buf), gpustat);
      }

      DrawTextBox(pRender, 0.0f, -0.15f, textHeight, buf, DrawText_HCenter);
    } break;

    case Text_Config: {
      char textBuff[2048];

      OVR_sprintf(textBuff, sizeof(textBuff), "Fov\t300 %9.4f\n"
          "EyeDistance\t300 %9.4f\n"
          "DistortionK0\t300 %9.4f\n"
          "DistortionK1\t300 %9.4f\n"
          "DistortionK2\t300 %9.4f\n"
          "DistortionK3\t300 %9.4f\n"
          "TexScale\t300 %9.4f", SConfig.GetYFOVDegrees(), SConfig.GetIPD(),
          SConfig.GetDistortionK(0), SConfig.GetDistortionK(1),
          SConfig.GetDistortionK(2), SConfig.GetDistortionK(3),
          SConfig.GetDistortionScale());

      DrawTextBox(pRender, 0.0f, 0.0f, textHeight, textBuff, DrawText_Center);
    } break;

    case Text_Help:
      DrawTextBox(pRender, 0.0f, -0.1f, textHeight, HelpText, DrawText_Center);
      break;

    default:
      break;
  }

  // Display colored quad if we're doing a latency test.
  Color colorToDisplay;
  if (LatencyUtil.DisplayScreenColor(colorToDisplay)) {
    pRender->FillRect(-0.4f, -0.4f, 0.4f, 0.4f, colorToDisplay);
  }

  pRender->FinishScene();
}

// Sets temporarily displayed message for adjustments
void HackulusApp::SetAdjustMessage(const char* format, ...) {
  Lock::Locker lock(pManager->GetHandlerLock());
  char textBuff[2048];
  va_list argList;
  va_start(argList, format);
  OVR_vsprintf(textBuff, sizeof(textBuff), format, argList);
  va_end(argList);

  // Message will time out in 4 seconds.
  AdjustMessage = textBuff;
  AdjustMessageTimeout = pPlatform->GetAppTime() + 4.0f;
}

void HackulusApp::SetAdjustMessageTimeout(float timeout) {
  AdjustMessageTimeout = pPlatform->GetAppTime() + timeout;
}

// ***** View Control Adjustments

void HackulusApp::AdjustFov(float dt) {
  float esd = SConfig.GetEyeToScreenDistance() + 0.01f * dt;
  SConfig.SetEyeToScreenDistance(esd);
  SetAdjustMessage("ESD:%6.3f  FOV: %6.3f", esd, SConfig.GetYFOVDegrees());
}

void HackulusApp::AdjustAspect(float dt) {
  float rawAspect = SConfig.GetAspect() / SConfig.GetAspectMultiplier();
  float newAspect = SConfig.GetAspect() + 0.01f * dt;
  SConfig.SetAspectMultiplier(newAspect / rawAspect);
  SetAdjustMessage("Aspect: %6.3f", newAspect);
}

void HackulusApp::AdjustDistortion(float dt, int kIndex, const char* label) {
  SConfig.SetDistortionK(kIndex, SConfig.GetDistortionK(kIndex) + 0.03f * dt);
  SetAdjustMessage("%s: %6.4f", label, SConfig.GetDistortionK(kIndex));
}

void HackulusApp::AdjustIPD(float dt) {
  SConfig.SetIPD(SConfig.GetIPD() + 0.025f * dt);
  SetAdjustMessage("EyeDistance: %6.4f", SConfig.GetIPD());
}

void HackulusApp::AdjustEyeHeight(float dt) {
  float dist = 0.5f * dt;

  ThePlayer.UserEyeHeight += dist;
  ThePlayer.EyePos.y += dist;

  SetAdjustMessage("UserEyeHeight: %4.2f", ThePlayer.UserEyeHeight);
}

void HackulusApp::AdjustMotionPrediction(float dt) {
  float motionPred = SFusion.GetPredictionDelta() + 0.01f * dt;

  if (motionPred < 0.0f) {
    motionPred = 0.0f;
  }

  SFusion.SetPrediction(motionPred);

  SetAdjustMessage("MotionPrediction: %6.3fs", motionPred);
}

void HackulusApp::AdjustDistortion(float val, int kIndex) {
  SConfig.SetDistortionK(kIndex, val);
  SetAdjustMessage("K%d: %6.4f", kIndex, SConfig.GetDistortionK(kIndex));
}

void HackulusApp::AdjustEsd(float val) {
  SConfig.SetEyeToScreenDistance(val);
  float esd = SConfig.GetEyeToScreenDistance();
  SetAdjustMessage("ESD:%6.3f  FOV: %6.3f", esd, SConfig.GetYFOVDegrees());
}

// Loads the scene data
void HackulusApp::PopulateScene(const char *fileName) {
  XmlHandler xmlHandler;
  if (!xmlHandler.ReadFile(fileName, pRender, &MainScene, &CollisionModels,
      &GroundCollisionModels)) {
    SetAdjustMessage(
        "---------------------------------\nFILE LOAD FAILED\n---------------------------------");
    SetAdjustMessageTimeout(10.0f);
  }

  MainScene.SetAmbient(Vector4f(1.0f, 1.0f, 1.0f, 1.0f));

  // Distortion debug grid (brought up by 'G' key).
  Ptr<Model> gridModel = *Model::CreateGrid(Vector3f(0, 0, 0),
      Vector3f(1.0f / 10, 0, 0), Vector3f(0, 1.0f / 10, 0), 10, 10, 5,
      Color(0, 255, 0, 255), Color(255, 50, 50, 255));
  GridScene.World.Add(gridModel);

  // Yaw angle marker and lines (brought up by ';' key).
  float shifty = -0.5f;
  Ptr<Model> yawMarkGreenModel = *Model::CreateBox(Color(0, 255, 0, 255),
      Vector3f(0.0f, shifty, -2.0f), Vector3f(0.05f, 0.05f, 0.05f));
  YawMarkGreenScene.World.Add(yawMarkGreenModel);
  Ptr<Model> yawMarkRedModel = *Model::CreateBox(Color(255, 0, 0, 255),
      Vector3f(0.0f, shifty, -2.0f), Vector3f(0.05f, 0.05f, 0.05f));
  YawMarkRedScene.World.Add(yawMarkRedModel);

  Ptr<Model> yawLinesModel = *new Model();
  Color c = Color(255, 200, 200, 255);
  float r = 1.5f;
  yawLinesModel->AddTriangle(
      yawLinesModel->AddVertex(Vector3f(-0.1f, 0, -r), c),
      yawLinesModel->AddVertex(Vector3f(0, 0, -r - 0.2f), c),
      yawLinesModel->AddVertex(Vector3f(0.1f, 0, -r), c));
  yawLinesModel->AddTriangle(
      yawLinesModel->AddVertex(Vector3f(-r - 0.1f, 0, -r), c),
      yawLinesModel->AddVertex(Vector3f(-r, 0, -r - 0.2f), c),
      yawLinesModel->AddVertex(Vector3f(-r + 0.1f, 0, -r), c));
  yawLinesModel->AddTriangle(
      yawLinesModel->AddVertex(Vector3f(r - 0.1f, 0, -r), c),
      yawLinesModel->AddVertex(Vector3f(r, 0, -r - 0.2f), c),
      yawLinesModel->AddVertex(Vector3f(r + 0.1f, 0, -r), c));
  yawLinesModel->SetPosition(Vector3f(0.0f, -1.2f, 0.0f));
  YawLinesScene.World.Add(yawLinesModel);

  Ptr<Model> testGreenBox = *Model::CreateBox(Color(0, 255, 0, 255),
      Vector3f(2.0f, 0.1f, 5.0f), Vector3f(1.0f, 1.0f, 1.0f));
  Ptr<ShaderFill> consistencyTestShader = *new ShaderFill(*pRender->CreateShaderSet());
  consistencyTestShader->GetShaders()->SetShader(
      pRender->LoadBuiltinShader(Shader_Vertex, VShader_MVP));
  consistencyTestShader->GetShaders()->SetShader(
      pRender->LoadBuiltinShader(Shader_Fragment, FShader_Solid));
//      pRender->LoadBuiltinShader(Shader_Fragment, FShader_Debug));
  testGreenBox->Fill = consistencyTestShader;
  MainScene.World.Add(testGreenBox);
  MainScene.Models.PushBack(testGreenBox);


  typedef std::vector<Color> ColorList;
  ColorList colorArray = {Color(255,0,0,255), Color(0,255,0,255), Color(0,0,255,255),
      Color(255,255,0,255), Color(255,0,255,255), Color(0,255,255,255),
      Color(255,128,0,255), Color(128,255,0,255), Color(128,0,255,255), Color(255,0,128,255)};

  fd::Mesh tesseract;
  Vector4f tesseractOrigin(1.0f, 0, 0, 0);
//  tesseract.buildCube(1.0f, fd::Vec4f(0.05f,0.05f,0.05f,0.05f), fd::Vec4f(0, 0, 0, 0));
  tesseract.buildTesseract(1.0f, fd::Vec4f(0.05f, 0.05f, 0.05f, 0.05f),
      fd::Vec4f(0.101f, 0.202f, 0.303f, 0.404f)); // fd::Vec4f(0,0,0,0)); // fd::Vec4f(0, 1, 2, 0));
  Ptr<Model> tesseractModel = *new Model(Prim_Triangles);
  // TODO: This is ugly inefficient, fix it.
  fd::Vec4f triA, triB, triC;
  int colorIndex = 0;
  for (int tri = 0; tri < tesseract.getNumberTriangles(); ++tri) {
    tesseract.getTriangle(tri, triA, triB, triC);

    Color vertColor = colorArray[colorIndex];
    tesseractModel->AddTriangle(
        tesseractModel->AddVertex(triA, vertColor),
        tesseractModel->AddVertex(triB, vertColor),
        tesseractModel->AddVertex(triC, vertColor));
    if ((tri+1) % 2 == 0) {
      colorIndex = (colorIndex + 1) % colorArray.size();
    }
  }
//  tesseract.printIt();
//  tesseractModel->PrintIt();
  Ptr<ShaderFill> shader = *new ShaderFill(*pRender->CreateShaderSet());
  shader->GetShaders()->SetShader(
//      pRender->LoadBuiltinShader(Shader_Vertex, VShader_Debug));
      pRender->LoadBuiltinShader(Shader_Vertex, VShader_FourToThree));
//      pRender->LoadBuiltinShader(Shader_Vertex, VShader_MVP));
  shader->GetShaders()->SetShader(
//      pRender->LoadBuiltinShader(Shader_Fragment, FShader_Debug));
        pRender->LoadBuiltinShader(Shader_Fragment, FShader_Solid));
  tesseractModel->Fill = shader;
  tesseractModel->SetPosition(tesseractOrigin.asV3());

  MainScene.World.Add(tesseractModel);
  MainScene.Models.PushBack(tesseractModel);
}

void HackulusApp::PopulatePreloadScene() {
  // Load-screen screen shot image
  String fileName = MainFilePath;
  fileName.StripExtension();

  Ptr<File> imageFile = *new SysFile(fileName + "_LoadScreen.tga");
  Ptr<Texture> imageTex;
  if (imageFile->IsValid())
    imageTex = *LoadTextureTga(pRender, imageFile);

  // Image is rendered as a single quad.
  if (imageTex) {
    imageTex->SetSampleMode(Sample_Anisotropic | Sample_Repeat);
    Ptr<Model> m = *new Model(Prim_Triangles);
    m->AddVertex(-0.5f, 0.5f, 0.0f, Color(255, 255, 255, 255), 0.0f, 0.0f);
    m->AddVertex(0.5f, 0.5f, 0.0f, Color(255, 255, 255, 255), 1.0f, 0.0f);
    m->AddVertex(0.5f, -0.5f, 0.0f, Color(255, 255, 255, 255), 1.0f, 1.0f);
    m->AddVertex(-0.5f, -0.5f, 0.0f, Color(255, 255, 255, 255), 0.0f, 1.0f);
    m->AddTriangle(2, 1, 0);
    m->AddTriangle(0, 3, 2);

    Ptr<ShaderFill> fill = *new ShaderFill(*pRender->CreateShaderSet());
    fill->GetShaders()->SetShader(
        pRender->LoadBuiltinShader(Shader_Vertex, VShader_MVP));
    fill->GetShaders()->SetShader(
        pRender->LoadBuiltinShader(Shader_Fragment, FShader_Texture));
    fill->SetTexture(0, imageTex);
    m->Fill = fill;

    LoadingScene.World.Add(m);
  }
}

void HackulusApp::ClearScene() {
  MainScene.Clear();
  GridScene.Clear();
  YawMarkGreenScene.Clear();
  YawMarkRedScene.Clear();
  YawLinesScene.Clear();
}

void HackulusApp::PopulateLODFileNames() {
  //OVR::String mainFilePath = MainFilePath;
  LODFilePaths.PushBack(MainFilePath);
  int LODIndex = 1;
  SPInt pos = strcspn(MainFilePath.ToCStr(), ".");
  SPInt len = strlen(MainFilePath.ToCStr());
  SPInt diff = len - pos;

  if (diff == 0)
    return;

  while (true) {
    char pathWithoutExt[250];
    char buffer[250];
    for (SPInt i = 0; i < pos; ++i) {
      pathWithoutExt[i] = MainFilePath[(int) i];
    }
    pathWithoutExt[pos] = '\0';
    OVR_sprintf(buffer, sizeof(buffer), "%s%i.xml", pathWithoutExt, LODIndex);
    FILE* fp = 0;
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
    errno_t err = fopen_s(&fp, buffer, "rb");
    if(!fp || err)
    {
#else
    fp = fopen(buffer, "rb");
    if (!fp) {
#endif
      break;
    }
    fclose(fp);
    OVR::String result = buffer;
    LODFilePaths.PushBack(result);
    LODIndex++;
  }
}

void HackulusApp::DropLOD() {
  if (CurrentLODFileIndex < (int) (LODFilePaths.GetSize() - 1)) {
    ClearScene();
    CurrentLODFileIndex++;
    PopulateScene(LODFilePaths[CurrentLODFileIndex].ToCStr());
  }
}

void HackulusApp::RaiseLOD() {
  if (CurrentLODFileIndex > 0) {
    ClearScene();
    CurrentLODFileIndex--;
    PopulateScene(LODFilePaths[CurrentLODFileIndex].ToCStr());
  }
}

//-----------------------------------------------------------------------------
void HackulusApp::CycleDisplay() {
  int screenCount = pPlatform->GetDisplayCount();

  // If Windowed, switch to the HMD screen first in Full-Screen Mode.
  // If already Full-Screen, cycle to next screen until we reach FirstScreenInCycle.

  if (pRender->IsFullscreen()) {
    // Right now, we always need to restore window before going to next screen.
    pPlatform->SetFullscreen(RenderParams, Display_Window);

    Screen++;
    if (Screen == screenCount)
      Screen = 0;

    RenderParams.Display = pPlatform->GetDisplay(Screen);

    if (Screen != FirstScreenInCycle) {
      pRender->SetParams(RenderParams);
      pPlatform->SetFullscreen(RenderParams, Display_Fullscreen);
    }
  } else {
    // Try to find HMD Screen, making it the first screen in full-screen Cycle.
    FirstScreenInCycle = 0;

    if (pHMD) {
      DisplayId HMD(SConfig.GetHMDInfo().DisplayDeviceName,
          SConfig.GetHMDInfo().DisplayId);
      for (int i = 0; i < screenCount; i++) {
        if (pPlatform->GetDisplay(i) == HMD) {
          FirstScreenInCycle = i;
          break;
        }
      }
    }

    // Switch full-screen on the HMD.
    Screen = FirstScreenInCycle;
    RenderParams.Display = pPlatform->GetDisplay(Screen);
    pRender->SetParams(RenderParams);
    pPlatform->SetFullscreen(RenderParams, Display_Fullscreen);
  }
}

void HackulusApp::GamepadStateChanged(const GamepadState& pad) {
  ThePlayer.GamepadMove = Vector3f(pad.LX * pad.LX * (pad.LX > 0 ? 1 : -1), 0,
      pad.LY * pad.LY * (pad.LY > 0 ? -1 : 1));
  ThePlayer.GamepadRotate = Vector3f(2 * pad.RX, -2 * pad.RY, 0);
}

//-------------------------------------------------------------------------------------

OVR_PLATFORM_APP(HackulusApp);

/************************************************************************************
 Modified from :
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
