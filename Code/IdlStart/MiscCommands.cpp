/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "IdlFunctions.h"
#include "IdlStart.h"
#include "IdlVersion.h"
#include "MiscCommands.h"
#include "PlugInArgList.h"
#include "PlugInResource.h"
#include "Progress.h"

#include <string>
#include <stdio.h>
#include <idl_export.h>

/**
 * \defgroup misccommands Miscellaneous Commands
 */
/*@{*/

/**
 * Execute a wizard.
 *
 * @param[in] [1]
 *            The full path name of the wizard file.
 * @param[in] BATCH @opt
 *            This flag runs the wizard in batch mode. Defaults to interactive mode.
 * @rsof
 * @usage print,execute_wizard("c:\path\to\wizard.wiz", /BATCH)
 * @endusage
 */
IDL_VPTR execute_wizard(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "A wizard needs to be specified.");
      return IDL_StrToSTRING("failure");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int batchExists;
      IDL_LONG batch;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BATCH", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(batchExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(batch))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   int batch = 0;

   if (kw->batchExists)
   {
      if (kw->batch != 0)
      {
         batch = 1;
      }
   }
   //the full path and name of the wizard file
   std::string wizardName = IDL_VarGetString(pArgv[0]);
   WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);
   bool bSuccess = false;
   if (pWizard != NULL)
   {
      bSuccess = true;

      //create the Wizard Executor plugin
      ExecutableResource pExecutor("Wizard Executor", "", NULL, batch == 0 ? false : true);
      if (pExecutor.get() == NULL)
      {
         bSuccess = false;
      }
      else
      {
         PlugInArgList& pArgList = pExecutor->getInArgList();
         pArgList.setPlugInArgValue("Wizard", pWizard);
         //execute the plugin
         bSuccess &= pExecutor->execute();
      }
   }
   else
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard file.");
   }
   if (bSuccess)
   {
      return IDL_StrToSTRING("success");
   }
   return IDL_StrToSTRING("failure");
}

/**
 * This procedure reports the progress of a process.
 *
 * This will use the progress reporter passed in to the IDL execution plugin.
 * This is most useful when executing IDL from a wizard as the lifespan of the
 * Progress when called from the scripting window is extremely short.
 *
 * @param[in] MESSAGE @opt
 *        The message to place in the progress dialog. Defaults to "Error."
 * @param[in] PERCENT @opt
 *        Integer percentage completion of a process. Defaults to 0.
 * @param[in] REPORTING_LEVEL @opt
 *        The status of the process. Defaults to Warning.
 * @usage report_progress,MESSAGE="Running algorithm", PERCENT=75, REPORTING_LEVEL="NORMAL"
 * @endusage
 */
void report_progress(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (spProgress == NULL)
   {
      return;
   }

   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int messageExists;
      IDL_STRING message;
      int percentExists;
      IDL_LONG percent;
      int reportingLevelExists;
      IDL_STRING reportingLevel;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"MESSAGE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(messageExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(message))},
      {"PERCENT", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(percentExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(percent))},
      {"REPORTING_LEVEL", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(reportingLevelExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(reportingLevel))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   //get the message
   std::string message = "Error.";
   if (kw->messageExists)
   {
      message = IDL_STRING_STR(&kw->message);
   }

   //get the percentage complete
   int percent = 0;
   if (kw->percentExists)
   {
      percent = kw->percent;
   }

   //get the level of reporting granularity
   std::string reportingLevelText = "WARNING";
   if (kw->reportingLevelExists)
   {
      reportingLevelText = IDL_STRING_STR(&kw->reportingLevel);
   }
   ReportingLevel reportingLevel;
   if (reportingLevelText == "NORMAL")
   {
      reportingLevel = NORMAL;
   }
   else if (reportingLevelText == "WARNING")
   {
      reportingLevel = WARNING;
   }
   else if (reportingLevelText == "ABORT")
   {
      reportingLevel = ABORT;
   }
   else if (reportingLevelText == "ERRORS")
   {
      reportingLevel = ERRORS;
   }

   //display progress
   spProgress->updateProgress(message, percent, reportingLevel);
}

/**
 * Retrieve the value of a configuration setting.
 *
 * @param[in] [1]
 *            The name of the configuration setting.
 * @return The string representation of the configuration setting's value.
 * @usage print,get_configuration_setting("General/ReleaseType")
 * @endusage
 */
IDL_VPTR get_configuration_setting(int argc, IDL_VPTR pArgv[])
{
   IDL_VPTR idlPtr;

   //the element
   std::string value;
   //the value
   std::string path(IDL_VarGetString(pArgv[0]));

   if (!path.empty())
   {
      const DataVariant& setting = Service<ConfigurationSettings>()->getSetting(path);
      if (setting.isValid())
      {
         value = setting.toDisplayString();
      }
   }
   idlPtr = IDL_StrToSTRING(const_cast<char*>(value.c_str()));

   return idlPtr;
}

/**
 * Retrieve version information.
 *
 * @param[in] OPTICKS @opt
 *            This flag, if true, will return the version string of Opticks.
 *            If false, the version string of the IDL plug-in will be returned.
 * @return The IDL plug-in version string or, if \c /OPTICKS is specified, the Opticks version string.
 * @usage print,get_version()
 * @endusage
 */
IDL_VPTR get_version(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int opticksExists;
      IDL_LONG opticks;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"OPTICKS", IDL_TYP_LONG, 1, IDL_KW_ZERO, reinterpret_cast<int*>(IDL_KW_OFFSETOF(opticksExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(opticks))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string verString = IDL_VERSION_NUMBER;
   if (kw->opticksExists && kw->opticks != 0)
   {
      verString = Service<ConfigurationSettings>()->getVersion();
   }
   idlPtr = IDL_StrToSTRING(const_cast<char*>(verString.c_str()));
   return idlPtr;
}

/*@}*/

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(execute_wizard), "EXECUTE_WIZARD",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_configuration_setting), "GET_CONFIGURATION_SETTING",0,1,0,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_version), "GET_VERSION",0,0,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

static IDL_SYSFUN_DEF2 proc_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(report_progress), "REPORT_PROGRESS",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addMiscCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   int procCnt = -1;
   while (proc_definitions[++procCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0 &&
          IDL_SysRtnAdd(proc_definitions, IDL_FALSE, procCnt) != 0;
}
