#pragma once
#ifndef ES_CORE_VOLUME_CONTROL_H
#define ES_CORE_VOLUME_CONTROL_H

#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>

/*!
Singleton pattern. Call getInstance() to get an object.
*/
class VolumeControl
{
	static std::string mixerName;
	static std::string mixerCard;
	int mixerIndex;
	snd_mixer_t* mixerHandle;
	snd_mixer_elem_t* mixerElem;
	snd_mixer_selem_id_t* mixerSelemId;

	int originalVolume;
	int internalVolume;

	static std::weak_ptr<VolumeControl> sInstance;

	VolumeControl();
	VolumeControl(const VolumeControl & right);
	VolumeControl & operator=(const VolumeControl & right);

	

public:
	static std::shared_ptr<VolumeControl> & getInstance();

	void init();
	void deinit();

	bool isAvailable();

	int getVolume() const;
	void setVolume(int volume);

	~VolumeControl();
};

#endif // ES_CORE_VOLUME_CONTROL_H
