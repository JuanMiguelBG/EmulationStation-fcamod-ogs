#pragma once
#ifndef ES_CORE_COMPONENTS_IMAGE_COMPONENT_H
#define ES_CORE_COMPONENTS_IMAGE_COMPONENT_H

#include "renderers/Renderer.h" 
#include "math/Vector2i.h"
#include "GuiComponent.h"
#include "resources/TextureResource.h"
#include "resources/Font.h" 

class IPlaylist
{
public:
	virtual std::string getNextItem() = 0;
};

class ImageComponent : public GuiComponent
{
public:
	ImageComponent(Window* window, bool forceLoad = false, bool dynamic = true);
	virtual ~ImageComponent();

	std::string getValue() const override { return mPath; }

	void setDefaultImage(std::string path);

	//Loads the image at the given filepath. Will tile if tile is true (retrieves texture as tiling, creates vertices accordingly).
	void setImage(std::string path, bool tile = false, MaxSizeInfo maxSize = MaxSizeInfo());
	//Loads an image from memory.
	void setImage(const char* image, size_t length, bool tile = false);
	//Use an already existing texture.
	void setImage(const std::shared_ptr<TextureResource>& texture);

	void onSizeChanged() override;
	void setOpacity(unsigned char opacity) override;

	// Resize the image to fit this size. If one axis is zero, scale that axis to maintain aspect ratio.
	// If both are non-zero, potentially break the aspect ratio.  If both are zero, no resizing.
	// Can be set before or after an image is loaded.
	// setMaxSize() and setResize() are mutually exclusive.
	void setResize(float width, float height);
	inline void setResize(const Vector2f& size) { setResize(size.x(), size.y()); }

	// Resize the image to be as large as possible but fit within a box of this size.
	// Can be set before or after an image is loaded.
	// Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
	void setMaxSize(float width, float height);
	inline void setMaxSize(const Vector2f& size) { setMaxSize(size.x(), size.y()); }

	void setMinSize(float width, float height);
	inline void setMinSize(const Vector2f& size) { setMinSize(size.x(), size.y()); }

	Vector2f getRotationSize() const override;

	// Applied AFTER image positioning and sizing
	// cropTop(0.2) will crop 20% of the top of the image.
	void cropLeft(float percent);
	void cropTop(float percent);
	void cropRight(float percent);
	void cropBot(float percent);
	void crop(float left, float top, float right, float bot);
	void uncrop();

	// Multiply all pixels in the image by this color when rendering.
	void setColorShift(unsigned int color);
	void setColorShiftEnd(unsigned int color);
	void setColorGradientHorizontal(bool horizontal);

	virtual void setColor(unsigned int color) { setColorShift(color); }

	void setFlipX(bool flip); // Mirror on the X axis.
	void setFlipY(bool flip); // Mirror on the Y axis.

	void setRotateByTargetSize(bool rotate);  // Flag indicating if rotation should be based on target size vs. actual size.

	// Returns the size of the current texture, or (0, 0) if none is loaded.  May be different than drawn size (use getSize() for that).
	Vector2i getTextureSize() const;

	Vector2f getSize() const override;

	bool hasImage();

	void render(const Transform4x4f& parentTrans) override;

	void setAllowFading(bool fade) { mAllowFading = fade; };
	void setMirroring(Vector2f mirror) { mMirror = mirror; };

	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

	std::shared_ptr<TextureResource> getTexture() { return mTexture; };

	const MaxSizeInfo getMaxSizeInfo() 
	{
		if (mTargetSize == Vector2f(0, 0))
			return MaxSizeInfo(mSize, mTargetIsMax);
		
		return MaxSizeInfo(mTargetSize, mTargetIsMax);		
	};

	void setPadding(const Vector4f padding) { mPadding = padding; updateVertices(); }

	void setHorizontalAlignment(Alignment align) { mHorizontalAlignment = align; }
	void setVerticalAlignment(Alignment align) { mVerticalAlignment = align; }

	float getRoundCorners() { return mRoundCorners; }
	void setRoundCorners(float value) { mRoundCorners = value; }

	virtual void onShow() override;
	virtual void onHide() override;
	virtual void update(int deltaTime);

	void setPlaylist(std::shared_ptr<IPlaylist> playList);

	std::string getImagePath() { return mPath; }
	bool isTiled();

	bool isLinear() { return mLinear; }
	void setIsLinear(bool value) { mLinear = value; }

private:
	Vector2f mTargetSize;

	bool mFlipX, mFlipY, mTargetIsMax, mTargetIsMin;

	// Calculates the correct mSize from our resizing information (set by setResize/setMaxSize).
	// Used internally whenever the resizing parameters or texture change.
	void resize();

	Renderer::Vertex mVertices[4];
//	GLubyte mColors[6*4];

	void updateVertices();
//	void updateColors();
	void fadeIn(bool textureLoaded);

	unsigned int mColorShift;
	unsigned int mColorShiftEnd;
	bool mColorGradientHorizontal;

	std::string mDefaultPath;

	std::shared_ptr<TextureResource> mTexture;
	unsigned char			mFadeOpacity;

	bool					mFading;
	bool					mForceLoad;
	bool					mDynamic;
	bool					mRotateByTargetSize;
	bool					mAllowFading;
	float					mRoundCorners;

	Vector2f mTopLeftCrop;
	Vector2f mBottomRightCrop;	

	Vector2f mMirror;
	bool     mReflectOnBorders;

	std::string mPath;
	std::shared_ptr<TextureResource> mLoadingTexture;

	Vector4f	mPadding;
	Alignment mHorizontalAlignment;
	Alignment mVerticalAlignment;

	std::shared_ptr<IPlaylist> mPlaylist;
	float mPlaylistTimer;

	bool mLinear;
};

#endif // ES_CORE_COMPONENTS_IMAGE_COMPONENT_H
