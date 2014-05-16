####### Detect system architecture

SYSARCH       = i386
ifeq ($(shell uname -m),x86_64)
SYSARCH       = x86_64
endif

####### Compiler, tools and options

CXX           = g++
LINK          = g++
MAKE          = make
DELETEFILE    = rm -f
DEFINES       = -DQT_WEBKIT -DGL_GLEXT_PROTOTYPES

####### Detect debug or release

DEBUG         = 0
ifeq ($(DEBUG), 1)
	CXXFLAGS      = -pipe -DDEBUG -g $(DEFINES)
	LFLAGS        = 
	RELEASETYPE   = Debug
else
	CXXFLAGS      = -pipe -O2 $(DEFINES)
	LFLAGS        = -O1
	RELEASETYPE   = Release
endif

####### Paths

LIBOVRPATH    = ../OculusSDK/LibOVR
COMMONRENDERPATH = ../CommonRender
3RDPARTYPATH  = ../OculusSDK/3rdParty
INCPATH       = -I. -I.. -I$(COMMONRENDERPATH) -I$(LIBOVRPATH)/Include -I$(LIBOVRPATH)/Src
OBJPATH       = ./Obj/Linux/$(RELEASETYPE)/$(SYSARCH)
CXX_BUILD     = $(CXX) -c $(CXXFLAGS) $(INCPATH) -o $(OBJPATH)/

####### Files

LIBS          = -L$(LIBOVRPATH)/Lib/Linux/$(RELEASETYPE)/$(SYSARCH) \
		-lovr \
		-ludev \
		-lpthread \
		-lGL \
		-lX11 \
		-lXinerama

OBJECTS       = $(OBJPATH)/Hackulus.o \
		$(OBJPATH)/Player.o \
		$(OBJPATH)/Platform.o \
		$(OBJPATH)/Linux_Platform.o \
		$(OBJPATH)/Linux_Gamepad.o \
		$(OBJPATH)/Render_Device.o \
		$(OBJPATH)/Render_GL_Device.o \
		$(OBJPATH)/Render_LoadTextureDDS.o \
		$(OBJPATH)/Render_LoadTextureTGA.o \
		$(OBJPATH)/Render_XmlSceneLoader.o 

TARGET        = ./Release/Hackulus_$(SYSARCH)_$(RELEASETYPE)

####### Rules

all:    $(TARGET)

$(TARGET):  $(LIBOVRPATH)/Lib/Linux/$(RELEASETYPE)/$(SYSARCH)/libovr.a
	$(MAKE) -C $(LIBOVRPATH) DEBUG=$(DEBUG)

$(TARGET):  $(OBJECTS)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

$(OBJPATH)/Hackulus.o: Hackulus.cpp 
	$(CXX_BUILD)Hackulus.o Hackulus.cpp

$(OBJPATH)/Player.o: Player.cpp 
	$(CXX_BUILD)Player.o Player.cpp

$(OBJPATH)/Platform.o: ../CommonRender/Platform/Platform.cpp 
	$(CXX_BUILD)Platform.o ../CommonRender/Platform/Platform.cpp

$(OBJPATH)/Linux_Platform.o: ../CommonRender/Platform/Linux_Platform.cpp 
	$(CXX_BUILD)Linux_Platform.o ../CommonRender/Platform/Linux_Platform.cpp

$(OBJPATH)/Linux_Gamepad.o: ../CommonRender/Platform/Linux_Gamepad.cpp 
	$(CXX_BUILD)Linux_Gamepad.o ../CommonRender/Platform/Linux_Gamepad.cpp

$(OBJPATH)/Render_Device.o: ../CommonRender/Render/Render_Device.cpp $
	$(CXX_BUILD)Render_Device.o ../CommonRender/Render/Render_Device.cpp

$(OBJPATH)/Render_GL_Device.o: ../CommonRender/Render/Render_GL_Device.cpp 
	$(CXX_BUILD)Render_GL_Device.o ../CommonRender/Render/Render_GL_Device.cpp

$(OBJPATH)/Render_LoadTextureDDS.o: ../CommonRender/Render/Render_LoadTextureDDS.cpp 
	$(CXX_BUILD)Render_LoadTextureDDS.o ../CommonRender/Render/Render_LoadTextureDDS.cpp

$(OBJPATH)/Render_LoadTextureTGA.o: ../CommonRender/Render/Render_LoadTextureTGA.cpp 
	$(CXX_BUILD)Render_LoadTextureTGA.o ../CommonRender/Render/Render_LoadTextureTGA.cpp

$(OBJPATH)/Render_XmlSceneLoader.o: ../CommonRender/Render/Render_XmlSceneLoader.cpp 
	$(CXX_BUILD)Render_XmlSceneLoader.o ../CommonRender/Render/Render_XmlSceneLoader.cpp

clean:
	-$(DELETEFILE) $(OBJECTS)
	-$(DELETEFILE) $(TARGET)
	
#############################################################################
# Modified from:
# Copyright   :   Copyright 2012 Oculus VR, Inc. All Rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################
	

