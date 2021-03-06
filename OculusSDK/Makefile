#############################################################################
#
# Filename    : Makefile
# Content     : Makefile for building linux libovr and OculusWorldDemo
# Created     : 2013
# Authors     : Simon Hallam and Peter Giokaris
# Copyright   : Copyright 2013 OculusVR, Inc. All Rights Reserved
# Instruction : The g++ compiler and stdndard lib packages need to be 
#               installed on the system.  Navigate in a shell to the 
#               directory where this Makefile is located and enter:
#
#               make                builds the release versions for the 
#                                   current architechture
#               make clean          delete intermediate release object files 
#                                   and library and executable
#               make DEBUG=1        builds the debug version for the current
#                                   architechture
#               make clean DEBUG=1  deletes intermediate debug object files 
#                                   and the library and executable
#
# Output      : Relative to the directory this Makefile lives in, libraries
#               and executables are built at the following locations 
#               depending upon the architechture of the system you are 
#               running:
#
#           ./LibOVR/Lib/Linux/Debug/i386/libovr.a
#           ./LibOVR/Lib/Linux/Debug/x86_64/libovr.a
#           ./LibOVR/Lib/Linux/Release/i386/libovr.a
#           ./LibOVR/Lib/Linux/Release/x86_64/libovr.a
#           ./Samples/OculusWorldDemo/Release/OculusWorldDemo_i386_Release
#           ./Samples/OculusWorldDemo/Release/OculusWorldDemo_x86_64_Release
#           ./Samples/OculusWorldDemo/Release/OculusWorldDemo_i386_Debug
#           ./Samples/OculusWorldDemo/Release/OculusWorldDemo_x86_64_Debug
#
# Copyright   :   Copyright 2013 Oculus VR, Inc. All Rights reserved.
#
# Licensed under the Oculus VR SDK License Version 2.0 (the "License"); 
# you may not use the Oculus VR SDK except in compliance with the License, 
# which is provided at the time of installation or download, or which 
# otherwise accompanies this software in either electronic or hard copy form.
#
# You may obtain a copy of the License at
#
# http://www.oculusvr.com/licenses/LICENSE-2.0 

# Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################

####### Detect system architecture

SYSARCH       = i386
ifeq ($(shell uname -m),x86_64)
SYSARCH       = x86_64
endif

####### Compiler, tools and options

CXX           = g++
LINK          = ar rvs
DELETEFILE    = rm -f

####### Detect debug or release

DEBUG         = 0
ifeq ($(DEBUG), 1)
	RELEASETYPE   = Debug
else
	RELEASETYPE   = Release
endif

####### Paths

LIBOVRPATH    = ./LibOVR
DEMOPATH      = ./Samples/OculusWorldDemo

####### Files

LIBOVRTARGET  = $(LIBOVRPATH)/Lib/Linux/$(RELEASETYPE)/$(SYSARCH)/libovr.a
DEMOTARGET    = $(DEMOPATH)/Release/OculusWorldDemo_$(RELEASETYPE)/$(SYSARCH)

####### Rules

all:    $(LIBOVRTARGET) $(DEMOTARGET)

$(DEMOTARGET): $(DEMOPATH)/Makefile
	$(MAKE) -C $(DEMOPATH) DEBUG=$(DEBUG)

$(LIBOVRTARGET): $(LIBOVRPATH)/Makefile
	$(MAKE) -C $(LIBOVRPATH) DEBUG=$(DEBUG)

clean:
	$(MAKE) -C $(LIBOVRPATH) clean DEBUG=$(DEBUG)
	$(MAKE) -C $(DEMOPATH) clean DEBUG=$(DEBUG)

