#ifndef LIBAUDIO_HXX
#define LIBAUDIO_HXX

#include <stdint.h>
#include <memory>
#include "fd.hxx"
#include "fileInfo.hxx"
#include "libAudio_Common.h"
#include "playback.hxx"

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
protected:
	audioType_t _type;
	fileInfo_t _fileInfo;
	fd_t _fd;
	std::unique_ptr<Playback> _player;
	std::unique_ptr<playback_t> _newPlayer;

	audioFile_t(audioType_t type, fd_t &&fd) noexcept : _type(type), _fileInfo(), _fd(std::move(fd)) { }

public:
	audioFile_t(audioFile_t &&) = default;
	virtual ~audioFile_t() noexcept = default;
	audioFile_t &operator =(audioFile_t &&) = default;
	static audioFile_t *openR(const char *const fileName) noexcept;
	static bool isAudio(const char *const fileName) noexcept;
	static bool isAudio(const int32_t fd) noexcept;
	const fileInfo_t &fileInfo() const noexcept { return _fileInfo; }
	fileInfo_t &fileInfo() noexcept { return _fileInfo; }
	audioType_t type() const noexcept { return _type; }
	const fd_t &fd() const noexcept { return _fd; }
	void fd(fd_t &&fd) noexcept { _fd.swap(fd); }
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
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	flac_t(fd_t &&fd) noexcept;
	static flac_t *openR(const char *const fileName) noexcept;
	static bool isFLAC(const char *const fileName) noexcept;
	static bool isFLAC(const int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
};

struct moduleFile_t : public audioFile_t
{
protected:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	moduleFile_t(audioType_t type, fd_t &&fd) noexcept;

public:
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
};

struct modMOD_t final : public moduleFile_t
{
public:
	modMOD_t(fd_t &&fd) noexcept;
	static modMOD_t *openR(const char *const fileName) noexcept;
	static bool isMOD(const char *const fileName) noexcept;
	static bool isMOD(const int32_t fd) noexcept;
};

struct modS3M_t final : public moduleFile_t
{
public:
	modS3M_t(fd_t &&fd) noexcept;
	static modS3M_t *openR(const char *const fileName) noexcept;
	static bool isS3M(const char *const fileName) noexcept;
	static bool isS3M(const int32_t fd) noexcept;
};

struct modSTM_t final : public moduleFile_t
{
public:
	modSTM_t(fd_t &&fd) noexcept;
	static modSTM_t *openR(const char *const fileName) noexcept;
	static bool isSTM(const char *const fileName) noexcept;
	static bool isSTM(const int32_t fd) noexcept;
};

struct modIT_t final : public moduleFile_t
{
public:
	modIT_t(fd_t &&fd) noexcept;
	static modIT_t *openR(const char *const fileName) noexcept;
	static bool isIT(const char *const fileName) noexcept;
	static bool isIT(const int32_t fd) noexcept;
};

template<typename T> struct makeUnique_ { using uniqueType = std::unique_ptr<T>; };
template<typename T> struct makeUnique_<T []> { using arrayType = std::unique_ptr<T []>; };
template<typename T, size_t N> struct makeUnique_<T [N]> { struct invalidType { }; };

template<typename T, typename... Args> inline typename makeUnique_<T>::uniqueType makeUnique(Args &&...args) noexcept(noexcept(T(std::forward<Args>(args)...)))
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
