#include "resources/TextureData.h"

#include "math/Misc.h"
#include "renderers/Renderer.h" 
#include "resources/ResourceManager.h"
#include "ImageIO.h"
#include "Log.h"
#include <nanosvg/nanosvg.h>
#include <nanosvg/nanosvgrast.h>
#include <assert.h>
#include <string.h>

#define DPI 96

bool TextureData::OPTIMIZEVRAM = false;

TextureData::TextureData(bool tile, bool linear) : mTile(tile), mLinear(linear), mTextureID(0), mDataRGBA(nullptr), mScalable(false),
									  mWidth(0), mHeight(0), mSourceWidth(0.0f), mSourceHeight(0.0f), mMaxSize(MaxSizeInfo()), mPackedSize(Vector2i(0,0)), mBaseSize(Vector2i(0, 0))
{
	mIsExternalDataRGBA = false;
}

TextureData::~TextureData()
{
	releaseVRAM();
	releaseRAM();
}

void TextureData::initFromPath(const std::string& path)
{
	// Just set the path. It will be loaded later
	mPath = path;
	// Only textures with paths are reloadable
	mReloadable = true;
}

bool TextureData::initSVGFromMemory(const unsigned char* fileData, size_t length)
{
	// If already initialised then don't read again
	std::unique_lock<std::mutex> lock(mMutex);
	if (mDataRGBA)
		return true;

	// nsvgParse excepts a modifiable, null-terminated string
	char* copy = (char*)malloc(length + 1);
	assert(copy != NULL);
	memcpy(copy, fileData, length);
	copy[length] = '\0';

	NSVGimage* svgImage = nsvgParse(copy, "px", DPI);
	free(copy);
	if (!svgImage)
	{
		LOG(LogError) << "TextureData::initSVGFromMemory() - ERROR: problem to parsing SVG image.";
		return false;
	}

	if (svgImage->width == 0 || svgImage->height == 0)
		return false;

	// We want to rasterise this texture at a specific resolution. If the source size
	// variables are set then use them otherwise set them from the parsed file
	if ((mSourceWidth == 0.0f) && (mSourceHeight == 0.0f))
	{
		mSourceWidth = svgImage->width;
		mSourceHeight = svgImage->height;
	}
	else 
		mSourceWidth = (mSourceHeight * svgImage->width) / svgImage->height; // FCATMP : Always keep source aspect ratio

	mWidth = (size_t)Math::round(mSourceWidth);
	mHeight = (size_t)Math::round(mSourceHeight);

	if (mWidth == 0)
	{
		// auto scale width to keep aspect
		mWidth = (size_t)Math::round(((float)mHeight / svgImage->height) * svgImage->width);
	}
	else if (mHeight == 0)
	{
		// auto scale height to keep aspect
		mHeight = (size_t)Math::round(((float)mWidth / svgImage->width) * svgImage->height);
	}

	mBaseSize = Vector2i(mWidth, mHeight);
	
	if (mMaxSize.x() > 0 && mMaxSize.y() > 0 && mHeight < mMaxSize.y() && mWidth < mMaxSize.x()) // FCATMP
	{
		Vector2i sz = ImageIO::adjustPictureSize(Vector2i(mWidth, mHeight), Vector2i(mMaxSize.x(), mMaxSize.y()), mMaxSize.externalZoom());
		mHeight = sz.y();
		mWidth = (int)((mHeight * svgImage->width) / svgImage->height);
	}
	
	if (OPTIMIZEVRAM && mMaxSize.x() > 0 && mMaxSize.y() > 0 && (mWidth > mMaxSize.x() || mHeight > mMaxSize.y()))
	{
		Vector2i sz = ImageIO::adjustPictureSize(Vector2i(mWidth, mHeight), Vector2i(mMaxSize.x(), mMaxSize.y()), mMaxSize.externalZoom());
		mHeight = sz.y();	
		mWidth = (mHeight * svgImage->width) / svgImage->height;

		mPackedSize = Vector2i(mWidth, mHeight);
	}
	else
		mPackedSize = Vector2i(0, 0);
	
	unsigned char* dataRGBA = new unsigned char[mWidth * mHeight * 4];

	NSVGrasterizer* rast = nsvgCreateRasterizer();

	float scale = Math::min(mHeight / svgImage->height, mWidth / svgImage->width);
	nsvgRasterize(rast, svgImage, 0, 0, scale, dataRGBA, (int)mWidth, (int)mHeight, (int)mWidth * 4);

	nsvgDeleteRasterizer(rast);

	ImageIO::flipPixelsVert(dataRGBA, mWidth, mHeight);

	mDataRGBA = dataRGBA;

	return true;
}

bool TextureData::isRequiredTextureSizeOk()
{
	if (!OPTIMIZEVRAM)
		return true;

	if (mPackedSize == Vector2i(0, 0))
		return true;

	if (mBaseSize == Vector2i(0, 0))
		return true;

	if (mMaxSize.empty())
		return true;

	if ((int) mMaxSize.x() <= mPackedSize.x() || (int) mMaxSize.y() <= mPackedSize.y())
		return true;

	if (mBaseSize.x() <= mPackedSize.x() || mBaseSize.y() <= mPackedSize.y())
		return true;

	return false;
}

bool TextureData::initImageFromMemory(const unsigned char* fileData, size_t length)
{
	size_t width, height;

	// If already initialised then don't read again
	{
		std::unique_lock<std::mutex> lock(mMutex);
		if (mDataRGBA)
			return true;
	}
	

	auto x = OPTIMIZEVRAM ? mMaxSize.x() : Renderer::getScreenWidth();
	if (x > Renderer::getScreenWidth())
		x = Renderer::getScreenWidth();

	auto y = OPTIMIZEVRAM ? mMaxSize.y() : Renderer::getScreenHeight();
	if (y > Renderer::getScreenHeight())
		y = Renderer::getScreenHeight();

	unsigned char* imageRGBA = ImageIO::loadFromMemoryRGBA32Ex((const unsigned char*)(fileData), length, width, height, x, y, mMaxSize.externalZoom(), mBaseSize, mPackedSize);
	if (imageRGBA == NULL)
	{
		LOG(LogError) << "TextureData::initImageFromMemory() - ERROR: Could not initialize texture from memory, invalid data!  (file path: " << mPath << ", data ptr: " << (size_t)fileData << ", reported size: " << length << ")";
		return false;
	}

	mSourceWidth = (float) width;
	mSourceHeight = (float) height;
	mScalable = false;

	return initFromRGBAEx(imageRGBA, width, height);
}


void TextureData::setMaxSize(MaxSizeInfo maxSize)
{
	if (mSourceWidth == 0 || mSourceHeight == 0)
		mMaxSize = maxSize;
	else
	{
		Vector2i value = ImageIO::adjustPictureSize(Vector2i(mSourceWidth, mSourceHeight), Vector2i(mMaxSize.x(), mMaxSize.y()), mMaxSize.externalZoom());
		Vector2i newVal = ImageIO::adjustPictureSize(Vector2i(mSourceWidth, mSourceHeight), Vector2i(maxSize.x(), maxSize.y()), mMaxSize.externalZoom());

		if (newVal.x() > value.x() || newVal.y() > value.y())
			mMaxSize = maxSize;

		//if (mMaxSize.x() < maxSize.x() || mMaxSize.y() < maxSize.y())
	}
};


bool TextureData::initFromRGBA(const unsigned char* dataRGBA, size_t width, size_t height)
{
	// If already initialised then don't read again
	std::unique_lock<std::mutex> lock(mMutex);

	if (mIsExternalDataRGBA)
	{
		mIsExternalDataRGBA = false;
		mDataRGBA = nullptr;
	}

	if (mDataRGBA)
		return true;

	// Take a copy
	mDataRGBA = new unsigned char[width * height * 4];
	memcpy(mDataRGBA, dataRGBA, width * height * 4);
	mWidth = width;
	mHeight = height;
	return true;
}

bool TextureData::initFromRGBAEx(unsigned char* dataRGBA, size_t width, size_t height)
{
	// If already initialised then don't read again
	std::unique_lock<std::mutex> lock(mMutex);

	if (mIsExternalDataRGBA)
	{
		mIsExternalDataRGBA = false;
		mDataRGBA = nullptr;
	}

	if (mDataRGBA)
		return true;

	// Take a copy
	mDataRGBA = dataRGBA;
	mWidth = width;
	mHeight = height;

	return true;
}

bool TextureData::initFromExternalRGBA(unsigned char* dataRGBA, size_t width, size_t height)
{
	// If already initialised then don't read again
	std::unique_lock<std::mutex> lock(mMutex);

	if (!mIsExternalDataRGBA && mDataRGBA != nullptr)
		delete[] mDataRGBA;

	mIsExternalDataRGBA = true;
	mDataRGBA = dataRGBA;
	mWidth = width;
	mHeight = height;

	if (mTextureID != 0)
		Renderer::updateTexture(mTextureID, Renderer::Texture::RGBA, -1, -1, mWidth, mHeight, mDataRGBA);

	return true;
}

bool TextureData::load(bool updateCache)
{
	bool retval = false;

	// Need to load. See if there is a file
	if (!mPath.empty())
	{
		std::shared_ptr<ResourceManager>& rm = ResourceManager::getInstance();

		const ResourceData& data = rm->getFileData(mPath);
		// is it an SVG?
		if (mPath.substr(mPath.size() - 4, std::string::npos) == ".svg")
		{
			mScalable = true; // ??? interest ?
			retval = initSVGFromMemory((const unsigned char*)data.ptr.get(), data.length);
		}
		else
			retval = initImageFromMemory((const unsigned char*)data.ptr.get(), data.length);

		if (updateCache && retval)
			ImageIO::updateImageCache(mPath, data.length, mBaseSize.x(), mBaseSize.y());
	}

	return retval;
}

bool TextureData::isLoaded()
{
	std::unique_lock<std::mutex> lock(mMutex);
	if (mDataRGBA || (mTextureID != 0))
		return true;

	return false;
}

bool TextureData::uploadAndBind()
{
	// See if it's already been uploaded
	std::unique_lock<std::mutex> lock(mMutex);
	if (mTextureID != 0)
	{
		Renderer::bindTexture(mTextureID);
	}
	else
	{
		// Load it if necessary
		if (!mDataRGBA)
		{
			return false;
		}
		// Make sure we're ready to upload
		if ((mWidth == 0) || (mHeight == 0) || (mDataRGBA == nullptr))
			return false;

		mTextureID = Renderer::createTexture(Renderer::Texture::RGBA, mLinear, mTile, mWidth, mHeight, mDataRGBA);
		if (mTextureID)
		{
			if (mDataRGBA != nullptr && !mIsExternalDataRGBA)
				delete[] mDataRGBA;

			mDataRGBA = nullptr;
		}
	}

	return true;
}

void TextureData::releaseVRAM()
{
	std::unique_lock<std::mutex> lock(mMutex);
	if (mTextureID != 0)
	{
		Renderer::destroyTexture(mTextureID);
		mTextureID = 0;
	}
}

void TextureData::releaseRAM()
{
	std::unique_lock<std::mutex> lock(mMutex);

	if (mDataRGBA != nullptr && !mIsExternalDataRGBA)
		delete[] mDataRGBA;

	mDataRGBA = 0;
}

size_t TextureData::width()
{
	if (mWidth == 0)
		load();
	return mWidth;
}

size_t TextureData::height()
{
	if (mHeight == 0)
		load();
	return mHeight;
}

float TextureData::sourceWidth()
{
	if (mSourceWidth == 0)
		load();
	return mSourceWidth;
}

float TextureData::sourceHeight()
{
	if (mSourceHeight == 0)
		load();
	return mSourceHeight;
}

void TextureData::setTemporarySize(float width, float height)
{
	mWidth = width;
	mHeight = height;
	mSourceWidth = width;
	mSourceHeight = height;
}

void TextureData::setSourceSize(float width, float height)
{
	if (mScalable)
	{
		//if ((mSourceWidth != width) || (mSourceHeight != height))
		if (mSourceHeight < height) // FCATMP
		{
			mSourceWidth = width;
			mSourceHeight = height;

			releaseVRAM();
			releaseRAM();
		}
	}
}

size_t TextureData::getVRAMUsage()
{
	if ((mTextureID != 0) || (mDataRGBA != nullptr))
		return mWidth * mHeight * 4;
	else
		return 0;
}
