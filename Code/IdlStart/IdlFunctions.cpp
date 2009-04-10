/*
* The information in this file is
* subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/
#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "DesktopResource.h"
#include "DesktopServices.h"
#include "DynamicObject.h"
#include "IdlFunctions.h"
#include "LayerList.h"
#include "ModelServices.h"
#include "ObjectResource.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterFileDescriptor.h"
#include "RasterLayer.h"
#include "RasterUtilities.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"
#include "StringUtilities.h"
#include "switchOnEncoding.h"
#include "TypeConverter.h"
#include "WizardItem.h"
#include "WizardObject.h"
#include "xmlreader.h"
#include <stdio.h>
#include <idl_export.h>
#include <idl_callproxy.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <string>
#include <vector>

template<typename T1, typename T2>
static void copyRow(T1* inData, T2* outData, int row, int band, 
                    int cols, int rows, int bands, int startRow, int startCol, int startBand,
                    InterleaveFormatType iType)
{
   for (int c = startCol; c < startCol+cols; ++c)
   {
      int offset = 0;
      if (iType == BSQ)
      {
         offset = (band-startBand)*rows*cols + (row-startRow)*cols + (c-startCol);
      }
      else if (iType = BIP)
      {
         offset = (row-startRow)*cols*bands + (c-startCol)*bands + (band-startBand);
      }
      else if (iType = BIL)
      {
         offset = (row-startRow)*cols*bands + (band-startBand)*cols + (c-startCol);
      }
      outData[c] = static_cast<T2>(inData[offset]);
   }
}

template<typename T>
static bool copyRowCaller(T* inData, void* outData, int row, int band, 
                          int cols, int rows, int bands, int startRow, int startCol, int startBand,
                          InterleaveFormatType iType, EncodingType type)
{
   bool bReturn = true;
   switch (type) 
   { 
   case INT1UBYTE: 
      copyRow(inData, (unsigned char*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   case INT1SBYTE: 
      copyRow(inData, (signed char*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   case INT2UBYTES: 
      copyRow(inData, (unsigned short*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   case INT2SBYTES: 
      copyRow(inData, (signed short*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   case INT4UBYTES: 
      copyRow(inData, (unsigned int*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   case INT4SBYTES: 
      copyRow(inData, (signed int*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   case FLT4BYTES: 
      copyRow(inData, (float*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   case FLT8BYTES: 
      copyRow(inData, (double*)outData, row, band, cols, 
         rows, bands, startRow, startCol, startBand, iType);
      break; 
   default: 
      bReturn = false;
      break; 
   }
   return bReturn;
}

RasterElement* IdlFunctions::getDataset(const std::string& name)
{
   RasterElement* pElement = NULL;
   if (name.empty())
   {
      ViewResource<SpatialDataView> pView(name, true);
      if (pView.get() == NULL)
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "No spatial data window selected.");
         return NULL;
      }
      LayerList* pList = pView->getLayerList();
      pElement = (pList == NULL) ? NULL : pList->getPrimaryRasterElement();
      pView.release();
   }
   else
   {
      QString str = QString::fromStdString(name);
      QStringList list = str.split(QString("=>"));

      bool first = true;
      Service<ModelServices> pModel;
      while (!list.isEmpty())
      {
         QString partName = list.front();
         list.pop_front();

         pElement = static_cast<RasterElement*>(pModel->getElement(name,
            TypeConverter::toString<RasterElement>(), pElement));
         if (pElement == NULL && first)
         {
            //the element was not found to be top level
            std::vector<DataElement*> elements = pModel->getElements(TypeConverter::toString<RasterElement>());
            for (std::vector<DataElement*>::iterator element = elements.begin();
               element != elements.end(); ++element)
            {
               if ((*element)->getName() == name)
               {
                  pElement = static_cast<RasterElement*>(*element);
                  break;
               }
            }
         }
         first = false;
      }
   }
   return pElement;
}

bool IdlFunctions::setWizardObjectValue(WizardObject* pObject, const std::string& name,
                                        const std::string& type, void* pValue)
{
   if (pObject != NULL)
   {
      std::vector<std::string> parts = StringUtilities::split(name, '/');
      if (parts.size() != 2)
      {
         return false;
      }

      std::string wizardItemName = parts[0];
      std::string wizardNodeName = parts[1];

      const std::vector<WizardItem*>& items = pObject->getItems();

      WizardItem* pItem = NULL;
      //traverse the list of items, looking for the one that matches the item name
      for (std::vector<WizardItem*>::const_iterator iter = items.begin(); iter != items.end(); ++iter)
      {
         pItem = *iter;
         if (pItem != NULL)
         {
            if (pItem->getName() == wizardItemName)
            {
               //get the specific node and set the value
               WizardNode* pNode = pItem->getOutputNode(wizardNodeName, type);
               if (pNode == NULL && type == TypeConverter::toString<std::string>())
               {
                  pNode = pItem->getOutputNode(wizardNodeName, TypeConverter::toString<Filename>());
                  if (pNode != NULL)
                  {
                     FactoryResource<Filename> pFilename;
                     std::string* pStr = reinterpret_cast<std::string*>(pValue);
                     pFilename->setFullPathAndName(*pStr);
                     pNode->setValue(pFilename.release());
                     return true;
                  }
               }
               else if (pNode != NULL)
               {
                  pNode->setValue(pValue);
                  return true;
               }
            }
         }
      }
   }
   return false;
}

/**
*  sets the value of a wizard node, the item it is associated with should precede the
*  name of the node with a '/' to separate them.
*
*  @return  
*
*/
void* IdlFunctions::getWizardObjectValue(const WizardObject* pObject, const std::string& name,
                                         std::string& type)
{
   void* value = NULL;
   if (pObject != NULL)
   {
      unsigned int size = strlen(name.c_str());
      char* tmpName = (char*)malloc(size);
      strcpy(tmpName, name.c_str());

      //break the name into two token's separated by '/', with the first being the wizard
      //item, and the second being the wizard node 
      char* p = strstr(tmpName, "/");
      if (p != NULL)
      {
         //insert a NULL at location of p to end the string so we can compare the found item
         *p = NULL;

         //increment the pointer to the string by the size of the separator
         p += 1;
         std::string wizardItemName = std::string(tmpName);
         std::string wizardNodeName = std::string(p);

         const std::vector<WizardItem*>& items = pObject->getItems();

         std::vector<WizardItem*>::const_iterator iter;
         WizardItem* pItem = NULL;
         const char* t = "";

         //traverse the list of items, looking for the one that matches the item name
         for (iter = items.begin(); iter != items.end(); ++iter)
         {
            pItem = *iter;
            if (pItem != NULL)
            {
               if (pItem->getName() == wizardItemName)
               {
                  std::vector<WizardNode*> nodes = pItem->getOutputNodes();
                  std::vector<WizardNode*>::const_iterator iter2;
                  for (iter2 = nodes.begin(); iter2 != nodes.end(); ++iter2)
                  {
                     WizardNode* pNode = *iter2;
                     if (pNode != NULL)
                     {
                        //get the specific node and get the value and type
                        if (pNode->getName() == wizardNodeName)
                        {
                           value = pNode->getValue();
                           type = pNode->getType();
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   return value;
}

RasterElement* IdlFunctions::createRasterElement(void* pData, const std::string& datasetName, const std::string& newName, EncodingType datatype, 
                                                 InterleaveFormatType iType, unsigned int rows, unsigned int cols, unsigned int bands)
{
   RasterElement* pInputRaster = getDataset(datasetName);
   DataElement* pParent = NULL;
   if (pInputRaster != NULL)
   {
      pParent = pInputRaster->getParent();
   }
   ModelResource<RasterElement> pRaster(RasterUtilities::createRasterElement(newName, rows, cols, bands, datatype, iType, true, pParent));
   if (pRaster.get() == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Could not create new RasterElement, may already exist.");
      return NULL;
   }
   RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
   if (pDesc != NULL)
   {
      if (pData != NULL)
      {
         try
         {
            size_t rowSize = pDesc->getBytesPerElement() * cols;
            //populate the resultsmatrix with the data
            for (unsigned int band = 0; band < bands; band++)
            {
               DataAccessor daImage = pRaster->getDataAccessor(getNewDataRequest(pDesc, 0, 0, rows-1, cols-1, band));
               for (unsigned int row = 0; row < rows; row++)
               {
                  if (!daImage.isValid())
                  {
                     throw std::exception();
                  }
                  memcpy(pData, daImage->getRow(), rowSize);
                  daImage->nextRow();
               }
            }
         }
         catch (...)
         {
            std::string msg = "error in copying array values to Opticks.";
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
            return false;
         }
      }
      if (pInputRaster != NULL)
      {
         RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
         RasterDataDescriptor* pInputDesc = static_cast<RasterDataDescriptor*>(pInputRaster->getDataDescriptor());
         VERIFY(pDesc != NULL && pInputDesc != NULL);
         //----- Set the Bands Info
         std::vector<DimensionDescriptor> bandsVec = RasterUtilities::generateDimensionVector(bands, true, true);
         pDesc->setBands(bandsVec);
      }
   }
   return pRaster.release();
}

bool IdlFunctions::changeRasterElement(RasterElement* pRasterElement, void* pData,
                                       EncodingType datatype, InterleaveFormatType iType, unsigned int startRow, 
                                       unsigned int rows, unsigned int startCol, unsigned int cols, 
                                       unsigned int startBand, unsigned int bands, EncodingType oldType)
{
   int index = 0;
   unsigned int i = 0;
   bool bSuccess = false;

   if (pRasterElement != NULL && pData != NULL)
   {
      RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pRasterElement->getDataDescriptor());
      if (pDesc == NULL)
      {
         return false;
      }

      try
      {
         std::size_t rowSize = pDesc->getBytesPerElement() * cols;
         for (unsigned int b = startBand; b < startBand+bands; b++)
         {
            DataRequest* pRequest = getNewDataRequest(pDesc, 0, 0, rows - 1, cols - 1, b);

            DataAccessor daImage = pRasterElement->getDataAccessor(pRequest);

            for (unsigned int r = startRow; r < startRow+rows; ++r)
            {
               if (!daImage.isValid())
               {
                  throw std::exception();
               }
               memcpy(pData, daImage->getRow(), rowSize);
               daImage->nextRow();
            }
         }
      }
      catch (...)
      {
         std::string msg = "error in copying array values to Opticks.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
         return false;
      }
      pRasterElement->updateData();
   }

   return bSuccess;
}

bool IdlFunctions::copyRasterElement(RasterElement* pRasterElement, RasterElement* pNewElement, 
                                     EncodingType datatype, InterleaveFormatType iType, unsigned int startRow, 
                                     unsigned int rows, unsigned int startCol, unsigned int cols, 
                                     unsigned int startBand, unsigned int bands, EncodingType oldType)
{
   int index = 0;
   unsigned int i = 0;
   bool bSuccess = false;

   if (pRasterElement != NULL && pNewElement != NULL)
   {
      DynamicObject* pObject = pNewElement->getMetadata();
      if (pObject != NULL)
      {
         pObject->merge(pRasterElement->getMetadata());
      }
      RasterDataDescriptor *pDesc = dynamic_cast<RasterDataDescriptor*>(pRasterElement->getDataDescriptor());
      if (pDesc == NULL)
      {
         return false;
      }

      //populate the resultsmatrix with the data
      try
      {
         for (unsigned int b = startBand; b < startBand + bands; ++b)
         {
            DataRequest* pRequestOld = getNewDataRequest(pDesc, 0, 0, rows - 1, cols - 1, b);

            DataRequest* pRequestNew = pRequestOld->copy();
            DataAccessor daImage = pRasterElement->getDataAccessor(pRequestOld);

            DataAccessor daOutput = pNewElement->getDataAccessor(pRequestNew);
            for (unsigned int r = startRow; r < startRow + rows; ++r)
            {
               if (!daImage.isValid() || !daOutput.isValid())
               {
                  throw std::exception();
               }
               switchOnEncoding(oldType, copyRowCaller, daImage->getRow(), daOutput->getRow(), r, b, cols, 
                  rows, bands, r, startCol, b, BSQ, datatype);
               daImage->nextRow();
               daOutput->nextRow();
            }
         }
      }
      catch (...)
      {
         std::string msg = "error in copying array values to Opticks.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
         return false;
      }
      pNewElement->updateData();
   }

   return bSuccess;
}

Layer* IdlFunctions::getLayerByRaster(RasterElement* pElement)
{
   VERIFYRV(pElement != NULL, NULL);
   Layer* pDatasetLayer = NULL;
   std::vector<Window*> windows;
   Service<DesktopServices>()->getWindows(SPATIAL_DATA_WINDOW, windows);

   for (std::vector<Window*>::const_iterator windowIter = windows.begin();
      windowIter != windows.end(); ++windowIter)
   {
      SpatialDataWindow* pSpatialDataWindow = dynamic_cast<SpatialDataWindow*>(*windowIter);
      if (pSpatialDataWindow != NULL)
      {
         SpatialDataView* pSpatialDataView = dynamic_cast<SpatialDataView*>(pSpatialDataWindow->getView());
         VERIFYRV(pSpatialDataView != NULL, NULL);

         LayerList* pLayerList = pSpatialDataView->getLayerList();
         VERIFYRV(pLayerList != NULL, NULL);

         // Retrieve all the raster layers in the current window
         std::vector<Layer*> layers;
         pLayerList->getLayers(RASTER, layers);
         for (std::vector<Layer*>::const_iterator layerIter = layers.begin();
            layerIter != layers.end(); ++layerIter)
         {
            RasterLayer* pRasterLayer = dynamic_cast<RasterLayer*>(*layerIter);
            VERIFYRV(pRasterLayer != NULL, NULL);

            DataElement* pDataElement = pRasterLayer->getDataElement();
            if (pDataElement == pElement)
            {
               pDatasetLayer = pRasterLayer;
               break;
            }
         }
      }
   }
   return pDatasetLayer;
}

Layer* IdlFunctions::getLayerByName(const std::string& windowName, const std::string& layerName, bool onlyRasterElements)
{
   Layer* pReturn = NULL;

   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(getViewByWindowName(windowName));
   if (pView != NULL)
   {
      LayerList* pList = pView->getLayerList();
      if (pList != NULL)
      {
         if (onlyRasterElements)
         {
            pReturn = pView->getTopMostLayer(RASTER);
         }
         else
         {
            pReturn = pView->getTopMostLayer();
         }
         std::vector<Layer*> layers;
         pList->getLayers(layers);
         Layer* pLayer = NULL;

         if (layerName != "")
         {
            std::string tmpName;
            bool bFound = false;
            //traverse the list of layers looking for a layer that matches the passed in name
            for (std::vector<Layer*>::const_iterator iter = layers.begin(); iter != layers.end(); ++iter)
            {
               pLayer = *iter;
               if (pLayer != NULL)
               {
                  tmpName = pLayer->getName();
                  if (tmpName == layerName)
                  {
                     pReturn = pLayer;
                     bFound = true;
                     break;
                  }
               }
            }
            if (!bFound)
            {
               pReturn = NULL;
            }
         }
      }
   }
   return pReturn;
}

Layer* IdlFunctions::getLayerByIndex(const std::string& windowName, int index)
{
   Layer* pReturn = NULL;

   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(getViewByWindowName(windowName));
   if (pView != NULL)
   {
      //get the current layer if the index is invalid
      if (index < 0)
      {
         pReturn = pView->getTopMostLayer();
      }
      else
      {
         LayerList* pList = pView->getLayerList();
         if (pList != NULL)
         {
            std::vector<Layer*> layers;
            pList->getLayers(layers);
            std::vector<Layer*>::const_iterator iter;
            if (index < static_cast<int>(layers.size()))
            {
               pReturn = layers.at(index);
            }
         }
      }
   }

   return pReturn;
}

View* IdlFunctions::getViewByWindowName(const std::string& windowName)
{
   Service<DesktopServices> pDesktop;
   SpatialDataWindow* pWindow = NULL;
   if (windowName != "")
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(pDesktop->getWindow(windowName.c_str(), SPATIAL_DATA_WINDOW));
   }
   else 
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(pDesktop->getCurrentWorkspaceWindow());
   }
   if (pWindow == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Opticks was unable to determine the current window.");
      return (NULL);
   }

   return (pWindow->getSpatialDataView());
}

WizardObject* IdlFunctions::getWizardObject(const std::string& wizardName)
{
   WizardObject* pWizard = NULL;
   std::vector<WizardObject*>::const_iterator iter;
   WizardObject* pItem = NULL;

   //traverse the list of items, looking for the one that matches the item name
   for (iter = mpWizards.begin(); iter != mpWizards.end(); ++iter)
   {
      pItem = *iter;
      if (pItem != NULL)
      {
         if (pItem->getName() == wizardName)
         {
            pWizard = pItem;
            break;
         }
      }
   }
   if (pWizard == NULL)
   {
      //the wizard object doesn't exist, read it from the file
      Service<ApplicationServices> pApp;
      ObjectFactory* pObj = pApp->getObjectFactory();
      VERIFYRV(pObj != NULL, NULL);

      pWizard = reinterpret_cast<WizardObject*>(pObj->createObject("WizardObject"));
      VERIFYRV(pWizard != NULL, NULL);

      FactoryResource<Filename> pWizardFilename;
      pWizardFilename->setFullPathAndName(wizardName);

      bool bSuccess = false;
      XmlReader xml;
      XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDocument = xml.parse(pWizardFilename.get());
      if (pDocument != NULL)
      {
         XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* pRoot = pDocument->getDocumentElement();
         if (pRoot != NULL)
         {
            unsigned int version = atoi(A(pRoot->getAttribute(X("version"))));
            bSuccess = pWizard->fromXml(pRoot, version);
         }
      }

      if (bSuccess == false)
      {
         pObj->destroyObject(pWizard, "WizardObject");
         pWizard = NULL;
         VERIFYRV_MSG(false, NULL, "Could not load the wizard from the file");
      }
      pWizard->setName(wizardName);
      mpWizards.push_back(pWizard);
   }
   return pWizard;
}

DataRequest* IdlFunctions::getNewDataRequest(RasterDataDescriptor* pParam,
                                             unsigned int rowStart, unsigned int colStart,
                                             unsigned int rowEnd, unsigned int colEnd,
                                             int band)
{
   FactoryResource<DataRequest> pRequest;
   if (pParam != NULL)
   {
      InterleaveFormatType type = pParam->getInterleaveFormat();
      if (band < 0 && type != BIP)
      {
         type = BIP;
      }
      else if (band >= 0 && type == BIP)
      {
         type = BSQ;
      }
      pRequest->setInterleaveFormat(type);
      pRequest->setRows(pParam->getActiveRow(rowStart), pParam->getActiveRow(rowEnd));
      pRequest->setColumns(pParam->getActiveColumn(colStart), pParam->getActiveColumn(colEnd));
      if (type != BIP)
      {
         pRequest->setBands(pParam->getActiveBand(band), pParam->getActiveBand(band));
      }
   }

   return pRequest.release();
}

void IdlFunctions::copyLayer(RasterLayer* pLayer, const RasterLayer* pOrigLayer)
{
   if (pLayer != NULL && pOrigLayer != NULL)
   {
      pLayer->setAnimation(pOrigLayer->getAnimation());
      pLayer->setXOffset(pOrigLayer->getXOffset());
      pLayer->setYOffset(pOrigLayer->getYOffset());
      pLayer->setXScaleFactor(pOrigLayer->getXScaleFactor());
      pLayer->setYScaleFactor(pOrigLayer->getYScaleFactor());
      pLayer->enableFilters(pOrigLayer->getEnabledFilterNames());
      pLayer->setAlpha(pOrigLayer->getAlpha());
      pLayer->setColorMap(pOrigLayer->getColorMapName(), pOrigLayer->getColorMap());
      pLayer->setComplexComponent(pOrigLayer->getComplexComponent());
      pLayer->enableGpuImage(pOrigLayer->isGpuImageEnabled());
   }
}

RasterChannelType IdlFunctions::getRasterChannelType(const std::string& color)
{
   RasterChannelType element = GRAY;
   if (_stricmp(color.c_str(), "red") == 0)
   {
      element = RED;
   }
   else if (_stricmp(color.c_str(), "green") == 0)
   {
      element = GREEN;
   }
   else if (_stricmp(color.c_str(), "blue") == 0)
   {
      element = BLUE;
   }
   return element;
}
