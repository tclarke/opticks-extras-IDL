/*
 * The information in this file is
 * subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef IDLSTART_H
#define IDLSTART_H

class External;
class Progress;

#ifdef WIN32
#define IDLSTART_EXPORT extern "C" __declspec(dllexport)
#else
#define IDLSTART_EXPORT extern "C"
#endif

IDLSTART_EXPORT int start_idl(const char* pLocation, External* pServices);
IDLSTART_EXPORT void execute_idl(const char* pCommand, const char** pOutput, const char** pErrorOutput, Progress* pProgress);
IDLSTART_EXPORT int close_idl();

extern Progress* spProgress;

#endif
