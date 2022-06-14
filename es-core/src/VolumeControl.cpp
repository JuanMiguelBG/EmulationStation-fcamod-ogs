#include "VolumeControl.h"

#include "math/Misc.h"
#include "Log.h"
#include "Settings.h"

std::string VolumeControl::mixerName = "Master";
std::string VolumeControl::mixerCard = "default";

std::weak_ptr<VolumeControl> VolumeControl::sInstance;


VolumeControl::VolumeControl()
	: originalVolume(0), internalVolume(0)
	, mixerIndex(0), mixerHandle(nullptr), mixerElem(nullptr), mixerSelemId(nullptr)
{
	init();

	//get original volume levels for system
	originalVolume = getVolume();
}

VolumeControl::~VolumeControl()
{
	//set original volume levels for system
	//setVolume(originalVolume);

	deinit();
}

std::shared_ptr<VolumeControl> & VolumeControl::getInstance()
{
	//check if an VolumeControl instance is already created, if not create one
	static std::shared_ptr<VolumeControl> sharedInstance = sInstance.lock();
	if (sharedInstance == nullptr) {
		sharedInstance.reset(new VolumeControl);
		sInstance = sharedInstance;
	}
	return sharedInstance;
}

void VolumeControl::init()
{
	//initialize audio mixer interface

	//try to open mixer device
	if (mixerHandle == nullptr)
	{
		// Allow users to override the AudioCard and MixerName in es_settings.cfg
		auto audioCard = Settings::getInstance()->getString("AudioCard");
		if (!audioCard.empty())
			mixerCard = audioCard;

		auto audioDevice = Settings::getInstance()->getString("AudioDevice");
		if (!audioDevice.empty())
			mixerName = audioDevice;

		snd_mixer_selem_id_alloca(&mixerSelemId);
		//sets simple-mixer index and name
		snd_mixer_selem_id_set_index(mixerSelemId, mixerIndex);
		snd_mixer_selem_id_set_name(mixerSelemId, mixerName.c_str());
		//open mixer
		if (snd_mixer_open(&mixerHandle, 0) >= 0)
		{
			LOG(LogDebug) << "VolumeControl::init() - Opened ALSA mixer";
			//ok. attach to defualt card
			if (snd_mixer_attach(mixerHandle, mixerCard.c_str()) >= 0)
			{
				LOG(LogDebug) << "VolumeControl::init() - Attached to default card";
				//ok. register simple element class
				if (snd_mixer_selem_register(mixerHandle, NULL, NULL) >= 0)
				{
					LOG(LogDebug) << "VolumeControl::init() - Registered simple element class";
					//ok. load registered elements
					if (snd_mixer_load(mixerHandle) >= 0)
					{
						LOG(LogDebug) << "VolumeControl::init() - Loaded mixer elements";
						//ok. find elements now
						mixerElem = snd_mixer_find_selem(mixerHandle, mixerSelemId);
						if (mixerElem != nullptr)
						{
							//wohoo. good to go...
							LOG(LogDebug) << "VolumeControl::init() - Mixer initialized";
						}
						else
						{
							LOG(LogInfo) << "VolumeControl::init() - Unable to find mixer " << mixerName << " -> Search for alternative mixer";

							snd_mixer_selem_id_t *mxid = nullptr;
							snd_mixer_selem_id_alloca(&mxid);

							for (snd_mixer_elem_t* mxe = snd_mixer_first_elem(mixerHandle); mxe != nullptr; mxe = snd_mixer_elem_next(mxe))
							{
								if (snd_mixer_selem_has_playback_volume(mxe) != 0 && snd_mixer_selem_is_active(mxe) != 0)
								{
									snd_mixer_selem_get_id(mxe, mxid);
									mixerName = snd_mixer_selem_id_get_name(mxid);

									LOG(LogInfo) << "VolumeControl::init() - mixername : " << mixerName;
									Settings::getInstance()->setString("AudioDevice", mixerName);

									snd_mixer_selem_id_set_name(mixerSelemId, mixerName.c_str());
									mixerElem = snd_mixer_find_selem(mixerHandle, mixerSelemId);
									if (mixerElem != nullptr)
									{
										//wohoo. good to go...
										LOG(LogDebug) << "VolumeControl::init() - Mixer initialized";
										break;
									}
									else
										LOG(LogDebug) << "VolumeControl::init() - Mixer not initialized";
								}
							}

							if (mixerElem == nullptr)
							{
								LOG(LogError) << "VolumeControl::init() - Failed to find mixer elements!";
								snd_mixer_close(mixerHandle);
								mixerHandle = nullptr;
							}
						}
					}
					else
					{
						LOG(LogError) << "VolumeControl::init() - Failed to load mixer elements!";
						snd_mixer_close(mixerHandle);
						mixerHandle = nullptr;
					}
				}
				else
				{
					LOG(LogError) << "VolumeControl::init() - Failed to register simple element class!";
					snd_mixer_close(mixerHandle);
					mixerHandle = nullptr;
				}
			}
			else
			{
				LOG(LogError) << "VolumeControl::init() - Failed to attach to default card!";
				snd_mixer_close(mixerHandle);
				mixerHandle = nullptr;
			}
		}
		else
		{
			LOG(LogError) << "VolumeControl::init() - Failed to open ALSA mixer!";
		}
	}
}

void VolumeControl::deinit()
{
	//deinitialize audio mixer interface

	if (mixerHandle != nullptr)
	{
		snd_mixer_detach(mixerHandle, mixerCard.c_str());
		snd_mixer_free(mixerHandle);
		snd_mixer_close(mixerHandle);
		mixerHandle = nullptr;
		mixerElem = nullptr;
	}
}

int VolumeControl::getVolume() const
{
	int volume = 0;

	if (mixerElem != nullptr)
	{
		if (mixerHandle != nullptr)
			snd_mixer_handle_events(mixerHandle);
		/*
		int mute_state;
		if (snd_mixer_selem_has_playback_switch(mixerElem)) 
		{
			snd_mixer_selem_get_playback_switch(mixerElem, SND_MIXER_SCHN_UNKNOWN, &mute_state);
			if (!mute_state) // system Muted
				return 0;
		}
		*/
		//get volume range
		long minVolume;
		long maxVolume;
		if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0)
		{
			//ok. now get volume
			long rawVolume;
			if (snd_mixer_selem_get_playback_volume(mixerElem, SND_MIXER_SCHN_MONO, &rawVolume) == 0)
			{
				//worked. bring into range 0-100
				rawVolume -= minVolume;
				if (rawVolume > 0)
					volume = (rawVolume * 100.0) / (maxVolume - minVolume) + 0.5;
			}
			else
			{
				LOG(LogError) << "VolumeControl::getVolume() - Failed to get mixer volume!";
			}
		}
		else
		{
			LOG(LogError) << "VolumeControl::getVolume() - Failed to get volume range!";
		}
	}

	// clamp to 0-100 range
	if (volume < 0)
		volume = 0;

	if (volume > 100)
		volume = 100;

	return volume;
}

void VolumeControl::setVolume(int volume)
{
	//clamp to 0-100 range
	if (volume < 0)
	{
		volume = 0;
	}
	if (volume > 100)
	{
		volume = 100;
	}
	//store values in internal variables
	internalVolume = volume;

	if (mixerElem != nullptr)
	{
		//get volume range
		long minVolume;
		long maxVolume;
		if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0)
		{
			//ok. bring into minVolume-maxVolume range and set
			long rawVolume = (volume * (maxVolume - minVolume) / 100) + minVolume;
			if (snd_mixer_selem_set_playback_volume(mixerElem, SND_MIXER_SCHN_FRONT_LEFT, rawVolume) < 0 
				|| snd_mixer_selem_set_playback_volume(mixerElem, SND_MIXER_SCHN_FRONT_RIGHT, rawVolume) < 0)
			{
				LOG(LogError) << "VolumeControl::getVolume() - Failed to set mixer volume!";
			}
		}
		else
		{
			LOG(LogError) << "VolumeControl::getVolume() - Failed to get volume range!";
		}
	}
}

bool VolumeControl::isAvailable()
{
	return mixerHandle != nullptr && mixerElem != nullptr;
}
