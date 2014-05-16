/************************************************************************************

Filename    :   Win32_OculusRoomTiny.cpp
Content     :   First-person view test application for Oculus Rift
Created     :   October 4, 2012
Authors     :   Michael Antonov, Andrew Reisse

Copyright   :   Copyright 2012 Oculus, Inc. All Rights reserved.

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

#include "OculusRoomTiny.h"
#include "RenderTiny_Device.h"
#include "MessageBox.h"

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    OculusRoomTinyApp * instance = (OculusRoomTinyApp *)glfwGetWindowUserPointer(window);
    instance->OnKey(key, action == GLFW_PRESS || action == GLFW_REPEAT);
}

void glfwErrorCallback(int error, const char* description) {
    LogText("Error code %d, message: %s", error, description);
    exit(1);
}

//-------------------------------------------------------------------------------------
// ***** OculusRoomTiny Class

OculusRoomTinyApp::OculusRoomTinyApp() :
    window(0), Width(1280), Height(800), Quit(false), LastUpdate(0), StartupTicks(OVR::Timer::GetTicks()), EyePos(0.0f, 1.6f, -5.0f), EyeYaw(YawInitial),
    EyePitch(0), EyeRoll(0), LastSensorYaw(0), MoveForward(0), MoveBack(0), MoveLeft(0), MoveRight(0), ShiftDown(0), ControlDown(0), PostProcess(PostProcess_Distortion)
{
}

OculusRoomTinyApp::~OculusRoomTinyApp()
{
    RemoveHandlerFromDevices();
    pSensor.Clear();
    pHMD.Clear();
    destroyWindow();
}

bool OculusRoomTinyApp::setupWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_DECORATED, 0);
    window = glfwCreateWindow(Width, Height, "OculusRoomTiny", NULL, NULL);
    assert(window != 0);
    glfwSetWindowPos(window, HMDInfo.DesktopX, HMDInfo.DesktopY);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, glfwKeyCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glewInit();
    return window != 0;
}

void OculusRoomTinyApp::destroyWindow()
{
    pRender.Clear();

    if (window)
    {
        // Release window resources.
        glfwDestroyWindow(window);
        window = 0;
        Width = Height = 0;
    }
}

int OculusRoomTinyApp::OnStartup(const char* args)
{
    OVR_UNUSED(args);
    // *** Oculus HMD & Sensor Initialization

    // Create DeviceManager and first available HMDDevice from it.
    // Sensor object is created from the HMD, to ensure that it is on the
    // correct device.

    pManager = *DeviceManager::Create();

    // We'll handle it's messages in this case.
    pManager->SetMessageHandler(this);

    bool initDone = false;
    while (!initDone) {
        // Release Sensor/HMD in case this is a retry.
        pSensor.Clear();
        pHMD.Clear();
        RenderParams.MonitorName.Clear();

        pHMD  = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
        if (pHMD)
        {
            pSensor = *pHMD->GetSensor();

            // This will initialize HMDInfo with information about configured IPD,
            // screen size and other variables needed for correct projection.
            // We pass HMD DisplayDeviceName into the renderer to select the
            // correct monitor in full-screen mode.
            if (pHMD->GetDeviceInfo(&HMDInfo))
            {
                RenderParams.MonitorName = HMDInfo.DisplayDeviceName;
                RenderParams.DisplayId = HMDInfo.DisplayId;
                SConfig.SetHMDInfo(HMDInfo);
            }
        }
        else
        {
            // If we didn't detect an HMD, try to create the sensor directly.
            // This is useful for debugging sensor interaction; it is not needed in
            // a shipping app.
            pSensor = *pManager->EnumerateDevices<SensorDevice>().CreateDevice();
        }


        const char* detectionMessage;

        if (!pHMD && !pSensor)
            detectionMessage = "Oculus Rift not detected.";
        else if (!pHMD)
            detectionMessage = "Oculus Sensor detected; HMD Display not detected.";
        else if (!pSensor)
            detectionMessage = "Oculus HMD Display detected; Sensor not detected.";
        else if (HMDInfo.DisplayDeviceName[0] == '\0')
            detectionMessage = "Oculus Sensor detected; HMD display EDID not detected.";
        else
            detectionMessage = 0;

        if (detectionMessage) {
            String messageText(detectionMessage);
            messageText += "\n\n"
                           "Press 'Try Again' to run retry detection.\n"
                           "Press 'Continue' to run full-screen anyway.";
            MessageBoxResult result = MessageBox(messageText);
            // Yeah, this is confusing, but it's correct.
            switch (result) {
            // semantic Retry means 'continue the loop'
            case Retry:
                continue;

            case Cancel:
                return 1;

            // semantic Continue means exit the loop
            case Continue:
                initDone = true;
                break;
            }
        } else {
            initDone = true;
        }
    }

    if (HMDInfo.HResolution > 0)
    {
        Width  = HMDInfo.HResolution;
        Height = HMDInfo.VResolution;
    }

    if (!setupWindow())
        return 1;

    if (pSensor)
    {
        // We need to attach sensor to SensorFusion object for it to receive
        // body frame messages and update orientation. SFusion.GetOrientation()
        // is used in OnIdle() to orient the view.
        SFusion.AttachToSensor(pSensor);
        SFusion.SetDelegateMessageHandler(this);
        SFusion.SetPredictionEnabled(true);
    }


    // *** Initialize Rendering

    // Enable multi-sampling by default.
    RenderParams.Multisample = 4;
    RenderParams.Fullscreen  = true;

    // *** Configure Stereo settings.

    SConfig.SetFullViewport(Viewport(0,0, Width, Height));
    SConfig.SetStereoMode(Stereo_LeftRight_Multipass);

    // Configure proper Distortion Fit.
    // For 7" screen, fit to touch left side of the view, leaving a bit of invisible
    // screen on the top (saves on rendering cost).
    // For smaller screens (5.5"), fit to the top.
    if (HMDInfo.HScreenSize > 0.0f)
    {
        if (HMDInfo.HScreenSize > 0.140f) // 7"
            SConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
        else
            SConfig.SetDistortionFitPointVP(0.0f, 1.0f);
    }

    SConfig.Set2DAreaFov(DegreeToRad(85.0f));

    // Setup Graphics.
    pRender = *RenderTiny::RenderDevice::CreateDevice(RenderParams, window);
    if (!pRender)
        return 1;
    pRender->SetSceneRenderScale(SConfig.GetDistortionScale());
    pRender->SetWindowSize(0, 0);

    // *** Populate Room Scene

    // This creates lights and models.
    PopulateRoomScene(&Scene, pRender);

    LastUpdate = GetAppTime();
    return 0;
}

void OculusRoomTinyApp::OnMessage(const Message& msg)
{
    if (msg.Type == Message_DeviceAdded && msg.pDevice == pManager)
    {
        LogText("DeviceManager reported device added.\n");
    }
    else if (msg.Type == Message_DeviceRemoved && msg.pDevice == pManager)
    {
        LogText("DeviceManager reported device removed.\n");
    }
    else if (msg.Type == Message_DeviceAdded && msg.pDevice == pSensor)
    {
        LogText("Sensor reported device added.\n");
    }
    else if (msg.Type == Message_DeviceRemoved && msg.pDevice == pSensor)
    {
        LogText("Sensor reported device removed.\n");
    }
}


void OculusRoomTinyApp::OnGamepad(float padLx, float padLy, float padRx, float padRy)
{
    GamepadMove   = Vector3f(padLx * padLx * (padLx > 0 ? 1 : -1),
                             0,
                             padLy * padLy * (padLy > 0 ? -1 : 1));
    GamepadRotate = Vector3f(2 * padRx, -2 * padRy, 0);
}

void OculusRoomTinyApp::OnMouseMove(int x, int y, int modifiers)
{
    OVR_UNUSED(modifiers);

    // Mouse motion here is always relative.
    int         dx = x, dy = y;
    const float maxPitch = ((3.1415f/2)*0.98f);

    // Apply to rotation. Subtract for right body frame rotation,
    // since yaw rotation is positive CCW when looking down on XZ plane.
    EyeYaw   -= (Sensitivity * dx)/ 360.0f;

    if (!pSensor)
    {
        EyePitch -= (Sensitivity * dy)/ 360.0f;

        if (EyePitch > maxPitch)
            EyePitch = maxPitch;
        if (EyePitch < -maxPitch)
            EyePitch = -maxPitch;
    }
}

void OculusRoomTinyApp::OnKey(unsigned vk, bool down)
{
    switch (vk)
    {
    case GLFW_KEY_Q:
        if (down && ControlDown)
            Quit = true;
        break;
    case GLFW_KEY_ESCAPE:
        if (!down)
            Quit = true;
        break;

    // Handle player movement keys.
    // We just update movement state here, while the actual translation is done in OnIdle()
    // based on time.
    case GLFW_KEY_W:      MoveForward = down ? (MoveForward | 1) : (MoveForward & ~1); break;
    case GLFW_KEY_S:      MoveBack    = down ? (MoveBack    | 1) : (MoveBack    & ~1); break;
    case GLFW_KEY_A:      MoveLeft    = down ? (MoveLeft    | 1) : (MoveLeft    & ~1); break;
    case GLFW_KEY_D:      MoveRight   = down ? (MoveRight   | 1) : (MoveRight   & ~1); break;
    case GLFW_KEY_UP:    MoveForward = down ? (MoveForward | 2) : (MoveForward & ~2); break;
    case GLFW_KEY_DOWN:  MoveBack    = down ? (MoveBack    | 2) : (MoveBack    & ~2); break;

    case GLFW_KEY_R:
        SFusion.Reset();
        break;

    case GLFW_KEY_P:
        if (down)
        {
            // Toggle chromatic aberration correction on/off.
            RenderDevice::PostProcessShader shader = pRender->GetPostProcessShader();

            if (shader == RenderDevice::PostProcessShader_Distortion)
            {
                pRender->SetPostProcessShader(RenderDevice::PostProcessShader_DistortionAndChromAb);
            }
            else if (shader == RenderDevice::PostProcessShader_DistortionAndChromAb)
            {
                pRender->SetPostProcessShader(RenderDevice::PostProcessShader_Distortion);
            }
            else
                OVR_ASSERT(false);
        }
        break;

    // Switch rendering modes/distortion.
    case GLFW_KEY_F1:
        SConfig.SetStereoMode(Stereo_None);
        PostProcess = PostProcess_None;
        break;
    case GLFW_KEY_F2:
        SConfig.SetStereoMode(Stereo_LeftRight_Multipass);
        PostProcess = PostProcess_None;
        break;
    case GLFW_KEY_F3:
        SConfig.SetStereoMode(Stereo_LeftRight_Multipass);
        PostProcess = PostProcess_Distortion;
        break;

    // Stereo IPD adjustments, in meter (default IPD is 64mm).
    case GLFW_KEY_KP_ADD:
    case GLFW_KEY_INSERT:
        if (down)
            SConfig.SetIPD(SConfig.GetIPD() + 0.0005f * (ShiftDown ? 5.0f : 1.0f));
        break;
    case GLFW_KEY_KP_SUBTRACT:
    case GLFW_KEY_DELETE:
        if (down)
            SConfig.SetIPD(SConfig.GetIPD() - 0.0005f * (ShiftDown ? 5.0f : 1.0f));
        break;

    case GLFW_KEY_BACKSLASH:
        if (down)
            // Swap eye positions.
            SConfig.SetIPD(SConfig.GetIPD() * -1);
        break;

    // Holding down Shift key accelerates adjustment velocity.
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
        ShiftDown = down;
        break;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
        ControlDown = down;
        break;
    }
}


void OculusRoomTinyApp::OnIdle()
{
    double curtime = GetAppTime();
    float  dt      = float(curtime - LastUpdate);
    LastUpdate     = curtime;

    // Handle Sensor motion.
    // We extract Yaw, Pitch, Roll instead of directly using the orientation
    // to allow "additional" yaw manipulation with mouse/controller.
    if (pSensor)
    {
        Quatf    hmdOrient = SFusion.GetOrientation();
        float    yaw = 0.0f;

        hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &EyePitch, &EyeRoll);

        EyeYaw += (yaw - LastSensorYaw);
        LastSensorYaw = yaw;
    }

    // Gamepad rotation.
    EyeYaw -= GamepadRotate.x * dt;

    if (!pSensor)
    {
        // Allow gamepad to look up/down, but only if there is no Rift sensor.
        EyePitch -= GamepadRotate.y * dt;

        const float maxPitch = ((3.1415f/2)*0.98f);
        if (EyePitch > maxPitch)
            EyePitch = maxPitch;
        if (EyePitch < -maxPitch)
            EyePitch = -maxPitch;
    }

    // Handle keyboard movement.
    // This translates EyePos based on Yaw vector direction and keys pressed.
    // Note that Pitch and Roll do not affect movement (they only affect view).
    if (MoveForward || MoveBack || MoveLeft || MoveRight)
    {
        Vector3f localMoveVector(0,0,0);
        Matrix4f yawRotate = Matrix4f::RotationY(EyeYaw);

        if (MoveForward)
            localMoveVector = ForwardVector;
        else if (MoveBack)
            localMoveVector = -ForwardVector;

        if (MoveRight)
            localMoveVector += RightVector;
        else if (MoveLeft)
            localMoveVector -= RightVector;

        // Normalize vector so we don't move faster diagonally.
        localMoveVector.Normalize();
        Vector3f orientationVector = yawRotate.Transform(localMoveVector);
        orientationVector *= MoveSpeed * dt * (ShiftDown ? 3.0f : 1.0f);

        EyePos += orientationVector;
    }

    else if (GamepadMove.LengthSq() > 0)
    {
        Matrix4f yawRotate = Matrix4f::RotationY(EyeYaw);
        Vector3f orientationVector = yawRotate.Transform(GamepadMove);
        orientationVector *= MoveSpeed * dt;
        EyePos += orientationVector;
    }


    // Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
    //
    Matrix4f rollPitchYaw = Matrix4f::RotationY(EyeYaw) * Matrix4f::RotationX(EyePitch) *
                            Matrix4f::RotationZ(EyeRoll);
    Vector3f up      = rollPitchYaw.Transform(UpVector);
    Vector3f forward = rollPitchYaw.Transform(ForwardVector);


    // Minimal head modelling.
    float headBaseToEyeHeight     = 0.15f;  // Vertical height of eye from base of head
    float headBaseToEyeProtrusion = 0.09f;  // Distance forward of eye from base of head

    Vector3f eyeCenterInHeadFrame(0.0f, headBaseToEyeHeight, -headBaseToEyeProtrusion);
    Vector3f shiftedEyePos = EyePos + rollPitchYaw.Transform(eyeCenterInHeadFrame);
    shiftedEyePos.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height

    View = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + forward, up);

    // This is what transformation would be without head modeling.
    // View = Matrix4f::LookAtRH(EyePos, EyePos + forward, up);

    switch(SConfig.GetStereoMode())
    {
    case Stereo_None:
        Render(SConfig.GetEyeRenderParams(StereoEye_Center));
        break;

    case Stereo_LeftRight_Multipass:
        Render(SConfig.GetEyeRenderParams(StereoEye_Left));
        Render(SConfig.GetEyeRenderParams(StereoEye_Right));
        break;
    }

    pRender->Present();
    // Force GPU to flush the scene, resulting in the lowest possible latency.
    pRender->ForceFlushGPU();
}

// Render the scene for one eye.
void OculusRoomTinyApp::Render(const StereoEyeParams& stereo)
{
    pRender->BeginScene(PostProcess);

    // Apply Viewport/Projection for the eye.
    pRender->ApplyStereoParams(stereo);
    pRender->Clear();
    pRender->SetDepthMode(true, true);

    Scene.Render(pRender, stereo.ViewAdjust * View);

    pRender->FinishScene();
}




int OculusRoomTinyApp::Run()
{
    pRender->SetWindowSize(Width, Height);
    pRender->SetViewport(0, 0, Width, Height);
    while (!Quit) {
        glfwPollEvents();
        OnIdle();
    }
    return 0;
}

//-------------------------------------------------------------------------------------
// ***** Program Startup
#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char ** argv)
#endif

{
    int exitCode = 0;

    // Initializes LibOVR. This LogMask_All enables maximum logging.
    // Custom allocator can also be specified here.
    OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));

    // Scope to force application destructor before System::Destroy.
    {
        OculusRoomTinyApp app;
        exitCode = app.OnStartup(0);
        if (!exitCode)
        {
            // Processes messages and calls OnIdle() to do rendering.
            exitCode = app.Run();
        }
    }

    // No OVR functions involving memory are allowed after this.
    OVR::System::Destroy();
    return exitCode;
}
