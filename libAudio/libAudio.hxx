#ifndef LIBAUDIO_HXX
#define LIBAUDIO_HXX

#include <stdint.h>
#include "fd.hxx"
#include "fileInfo.hxx"
#include "libAudio_Common.h"
#include "playback.hxx"
#include "uniquePtr.hxx"

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

struct audioModeRead_t { };
struct audioModeWrite_t { };

struct audioFile_t
{
protected:
	audioType_t _type;
	fileInfo_t _fileInfo;
	fd_t _fd;
	std::unique_ptr<playback_t> _player;

	audioFile_t(audioType_t type, fd_t &&fd) noexcept : _type(type), _fileInfo(), _fd(std::move(fd)) { }

public:
	audioFile_t(audioFile_t &&) = default;
	virtual ~audioFile_t() noexcept = default;
	audioFile_t &operator =(audioFile_t &&) = default;
	static audioFile_t *openR(const char *const fileName) noexcept;
	static audioFile_t *openW(const char *const fileName) noexcept;
	static bool isAudio(const char *const fileName) noexcept;
	static bool isAudio(const int32_t fd) noexcept;
	const fileInfo_t &fileInfo() const noexcept { return _fileInfo; }
	fileInfo_t &fileInfo() noexcept { return _fileInfo; }
	audioType_t type() const noexcept { return _type; }
	const fd_t &fd() const noexcept { return _fd; }
	void fd(fd_t &&fd) noexcept { _fd.swap(fd); }
	void player(std::unique_ptr<playback_t> &&player) noexcept { _player = std::move(player); }

	virtual int64_t fillBuffer(void *const buffer, const uint32_t length) = 0;
	virtual int64_t writeBuffer(const void *const buffer, const uint32_t length);
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
	struct encoderContext_t;
	std::unique_ptr<decoderContext_t> decoderCtx;

public:
	flac_t(fd_t &&fd, audioModeRead_t) noexcept;
	flac_t(fd_t &&fd, audioModeWrite_t) noexcept;
	static flac_t *openR(const char *const fileName) noexcept;
	static flac_t *openW(const char *const fileName) noexcept;
	static bool isFLAC(const char *const fileName) noexcept;
	static bool isFLAC(const int32_t fd) noexcept;
	decoderContext_t *decoderContext() const noexcept { return decoderCtx.get(); }
	bool valid() const noexcept { return bool(decoderCtx) && _fd.valid(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
	int64_t writeBuffer(const void *const buffer, const uint32_t length) final override;
};

struct m4a_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	m4a_t(fd_t &&fd) noexcept;
	static m4a_t *openR(const char *const fileName) noexcept;
	static bool isM4A(const char *const fileName) noexcept;
	static bool isM4A(const int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
	void fetchTags() noexcept;
};

struct aac_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	uint8_t *nextFrame() noexcept;

public:
	aac_t(fd_t &&fd) noexcept;
	static aac_t *openR(const char *const fileName) noexcept;
	static bool isAAC(const char *const fileName) noexcept;
	static bool isAAC(const int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
};

struct mp3_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	bool readMetadata() noexcept WARN_UNUSED;

public:
	mp3_t(fd_t &&fd) noexcept;
	static mp3_t *openR(const char *const fileName) noexcept;
	static bool isMP3(const char *const fileName) noexcept;
	static bool isMP3(const int32_t fd) noexcept;
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

struct mpc_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	mpc_t(fd_t &&fd) noexcept;
	static mpc_t *openR(const char *const fileName) noexcept;
	static bool isMPC(const char *const fileName) noexcept;
	static bool isMPC(const int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
};

struct wavPack_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	wavPack_t(fd_t &&fd, const char *const fileName) noexcept;
	static wavPack_t *openR(const char *const fileName) noexcept;
	static bool isWavPack(const char *const fileName) noexcept;
	static bool isWavPack(const int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *const buffer, const uint32_t length) final override;
};

#endif /*LIBAUDIO_HXX*/
