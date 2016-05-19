#pragma once

#include "FileOutputComponent.hpp"

#include <string>

namespace PV
{
	class FileOutputComponent;
	
	class ImageOutputComponent : public FileOutputComponent
	{		
		public:
			virtual void updateFileBuffer(std::string fileName, std::vector<pvdata_t> &fileBuffer);
	};
	
	BaseObject * createImageOutputComponent(char const * name, HyPerCol * hc);
}


