/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "DataVariant.h"
#include "DynamicObject.h"
#include "Filename.h"
#include "IdlFunctions.h"
#include "MetadataCommands.h"

#include <string>
#include <stdio.h>
#include <idl_export.h>

/*!@cond INTERNAL */
namespace
{
   template<typename T>
   IDL_VPTR vector_to_idl(const DataVariant& value, int idlType)
   {
      std::vector<T> vec = dv_cast<std::vector<T> >(value);
      IDL_MEMINT pDims[] = {vec.size()};
      return IDL_ImportArray(1, pDims, idlType, reinterpret_cast<UCHAR*>(&vec.front()), NULL, NULL);
   }

   template<typename T>
   DataVariant idl_to_DataVariant(size_t total, char* pValue)
   {
      T* pValReal = reinterpret_cast<T*>(pValue);
      if (total == 1)
      {
         return DataVariant(pValReal[0]);
      }
      else
      {
         std::vector<T> tmpVal;
         for (size_t i = 0; i < total; ++i)
         {
            tmpVal.push_back(pValReal[i]);
         }
         return DataVariant(tmpVal);
      }
   }

   template<>
   DataVariant idl_to_DataVariant<bool>(size_t total, char* pValue)
   {
      // special case since bool isn't always 1 byte
      if (total == 1)
      {
         return DataVariant((pValue[0] == 0) ? false : true);
      }
      else
      {
         std::vector<bool> tmpVal;
         for (size_t i = 0; i < total; ++i)
         {
            tmpVal.push_back((pValue[i] == 0) ? false : true);
         }
         return DataVariant(tmpVal);
      }
   }
}
/*!@endcond*/

/**
 * \defgroup metadatacommands Metadata Commands
 */
/*@{*/

/**
 * Access DataElement metadata and wizard arguments.
 *
 * This function gets metadata values from DataElement metadata.
 * It also gets the values of output nodes in wizards which can be used
 * to get return values after wizard execution.
 *
 * @param[in] [1]
 *            The metadata item name. If \p DATASET is specified, this
 *            is the name of the metadata element to access. If \p WIZARD
 *            is specified, this specifies the "Wizard Item/Output Node".
 * @param[in] DATASET @opt
 *            The name of the data element. Defaults to the
 *            primary raster element of the active window if
 *            a wizard is not specified.
 * @param[in] WIZARD @opt
 *            The full path name of the wizard file. If not specified
 *            a data element will be used.
 * @return The metadata item's value. This will be converted to an appropriate
 *         IDL data type.
 * @usage print,get_metadata("NITF/File Header/FDT", DATASET="test.ntf-I1")
 * @endusage
 */
IDL_VPTR get_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
      int wizardExists;
      IDL_STRING wizardName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {"WIZARD", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(wizardExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(wizardName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "GET_METADATA takes a std::string noting which "
         "piece of metadata to get, it also takes a 'wizard' name as keywords.");
      return IDL_StrToSTRING("failure");
   }

   std::string filename;
   std::string wizardName;
   int wizard = 0;
   bool bArray = false;
   //get the value to change as a parameter
   std::string element = IDL_VarGetString(pArgv[0]);
   DynamicObject* pObject = NULL;

   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   if (kw->wizardExists)
   {
      wizardName = IDL_STRING_STR(&kw->wizardName);
      wizard = 1;
   }

   DataVariant value;
   if (!wizard)
   {
      RasterElement* pElement = IdlFunctions::getDataset(filename);

      if (pElement == NULL)
      {
         std::string msg = "Error could not find array.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
         return IDL_StrToSTRING("");
      }
      //not a wizard, so we use the dynamic object functions to get data
      pObject = pElement->getMetadata();
      value = pObject->getAttributeByPath(element);
   }
   else
   {
      //this is a wizard, so get the wizard object and use it to get a value
      WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);

      if (pWizard != NULL)
      {
         value = IdlFunctions::getWizardObjectValue(pWizard, element);
         if (!value.isValid())
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard item.");
            return IDL_StrToSTRING("");
         }
      }
      else
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard file.");
      }
   }

   int type = IDL_TYP_UNDEF;
   std::string valType = value.getTypeName();
   if (valType == "unsigned char")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.c = dv_cast<unsigned char>(value);
      idlPtr->type = IDL_TYP_BYTE;
   }
   else if (valType == "char")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.sc = dv_cast<char>(value);
      idlPtr->type = IDL_TYP_BYTE;
   }
   else if (valType == "bool")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.sc = dv_cast<bool>(value) ? 1 : 0;
      idlPtr->type = IDL_TYP_BYTE;
   }
   else if (valType == "vector<unsigned char>")
   {
      idlPtr = vector_to_idl<unsigned char>(value, IDL_TYP_BYTE);
   }
   else if (valType == "vector<char>")
   {
      idlPtr = vector_to_idl<char>(value, IDL_TYP_BYTE);
   }
   else if (valType == "vector<bool>")
   {
      std::vector<bool> vec = dv_cast<std::vector<bool> >(value);
      std::vector<unsigned char> copyvec;
      copyvec.reserve(vec.size());
      for (std::vector<bool>::const_iterator val = vec.begin(); val != vec.end(); ++val)
      {
         copyvec.push_back((*val) ? 1 : 0);
      }
      IDL_MEMINT dims[] = {copyvec.size()};
      idlPtr = IDL_ImportArray(1, dims, IDL_TYP_BYTE, &copyvec.front(), NULL, NULL);
   }
   else if (valType == "short")
   {
      idlPtr = IDL_GettmpInt(dv_cast<short>(value));
   }
   else if (valType == "vector<short>")
   {
      idlPtr = vector_to_idl<short>(value, IDL_TYP_INT);
   }
   else if (valType == "unsigned short")
   {
      idlPtr = IDL_GettmpUInt(dv_cast<unsigned short>(value));
   }
   else if (valType == "vector<unsigned short>")
   {
      idlPtr = vector_to_idl<unsigned short>(value, IDL_TYP_UINT);
   }
   else if (valType == "int")
   {
      idlPtr = IDL_GettmpLong(dv_cast<int>(value));
   }
   else if (valType == "vector<int>")
   {
      idlPtr = vector_to_idl<int>(value, IDL_TYP_LONG);
   }
   else if (valType == "unsigned int")
   {
      idlPtr = IDL_GettmpLong(dv_cast<unsigned int>(value));
   }
   else if (valType == "vector<unsigned int>")
   {
      idlPtr = vector_to_idl<unsigned int>(value, IDL_TYP_ULONG);
   }
   else if (valType == "float")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.f = dv_cast<float>(value);
      idlPtr->type = IDL_TYP_FLOAT;
   }
   else if (valType == "vector<float>")
   {
      idlPtr = vector_to_idl<float>(value, IDL_TYP_FLOAT);
   }
   else if (valType == "double")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.d = dv_cast<double>(value);
      idlPtr->type = IDL_TYP_DOUBLE;
   }
   else if (valType == "vector<double>")
   {
      idlPtr = vector_to_idl<double>(value, IDL_TYP_DOUBLE);
   }
   else if (valType == "Filename")
   {
      idlPtr = IDL_StrToSTRING(const_cast<char*>(dv_cast<Filename>(value).getFullPathAndName().c_str()));
   }
   else if (valType == "vector<Filename>")
   {
      std::vector<Filename*> vec = dv_cast<std::vector<Filename*> >(value);
      IDL_STRING* pStrarr = reinterpret_cast<IDL_STRING*>(IDL_MemAlloc(vec.size() * sizeof(IDL_STRING), NULL, 0));
      for (size_t idx = 0; idx < vec.size(); ++idx)
      {
         IDL_StrStore(&(pStrarr[idx]), const_cast<char*>(vec[idx]->getFullPathAndName().c_str()));
      }
      IDL_MEMINT pDims[] = {vec.size()};
      idlPtr = IDL_ImportArray(1, pDims, IDL_TYP_STRING, reinterpret_cast<UCHAR*>(pStrarr), NULL, NULL);
   }
   else if (valType == "string")
   {
      idlPtr = IDL_StrToSTRING(const_cast<char*>(dv_cast<std::string>(value).c_str()));
   }
   else if (valType == "vector<string>")
   {
      std::vector<std::string> vec = dv_cast<std::vector<std::string> >(value);
      IDL_STRING* pStrarr = reinterpret_cast<IDL_STRING*>(IDL_MemAlloc(vec.size() * sizeof(IDL_STRING), NULL, 0));
      for (size_t idx = 0; idx < vec.size(); ++idx)
      {
         IDL_StrStore(&(pStrarr[idx]), const_cast<char*>(vec[idx].c_str()));
      }
      IDL_MEMINT pDims[] = {vec.size()};
      idlPtr = IDL_ImportArray(1, pDims, IDL_TYP_STRING, reinterpret_cast<UCHAR*>(pStrarr), NULL, NULL);
   }
   else
   {
      DataVariant::Status status;
      std::string val = value.toDisplayString(&status);
      if (status == DataVariant::SUCCESS)
      {
         idlPtr = IDL_StrToSTRING(const_cast<char*>(val.c_str()));
      }
      else
      {
         idlPtr = IDL_GettmpInt(0);
         std::string msg = "unable to convert data of type " + valType + ".";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
      }
   }

   return idlPtr;
}

/**
 * Set DataElement metadata and wizard arguments.
 *
 * This function sets metadata values in DataElement metadata.
 * It also sets the values of input nodes in wizards which can be used
 * to set parameters before wizard execution.
 *
 * @param[in] [1]
 *            The metadata item name. If \p DATASET is specified, this
 *            is the name of the metadata element to set. If \p WIZARD
 *            is specified, this specifies the "Wizard Item/Input Node".
 * @param[in] [2]
 *            The new item value. IDL data types will be converted to
 *            corresponding Opticks data types.
 * @param[in] DATASET @opt
 *            The name of the data element. Defaults to the
 *            primary raster element of the active window if
 *            a wizard is not specified.
 * @param[in] FILENAME @opt
 *            This flag indicates that \p [2] is a filename and will be expanded
 *            to a canonical, absolute filename.
 * @param[in] WIZARD @opt
 *            The full path name of the wizard file. If not specified
 *            a data element will be used.
 * @param[in] BOOL @opt
 *            If this flag is present, treat \p [2] as a boolean type.
 * @rsof
 * @usage print,set_metadata("Value - Annotation Name/AnnotationName",
 *        "Annotation 1", WIZARD="C:\Wizards\ProcessAnnotation.wiz")
 * @endusage
 */
IDL_VPTR set_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   if (argc > 5 || argc < 2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "SET_METADATA takes a string noting which "
         "piece of metadata to set, and a value to set, it also takes a 'wizard' name as keywords.");
      return IDL_StrToSTRING("failure");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
      int wizardExists;
      IDL_STRING wizardName;
      int boolExists;
      IDL_LONG useBool;
      int filenameExists;
      IDL_LONG useFilename;
   } KW_RESULT;
   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BOOL", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(boolExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(useBool))},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {"FILENAME", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(filenameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(useFilename))},
      {"WIZARD", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(wizardExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(wizardName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);
   std::string elementName = IDL_VarGetString(pArgv[0]);

   IDL_MEMINT total;
   char* pValue = NULL;
   IDL_VarGetData(pArgv[1], &total, &pValue, 0);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   std::string wizardName;
   if (kw->wizardExists)
   {
      wizardName = IDL_STRING_STR(&kw->wizardName);
   }
   bool bUseBool = false;
   if (kw->boolExists)
   {
      if (kw->useBool != 0)
      {
         bUseBool = true;
      }
   }
   bool bUseFilename = false;
   if (kw->filenameExists)
   {
      if (kw->useFilename != 0)
      {
         bUseFilename = true;
      }
   }

   DataVariant value;
   int type = pArgv[1]->type;
   bool bSuccess = false;
   if (bUseBool)
   {
      value = idl_to_DataVariant<bool>(total, pValue);
   }
   else if (bUseFilename)
   {
      if (total == 1)
      {
         //if there is just one, get the contents of the variable as a std::string
         FactoryResource<Filename> pFilename;
         pFilename->setFullPathAndName(reinterpret_cast<char*>(IDL_VarGetString(pArgv[1])));
         value = DataVariant(*(pFilename.get())); // DataVariant will deep copy, so don't release here
      }
      else
      {
         std::vector<Filename*> tmpVal;
         UCHAR* pArrptr  = pArgv[1]->value.arr->data;
         for (int i = 0; i < total; ++i)
         {
            if (reinterpret_cast<IDL_STRING*>(pArrptr)->s != NULL)
            {
               FactoryResource<Filename> pFilename;
               std::string tmpName(reinterpret_cast<IDL_STRING*>(pArrptr)->s);
               pFilename->setFullPathAndName(tmpName);
               tmpVal.push_back(pFilename.release());
            }
            pArrptr += sizeof(IDL_STRING);
         }
         value = DataVariant(tmpVal);
      }
   }
   else
   {
      //for each datatype, populate the type std::string and build a vector if necessary
      switch (type)
      {
      case IDL_TYP_BYTE:
         value = idl_to_DataVariant<char>(total, pValue);
         break;
      case IDL_TYP_INT:
         value = idl_to_DataVariant<short>(total, pValue);
         break;
      case IDL_TYP_UINT:
         value = idl_to_DataVariant<unsigned short>(total, pValue);
         break;
      case IDL_TYP_LONG:
         value = idl_to_DataVariant<int>(total, pValue);
         break;
      case IDL_TYP_ULONG:
         value = idl_to_DataVariant<unsigned int>(total, pValue);
         break;
      case IDL_TYP_FLOAT:
         value = idl_to_DataVariant<float>(total, pValue);
         break;
      case IDL_TYP_DOUBLE:
         value = idl_to_DataVariant<double>(total, pValue);
         break;
      case IDL_TYP_STRING:
         // need to handle string conversion differently
         if (total == 1)
         {
            std::string tmpStr(reinterpret_cast<char*>(IDL_VarGetString(pArgv[1])));
            value = DataVariant(tmpStr);
         }
         else
         {
            std::vector<std::string> tmpVal;
            IDL_STRING* pArrptr = reinterpret_cast<IDL_STRING*>(pArgv[1]->value.arr->data);
            for (int i = 0; i < total && pArrptr[i].s != NULL; ++i)
            {
               tmpVal.push_back(std::string(reinterpret_cast<char*>(pArrptr[i].s)));
            }
            value = DataVariant(tmpVal);
         }
         break;
      case IDL_TYP_UNDEF :
      default:
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
         break;
      }
   }
   if (wizardName.empty())
   {
      RasterElement* pElement = IdlFunctions::getDataset(filename);
      if (pElement == NULL)
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error: could not find data.");
         return IDL_StrToSTRING("");
      }
      DynamicObject* pObject = pElement->getMetadata();
      bSuccess = pObject->setAttributeByPath(elementName, value);
   }
   else
   {
      //this is a wizard object, so first get the wizard, then set a value in it
      WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);

      if (pWizard != NULL)
      {
         if (!IdlFunctions::setWizardObjectValue(pWizard, elementName, value))
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find item in wizard file.");
         }
         else
         {
            bSuccess = true;
         }
      }
      else
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard file.");
      }
   }
   if (bSuccess)
   {
      idlPtr = IDL_StrToSTRING("success");
   }
   else
   {
      idlPtr = IDL_StrToSTRING("failure");
   }

   return idlPtr;
}

/**
 * Copy the metadata from one data element to another.
 *
 * @param[in] [1]
 *            The name of the source data element.
 * @param[in] [2]
 *            The name of the destination data element.
 * @rsof
 * @usage print,copy_metadata("Dataset1", "Dataset2")
 * @endusage
 */
IDL_VPTR copy_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc < 2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "COPY_METADATA takes a source "
         "element, it also takes a destination element.");
      return IDL_StrToSTRING("failure");
   }

   const std::string sourceElementName = IDL_VarGetString(pArgv[0]);
   const std::string destElementName = IDL_VarGetString(pArgv[1]);

   const RasterElement* pSourceElement = IdlFunctions::getDataset(sourceElementName);
   if (pSourceElement == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate source element.");
      return IDL_StrToSTRING("failure");
   }
   RasterElement* pDestElement = IdlFunctions::getDataset(destElementName);
   if (pDestElement == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate destination element.");
      return IDL_StrToSTRING("failure");
   }

   const DynamicObject* pSourceMetadata = pSourceElement->getMetadata();
   if (pSourceMetadata == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate metadata "
         "source element.");
      return IDL_StrToSTRING("failure");
   }
   DynamicObject* pDestMetadata = pDestElement->getMetadata();
   if (pDestMetadata == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate metadata "
         "destination element.");
      return IDL_StrToSTRING("failure");
   }
   pDestMetadata->merge(pSourceMetadata);
   return IDL_StrToSTRING("success");
}

/**
 * Reload a wizard from disk.
 *
 * If wizard nodes have been accessed or mutated, this function
 * will force the wizard to reload from disk the next time it is
 * used. If the wizard has not been accessed previously, this function
 * will return "failure". Any value nodes which have been modified will be
 * reset to the on-disk values.
 *
 * @param[in] [1] @opt
 *            The name of the wizard file. If this is not specified,
 *            all previously loaded wizards will be reloaded.
 * @rsof
 * @usage print,reload_wizard()
 * @endusage
 */
IDL_VPTR reload_wizard(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   std::string wizardName;
   if (argc >= 1)
   {
      wizardName = IDL_VarGetString(pArgv[0]);
   }
   if (!IdlFunctions::clearWizardObject(wizardName))
   {
      return IDL_StrToSTRING("failure");
   }
   return IDL_StrToSTRING("success");
}
/*@}*/

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(copy_metadata), "COPY_METADATA",0,2,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_metadata), "GET_METADATA",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_metadata), "SET_METADATA",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(reload_wizard), "RELOAD_WIZARD",0,1,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addMetadataCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}
