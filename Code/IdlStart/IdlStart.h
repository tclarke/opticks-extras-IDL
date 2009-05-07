/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef IDLSTART_H
#define IDLSTART_H

#include "AppConfig.h"

class External;
class Progress;

extern "C" LINKAGE int start_idl(const char* pLocation, External* pServices, const char** pOutput, const char** pErrorOutput);
extern "C" LINKAGE void execute_idl(const char* pCommand, const char** pOutput, const char** pErrorOutput, Progress* pProgress);
extern "C" LINKAGE int close_idl();

extern Progress* spProgress;

#endif
