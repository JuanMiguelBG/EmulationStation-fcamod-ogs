#include "SystemScreenSaver.h"

#include "components/VideoVlcComponent.h"
#include "utils/Randomizer.h"
#include "utils/FileSystemUtil.h"
#include "views/gamelist/IGameListView.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "PowerSaver.h"
#include "Sound.h"
#include "SystemData.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include <unordered_map>
#include <time.h>
#include "AudioManager.h"
#include "math/Vector2i.h"
#include "platform.h"
#include "Scripting.h"
#include "ApiSystem.h"

#define FADE_TIME 			500

SystemScreenSaver::SystemScreenSaver(Window* window) :
	mVideoScreensaver(NULL),
	mImageScreensaver(NULL),
	mWindow(window),
	mVideosCounted(false),
	mVideoCount(0),
	mImagesCounted(false),
	mImageCount(0),
	mState(STATE_INACTIVE),
	mOpacity(0.0f),
	mTimer(0),
	mSystemName(""),
	mGameName(""),
	mCurrentGame(NULL),
	mLoadingNext(false)
{

	mWindow->setScreenSaver(this);
	std::string path = getTitleFolder();
	if(!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);
	srand((unsigned int)time(NULL));
	mVideoChangeTime = 30000;
}

SystemScreenSaver::~SystemScreenSaver()
{
	// Delete subtitle file, if existing
	remove(getTitlePath().c_str());
	mCurrentGame = NULL;
}

bool SystemScreenSaver::allowSleep()
{
	return Settings::getInstance()->getString("ScreenSaverBehavior") == "black";
}

bool SystemScreenSaver::isScreenSaverActive()
{
	return (mState != STATE_INACTIVE);
}

bool SystemScreenSaver::isEnabled()
{
	return Settings::getInstance()->getString("ScreenSaverBehavior") != "none";
}

void SystemScreenSaver::startScreenSaver()
{
	LOG(LogInfo) << "SystemScreenSaver::startScreenSaver() - Enter";

	bool loadingNext = mLoadingNext;

	std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
	if (!isEnabled())
	{
		LOG(LogInfo) << "SystemScreenSaver::startScreenSaver() - exit: running Screensaver not enabled";
		return;
	}

	// get the bightness level before call stopScreenSaver
	if (screensaver_behavior == "black")
	{
		mCurrentBrightnessLevel = ApiSystem::getInstance()->getBrightnessLevel();
		LOG(LogDebug) << "SystemScreenSaver::startScreenSaver() - before 'stopScreenSaver()' - mCurrentBrightnessLevel: " << std::to_string(mCurrentBrightnessLevel);
	}

	stopScreenSaver();

	if (!loadingNext && Settings::getInstance()->getBool("StopMusicOnScreenSaver")) //Settings::getInstance()->getBool("VideoAudio"))
	{
		LOG(LogInfo) << "SystemScreenSaver::startScreenSaver() - calling AudioManager::deinit()";
		AudioManager::getInstance()->deinit();
	}

	if (screensaver_behavior == "random video")
	{
		mVideoChangeTime = Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout");

		// Configure to fade out the windows, Skip Fading if Instant mode
		mState =  PowerSaver::getMode() == PowerSaver::INSTANT
					? STATE_SCREENSAVER_ACTIVE
					: STATE_FADE_OUT_WINDOW;
	
		if (mState == STATE_FADE_OUT_WINDOW)
		{
			mState = STATE_FADE_IN_VIDEO;
			mOpacity = 1.0f;
		}
		else
			mOpacity = 0.0f;
			
		// Load a random video
		std::string path = pickRandomVideo();

		int retry = 200;
		while (retry > 0 && !Utils::FileSystem::exists(path))
		{
			retry--;
			path = pickRandomVideo();
		}

		if (!path.empty() && Utils::FileSystem::exists(path))
		{
			LOG(LogInfo) << "VideoScreenSaver::startScreenSaver() - video path: " << path.c_str();

			mVideoScreensaver = std::make_shared<VideoScreenSaver>(mWindow);
			mVideoScreensaver->setGame(mCurrentGame, "randomvideo");
			mVideoScreensaver->setVideo(path);

			PowerSaver::runningScreenSaver(true);
			mTimer = 0;
			LOG(LogInfo) << "SystemScreenSaver::startScreenSaver() - exit: running 'Video Screensaver'";
			return;
		}
	}
	else if (screensaver_behavior == "slideshow")
	{
		mVideoChangeTime = Settings::getInstance()->getInt("ScreenSaverSwapImageTimeout");

		// Configure to fade out the windows, Skip Fading if Instant mode
		mState = PowerSaver::getMode() == PowerSaver::INSTANT
			? STATE_SCREENSAVER_ACTIVE
			: STATE_FADE_OUT_WINDOW;

		if (mState == STATE_FADE_OUT_WINDOW)
		{
			mState = STATE_FADE_IN_VIDEO;
			mOpacity = 1.0f;
		}
		else
			mOpacity = 0.0f;

		// Load a random image
		std::string path;
		if (Settings::getInstance()->getBool("SlideshowScreenSaverCustomImageSource"))
		{
			path = pickRandomCustomImage();
			// Custom images are not tied to the game list
			mCurrentGame = NULL;
		}
		else
			path = pickRandomGameListImage();

		if (!path.empty() && Utils::FileSystem::exists(path))
		{
			LOG(LogInfo) << "ImageScreenSaver::startScreenSaver " << path.c_str();

			mImageScreensaver = std::make_shared<ImageScreenSaver>(mWindow);
			mImageScreensaver->setGame(mCurrentGame, "slideshow");
			mImageScreensaver->setImage(path);

			PowerSaver::runningScreenSaver(true);
			mTimer = 0;
			LOG(LogInfo) << "SystemScreenSaver::startScreenSaver() - exit: running 'Image Screensaver'";
			return;
		}	
	}
	else if (screensaver_behavior == "suspend")
	{
		if (ApiSystem::getInstance()->isDeviceAutoSuspendStayAwakeCharging() && ApiSystem::getInstance()->isBatteryCharging())
		{
			LOG(LogInfo) << "SystemScreenSaver::startScreenSaver() - exit: screensaver_behavior 'suspend' and active 'AutoSuspendStayAwakeCharging' config";
			return;
		}

		Scripting::fireEvent("quit", "suspend");
		Scripting::fireEvent("suspend");
		if (quitES(QuitMode::SUSPEND) != 0)
			LOG(LogWarning) << "SystemScreenSaver::startScreenSaver() - Suspend terminated with non-zero result!";
	}
	else if (screensaver_behavior == "black")
	{
		mWindow->getBrightnessInfoComponent()->lock();
		//mCurrentBrightnessLevel = ApiSystem::getInstance()->getBrightnessLevel();
		LOG(LogDebug) << "SystemScreenSaver::startScreenSaver() - screensaver_behavior == 'black' - mCurrentBrightnessLevel: " << std::to_string(mCurrentBrightnessLevel);
		if (mCurrentBrightnessLevel < 1)
			mCurrentBrightnessLevel = 50;

		mWindow->getBrightnessInfoComponent()->setBrightness(0);
		ApiSystem::getInstance()->setBrightnessLevel(0);
	}

	// No videos. Just use a standard screensaver
	mState = STATE_SCREENSAVER_ACTIVE;
	mCurrentGame = NULL;
	LOG(LogInfo) << "SystemScreenSaver::startScreenSaver() - exit: at the end of the function";
}

void SystemScreenSaver::stopScreenSaver()
{
	LOG(LogInfo) << "SystemScreenSaver::stopScreenSaver() - Enter";

	std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
	if (!isEnabled() || (screensaver_behavior == "suspend"))
	{
		LOG(LogInfo) << "SystemScreenSaver::stopScreenSaver() - exit: screensaver_behavior 'none' or 'suspend'";
		return;
	}
	else if (screensaver_behavior == "black")
	{
		LOG(LogDebug) << "SystemScreenSaver::stopScreenSaver() - mCurrentBrightnessLevel: " << std::to_string(mCurrentBrightnessLevel);
		if (mCurrentBrightnessLevel < 1)
			mCurrentBrightnessLevel = 50;

		mWindow->getBrightnessInfoComponent()->setBrightness(mCurrentBrightnessLevel);
		ApiSystem::getInstance()->setBrightnessLevel(mCurrentBrightnessLevel);
		mWindow->getBrightnessInfoComponent()->unlock();
	}

	bool isExitingScreenSaver = !mLoadingNext;

	if (mLoadingNext)
		mFadingImageScreensaver = mImageScreensaver;
	else
		mFadingImageScreensaver = nullptr;

	// so that we stop the background audio next time, unless we're restarting the screensaver
	mLoadingNext = false;

	mVideoScreensaver = nullptr;
	mImageScreensaver = nullptr;

	// we need this to loop through different videos
	mState = STATE_INACTIVE;
	PowerSaver::runningScreenSaver(false);

	// Exiting video screen saver -> Restore sound
	if (isExitingScreenSaver && Settings::getInstance()->getBool("StopMusicOnScreenSaver")) // && Settings::getInstance()->getBool("VideoAudio") && mVideoScreensaver)
	{
		AudioManager::getInstance()->init();

		if (Settings::getInstance()->getBool("audio.bgmusic"))
		{
			if (ViewController::get()->getState().viewing == ViewController::GAME_LIST || ViewController::get()->getState().viewing == ViewController::SYSTEM_SELECT)
				AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme(), true);
			else
				AudioManager::getInstance()->playRandomMusic();
		}

	}
}

void SystemScreenSaver::renderScreenSaver()
{
	std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
	if (!isEnabled() || (screensaver_behavior == "suspend"))
		return;

	Transform4x4f transform = Transform4x4f::Identity();
	
	if (mVideoScreensaver)
	{
		// Render black background
		Renderer::setMatrix(Transform4x4f::Identity());
		Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x000000FF, 0x000000FF);

		// Only render the video if the state requires it
		if ((int)mState >= STATE_FADE_IN_VIDEO)
		{
			unsigned int opacity = 255 - (unsigned char)(mOpacity * 255);

			mVideoScreensaver->setOpacity(opacity);
			mVideoScreensaver->render(transform);
		}
	}
	else if (mImageScreensaver)
	{
		// Render black background
		Renderer::setMatrix(transform);
		Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x000000FF);

		if (mFadingImageScreensaver != nullptr)
			mFadingImageScreensaver->render(transform);

		// Only render the video if the state requires it
		if ((int)mState >= STATE_FADE_IN_VIDEO)
		{
			if (mImageScreensaver->hasImage())
			{
				unsigned int opacity = 255 - (unsigned char)(mOpacity * 255);

				Renderer::setMatrix(transform);
				Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x00000000 | opacity);
				mImageScreensaver->setOpacity(opacity);
				mImageScreensaver->render(transform);
			}
		}
	}
	else if (mState != STATE_INACTIVE)
	{
		std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");

		if ( (screensaver_behavior == "dim") && ApiSystem::getInstance()->isDisplayAutoDimStayAwakeCharging() && ApiSystem::getInstance()->isBatteryCharging() )
			return;

		Renderer::setMatrix(Transform4x4f::Identity());
		unsigned char color = screensaver_behavior == "dim" ? 0x000000A0 : 0x000000FF;
		Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), color, color);
		if (screensaver_behavior == "black")
		{
			LOG(LogDebug) << "SystemScreenSaver::renderScreenSaver() - mCurrentBrightnessLevel: " << std::to_string(mCurrentBrightnessLevel);
			mWindow->getBrightnessInfoComponent()->setBrightness(0);
			ApiSystem::getInstance()->setBrightnessLevel(0);
		}
	}
}

unsigned long SystemScreenSaver::countGameListNodes(const char *nodeName)
{
	unsigned long nodeCount = 0;
	std::vector<SystemData*>::const_iterator it;
	for (it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); ++it)
	{
		// We only want nodes from game systems that are not collections
		if (!(*it)->isGameSystem() || (*it)->isCollection())
			continue;

		FolderData* rootFileData = (*it)->getRootFolder();

		std::vector<FileData*> allFiles = rootFileData->getFilesRecursive(GAME, true);
		std::vector<FileData*>::const_iterator itf;  // declare an iterator to a vector of strings

		for (itf=allFiles.cbegin() ; itf < allFiles.cend(); itf++) 
		{
			if ((strcmp(nodeName, "video") == 0 && (*itf)->getVideoPath() != "") ||
				(strcmp(nodeName, "image") == 0 && (*itf)->getImagePath() != ""))
			{
				nodeCount++;
			}
		}
	}
	return nodeCount;
}

void SystemScreenSaver::countVideos()
{
	if (!mVideosCounted)
	{
		mVideoCount = countGameListNodes("video");
		mVideosCounted = true;
	}
}

void SystemScreenSaver::countImages()
{
	if (!mImagesCounted)
	{
		mImageCount = countGameListNodes("image");
		mImagesCounted = true;
	}
}

std::string SystemScreenSaver::pickGameListNode(unsigned long index, const char *nodeName)
{
	std::string path;

	std::vector<SystemData*>::const_iterator it;
	for (it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); ++it)
	{
		// We only want nodes from game systems that are not collections
		if (!(*it)->isGameSystem() || (*it)->isCollection())
			continue;

		FolderData* rootFileData = (*it)->getRootFolder();

		FileType type = GAME;
		std::vector<FileData*> allFiles = rootFileData->getFilesRecursive(type, true);
		std::vector<FileData*>::const_iterator itf;  // declare an iterator to a vector of strings

		for(itf=allFiles.cbegin() ; itf < allFiles.cend(); itf++) 
		{
			if ((strcmp(nodeName, "video") == 0 && (*itf)->getVideoPath() != "") ||
				(strcmp(nodeName, "image") == 0 && (*itf)->getImagePath() != ""))
			{
				if (index-- <= 0)
				{
					// We have it
					if (strcmp(nodeName, "video") == 0)
						path = (*itf)->getVideoPath();
					else if (strcmp(nodeName, "image") == 0)
						path = (*itf)->getImagePath();

					if (!Utils::FileSystem::exists(path))
						continue;

					mSystemName = (*it)->getFullName();
					mGameName = (*itf)->getName();
					mCurrentGame = (*itf);

					return path;
				}
			}
		}
	}

	return "";
}

std::string SystemScreenSaver::pickRandomVideo()
{
	countVideos();
	mCurrentGame = NULL;
	if (mVideoCount == 0)
		return "";
	
	int video = Randomizer::random(mVideoCount);
	return pickGameListNode(video, "video");
}

std::string SystemScreenSaver::pickRandomGameListImage()
{
	countImages();
	mCurrentGame = NULL;
	if (mImageCount == 0)
		return "";
	
	int image = Randomizer::random(mImageCount);
	return pickGameListNode(image, "image");
}

std::string SystemScreenSaver::pickRandomCustomImage()
{
	std::string path;

	std::string imageDir = Settings::getInstance()->getString("SlideshowScreenSaverImageDir");
	if ((imageDir != "") && (Utils::FileSystem::exists(imageDir)))
	{
		std::string                   imageFilter = Settings::getInstance()->getString("SlideshowScreenSaverImageFilter");
		std::vector<std::string>      matchingFiles;
		Utils::FileSystem::stringList dirContent  = Utils::FileSystem::getDirContent(imageDir, Settings::getInstance()->getBool("SlideshowScreenSaverRecurse"));

		for(Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isRegularFile(*it))
			{
				// If the image filter is empty, or the file extension is in the filter string,
				//  add it to the matching files list
				if ((imageFilter.length() <= 0) ||
					(imageFilter.find(Utils::FileSystem::getExtension(*it)) != std::string::npos))
				{
					matchingFiles.push_back(*it);
				}
			}
		}

		int fileCount = (int)matchingFiles.size();
		if (fileCount > 0)
		{
			// get a random index in the range 0 to fileCount (exclusive)
			int randomIndex = Randomizer::random(fileCount);
			path = matchingFiles[randomIndex];
		}
		else
		{
			LOG(LogError) << "Slideshow Screensaver - No image files found\n";
		}
	}
	else
	{
		LOG(LogError) << "Slideshow Screensaver - Image directory does not exist: " << imageDir << "\n";
	}

	return path;
}

void SystemScreenSaver::update(int deltaTime)
{
	if (Settings::getInstance()->getString("ScreenSaverBehavior") == "suspend")
		return;

	// Use this to update the fade value for the current fade stage
	if (mState == STATE_FADE_OUT_WINDOW)
	{
		mOpacity += (float)deltaTime / FADE_TIME;
		if (mOpacity >= 1.0f)
		{
			mOpacity = 1.0f;

			// Update to the next state
			mState = STATE_FADE_IN_VIDEO;			
		}
	}
	else if (mState == STATE_FADE_IN_VIDEO)
	{
		mOpacity -= (float)deltaTime / FADE_TIME;
		if (mOpacity <= 0.0f)
		{
			mOpacity = 0.0f;
			// Update to the next state
			mState = STATE_SCREENSAVER_ACTIVE;
			mFadingImageScreensaver = nullptr;
		}
	}
	else if (mState == STATE_SCREENSAVER_ACTIVE)
	{
		// Update the timer that swaps the videos
		mTimer += deltaTime;
		if (mTimer > mVideoChangeTime)
			nextVideo();
	}

	// If we have a loaded video then update it
	if (mVideoScreensaver)
		mVideoScreensaver->update(deltaTime);

	if (mImageScreensaver)
		mImageScreensaver->update(deltaTime);
}

void SystemScreenSaver::nextVideo() 
{
	mLoadingNext = true;
	startScreenSaver();
}

FileData* SystemScreenSaver::getCurrentGame()
{
	return mCurrentGame;
}

void SystemScreenSaver::launchGame()
{
	if (mCurrentGame != NULL)
	{
		// launching Game
		auto view = ViewController::get()->getGameListView(mCurrentGame->getSystem(), false);
		if (view != nullptr)
			view->setCursor(mCurrentGame);

		if (Settings::getInstance()->getBool("ScreenSaverControls"))
			mCurrentGame->launchGame(mWindow);
		else
			ViewController::get()->goToGameList(mCurrentGame->getSystem());
	}
}


// ------------------------------------------------------------------------------------------------------------------------
// GAME SCREEN SAVER BASE CLASS
// ------------------------------------------------------------------------------------------------------------------------

GameScreenSaverBase::GameScreenSaverBase(Window* window) : GuiComponent(window),
	mViewport(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight())
{
	mDecoration = nullptr;
	mMarquee = nullptr;
	mLabelGame = nullptr;
	mLabelSystem = nullptr;
}

GameScreenSaverBase::~GameScreenSaverBase()
{
	if (mMarquee != nullptr)
	{
		delete mMarquee;
		mMarquee = nullptr;
	}

	if (mDecoration != nullptr)
	{
		delete mDecoration;
		mDecoration = nullptr;
	}

	if (mLabelGame != nullptr)
	{
		delete mLabelGame;
		mLabelGame = nullptr;
	}

	if (mLabelSystem != nullptr)
	{
		delete mLabelSystem;
		mLabelSystem = nullptr;
	}
}

#include "guis/GuiMenu.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>

void GameScreenSaverBase::setGame(FileData* game, const std::string& event)
{	
	if (mLabelGame != nullptr)
	{
		delete mLabelGame;
		mLabelGame = nullptr;
	}

	if (mLabelSystem != nullptr)
	{
		delete mLabelSystem;
		mLabelSystem = nullptr;
	}

	if (mMarquee != nullptr)
	{
		delete mMarquee;
		mMarquee = nullptr;
	}

	if (mDecoration != nullptr)
	{
		delete mDecoration;
		mDecoration = nullptr;
	}

	if (game == nullptr)
		return;

	if (game != NULL) {
		Scripting::fireEvent("screensaver-game-select", game->getSystem()->getName(), game->getPath(), game->getName(), event);
	}

	mViewport = Renderer::Rect(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight());

	/*
	if (Settings::getInstance()->getBool("ScreenSaverDecoration"))
	{
		auto sets = GuiMenu::getDecorationsSets(game->getSystem());
		int setId = Randomizer::random(sets.size());

		if (setId >= 0 && setId < sets.size() && Utils::FileSystem::exists(sets[setId].imageUrl))
		{
			std::string infoFile = Utils::String::replace(sets[setId].imageUrl, ".png", ".info");
			if (Utils::FileSystem::exists(infoFile))
			{
				FILE* fp = fopen(infoFile.c_str(), "r"); // non-Windows use "r"
				if (fp)
				{
					char readBuffer[65536];
					rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
					rapidjson::Document doc;
					doc.ParseStream(is);

					if (!doc.HasParseError())
					{
						if (doc.HasMember("top") && doc.HasMember("left") && doc.HasMember("bottom") && doc.HasMember("right") && doc.HasMember("width") && doc.HasMember("height"))
						{
							auto width = doc["width"].GetInt();
							auto height = doc["height"].GetInt();
							if (width > 0 && height > 0)
							{
								float px = Renderer::getScreenWidth() / (float)width;
								float py = Renderer::getScreenHeight() / (float)height;

								auto top = doc["top"].GetInt();
								auto left = doc["left"].GetInt();
								auto bottom = doc["bottom"].GetInt();
								auto right = doc["right"].GetInt();

								mViewport = Renderer::Rect(left * px, top * py, (width - right) * px, (height - bottom) * py);
							}
						}
					}

					fclose(fp);
				}
			}

			mDecoration = new ImageComponent(mWindow, true);
			mDecoration->setImage(sets[setId].imageUrl);
			mDecoration->setOrigin(0.5f, 0.5f);
			mDecoration->setPosition(Renderer::getScreenWidth() / 2.0f, (float)Renderer::getScreenHeight() / 2.0f);
			mDecoration->setMaxSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
		}
	}
	*/
	if (!Settings::getInstance()->getBool("SlideshowScreenSaverGameName"))
		return;

	if (Settings::getInstance()->getBool("ScreenSaverMarquee") && Utils::FileSystem::exists(game->getMarqueePath()))
	{
		mMarquee = new ImageComponent(mWindow, true);
		mMarquee->setImage(game->getMarqueePath());
		mMarquee->setOrigin(0.5f, 0.5f);
		mMarquee->setPosition(mViewport.x / 2.0f + mViewport.w * 0.50f, mViewport.y / 2.0f + mViewport.h * 0.18f);
		mMarquee->setMaxSize((float)mViewport.w * 0.40f, (float)mViewport.h * 0.22f);
	}
	
	auto ph = ThemeData::getMenuTheme()->Text.font->getPath();
	auto sz = mViewport.h / 16.f;
	auto font = Font::get(sz, ph);

	int h = mViewport.h / 4.0f;
	int fh = font->getLetterHeight();

	mLabelGame = new TextComponent(mWindow);
	mLabelGame->setPosition(mViewport.x / 2.0f, mViewport.y / 2.0f + mViewport.h - h - fh / 2);
	mLabelGame->setSize(mViewport.w, h - fh / 2);
	mLabelGame->setHorizontalAlignment(ALIGN_CENTER);
	mLabelGame->setVerticalAlignment(ALIGN_CENTER);
	mLabelGame->setColor(0xFFFFFFFF);
	mLabelGame->setGlowColor(0x00000040);
	mLabelGame->setGlowSize(3);
	mLabelGame->setFont(font);
	mLabelGame->setText(game->getName());

	mLabelSystem = new TextComponent(mWindow);
	mLabelSystem->setPosition(mViewport.x / 2.0f, mViewport.y / 2.0f + mViewport.h - h + fh / 2);
	mLabelSystem->setSize(mViewport.w, h + fh / 2);
	mLabelSystem->setHorizontalAlignment(ALIGN_CENTER);
	mLabelSystem->setVerticalAlignment(ALIGN_CENTER);
	mLabelSystem->setColor(0xD0D0D0FF);
	mLabelSystem->setGlowColor(0x00000060);
	mLabelSystem->setGlowSize(2);
	mLabelSystem->setFont(ph, sz * 0.66);
	mLabelSystem->setText(game->getSystem()->getFullName());
}

void GameScreenSaverBase::render(const Transform4x4f& transform)
{
	if (mMarquee)
	{
		mMarquee->setOpacity(mOpacity);
		mMarquee->render(transform);
	}
	else if (mLabelGame)
	{
		mLabelGame->setOpacity(mOpacity);
		mLabelGame->render(transform);
	}

	if (mLabelSystem)
	{
		mLabelSystem->setOpacity(mOpacity);
		mLabelSystem->render(transform);
	}

	if (mDecoration)
	{
		mDecoration->setOpacity(mOpacity);
		mDecoration->render(transform);
	}
}

void GameScreenSaverBase::setOpacity(unsigned char opacity)
{
	mOpacity = opacity;
}


// ------------------------------------------------------------------------------------------------------------------------
// IMAGE SCREEN SAVER CLASS
// ------------------------------------------------------------------------------------------------------------------------

ImageScreenSaver::ImageScreenSaver(Window* window) : GameScreenSaverBase(window)
{
	mImage = nullptr;
}

ImageScreenSaver::~ImageScreenSaver()
{
	if (mImage != nullptr)
		delete mImage;
}

void ImageScreenSaver::setImage(const std::string path)
{
	if (mImage == nullptr)
	{
		mImage = new ImageComponent(mWindow, true);
		mImage->setOrigin(0.5f, 0.5f);
		mImage->setPosition((mViewport.x + mViewport.w) / 2.0f, (mViewport.y + mViewport.h) / 2.0f);

		if (Settings::getInstance()->getBool("SlideshowScreenSaverStretch"))
			mImage->setMinSize((float)mViewport.w, (float)mViewport.h);
		else
			mImage->setMaxSize((float)mViewport.w, (float)mViewport.h);
	}

	mImage->setImage(path);
}

bool ImageScreenSaver::hasImage()
{
	return mImage != nullptr && mImage->hasImage();
}

void ImageScreenSaver::render(const Transform4x4f& transform)
{
	if (mImage)
	{
		mImage->setOpacity(mOpacity);

		Renderer::pushClipRect(Vector2i(mViewport.x, mViewport.y), Vector2i(mViewport.w, mViewport.h));
		mImage->render(transform);
		Renderer::popClipRect();
	}

	GameScreenSaverBase::render(transform);
}


// ------------------------------------------------------------------------------------------------------------------------
// VIDEO SCREEN SAVER CLASS
// ------------------------------------------------------------------------------------------------------------------------

VideoScreenSaver::VideoScreenSaver(Window* window) : GameScreenSaverBase(window)
{
	mVideo = nullptr;
	mTime = 0;
	mFade = 1.0;
}

VideoScreenSaver::~VideoScreenSaver()
{
	if (mVideo != nullptr)
		delete mVideo;
}

void VideoScreenSaver::setVideo(const std::string path)
{
	if (mVideo == nullptr)
	{
		mVideo = new VideoVlcComponent(mWindow);

		mVideo->topWindow(true);
		mVideo->setOrigin(0.5f, 0.5f);
		
		mVideo->setPosition((mViewport.x + mViewport.w) / 2.0f, (mViewport.y + mViewport.h) / 2.0f);

		if (Settings::getInstance()->getBool("StretchVideoOnScreenSaver"))
			mVideo->setMinSize((float)mViewport.w, (float)mViewport.h);
		else
			mVideo->setMaxSize((float)mViewport.w, (float)mViewport.h);

		mVideo->setVideo(path);
		mVideo->setScreensaverMode(true);
		mVideo->onShow();
	}

	mFade = 1.0;
	mTime = 0;
	mVideo->setVideo(path);
}

#define SUBTITLE_DURATION 4000
#define SUBTITLE_FADE 150

void VideoScreenSaver::render(const Transform4x4f& transform)
{	
	if (mVideo)
	{		
		mVideo->setOpacity(mOpacity);

		Renderer::pushClipRect(Vector2i(mViewport.x, mViewport.y), Vector2i(mViewport.w, mViewport.h));
		mVideo->render(transform);
		Renderer::popClipRect();
	}

	if (Settings::getInstance()->getString("ScreenSaverGameInfo") != "never")
	{
		if (mMarquee && mFade != 0)
		{
			mMarquee->setOpacity(mOpacity * mFade);
			mMarquee->render(transform);
		}
		else if (mLabelGame && mFade != 0)
		{
			mLabelGame->setOpacity(mOpacity * mFade);
			mLabelGame->render(transform);
		}

		if (mLabelSystem && mFade != 0)
		{
			mLabelSystem->setOpacity(mOpacity * mFade);
			mLabelSystem->render(transform);
		}
	}
	
	if (mDecoration)
	{
		mDecoration->setOpacity(mOpacity);
		mDecoration->render(transform);		
	}
}

void VideoScreenSaver::update(int deltaTime)
{
	GameScreenSaverBase::update(deltaTime);

	if (mVideo)
	{
		if (Settings::getInstance()->getString("ScreenSaverGameInfo") == "start & end")
		{
			int duration = SUBTITLE_DURATION;
			int end = Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout") - duration;

			if (mTime >= duration - SUBTITLE_FADE && mTime < duration)
			{
				mFade -= (float)deltaTime / SUBTITLE_FADE;
				if (mFade < 0)
					mFade = 0;
			}
			else if (mTime >= end - SUBTITLE_FADE && mTime < end)
			{
				mFade += (float)deltaTime / SUBTITLE_FADE;
				if (mFade > 1)
					mFade = 1;
			}
			else if (mTime > duration && mTime < end - SUBTITLE_FADE)
				mFade = 0;
		}
	
		mTime += deltaTime;	
		mVideo->update(deltaTime);
	}
}
