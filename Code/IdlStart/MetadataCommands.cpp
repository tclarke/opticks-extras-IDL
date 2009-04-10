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
#include <idl_callproxy.h>

namespace
{
   template<typename T>
   IDL_VPTR vector_to_idl(const DataVariant& value, int idlType)
   {
      std::vector<T> vec = dv_cast<std::vector<T> >(value);
      IDL_MEMINT pDims[] = {vec.size()};
      return IDL_ImportArray(1, pDims, idlType, reinterpret_cast<UCHAR*>(&vec.front()), NULL, NULL);
   }
}

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
   KW_RESULT kw;

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

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);
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

   if (kw.datasetExists)
   {
      filename = IDL_STRING_STR(&kw.datasetName);
   }
   if (kw.wizardExists)
   {
      wizardName = IDL_STRING_STR(&kw.wizardName);
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
   /*else
   {
      //this is a wizard, so get the wizard object and use it to get a value
      WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);

      if (pWizard != NULL)
      {
         pValue = IdlFunctions::getWizardObjectValue(pWizard, element, strType);
         if (pValue == NULL)
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard item.");
            return IDL_StrToSTRING("");
         }
      }
      else
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard file.");
      }
   }*/

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
   IDL_KW_FREE;

   return idlPtr;
}
#if 0

/**
*    SET_METADATA
*
*    Method for setting information is support areas of Opticks, such as the
*    metadata or wizard objects.  The first parameter,
*    element specifies what should be updated.  Its a single std::string, so layers of
*    objects are separated by '/', so the value of an element used to update a
*    Value type named 'Red' in a wizard object would be, "Value - Red/Red".
*
*    @param element
*           Looks for a meta data item with the given name, for
*           Objects within the other objects, the name of each is
*           separated by '/'.
*    @param value
*           The value to place in the metadata
*    @param BOOL - keyword
*           a keyword boolean specifying the value should be set as a boolean, values
*           for 1 are true, and 0 are false
*    @param WIZARD - keyword
*           a keyword std::string specifying if the meta data object to set values for
*           is from a wizard.  The std::string should contain the full path and name
*           of the wizard file to load and alter
*    @param DATASET - keyword
*           a keyword specifying which dataset should be altered.
*    @return
*           returns, "success" or "failure"
*
*/
IDL_VPTR set_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   if (argc>5 || argc<2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "SET_METADATA takes a std::string noting which "
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
   KW_RESULT kw;
   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
   {"BOOL", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(boolExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(useBool)},
   {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName)},
   {"FILENAME", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(filenameExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(useFilename)},
   {"WIZARD", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(wizardExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(wizardName)},
   {NULL}};

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string filename;
   int wizard = 0;
   IDL_MEMINT total;
   //the element
   std::string element = IDL_VarGetString(pArgv[0]);
   char* value = NULL;
   std::string strType;
   std::string wizardName;
   bool bUseFilename = false;
   bool bUseBool = false;
   DynamicObject* pObject = NULL;
   //the value
   IDL_VarGetData(pArgv[1], &total, &value, 0);

   //if there is more than one value in the variable, then make a vector
   if (total > 1)
   {
      strType = "vector<";
   }
   if (kw.datasetExists)
   {
      filename = IDL_STRING_STR(&kw.datasetName);
   }
   if (kw.wizardExists)
   {
      wizardName = IDL_STRING_STR(&kw.wizardName);
      wizard = 1;
   }
   if (kw.boolExists)
   {
      if (kw.useBool != 0)
      {
         bUseBool = true;
      }
   }
   if (kw.filenameExists)
   {
      if (kw.useFilename != 0)
      {
         bUseFilename = true;
      }
   }
   //setup the vectors to pass
   int type = pArgv[1]->type;
   std::vector<bool> boolValue;
   std::vector<unsigned char> charValue;
   std::vector<int> intValue;
   std::vector<unsigned int> uIntValue;
   std::vector<float> floatValue;
   std::vector<double> doubleValue;
   std::vector<short> shortValue;
   std::vector<unsigned short> uShortValue;
   std::vector<std::string> stringValueA;
   std::vector<Filename*> fileValue;
   void* arrayValue = NULL;
   std::string strValue;
   bool bSuccess = false;

   int i = 0;
   if (bUseBool)
   {
      strType = strType + "bool";
      for (i = 0; i < total; i++)
      {
         boolValue.push_back((bool)value[i]);
      }
      arrayValue = &boolValue;
   }
   else if (bUseFilename)
   {
      strType = strType + "Filename";
      if (total == 1)
      {
         //if there is just one, get the contents of the variable as a std::string
         strValue = IDL_VarGetString(pArgv[1]);
         FactoryResource<Filename> pFilename;
         pFilename->setFullPathAndName(strValue);
         value = reinterpret_cast<char*>(pFilename.release();
      }
      else
      {
         UCHAR* arrptr  = pArgv[1]->value.arr->data;
         for (i=0; i < total; i++)
         {
            if (((IDL_STRING*)arrptr)->s != NULL)
            {
               FactoryResource<Filename> pFilename;
               pFilename->setFullPathAndName(std::string(((IDL_STRING*)arrptr)->s));
               fileValue.push_back(pFilename.release());
            }
            arrptr += sizeof(IDL_STRING);
         }
         arrayValue = &fileValue;
      }
   }
   else
   {
      //for each datatype, populate the type std::string and build a vector if necessary
      switch (type)
      {
      case IDL_TYP_BYTE :
         strType = strType + "unsigned char";
         for (i = 0; i < total; i++)
         {
            charValue.push_back(value[i]);
         }
         arrayValue = &charValue;
         break;
      case IDL_TYP_INT :
         strType = strType + "short";
         for (i = 0; i < total; i++)
         {
            short* shortptr = (short*)value;
            shortValue.push_back(shortptr[i]);
         }
         arrayValue = &shortValue;
         break;
      case IDL_TYP_UINT :
         strType = strType + "unsigned short";
         for (i = 0; i < total; i++)
         {
            unsigned short* ushortptr = (unsigned short*)value;
            uShortValue.push_back(ushortptr[i]);
         }
         arrayValue = &uShortValue;
         break;
      case IDL_TYP_LONG :
         strType = strType + "int";
         for (i = 0; i < total; i++)
         {
            int* intptr = reinterpret_cast<int*>(value;
            intValue.push_back(intptr[i]);
         }
         arrayValue = &intValue;
         break;
      case IDL_TYP_ULONG :
         strType = strType + "unsigned int";
         for (i = 0; i < total; i++)
         {
            unsigned int* uintptr = (unsigned int*)value;
            uIntValue.push_back(uintptr[i]);
         }
         arrayValue = &uIntValue;
         break;
      case IDL_TYP_FLOAT :
         strType = strType + "float";
         for (i = 0; i < total; i++)
         {
            float* floatptr = (float*)value;
            floatValue.push_back(floatptr[i]);
         }
         arrayValue = &floatValue;
         break;
      case IDL_TYP_DOUBLE :
         strType = strType + "double";
         for (i = 0; i < total; i++)
         {
            double* doubleptr = (double*)value;
            doubleValue.push_back(doubleptr[i]);
         }
         arrayValue = &doubleValue;
         break;
      case IDL_TYP_STRING :
         strType = "std::string";
         if (total == 1)
         {
            //if there is just one, get the contents of the variable as a std::string
            strValue = IDL_VarGetString(pArgv[1]);
            value = reinterpret_cast<char*>(&strValue;
         }
         else
         {
            UCHAR* arrptr  = pArgv[1]->value.arr->data;
            for (i=0; i < total; i++)
            {
               if (((IDL_STRING*)arrptr)->s != NULL)
               {
                  stringValueA.push_back(std::string(((IDL_STRING*)arrptr)->s));
               }
               arrptr += sizeof(IDL_STRING);
            }
            arrayValue = &stringValueA;
         }
         break;
      case IDL_TYP_UNDEF :
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
         break;
      }
   }
   if (total > 1)
   {
      strType = strType + ">";
      value = reinterpret_cast<char*>(arrayValue;
   }

   if (!wizard)
   {
      RasterElement* pSensor = (IdlFunctions::getDataset(filename));

      if (pSensor == NULL)
      {
         std::string msg = "Error could not find array.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
         return IDL_StrToSTRING("");
      }
      //this is not a wizard, so use the DynamicObject setting code
      pObject = (DynamicObject*)(pSensor->getMetadata());
      IdlFunctions::setDynamicObjectValue(pObject, element, strType, (void*)value);
      bSuccess = true;
   }
   else
   {
      //this is a wizard object, so first get the wizard, then set a value in it
      WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);

      if (pWizard != NULL)
      {
         if (!IdlFunctions::setWizardObjectValue(pWizard, element, strType, (void*)value))
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
   IDL_KW_FREE;
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
*    COPY_METADATA
*
*    Method for copying all the metadata from one data element, to another data
*    element.
*
*    @param source_element
*           The element whose metadata should be copied.
*    @param dest_element
*           The element to where the metadata will be copied.
*    @return
*           returns, "success" or "failure"
*/
IDL_VPTR copy_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "COPY_METADATA takes a source "
         "element, it also takes a destination element.");
      return IDL_StrToSTRING("failure");
   }

   const std::string sourceElementName = IDL_VarGetString(pArgv[0]);
   const std::string destElementName = IDL_VarGetString(pArgv[1]);

   const RasterElement* const pSourceElement =
      IdlFunctions::getDataset(sourceElementName);
   if (NULL == pSourceElement)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate source element.");
      return IDL_StrToSTRING("failure");
   }
   RasterElement* const pDestElement =
      IdlFunctions::getDataset(destElementName);
   if (NULL == pDestElement)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate destination element.");
      return IDL_StrToSTRING("failure");
   }

   const DynamicObject* const pSourceMetadata = pSourceElement->getMetadata();
   if (NULL == pSourceMetadata)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate metadata "
         "source element.");
      return IDL_StrToSTRING("failure");
   }
   DynamicObject* const pDestMetadata = pDestElement->getMetadata();
   if (NULL == pSourceMetadata)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate metadata "
         "destination element.");
      return IDL_StrToSTRING("failure");
   }

   std::vector<std::string> attributeNames;
   pSourceMetadata->getAttributeNames(attributeNames);
   for (std::vector<std::string>::const_iterator attributeNamesIter = attributeNames.begin();
      attributeNamesIter != attributeNames.end();
      ++attributeNamesIter)
   {
      const std::string attributeName = *attributeNamesIter;
      const DataVariant& attribute = pSourceMetadata->getAttribute(attributeName);
      if (!pDestMetadata->setAttribute(attributeName, attribute))
      {
         const std::string message = std::string("unable to copy attribute: ")
            + attributeName;
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, message);
         return IDL_StrToSTRING("failure");
      }
   }

   return IDL_StrToSTRING("success");
}
#endif

static IDL_SYSFUN_DEF2 func_definitions[] = {
//   {reinterpret_cast<IDL_SYSRTN_GENERIC>(copy_metadata), "COPY_METADATA",0,2,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_metadata), "GET_METADATA",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
//   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_metadata), "SET_METADATA",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addMetadataCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}
