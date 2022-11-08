#include "AudioManager.h"

#include "Log.h"
#include "Settings.h"
#include "Sound.h"
#include <SDL.h>
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/Randomizer.h"
#include "SystemConf.h"
#include "ThemeData.h"
#include <unistd.h>


// batocera
// Size of last played music history as a percentage of file total
#define LAST_PLAYED_SIZE 0.4

AudioManager* AudioManager::sInstance = NULL;
std::vector<std::shared_ptr<Sound>> AudioManager::sSoundVector;

AudioManager::AudioManager() : mInitialized(false), mCurrentMusic(nullptr), mMusicVolume(MIX_MAX_VOLUME), mVideoPlaying(false)
{
	init();
}

AudioManager::~AudioManager()
{
	deinit();
}

AudioManager* AudioManager::getInstance()
{
	//check if an AudioManager instance is already created, if not create one
	if (sInstance == nullptr)
		sInstance = new AudioManager();

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

	mSongNameChanged = false;
	mMusicVolume = 0;
	mPlayingSystemThemeSong = "none";
	std::deque<std::string> mLastPlayed;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		LOG(LogError) << "AudioManager::init() - Error initializing SDL audio!\n" << SDL_GetError();
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

	LOG(LogDebug) << "AudioManager::deinit() - AudioManager::deinit";

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
	LOG(LogWarning) << "AudioManager::unregisterSound() - Error - tried to unregister a sound that wasn't registered!";
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

void AudioManager::getMusicIn(const std::string &path, std::vector<std::string>& all_matching_files)
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
				getMusicIn(*it, all_matching_files);
		}
		else
		{
			std::string extension = Utils::String::toLower(Utils::FileSystem::getExtension(*it));
			if (extension == ".mp3" || extension == ".ogg" || extension == ".wav")
				all_matching_files.push_back(*it);
		}
	}
}

// batocera
// Add the current song to the last played history, truncating as needed
void AudioManager::addLastPlayed(const std::string& newSong, int totalMusic)
{
	int historySize = std::floor(totalMusic * LAST_PLAYED_SIZE);
	if (historySize < 1)
	{
		// Number of songs is too small to bother with
		return;
	}

	while (mLastPlayed.size() > historySize) {
		mLastPlayed.pop_back();
	}
	mLastPlayed.push_front(newSong);

	LOG(LogDebug) << "AudioManager::addLastPlayed() - Adding " << newSong << " to last played, " << mLastPlayed.size() << " in history";
}

// batocera
// Check if current song exists in last played history
bool AudioManager::songWasPlayedRecently(const std::string& song)
{
	for (std::string i : mLastPlayed)
	{
		if (song == i)
		{
			return true;
		}
	}
	return false;
}

void AudioManager::playRandomMusic(bool continueIfPlaying)
{
	if (!Settings::getInstance()->getBool("audio.bgmusic"))
		return;

	std::vector<std::string> musics;

	// check in Theme music directory
	if (!mCurrentThemeMusicDirectory.empty())
		getMusicIn(mCurrentThemeMusicDirectory, musics);

	// check in User music directory
	std::string path = Settings::getInstance()->getString("UserMusicDirectory");
	if (musics.empty() && !path.empty())
		getMusicIn(path, musics);

	// check in system sound directory
	path = Settings::getInstance()->getString("MusicDirectory");
	if (musics.empty())
		getMusicIn(path, musics);

	// check in .emulationstation/music directory
	if (musics.empty())
		getMusicIn(Utils::FileSystem::getEsConfigPath() + "/music", musics);

	if (musics.empty())
		return;

	int randomIndex = Randomizer::random(musics.size());
	while (songWasPlayedRecently(musics.at(randomIndex)))
	{
		LOG(LogDebug) << "AudioManager::playRandomMusic() - Music \"" << musics.at(randomIndex) << "\" was played recently, trying again";
		randomIndex = Randomizer::random(musics.size());
	}

	// continue playing ?
	if (mCurrentMusic != nullptr && continueIfPlaying)
		return;

	playMusic(musics.at(randomIndex));
	playSong(musics.at(randomIndex));
	addLastPlayed(musics.at(randomIndex), musics.size());
	mPlayingSystemThemeSong = "";
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
		LOG(LogError) << "AudioManager::playMusic() - " << Mix_GetError() << " for " << path;
		return;
	}

	if (Mix_FadeInMusic(mCurrentMusic, 1, 1000) == -1)
	{
		stopMusic();
		return;
	}

	mCurrentMusicPath = path;
	Mix_HookMusicFinished(AudioManager::musicEnd_callback);
}

void AudioManager::musicEnd_callback()
{
	if (!AudioManager::getInstance()->mPlayingSystemThemeSong.empty())
	{
		AudioManager::getInstance()->playMusic(AudioManager::getInstance()->mPlayingSystemThemeSong);
		return;
	}

	AudioManager::getInstance()->playRandomMusic(false);
}

void AudioManager::stopMusic(bool fadeOut)
{
	if (mCurrentMusic == NULL)
		return;

	Mix_HookMusicFinished(nullptr);

	if (fadeOut)
	{
		// Fade-out is nicer !
		while (!Mix_FadeOutMusic(500) && Mix_PlayingMusic())
			SDL_Delay(100);
	}

	Mix_HaltMusic();
	Mix_FreeMusic(mCurrentMusic);
	mCurrentMusicPath = "";
	mCurrentMusic = NULL;
}

// Fast string hash in order to use strings in switch/case
// How does this work? Look for Dan Bernstein hash on the internet
constexpr unsigned int sthash(const char *s, int off = 0)
{
	return !s[off] ? 5381 : (sthash(s, off+1)*33) ^ s[off];
}

void AudioManager::setSongName(const std::string& song)
{
	if (song == mCurrentSong)
		return;

	mCurrentSong = Utils::String::replace(song, "_", " ");
	mSongNameChanged = true;
}

void AudioManager::playSong(const std::string& song)
{
	if (song == mCurrentSong)
		return;

	if (song.empty())
	{
		mSongNameChanged = true;
		mCurrentSong = "";
		return;
	}

	setSongName(Utils::FileSystem::getStem(song.c_str()));
}

void AudioManager::changePlaylist(const std::shared_ptr<ThemeData>& theme, bool force)
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
			mPlayingSystemThemeSong = bgSound;
			playMusic(bgSound);
			return;
		}
	}

	if (force || !mPlayingSystemThemeSong.empty() || Settings::getInstance()->getBool("audio.persystem"))
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
