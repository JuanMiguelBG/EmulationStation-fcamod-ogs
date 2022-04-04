#include "AudioManager.h"

#include "Log.h"
#include "Settings.h"
#include "Sound.h"
#include <SDL.h>
#include <time.h>
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <unistd.h>

std::vector<std::shared_ptr<Sound>> AudioManager::sSoundVector;
std::shared_ptr<AudioManager> AudioManager::sInstance;

AudioManager::AudioManager() : mCurrentMusic(NULL), mInitialized(false), mMusicVolume(MIX_MAX_VOLUME), mVideoPlaying(false)
{
	init();
}

AudioManager::~AudioManager()
{
	deinit();
}

std::shared_ptr<AudioManager> & AudioManager::getInstance()
{
	if (sInstance == nullptr)
		sInstance = std::shared_ptr<AudioManager>(new AudioManager);

	return sInstance;
}

bool AudioManager::isInitialized()
{
	if (sInstance == nullptr)
		return false;

	return sInstance->mInitialized;
}

void AudioManager::init()
{
	if (mInitialized)
		return;

	mRunningFromPlaylist = false;
	mMusicVolume = 0;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		LOG(LogError) << "AudioManager::init() - ERROR: problem to initializing SDL audio!\n" << SDL_GetError();
		return;
	}

	// Open the audio device and pause
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
		LOG(LogError) << "AudioManager::init() - MUSIC Error - Unable to open SDLMixer audio: " << SDL_GetError() << std::endl;
	else
	{
		LOG(LogInfo) << "AudioManager::init() - SDL AUDIO Initialized";
		mInitialized = true;

		// Reload known sounds
		for (unsigned int i = 0; i < sSoundVector.size(); i++)
			sSoundVector[i]->init();
	}
}

void AudioManager::deinit()
{
	if (!mInitialized)
		return;

	LOG(LogInfo) << "AudioManager::deinit()";

	mInitialized = false;

	//stop all playback
	stop();
	stopMusic();

	// Free known sounds from memory
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
		sSoundVector[i]->deinit();

	Mix_HookMusicFinished(nullptr);
	Mix_HaltMusic();

	//completely tear down SDL audio. else SDL hogs audio resources and emulators might fail to start...
	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	LOG(LogInfo) << "AudioManager::deinit() - SDL AUDIO Deinitialized";
}

void AudioManager::registerSound(std::shared_ptr<Sound> & sound)
{
	getInstance();
	sSoundVector.push_back(sound);
}

void AudioManager::unregisterSound(std::shared_ptr<Sound> & sound)
{
	getInstance();
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
	{
		if (sSoundVector.at(i) == sound)
		{
			sSoundVector[i]->stop();
			sSoundVector.erase(sSoundVector.cbegin() + i);
			return;
		}
	}
	LOG(LogWarning) << "AudioManager::unregisterSound() - ERROR: tried to unregister a sound that wasn't registered!";
}

void AudioManager::play()
{
	getInstance();
}

void AudioManager::stop()
{
	// Stop playing all Sounds
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
		if (sSoundVector.at(i)->isPlaying())
			sSoundVector[i]->stop();
}

void AudioManager::findMusic(const std::string &path, std::vector<std::string>& all_matching_files)
{
	if (!Utils::FileSystem::isDirectory(path))
		return;

	bool anySystem = !Settings::getInstance()->getBool("audio.persystem");

	auto dirContent = Utils::FileSystem::getDirContent(path);
	for (auto it = dirContent.cbegin(); it != dirContent.cend(); ++it)
	{
		if (Utils::FileSystem::isDirectory(*it))
		{
			if (*it == "." || *it == "..")
				continue;

			if (anySystem || mSystemName == Utils::FileSystem::getFileName(*it))
				findMusic(*it, all_matching_files);
		}
		else
		{
			std::string extension = Utils::String::toLower(Utils::FileSystem::getExtension(*it));
			if (extension == ".mp3" || extension == ".ogg")
				all_matching_files.push_back(*it);
		}
	}
}

void AudioManager::playRandomMusic(bool continueIfPlaying)
{
	if (!mInitialized || !Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	std::vector<std::string> musics;

	// check in Theme music directory
	if (!mCurrentThemeMusicDirectory.empty())
		findMusic(mCurrentThemeMusicDirectory, musics);

	// check in User music directory
	if (musics.empty() && !Settings::getInstance()->getString("UserMusicDirectory").empty())
		findMusic(Settings::getInstance()->getString("MusicDirectory"), musics);

	// check in System music directory
	if (musics.empty() && !Settings::getInstance()->getString("MusicDirectory").empty())
		findMusic(Settings::getInstance()->getString("MusicDirectory"), musics);

	// check in .emulationstation/music directory
	if (musics.empty())
		findMusic(Utils::FileSystem::getEsConfigPath() + "/music", musics);

	if (musics.empty())
		return;

	srand(time(NULL) % getpid() + getppid());

	int randomIndex = rand() % musics.size();

	// continue playing ?
	if (mCurrentMusic != NULL && continueIfPlaying)
		return;

	playMusic(musics.at(randomIndex));
	mRunningFromPlaylist = true;
}

void AudioManager::playMusic(std::string path)
{
	if (!mInitialized)
		return;

	// free the previous music
	stopMusic(false);

	if (!Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	// load a new music
	mCurrentMusic = Mix_LoadMUS(path.c_str());
	if (mCurrentMusic == NULL)
	{
		LOG(LogError) << Mix_GetError() << " for " << path;
		return;
	}

	if (Mix_FadeInMusic(mCurrentMusic, 1, 1000) == -1)
	{
		stopMusic();
		return;
	}

	Mix_HookMusicFinished(AudioManager::onMusicFinished);
	mCurrentMusicPath = path;
	mCurrentSong = Utils::FileSystem::getStem(path);
}

void AudioManager::onMusicFinished()
{
	AudioManager::getInstance()->playRandomMusic(false);
}

void AudioManager::stopMusic(bool fadeOut)
{
	if (mCurrentMusic == NULL)
		return;

	Mix_HookMusicFinished(nullptr);

	if (fadeOut)
	{
		// Fade-out is nicer on Batocera!
		while (!Mix_FadeOutMusic(500) && Mix_PlayingMusic())
			SDL_Delay(100);
	}

	Mix_HaltMusic();
	Mix_FreeMusic(mCurrentMusic);
	mCurrentMusicPath = "";
	mCurrentMusic = NULL;
}

void AudioManager::themeChanged(const std::shared_ptr<ThemeData>& theme, bool force)
{
	if (theme == nullptr)
		return;

	if (!force && mSystemName == theme->getSystemThemeFolder())
		return;

	mSystemName = theme->getSystemThemeFolder();
	mCurrentThemeMusicDirectory = "";

	if (!Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	const ThemeData::ThemeElement* elem = theme->getElement("system", "directory", "sound");

	if (Settings::getInstance()->getBool("audio.thememusics"))
	{
		if (elem && elem->has("path") && !Settings::getInstance()->getBool("audio.persystem"))
			mCurrentThemeMusicDirectory = elem->get<std::string>("path");

		std::string bgSound;

		elem = theme->getElement("system", "bgsound", "sound");
		if (elem && elem->has("path") && Utils::FileSystem::exists(elem->get<std::string>("path")))
		{
			bgSound = Utils::FileSystem::getCanonicalPath(elem->get<std::string>("path"));
			if (bgSound == mCurrentMusicPath)
				return;
		}

		// Found a music for the system
		if (!bgSound.empty())
		{
			mRunningFromPlaylist = false;
			playMusic(bgSound);
			return;
		}
	}


	mSystemName = theme->getSystemThemeFolder();
	if (!mRunningFromPlaylist || Settings::getInstance()->getBool("audio.persystem"))
		playRandomMusic(false);
}

void AudioManager::setVideoPlaying(bool state)
{
	if (sInstance == nullptr || !sInstance->mInitialized || !Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	if (state && !Settings::getInstance()->getBool("VideoLowersMusic"))
		return;

	sInstance->mVideoPlaying = state;
}

int AudioManager::getMaxMusicVolume()
{
	int ret = (Settings::getInstance()->getInt("MusicVolume") * MIX_MAX_VOLUME) / 100;
	if (ret > MIX_MAX_VOLUME)
		return MIX_MAX_VOLUME;

	if (ret < 0)
		return 0;

	return ret;
}

void AudioManager::update(int deltaTime)
{
	if (sInstance == nullptr || !sInstance->mInitialized || !Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	float deltaVol = deltaTime / 8.0f;

//	#define MINVOL 5

	int maxVol = getMaxMusicVolume();
	int minVol = maxVol / 20;
	if (maxVol > 0 && minVol == 0)
		minVol = 1;

	if (sInstance->mVideoPlaying && sInstance->mMusicVolume != minVol)
	{
		if (sInstance->mMusicVolume > minVol)
		{
			sInstance->mMusicVolume -= deltaVol;
			if (sInstance->mMusicVolume < minVol)
				sInstance->mMusicVolume = minVol;
		}

		Mix_VolumeMusic((int)sInstance->mMusicVolume);
	}
	else if (!sInstance->mVideoPlaying && sInstance->mMusicVolume != maxVol)
	{
		if (sInstance->mMusicVolume < maxVol)
		{
			sInstance->mMusicVolume += deltaVol;
			if (sInstance->mMusicVolume > maxVol)
				sInstance->mMusicVolume = maxVol;
		}
		else
			sInstance->mMusicVolume = maxVol;

		Mix_VolumeMusic((int)sInstance->mMusicVolume);
	}
}

std::string AudioManager::getSongName()
{
	LOG(LogDebug) << "AudioManager::getSongName() - mCurrentSong: " << mCurrentSong << ", mCurrentMusicPath: " << mCurrentMusicPath;

	if (mCurrentSong.empty() && isSongPlaying())
		return Utils::FileSystem::getStem(mCurrentMusicPath);

	return mCurrentSong;
}
