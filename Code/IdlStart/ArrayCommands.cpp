/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "ArrayCommands.h"
#include "DesktopServices.h"
#include "IdlFunctions.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterLayer.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"
#include "StringUtilities.h"
#include "switchOnEncoding.h"
#include "Undo.h"

#include <string>
#include <stdio.h>
#include <idl_export.h>
#include <idl_callproxy.h>

IDL_VPTR array_to_idl(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_IDL takes a 'dataset' name or a 'matrix' name as keywords,"
         "if used with on disk data, you can also use, 'bands_start', 'bands_end', 'height_start', 'height_end', "
         "'width_start', 'width_end' as keywords. If you don't know the dimesions of the array, the keywords 'bands', "
         "'height', and 'width' will return them so you can reform the array to the appropriate dimensions");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
      int matrixExists;
      IDL_STRING matrixName;
      int startyheightExists;
      IDL_LONG startyheight;
      int endyheightExists;
      IDL_LONG endyheight;
      int endxwidthExists;
      IDL_LONG endxwidth;
      int startxwidthExists;
      IDL_LONG startxwidth;
      int bandstartExists;
      IDL_LONG bandstart;
      int bandendExists;
      IDL_LONG bandend;
      int heightExists;
      IDL_VPTR height;
      int widthExists;
      IDL_VPTR width;
      int bandsExists;
      IDL_VPTR bands;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BANDS_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandendExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bandend))},
      {"BANDS_OUT", IDL_TYP_LONG, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandsExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bands))},
      {"BANDS_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandstartExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bandstart))},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {"HEIGHT_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(endyheightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(endyheight))},
      {"HEIGHT_OUT", IDL_TYP_LONG, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(heightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(height))},
      {"HEIGHT_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startyheightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(startyheight))},
      {"MATRIX", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(matrixExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(matrixName))},
      {"WIDTH_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(endxwidthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(endxwidth))},
      {"WIDTH_OUT", IDL_TYP_LONG, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(widthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(width))},
      {"WIDTH_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startxwidthExists))
         , reinterpret_cast<char*>(IDL_KW_OFFSETOF(startxwidth))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string filename;
   std::string matrixName;
   if (kw.datasetExists)
   {
      filename = IDL_STRING_STR(&kw.datasetName);
   }
   if (kw.matrixExists)
   {
      matrixName = IDL_STRING_STR(&kw.matrixName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      std::string msg = "Error could not find array.";
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
      return IDL_StrToSTRING("");
   }
   unsigned char* pRawData = NULL;

   int row = 0;
   int column = 0;
   int band = 0;
   EncodingType encoding = INT1SBYTE;
   InterleaveFormatType iType = BSQ;

   int type = IDL_TYP_UNDEF;
   int dimensions = 3;
   RasterElement* pMatrix = NULL;

   if (!kw.matrixExists)
   {
      pRawData = reinterpret_cast<unsigned char*>(pData->getRawData());
      if (pRawData == NULL || kw.startyheightExists || kw.endyheightExists || kw.startxwidthExists ||
          kw.endxwidthExists || kw.bandstartExists || kw.bandendExists)
      {
         pRawData = NULL;
         // can't get rawdata pointer or subcube selected, have to copy
         if (pData != NULL)
         {
            const RasterDataDescriptor* pDesc = dynamic_cast<const RasterDataDescriptor*>(pData->getDataDescriptor());
            if (pDesc != NULL)
            {
               iType = pDesc->getInterleaveFormat();
               unsigned int heightStart = 0;
               unsigned int heightEnd = pDesc->getRowCount()-1;
               unsigned int widthStart= 0;
               unsigned int widthEnd = pDesc->getColumnCount()-1;
               unsigned int bandStart= 0;
               unsigned int bandEnd = pDesc->getBandCount()-1;
               encoding = pDesc->getDataType();
               if (kw.startyheightExists)
               {
                  heightStart = kw.startyheight;
               }
               if (kw.endyheightExists)
               {
                  heightEnd = kw.endyheight;
               }
               if (kw.startxwidthExists)
               {
                  widthStart = kw.startxwidth;
               }
               if (kw.endxwidthExists)
               {
                  widthEnd = kw.endxwidth;
               }
               if (kw.bandstartExists)
               {
                  bandStart = kw.bandstart;
               }
               if (kw.bandendExists)
               {
                  bandEnd = kw.bandend;
               }
               column = widthEnd - widthStart+1;
               row = heightEnd - heightStart+1;
               band = bandEnd - bandStart+1;
               //copy the subcube, determine the type
               switchOnComplexEncoding(encoding, IdlFunctions::copySubcube, pRawData, pData,
                     heightStart, heightEnd, widthStart, widthEnd, bandStart, bandEnd);
            }
         }
      }
      else
      {
         RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
         if (pDesc == NULL)
         {
            return IDL_StrToSTRING("failure");
         }

         iType = pDesc->getInterleaveFormat();

         row = pDesc->getRowCount();
         column = pDesc->getColumnCount();
         band = pDesc->getBandCount();
         encoding = pDesc->getDataType();
      }
   }
   else
   {
      //the user specified a ResultsMatrix to get data from
      pMatrix = dynamic_cast<RasterElement*>(Service<ModelServices>()->getElement(
         matrixName, TypeConverter::toString<RasterElement>(), pData));
      if (pMatrix == NULL)
      {
         std::string msg = "Error could not find array.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
         return IDL_StrToSTRING("");
      }
      const RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pMatrix->getDataDescriptor());
      if (pDesc != NULL)
      {
         iType = pDesc->getInterleaveFormat();
         row = pDesc->getRowCount();
         column = pDesc->getRowCount();
         band = pDesc->getBandCount();
         encoding = pDesc->getDataType();
         pRawData = reinterpret_cast<unsigned char*>(pMatrix->getRawData());
      }
   }

   //set the datatype based on the encoding
   switch (encoding)
   {
      case INT1SBYTE:
         type = IDL_TYP_BYTE;
         break;
      case INT1UBYTE:
         type = IDL_TYP_BYTE;
         break;
      case INT2SBYTES:
         type = IDL_TYP_INT;
         break;
      case INT2UBYTES:
         type = IDL_TYP_UINT;
         break;
      case INT4SCOMPLEX:
         type = IDL_TYP_UNDEF;
         break;
      case INT4SBYTES:
         type = IDL_TYP_LONG;
         break;
      case INT4UBYTES:
         type = IDL_TYP_ULONG;
         break;
      case FLT4BYTES:
         type = IDL_TYP_FLOAT;
         break;
      case FLT8COMPLEX:
         type = IDL_TYP_DCOMPLEX;
         break;
      case FLT8BYTES:
         type = IDL_TYP_DOUBLE;
         break;
      default:
         type = IDL_TYP_UNDEF;
         break;
   }

   if (kw.widthExists)
   {
      //we don't know the datatype passed in, so set all of them
      kw.width->value.d = 0.0;
   }
   if (kw.heightExists)
   {
      //we don't know the datatype passed in to populate, so set all of them
      kw.width->value.d = 0.0;
   }
   if (kw.bandsExists)
   {
      //we don't know the datatype passed in to populate, so set all of them
      kw.width->value.d = 0.0;
   }
   IDL_KW_FREE;
   static IDL_MEMINT dims[] = {row, column, band};
   if (band == 1)
   {
      dimensions = 2;
      // reverse the order for IDL (column major)
      dims[0] = column;
      dims[1] = row;
   }
   else
   {
      switch (iType)
      {
      // reverse the order for IDL (column major)
      case BSQ:
         dims[2] = row;
         dims[1] = column;
         dims[0] = band;
         break;
      case BIL:
         dims[2] = row;
         dims[1] = band;
         dims[0] = column;
         break;
      case BIP:
         dims[2] = band;
         dims[1] = row;
         dims[0] = column;
         break;
      }
   }
   return IDL_ImportArray(dimensions, dims, type, pRawData, NULL, NULL);
}

IDL_VPTR array_to_opticks(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int newWindowExists;
      IDL_LONG newWindow;
      int interleaveExists;
      IDL_STRING idlInterleave;
      int unitsExists;
      IDL_STRING idlUnits;
      int datasetExists;
      IDL_STRING idlDataset;
      int heightExists;
      IDL_LONG height;
      int widthExists;
      IDL_LONG width;
      int bandsExists;
      IDL_LONG bands;
      int overwriteExists;
      IDL_LONG overwrite;
      int startxwidthExists;
      IDL_LONG startxwidth;
      int bandstartExists;
      IDL_LONG bandstart;
      int startyheightExists;
      IDL_LONG startyheight;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BANDS_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandsExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bands))},
      {"BANDS_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandstartExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bandstart))},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlDataset))},
      {"HEIGHT_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(heightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(height))},
      {"HEIGHT_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startyheightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(startyheight))},
      {"INTERLEAVE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(interleaveExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlInterleave))},
      {"NEW_WINDOW", IDL_TYP_LONG, 1, IDL_KW_ZERO, reinterpret_cast<int*>(IDL_KW_OFFSETOF(newWindowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(newWindow))},
      {"OVERWRITE", IDL_TYP_LONG, 1, IDL_KW_ZERO, reinterpret_cast<int*>(IDL_KW_OFFSETOF(overwriteExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(overwrite))},
      {"UNITS", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(unitsExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlUnits))},
      {"WIDTH_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(widthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(width))},
      {"WIDTH_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startxwidthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(startxwidth))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   IDL_MEMINT total;
   std::string unitName;
   unsigned int height = 0;
   unsigned int width = 0;
   unsigned int bands = 1;
   char* pRawData = NULL;
   std::string newDataName;
   std::string datasetName;
   int newWindow = 0;
   int overwrite = 0;
   if (argc < 2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_OPTICKS was not passed needed parameters.  It takes a 1 "
         "dimenstional array of data along with a name to represent the data, it has HEIGHT_START, HEIGHT_END, "
         "WIDTH_START, WIDTH_END, and BANDS_START, BANDS_END keywords "
         "to describe the dimensions.  A DATASET keyword to specify a dataset to associate the new data with "
         "an existing window, needed if not using the NEW_WINDOW keyword.  There also exists an INTERLEAVE keyword "
         "which defaults to BSQ, but allows BIP and BIL.  The last keyword is UNITS, which should hold "
         "a std::string you wish to label the data values with.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   else
   {
      IDL_VarGetData(pArgv[0], &total, &pRawData, 0);
      newDataName = IDL_VarGetString(pArgv[1]);
   }

   bool bSuccess = false;
   EncodingType encoding;
   int type = pArgv[0]->type;
   if (kw.unitsExists)
   {
      unitName = IDL_STRING_STR(&kw.idlUnits);
   }
   if (kw.datasetExists)
   {
      datasetName = IDL_STRING_STR(&kw.idlDataset);
   }
   if (kw.heightExists)
   {
      height = kw.height;
   }
   if (kw.widthExists)
   {
      width = kw.width;
   }
   if (kw.bandsExists)
   {
      bands = kw.bands;
   }
   if (kw.newWindowExists)
   {
      if (kw.newWindow != 0)
      {
         newWindow = 1;
      }
   }
   //handle the ability to override the default interleave type
   InterleaveFormatType iType = BSQ;
   if (kw.interleaveExists)
   {
      iType = StringUtilities::fromXmlString<InterleaveFormatType>(IDL_STRING_STR(&kw.idlInterleave));
   }
   if (total != height*width*bands)
   {
      IDL_KW_FREE;
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_OPTICKS error.  Passed in array size does not match size keywords.");
      return IDL_StrToSTRING("failure");
   }
   if (kw.overwriteExists)
   {
      if (kw.overwrite != 0)
      {
         overwrite = 1;
      }
   }

   //add the data as a new results matrix and view to the current dataset and window
   switch (type)
   {
      case IDL_TYP_BYTE :
         encoding = INT1SBYTE;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<char*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_INT :
         encoding = INT2SBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<short*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_UINT :
         encoding = INT2UBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<unsigned short*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_LONG :
         encoding = INT4SBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<int*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_ULONG :
         encoding = INT4UBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<unsigned int*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_FLOAT :
         encoding = FLT4BYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<float*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_DOUBLE :
         encoding = FLT8BYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<double*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_DCOMPLEX:
         encoding = FLT8COMPLEX;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<FloatComplex*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_UNDEF :
         encoding = UNKNOWN;
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
         break;
   }
   if ((kw.newWindowExists && kw.newWindow != 0) ||
       (kw.overwriteExists && kw.overwrite != 0))
   {
      if (kw.newWindowExists && kw.newWindow != 0)
      {
         //user wants to create a new RasterElement and window
         RasterElement* pRaster = IdlFunctions::createRasterElement(pRawData, datasetName,
            newDataName, encoding, iType, height, width, bands);
         if (pRaster != NULL)
         {
            //----- Now create the spatial data window to display the data
            SpatialDataWindow* pWindow = static_cast<SpatialDataWindow*>(
               Service<DesktopServices>()->createWindow(newDataName, SPATIAL_DATA_WINDOW));

            if (pWindow != NULL)
            {
               SpatialDataView* pView = pWindow->getSpatialDataView();

               if (pView != NULL)
               {
                  UndoLock undo(pView);
                  //----- Set the spatial data in the view
                  pView->setPrimaryRasterElement(pRaster);
                  //----- Create the cube layer
                  RasterLayer* pLayer = static_cast<RasterLayer*>(pView->createLayer(RASTER, pRaster));
                  bSuccess = true;
               }
            }
         }
      }
      else
      {
         //the user wants to replace the spectral cube of the RasterElement with all new data
         RasterElement* pParent = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(datasetName));
         RasterElement* pRaster = static_cast<RasterElement*>(Service<ModelServices>()->getElement(newDataName,
            TypeConverter::toString<RasterElement>(), pParent));
         if (pRaster == NULL)
         {
            pRaster = pParent;
         }
         if (pRaster != NULL)
         {
            RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
            if (pDesc != NULL)
            {
               unsigned int heightStart = 0;
               unsigned int widthStart= 0;
               unsigned int bandStart= 0;
               if (kw.startyheightExists)
               {
                  heightStart = kw.startyheight;
               }
               if (kw.startxwidthExists)
               {
                  widthStart = kw.startxwidth;
               }
               if (kw.bandstartExists)
               {
                  bandStart = kw.bandstart;
               }
               EncodingType oldType = pDesc->getDataType();
               if (oldType != encoding)
               {
                  IDL_KW_FREE;
                  IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_OPTICKS error.  data type of new array is not the same as the old.");
                  return IDL_StrToSTRING("failure");
               }
               IdlFunctions::changeRasterElement(pRaster, pRawData, encoding, iType, heightStart, height, widthStart,
                  width, bandStart, bands, oldType);
               bSuccess = true;
            }
         }
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

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(array_to_idl), "ARRAY_TO_IDL",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(array_to_opticks), "ARRAY_TO_OPTICKS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addArrayCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}
