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

/**
 * \defgroup arraycommands Array Commands
 */
/*@{*/

/**
 * Make raster data available to IDL.
 *
 * @note You must zero out the return value of this function before destroying
 *       the corresponding raster element in Opticks or the application may crash.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @param[out] BANDS_OUT @opt
 *             Returns the number of active bands.
 * @param[out] HEIGHT_OUT @opt
 *             Returns the number of active rows.
 * @param[out] WIDTH_OUT @opt
 *             Returns the number of active columns.
 * @param[in] BANDS_START @opt
 *            The starting band in active band numbers. Defaults to 0.
 * @param[in] BANDS_END @opt
 *            The end band in active band numbers. Defaults to the last band.
 * @param[in] HEIGHT_START @opt
 *            The starting row in active row numbers. Defaults to 0.
 * @param[in] HEIGHT_END @opt
 *            The end row in active row numbers. Defaults to the last row.
 * @param[in] WIDTH_START @opt
 *            The starting column in active column numbers. Defaults to 0.
 * @param[in] WIDTH_END @opt
 *            The end column in active column numbers. Defaults to the last column.
 * @return An array containing the requested data.
 * @usage data = array_to_idl(BANDS_START=1, BANDS_END=2)
 * @endusage
 */
IDL_VPTR array_to_idl(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_IDL takes a 'dataset' name as a keyword,"
         "if used with on disk data, you can also use, 'bands_start', 'bands_end', 'height_start', 'height_end', "
         "'width_start', 'width_end' as keywords.");
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
      {"WIDTH_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(endxwidthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(endxwidth))},
      {"WIDTH_OUT", IDL_TYP_LONG, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(widthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(width))},
      {"WIDTH_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startxwidthExists))
         , reinterpret_cast<char*>(IDL_KW_OFFSETOF(startxwidth))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
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

   bool gotReData = true;
   pRawData = reinterpret_cast<unsigned char*>(pData->getRawData());
   if (pRawData == NULL || kw->startyheightExists || kw->endyheightExists || kw->startxwidthExists ||
      kw->endxwidthExists || kw->bandstartExists || kw->bandendExists)
   {
      gotReData = false;
      pRawData = NULL;
      // can't get rawdata pointer or subcube selected, have to copy
      if (pData != NULL)
      {
         const RasterDataDescriptor* pDesc = dynamic_cast<const RasterDataDescriptor*>(pData->getDataDescriptor());
         if (pDesc != NULL)
         {
            unsigned int heightStart = 0;
            unsigned int heightEnd = pDesc->getRowCount()-1;
            unsigned int widthStart= 0;
            unsigned int widthEnd = pDesc->getColumnCount()-1;
            unsigned int bandStart= 0;
            unsigned int bandEnd = pDesc->getBandCount()-1;
            encoding = pDesc->getDataType();
            unsigned int bytesPerElement = pDesc->getBytesPerElement();
            if (kw->startyheightExists)
            {
               heightStart = kw->startyheight;
            }
            if (kw->endyheightExists)
            {
               heightEnd = kw->endyheight;
            }
            if (kw->startxwidthExists)
            {
               widthStart = kw->startxwidth;
            }
            if (kw->endxwidthExists)
            {
               widthEnd = kw->endxwidth;
            }
            if (kw->bandstartExists)
            {
               bandStart = kw->bandstart;
            }
            if (kw->bandendExists)
            {
               bandEnd = kw->bandend;
            }
            column = widthEnd - widthStart+1;
            row = heightEnd - heightStart+1;
            band = bandEnd - bandStart+1;
            //copy the subcube, determine the type
            pRawData = reinterpret_cast<unsigned char*>(
                  malloc(column*row*band*bytesPerElement));
            if (pRawData == NULL)
            {
               std::string msg = "Not enough memory to allocate array";
               IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
               return IDL_StrToSTRING("failure");
            }

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
         type = IDL_TYP_COMPLEX;
         break;
      case FLT8BYTES:
         type = IDL_TYP_DOUBLE;
         break;
      default:
         type = IDL_TYP_UNDEF;
         break;
   }

   if (kw->widthExists)
   {
      IDL_ALLTYPES tempVal;
      tempVal.ul = column;
      IDL_StoreScalar(kw->width, IDL_TYP_ULONG, &tempVal);
   }
   if (kw->heightExists)
   {
      IDL_ALLTYPES tempVal;
      tempVal.ul = row;
      IDL_StoreScalar(kw->height, IDL_TYP_ULONG, &tempVal);
   }
   if (kw->bandsExists)
   {
      IDL_ALLTYPES tempVal;
      tempVal.ul = band;
      IDL_StoreScalar(kw->bands, IDL_TYP_ULONG, &tempVal);
   }
   IDL_MEMINT dims[] = {0, 0, 0};
   if (band == 1)
   {
      dimensions = 2;
      // set the order for IDL (column major)
      dims[1] = row;
      dims[0] = column;
   }
   else
   {
      switch (iType)
      {
      // set the order for IDL (column major)
      case BSQ:
         dims[2] = band;
         dims[1] = row;
         dims[0] = column;
         break;
      case BIL:
         dims[2] = row;
         dims[1] = band;
         dims[0] = column;
         break;
      case BIP:
         dims[2] = row;
         dims[1] = column;
         dims[0] = band;
         break;
      default:
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Invalid interleave.");
         return IDL_StrToSTRING("failure");
      }
   }
   IDL_VPTR arrayRef;
   if (gotReData)
   {
      arrayRef = IDL_ImportArray(dimensions, dims, type, pRawData, NULL, NULL);
   }
   else
   {
      arrayRef = IDL_ImportArray(dimensions, dims, type, pRawData,
         reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
   }
   return arrayRef;
}

/**
 * Turn an IDL array into a raster element.
 *
 * @param[in] [1]
 *            A one dimensional IDL array containing the data. The Opticks
 *            data type will be inferred from the IDL data type.
 * @param[in] [2]
 *            The name of the raster element.
 * @param[in] BANDS_END
 *            The number of bands in the array.
 * @param[in] HEIGHT_END
 *            The number of rows in the array.
 * @param[in] WIDTH_END
 *            The number of columns in the array.
 * @param[in] DATASET @opt
 *            The name of the data set which will be the parent of the raster element.
 *            Defaults to no parent.
 * @param[in] INTERLEAVE @opt
 *            The interleave of the data. Defaults to BSQ. Valid values are: BIP, BIL, and BSQ.
 * @param[in] NEW_WINDOW @opt
 *            If this flag is true, a new window is created for the data. If it is
 *            false, a new layer in the active window is created.
 * @param[in] ON_DISK @opt
 *            If this flag is true, the data is stored on the hard disk when pushed back to Opticks. If it is
 *            false, the data is stored in RAM when pushed back to Opticks.
 * @param[in] UNITS @opt
 *            The name of the units represented by the data. Defaults to no units.
 * @param[in] OVERWITE @opt
 *            If this flag is true and the target raster element exists, the data will be
 *            overwritten. If the raster element does not exist or the \p NEW_WINDOW flag
 *            is true, this flag is ignored.
 * @param[in] BANDS_START @opt
 *            The starting band in active band numbers if the \p OVERWRITE flag is specified.
 *            Defaults to 0.
 * @param[in] HEIGHT_START @opt
 *            The starting row in active row numbers if the \p OVERWRITE flag is specified.
 *            Defaults to 0.
 * @param[in] WIDTH_START @opt
 *            The starting column in active column numbers if the \p OVERWRITE flag is specified.
 *            Defaults to 0.
 * @rsof
 * @usage array = indgen(20000,/FLOAT)
 * print,array_to_opticks(array, "new", BANDS_END=2, HEIGHT_END=100, WIDTH_END=100, /NEW_WINDOW)
 * @endusage
 */
IDL_VPTR array_to_opticks(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int newWindowExists;
      IDL_LONG newWindow;
      int onDiskExists;
      IDL_LONG onDisk;
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
      {"ON_DISK", IDL_TYP_LONG, 1, IDL_KW_ZERO, reinterpret_cast<int*>(IDL_KW_OFFSETOF(onDiskExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(onDisk))},
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

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
         "dimensional array of data along with a name to represent the data, it has HEIGHT_START, HEIGHT_END, "
         "WIDTH_START, WIDTH_END, and BANDS_START, BANDS_END keywords "
         "to describe the dimensions.  A DATASET keyword to specify a dataset to associate the new data with "
         "an existing window, needed if not using the NEW_WINDOW keyword.  There also exists an INTERLEAVE keyword "
         "which defaults to BSQ, but allows BIP and BIL.  The last keyword is UNITS, which should hold "
         "a std::string you wish to label the data values with.");
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
   if (kw->unitsExists)
   {
      unitName = IDL_STRING_STR(&kw->idlUnits);
   }
   if (kw->datasetExists)
   {
      datasetName = IDL_STRING_STR(&kw->idlDataset);
   }
   if (kw->heightExists)
   {
      height = kw->height;
   }
   if (kw->widthExists)
   {
      width = kw->width;
   }
   if (kw->bandsExists)
   {
      bands = kw->bands;
   }
   if (kw->newWindowExists)
   {
      if (kw->newWindow != 0)
      {
         newWindow = 1;
      }
   }
   //handle the ability to override the default interleave type
   InterleaveFormatType iType = BSQ;
   if (kw->interleaveExists)
   {
      bool error = false;
      iType = StringUtilities::fromXmlString<InterleaveFormatType>(IDL_STRING_STR(&kw->idlInterleave), &error);
      if (error || !iType.isValid())
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET,
            "ARRAY_TO_OPTICKS error.  INTERLEAVE argument must be one of the following: BIP, BSQ or BIL");
         return IDL_StrToSTRING("failure");
      }
   }
   if (total != height*width*bands)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET,
         "ARRAY_TO_OPTICKS error.  Passed in array size does not match size keywords.");
      return IDL_StrToSTRING("failure");
   }
   if (kw->overwriteExists)
   {
      if (kw->overwrite != 0)
      {
         overwrite = 1;
      }
   }
   bool inMemory = true;
   if (kw->onDiskExists)
   {
      if (kw->onDisk != 0)
      {
         inMemory = false;
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
               height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_INT :
         encoding = INT2SBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<short*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_UINT :
         encoding = INT2UBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<unsigned short*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_LONG :
         encoding = INT4SBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<int*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_ULONG :
         encoding = INT4UBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<unsigned int*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_FLOAT :
         encoding = FLT4BYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<float*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_DOUBLE :
         encoding = FLT8BYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<double*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_COMPLEX:
         encoding = FLT8COMPLEX;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<FloatComplex*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, inMemory, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_UNDEF :
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
         break;
   }
   if (newWindow != 0)
   {
      //user wants to create a new RasterElement and window
      RasterElement* pRaster = IdlFunctions::createRasterElement(pRawData, datasetName,
         newDataName, encoding, inMemory, iType, unitName, height, width, bands);
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
   else if (overwrite != 0)
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
            if (kw->startyheightExists)
            {
               heightStart = kw->startyheight;
            }
            if (kw->startxwidthExists)
            {
               widthStart = kw->startxwidth;
            }
            if (kw->bandstartExists)
            {
               bandStart = kw->bandstart;
            }
            EncodingType oldType = pDesc->getDataType();
            if (oldType != encoding)
            {
               IDL_Message(IDL_M_GENERIC, IDL_MSG_RET,
                  "ARRAY_TO_OPTICKS error.  data type of new array is not the same as the old.");
               return IDL_StrToSTRING("failure");
            }
            bSuccess = IdlFunctions::changeRasterElement(pRaster, pRawData, encoding, iType, heightStart,
               height, widthStart, width, bandStart, bands, oldType);
         }
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
 * Return details about Opticks raster data.  This is very useful to determine the amount
 * and layout of the data that is returned from array_to_idl() without having to copy the
 * data into IDL.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @param[out] BANDS_OUT @opt
 *             Returns the number of active bands.
 * @param[out] HEIGHT_OUT @opt
 *             Returns the number of active rows.
 * @param[out] WIDTH_OUT @opt
 *             Returns the number of active columns.
 * @param[out] INTERLEAVE_OUT @opt
 *             Returns "BIP", "BSQ" or "BIL".  See the following table for the dimension
 *             arrangement of the IDL array that will be returned when array_to_idl() is
 *             called for the same \p DATASET.
 *                - BIP - BANDS_OUT, WIDTH_OUT, HEIGHT_OUT
 *                - BSQ - WIDTH_OUT, HEIGHT_OUT, BANDS_OUT
 *                - BIL - WIDTH_OUT, BANDS_OUT, HEIGHT_OUT
 * @param[out] BPE_OUT @opt
 *             Returns number of bytes required to store a single value in the IDL array
 *             that will be returned when array_to_idl() is called for the same \p DATASET.
 * @rsof
 * @usage
 * print,opticks_array_dimensions(BANDS_OUT=num_bands, HEIGHT_OUT=num_rows, WIDTH_OUT=num_columns)
 * print,num_bands
 * print,num_rows
 * print,num_columns
 * @endusage
 */
IDL_VPTR opticks_array_dimensions(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
      int heightExists;
      IDL_VPTR height;
      int widthExists;
      IDL_VPTR width;
      int bandsExists;
      IDL_VPTR bands;
      int interleaveExists;
      IDL_VPTR interleave;
      int bpeExists;
      IDL_VPTR bpe;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BANDS_OUT", IDL_TYP_UNDEF, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandsExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bands))},
      {"BPE_OUT", IDL_TYP_UNDEF, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bpeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bpe))},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {"HEIGHT_OUT", IDL_TYP_UNDEF, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(heightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(height))},
      {"INTERLEAVE_OUT", IDL_TYP_UNDEF, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(interleaveExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(interleave))},
      {"WIDTH_OUT", IDL_TYP_UNDEF, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(widthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(width))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));
   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }
   RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDesc == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   if (kw->widthExists)
   {
      unsigned int column = pDesc->getColumnCount();
      IDL_ALLTYPES tempVal;
      tempVal.ul = column;
      IDL_StoreScalar(kw->width, IDL_TYP_ULONG, &tempVal);
   }
   if (kw->heightExists)
   {
      unsigned int row = pDesc->getRowCount();
      IDL_ALLTYPES tempVal;
      tempVal.ul = row;
      IDL_StoreScalar(kw->height, IDL_TYP_ULONG, &tempVal);
   }
   if (kw->bandsExists)
   {
      unsigned int band = pDesc->getBandCount();
      IDL_ALLTYPES tempVal;
      tempVal.ul = band;
      IDL_StoreScalar(kw->bands, IDL_TYP_ULONG, &tempVal);
   }
   if (kw->interleaveExists)
   {
      InterleaveFormatType iType = pDesc->getInterleaveFormat();
      bool error = false;
      std::string value = StringUtilities::toXmlString(iType, &error);
      if (error)
      {
         const char* pMsg = "Array has an invalid interleave type. This is a bug in the application, not IDL.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, pMsg);
         return IDL_StrToSTRING("failure");
      }
      IDL_ALLTYPES tempVal;
      IDL_StrStore(&(tempVal.str), const_cast<char*>(value.c_str()));
      IDL_StoreScalar(kw->interleave, IDL_TYP_STRING, &tempVal);
   }
   if (kw->bpeExists)
   {
      EncodingType encoding = pDesc->getDataType();
      unsigned char type = IDL_TYP_UNDEF;

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
            type = IDL_TYP_COMPLEX;
            break;
         case FLT8BYTES:
            type = IDL_TYP_DOUBLE;
            break;
         default:
            type = IDL_TYP_UNDEF;
            break;
      }
      IDL_ALLTYPES temp;
      temp.l = IDL_TypeSizeFunc(type);
      IDL_StoreScalar(kw->bpe, IDL_TYP_LONG, &temp);
   }
   return IDL_StrToSTRING("success");
}

/**
 * Return details about the on-disk row numbers for an Opticks raster data.
 *
 * This will return an IDL array, where the value for each item in the
 * array is the on-disk row number for that given loaded row.  This functionality
 * is provided because some Opticks wizard items require the on-disk number
 * in order to function properly.  The Opticks IDL API does not require this
 * number to function properly.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @return An IDL array containing the on-disk row numbers.  If the \b DATASET could not
 *         found, then the IDL string of "failure" will be returned.
 * @usage 
 * print,opticks_array_dimensions(HEIGHT_OUT=num_rows)
 * print,num_rows
 * row_list = opticks_array_ondisk_rows()
 * ; n_elements(row_list) would be equal to num_rows
 * ; row_list[0] would return the on-disk number of the first row
 * @endusage
 */
IDL_VPTR opticks_array_ondisk_rows(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "OPTICKS_ARRAY_ONDISK_ROWS takes a 'dataset' name as a keyword.");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDesc == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   const std::vector<DimensionDescriptor>& rows = pDesc->getRows();
   if (rows.empty())
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }
   UCHAR* pCopyvec = reinterpret_cast<UCHAR*>(malloc(rows.size() * sizeof(unsigned int)));
   VERIFYRV(pCopyvec, NULL);
   unsigned int* pTempPtr = reinterpret_cast<unsigned int*>(pCopyvec);
   for (std::vector<DimensionDescriptor>::const_iterator rowIter = rows.begin();
        rowIter != rows.end();
        ++rowIter, ++pTempPtr)
   {
      *pTempPtr = rowIter->getOnDiskNumber();
   }
   IDL_MEMINT pDims[] = {rows.size()};
   return IDL_ImportArray(1, pDims, IDL_TYP_ULONG, pCopyvec,
      reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
}

/**
 * Return details about the on-disk column numbers for an Opticks raster data.
 *
 * This will return an IDL array, where the value for each item in the
 * array is the on-disk column number for that given loaded column.  This functionality
 * is provided because some Opticks wizard items require the on-disk number
 * in order to function properly.  The Opticks IDL API does not require this
 * number to function properly.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @return An IDL array containing the on-disk column numbers.  If the \b DATASET could not
 *         found, then the IDL string of "failure" will be returned.
 * @usage 
 * print,opticks_array_dimensions(WIDTH_OUT=num_columns)
 * print,num_columns
 * col_list = opticks_array_ondisk_columns()
 * ; n_elements(col_list) would be equal to num_columns
 * ; col_list[0] would return the on-disk number of the first column
 * @endusage
 */
IDL_VPTR opticks_array_ondisk_columns(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "OPTICKS_ARRAY_ONDISK_COLUMNS takes a 'dataset' name as a keyword.");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDesc == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   const std::vector<DimensionDescriptor>& columns = pDesc->getColumns();
   if (columns.empty())
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }
   UCHAR* pCopyvec = reinterpret_cast<UCHAR*>(malloc(columns.size() * sizeof(unsigned int)));
   VERIFYRV(pCopyvec, NULL);
   unsigned int* pTempPtr = reinterpret_cast<unsigned int*>(pCopyvec);
   for (std::vector<DimensionDescriptor>::const_iterator colIter = columns.begin();
        colIter != columns.end();
        ++colIter, ++pTempPtr)
   {
      *pTempPtr = colIter->getOnDiskNumber();
   }
   IDL_MEMINT pDims[] = {columns.size()};
   return IDL_ImportArray(1, pDims, IDL_TYP_ULONG, pCopyvec,
      reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
}

/**
 * Return details about the on-disk band numbers for an Opticks raster data.
 *
 * This will return an IDL array, where the value for each item in the
 * array is the on-disk band number for that given loaded band.  This functionality
 * is provided because some Opticks wizard items require the on-disk number
 * in order to function properly.  The Opticks IDL API does not require this
 * number to function properly.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @return An IDL array containing the on-disk band numbers.  If the \b DATASET could not
 *         found, then the IDL string of "failure" will be returned.
 * @usage 
 * print,opticks_array_dimensions(BANDS_OUT=num_bands)
 * print,num_bands
 * band_list = opticks_array_ondisk_bands()
 * ; n_elements(band_list) would be equal to num_bands
 * ; band_list[0] would return the on-disk number of the first band
 * @endusage
 */
IDL_VPTR opticks_array_ondisk_bands(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "OPTICKS_ARRAY_ONDISK_BANDS takes a 'dataset' name as a keyword.");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDesc == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   const std::vector<DimensionDescriptor>& bands = pDesc->getBands();
   if (bands.empty())
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }
   UCHAR* pCopyvec = reinterpret_cast<UCHAR*>(malloc(bands.size() * sizeof(unsigned int)));
   VERIFYRV(pCopyvec, NULL);
   unsigned int* pTempPtr = reinterpret_cast<unsigned int*>(pCopyvec);
   for (std::vector<DimensionDescriptor>::const_iterator bandIter = bands.begin();
        bandIter != bands.end();
        ++bandIter, ++pTempPtr)
   {
      *pTempPtr = bandIter->getOnDiskNumber();
   }
   IDL_MEMINT pDims[] = {bands.size()};
   return IDL_ImportArray(1, pDims, IDL_TYP_ULONG, pCopyvec,
      reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
}

/**
 * Return details about the original row numbers for an Opticks raster data.
 *
 * This will return an IDL array, where the value for each item in the
 * array is the original row number for that given loaded row.  This functionality
 * is provided because some Opticks wizard items require the original number
 * in order to function properly.  The Opticks IDL API does not require this
 * number to function properly.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @return An IDL array containing the original row numbers.  If the \b DATASET could not
 *         found, then the IDL string of "failure" will be returned.
 * @usage 
 * print,opticks_array_dimensions(HEIGHT_OUT=num_rows)
 * print,num_rows
 * row_list = opticks_array_original_rows()
 * ; n_elements(row_list) would be equal to num_rows
 * ; row_list[0] would return the original number of the first row
 * @endusage
 */
IDL_VPTR opticks_array_original_rows(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "OPTICKS_ARRAY_ORIGINAL_ROWS takes a 'dataset' name as a keyword.");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDesc == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   const std::vector<DimensionDescriptor>& rows = pDesc->getRows();
   if (rows.empty())
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }
   UCHAR* pCopyvec = reinterpret_cast<UCHAR*>(malloc(rows.size() * sizeof(unsigned int)));
   VERIFYRV(pCopyvec, NULL);
   unsigned int* pTempPtr = reinterpret_cast<unsigned int*>(pCopyvec);
   for (std::vector<DimensionDescriptor>::const_iterator rowIter = rows.begin();
        rowIter != rows.end();
        ++rowIter, ++pTempPtr)
   {
      *pTempPtr = rowIter->getOriginalNumber();
   }
   IDL_MEMINT pDims[] = {rows.size()};
   return IDL_ImportArray(1, pDims, IDL_TYP_ULONG, pCopyvec,
      reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
}

/**
 * Return details about the original column numbers for an Opticks raster data.
 *
 * This will return an IDL array, where the value for each item in the
 * array is the original column number for that given loaded column.  This functionality
 * is provided because some Opticks wizard items require the original number
 * in order to function properly.  The Opticks IDL API does not require this
 * number to function properly.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @return An IDL array containing the original column numbers.  If the \b DATASET could not
 *         found, then the IDL string of "failure" will be returned.
 * @usage 
 * print,opticks_array_dimensions(WIDTH_OUT=num_columns)
 * print,num_columns
 * col_list = opticks_array_original_columns()
 * ; n_elements(col_list) would be equal to num_columns
 * ; col_list[0] would return the original number of the first column
 * @endusage
 */
IDL_VPTR opticks_array_original_columns(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "OPTICKS_ARRAY_ORIGINAL_COLUMNS takes a 'dataset' name as a keyword.");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDesc == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   const std::vector<DimensionDescriptor>& columns = pDesc->getColumns();
   if (columns.empty())
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }
   UCHAR* pCopyvec = reinterpret_cast<UCHAR*>(malloc(columns.size() * sizeof(unsigned int)));
   VERIFYRV(pCopyvec, NULL);
   unsigned int* pTempPtr = reinterpret_cast<unsigned int*>(pCopyvec);
   for (std::vector<DimensionDescriptor>::const_iterator colIter = columns.begin();
        colIter != columns.end();
        ++colIter, ++pTempPtr)
   {
      *pTempPtr = colIter->getOriginalNumber();
   }
   IDL_MEMINT pDims[] = {columns.size()};
   return IDL_ImportArray(1, pDims, IDL_TYP_ULONG, pCopyvec,
      reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
}

/**
 * Return details about the original band numbers for an Opticks raster data.
 *
 * This will return an IDL array, where the value for each item in the
 * array is the original band number for that given loaded band.  This functionality
 * is provided because some Opticks wizard items require the original number
 * in order to function properly.  The Opticks IDL API does not require this
 * number to function properly.
 *
 * @param[in] DATASET @opt
 *            The name of the raster element to get. Defaults to
 *            the primary raster element of the active window.
 * @return An IDL array containing the original band numbers.  If the \b DATASET could not
 *         found, then the IDL string of "failure" will be returned.
 * @usage 
 * print,opticks_array_dimensions(BANDS_OUT=num_bands)
 * print,num_bands
 * band_list = opticks_array_original_bands()
 * ; n_elements(band_list) would be equal to num_bands
 * ; band_list[0] would return the original number of the first band
 * @endusage
 */
IDL_VPTR opticks_array_original_bands(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "OPTICKS_ARRAY_ORIGINAL_BANDS takes a 'dataset' name as a keyword.");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   if (kw->datasetExists)
   {
      filename = IDL_STRING_STR(&kw->datasetName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDesc == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }

   const std::vector<DimensionDescriptor>& bands = pDesc->getBands();
   if (bands.empty())
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error could not find array.");
      return IDL_StrToSTRING("failure");
   }
   UCHAR* pCopyvec = reinterpret_cast<UCHAR*>(malloc(bands.size() * sizeof(unsigned int)));
   VERIFYRV(pCopyvec, NULL);
   unsigned int* pTempPtr = reinterpret_cast<unsigned int*>(pCopyvec);
   for (std::vector<DimensionDescriptor>::const_iterator bandIter = bands.begin();
        bandIter != bands.end();
        ++bandIter, ++pTempPtr)
   {
      *pTempPtr = bandIter->getOriginalNumber();
   }
   IDL_MEMINT pDims[] = {bands.size()};
   return IDL_ImportArray(1, pDims, IDL_TYP_ULONG, pCopyvec,
      reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
}

/*@}*/

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(array_to_idl), "ARRAY_TO_IDL",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(array_to_opticks), "ARRAY_TO_OPTICKS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(opticks_array_dimensions),
      "OPTICKS_ARRAY_DIMENSIONS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(opticks_array_ondisk_rows),
      "OPTICKS_ARRAY_ONDISK_ROWS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(opticks_array_ondisk_columns),
      "OPTICKS_ARRAY_ONDISK_COLUMNS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(opticks_array_ondisk_bands),
      "OPTICKS_ARRAY_ONDISK_BANDS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(opticks_array_original_rows),
      "OPTICKS_ARRAY_ORIGINAL_ROWS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(opticks_array_original_columns),
      "OPTICKS_ARRAY_ORIGINAL_COLUMNS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(opticks_array_original_bands),
      "OPTICKS_ARRAY_ORIGINAL_BANDS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addArrayCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}
