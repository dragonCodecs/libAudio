#ifndef LIBAUDIO_HXX
#define LIBAUDIO_HXX

#include <stdint.h>
#include <memory>
#include "fd.hxx"
#include "fileInfo.hxx"
#include "libAudio_Common.h"

enum class audioType_t : uint8_t
{
	oggVorbis = 1,
	flac = 2,
	wave = 3,
	m4a = 4,
	aac = 5,
	mp3 = 6,
	moduleIT = 7,
	musePack = 8,
	wavPack = 9,
	optimFROG = 10,
	realAudio = 11,
	wma = 12,
	moduleMOD = 13,
	moduleS3M = 14,
	moduleSTM = 15,
	moduleAON = 16,
	moduleFC1x = 17
};

struct audioFile_t
{
public: // protected:
	audioType_t _type;
	fileInfo_t _fileInfo;
	fd_t _fd;
	std::unique_ptr<Playback> _player;

	audioFile_t(audioType_t type, fd_t &&fd) noexcept : _type(type), _fileInfo(), _fd(std::move(fd)) { }

public:
	audioFile_t(audioFile_t &&) = default;
	virtual ~audioFile_t() noexcept { }
	audioFile_t &operator =(audioFile_t &&) = default;
	static audioFile_t *openR(const char *const fileName) noexcept;
	static bool isAudio(const char *const fileName) noexcept;
	static bool isAudio(const int fd) noexcept;
	const fileInfo_t &fileInfo() const noexcept { return _fileInfo; }
	fileInfo_t &fileInfo() noexcept { return _fileInfo; }
	audioType_t type() const noexcept { return _type; }
	const fd_t &fd() const noexcept { return _fd; }
	void player(std::unique_ptr<Playback> &&player) noexcept { _player = std::move(player); }

	virtual int64_t fillBuffer(void *const buffer, const uint32_t length) = 0;
	void play();
	void pause();
	void stop();

	audioFile_t(const audioFile_t &) = delete;
	audioFile_t &operator =(const audioFile_t &) = delete;
};

struct oggVorbis_t final : public audioFile_t
{
public:
	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
};

struct flac_t final : public audioFile_t
{
public: // private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	flac_t() noexcept;
	flac_t(fd_t &&fd) noexcept;

public:
	static flac_t *openR(const char *const fileName) noexcept;
	static bool isFLAC(const char *const fileName) noexcept;
	static bool isFLAC(const int fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
};

template<typename T> struct makeUnique_ { using uniqueType = std::unique_ptr<T>; };
template<typename T> struct makeUnique_<T []> { using arrayType = std::unique_ptr<T []>; };
template<typename T, size_t N> struct makeUnique_<T [N]> { struct invalidType { }; };

template<typename T, typename... Args> inline typename makeUnique_<T>::uniqueType makeUnique(Args &&...args) noexcept
{
	using consT = typename std::remove_const<T>::type;
	return std::unique_ptr<T>(new (std::nothrow) consT(std::forward<Args>(args)...));
}

template<typename T> inline typename makeUnique_<T>::arrayType makeUnique(const size_t num) noexcept
{
	using consT = typename std::remove_const<typename std::remove_extent<T>::type>::type;
	return std::unique_ptr<T>(new (std::nothrow) consT[num]());
}

template<typename T, typename... Args> inline typename makeUnique_<T>::invalidType makeUnique(Args &&...) noexcept = delete;

#endif /*LIBAUDIO_HXX*/
