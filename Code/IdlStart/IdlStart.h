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

typedef void (*output_callback_t)(char* pBuf, int num_chars, int error, int addNewline);

extern "C" LINKAGE int start_idl(
             const char* pLocation, External* pServices, const char** pOutput, output_callback_t pReportOutput);
extern "C" LINKAGE int execute_idl(
             int commandCount, char** pCommands, int scope, Progress* pProgress);
extern "C" LINKAGE int close_idl();

extern Progress* spProgress;

#endif
