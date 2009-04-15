/*
 * The information in this file is
 * subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "ModuleManager.h"
#include "PlugInFactory.h"
#include <algorithm>
#include <vector>

const char *ModuleManager::mspName = "IdlStart";
const char *ModuleManager::mspVersion = "";
const char *ModuleManager::mspDescription = "";
const char *ModuleManager::mspValidationKey = "none";
const char *ModuleManager::mspUniqueId = "{c22522cb-c438-47a6-9440-3eb59a7098a4}";

unsigned int ModuleManager::getTotalPlugIns()
{
   return 0;
}

PlugIn *ModuleManager::getPlugIn(unsigned int plugInNumber)
{
   return NULL;
}
